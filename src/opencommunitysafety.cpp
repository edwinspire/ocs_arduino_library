#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <Preferences.h>
#include "./lib/arduinoOutputpin/src/Outputpin.cpp"

using namespace websockets;

namespace ocs
{
    


    enum StatusAlarm
    {
        ALARM,
        NORMAL
    };

    enum AlarmType
    {
        OFF,
        EMERGENCY,
        PULSE,
        TEST
    };

    namespace input
    {

        enum Status
        {
            TROUBLE,
            NORMAL,
            ALARM,
            UNDEFINED
        };

        class Input
        {

            //          using setAlarmCallback = void (*)(ocs::AlarmType at);

        private:
            //            setAlarmCallback setAlarm;
            int value;
            int gpio;
            ulong last_time;
            ulong interval = 500;
            float zone_threshold = 40;
            Status last_status = Status::UNDEFINED;

        public:
            String name = "Physical Button";
            Status status = Status::UNDEFINED;

            DynamicJsonDocument toJson()
            {
                DynamicJsonDocument doc(256);
                doc["gpio"] = this->gpio;
                doc["status"] = this->status;
                doc["name"] = this->name;

                return doc;
            }
            int getvalue()
            {
                this->value = analogRead(this->gpio); // read the input pin
                // Serial.println(this->value);
                return this->value;
            }
            void setup(int gpio_input)
            {
                this->gpio = gpio_input;
                //               this->setAlarm = sac;
                pinMode(this->gpio, INPUT);
            }

            bool changed()
            {
                bool Change = false;
                // numzone > 1000 only soft button
                if (millis() - last_time > interval)
                {

                    this->last_time = millis();
                    float center = 4096 / 2;
                    float upper_threshold = center + ((this->zone_threshold / 100) * center);
                    float lower_threshold = center - ((this->zone_threshold / 100) * center);
                    this->getvalue();

                    // Serial.printf("up %f - low %f - center %f - value %i\n", upper_threshold, lower_threshold, center, this->value);

                    if (this->value <= lower_threshold)
                    {
                        this->status = Status::ALARM;
                        //           Serial.println("ALARMA");
                        // setAlarm(AlarmType::EMERGENCY);
                    }
                    else if (this->value >= upper_threshold)
                    {
                        this->status = Status::TROUBLE;
                        //             Serial.println("PROBLEMA");
                    }
                    else
                    {
                        this->status = Status::NORMAL;
                        //               Serial.println("NORMAL");
                    }

                    if (last_status != status)
                    {
                        Change = true;
                        last_status = this->status;
                    }
                }

                return Change;
            }
        };

    };

    class OpenCommunitySafety
    {

    public:
        Preferences pref;
        String websocketHost;
        String deviceId;
        input::Input input01;

        void setAlarm(ocs::AlarmType at)
        {

            Serial.println("Entra en setAlarm");
            this->out01.low();
            sleep(1000);
            this->out01.high();
            //       digitalWrite(21, HIGH);

            switch (at)
            {
            case ocs::AlarmType::TEST:
                this->out01.blink(2000, 1500, 0, 4); // 1500 milliseconds ON, 2000 milliseconds OFF, start immidiately, blink 10 times (5 times OFF->ON, 5 times ON->OFF, interleavedly)
                break;
            case ocs::AlarmType::EMERGENCY:
                this->out01.blink(500, 5 * 60 * 1000, 0, 2); // 5 minutes
                break;
            case ocs::AlarmType::PULSE:
                this->out01.blink(3000, 4000, 0, 50); // 5 minutes
                break;
            default:
                this->out01.low();
                break;
            }
        }

        void loop()
        {
            // let the websockets client check for incoming messages
            if (this->wsclient.available())
            {
                this->wsclient.poll();
            }
            else
            {
                this->connectWS();
            }

            if (this->input01.changed())
            {

                if (this->input01.status == ocs::input::Status::ALARM)
                {
                    this->setAlarm(ocs::AlarmType::EMERGENCY);
                }

                DynamicJsonDocument doc(256);
                doc["data"] = this->input01.toJson();
                doc["event"] = "SZ"; // Change status GPIO

                this->wssend(doc);
            }
        }

        void wssend(DynamicJsonDocument json_doc)
        {
            String outputJson = "";
            serializeJson(json_doc, outputJson);
            Serial.println(outputJson);
            this->wsclient.send(outputJson);
        }

        void setup(String websocketHost, String deviceId, int gpio_in_01, int gpio_out_01, const char *ssl_ca_cert)
        {
            this->websocketHost = websocketHost;
            this->deviceId = deviceId;
            this->wsclient.setCACert(ssl_ca_cert);
            this->input01.setup(gpio_in_01);
            this->out01.setup(gpio_out_01);

            //   run callback when messages are received
            this->wsclient.onMessage([&](WebsocketsMessage message) -> void
                                     {
                                         Serial.print("Got Message: ");
                                         Serial.println(message.data());

                                         StaticJsonDocument<100> doc;
                                         DeserializationError error = deserializeJson(doc, message.data());

                                         // Test if parsing succeeds.
                                         if (error)
                                         {
                                             Serial.print(F("deserializeJson() failed: "));
                                             Serial.println(error.f_str());
                                         }
                                         else
                                         {

                                             // Serial.println(doc["command"]);
                                             uint command = doc["command"];

                                             switch (command)
                                             {
                                             case 1: // Set Alarm
                                             {
                                                Serial.println("SET ALARM...");
                                                 Serial.println(doc["alarm_type"].as<const char *>());
                                                 ocs::AlarmType alarm_type = doc["alarm_type"];
                                                 this->setAlarm(alarm_type);
                                             }
                                             break;
                                             case 1000:
                                                 Serial.println("Reasignar UUID");
                                                 String dev_Id = doc["deviceid"];
                                                 deviceId = dev_Id;

                                                  pref.begin("deviceid", false);
                                                  pref.putString("deviceid", dev_Id);
                                                  pref.end();

                                                 break;
                                             }
                                         } });

            // run callback when events are occuring
            this->wsclient.onEvent([](WebsocketsEvent event, String data) -> void
                                   {
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        Serial.println("Connnection Opened");
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection Closed");
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        Serial.println("Got a Ping!");
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        Serial.println("Got a Pong!");
    } });
        }

        void connectWS()
        {

            String urlws = this->websocketHost + "?device=" + this->deviceId;
            Serial.println("try connect ws: " + urlws);

            bool connected = this->wsclient.connect(urlws);
            if (connected)
            {
                Serial.println("WS Connected");
                this->wsclient.send("{\"request\": 1000}");
                this->wsclient.ping();
            }
            else
            {
                Serial.println("WS Not Connected!");
                delay(1500);
            }
        }

    private:
        WebsocketsClient wsclient;
        edwinspire::OutputPin out01;
    };

}
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <Preferences.h>
#include "Outputpin.cpp"

using namespace websockets;

namespace ocs
{

    struct WifiParams
    {
        String ssid;
        String pwd;
    };

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

        private:
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
                return this->value;
            }
            void setup(int gpio_input)
            {
                this->gpio = gpio_input;
                pinMode(this->gpio, INPUT);
            }

            bool changed()
            {
                bool Change = false;

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
                        // Serial.println("ALARMA");
                    }
                    else if (this->value >= upper_threshold)
                    {
                        this->status = Status::TROUBLE;
                        // Serial.println("PROBLEMA");
                    }
                    else
                    {
                        this->status = Status::NORMAL;
                        // Serial.println("NORMAL");
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
        String websocketHost;
        String deviceId;
        input::Input input01;
        WifiParams listSSID[3];

        bool deleteSSID(String ssid)
        {
            bool r = false;
            int size = sizeof(this->listSSID) / sizeof(this->listSSID[0]);
            for (byte i = 0; i < size; i = i + 1)
            {

                if (this->listSSID[i].ssid == ssid)
                {
                    WifiParams wp;
                    wp.ssid = "";
                    wp.pwd = "";

                    listSSID[i] = wp;
                    this->removePreference(ssid);
                    r = true;
                    break;
                }
            }
            return r;
        }

        bool setSSID(String ssid, String pwd)
        {
            bool r = false;

            int size = sizeof(this->listSSID) / sizeof(this->listSSID[0]);

            // Busca el ssid para actualizarlo
            for (byte i = 0; i < size; i = i + 1)
            {

                if (this->listSSID[i].ssid == ssid)
                {
                    WifiParams wp;
                    wp.ssid = ssid;
                    wp.pwd = pwd;

                    listSSID[i] = wp;
                    r = true;
                    this->setPreference(ssid, pwd);
                    break;
                }
            }

            // si no ha sido encontrado entonces lo inserta en un espacio libre
            if (!r)
            {
                Serial.println("ssid no encontrada, se intenta ingresarlo");
                for (byte i = 0; i < size; i = i + 1)
                {

                    if (this->listSSID[i].ssid.length() < 5)
                    {
                        WifiParams wp;
                        wp.ssid = ssid;
                        wp.pwd = pwd;

                        listSSID[i] = wp;
                        r = true;
                        this->setPreference(ssid, pwd);
                        break;
                    }
                }
            }

            return r;
        }

        void setAlarm(ocs::AlarmType at)
        {

            Serial.println("Entra en setAlarm");
            this->out01.low();

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
                if (millis() - this->last_time_ws_ping > this->intervalWsPing)
                {
                    this->wsclient.ping();
                    this->last_time_ws_ping = millis();
                }
            }
            else
            {
                this->connectWS();
            }

            this->out01.loop();

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

        void setup(int gpio_in_01, int gpio_out_01, const char *ssl_ca_cert)
        {
            this->websocketHost = this->getPreference("websocketHost", "wss://open-community-safety.herokuapp.com/ws/device");
            this->deviceId = this->getPreference("deviceid", "00a0aa00-aa00-0000-0000-000000000000");
            this->setup(this->websocketHost, this->deviceId, gpio_in_01, gpio_out_01, ssl_ca_cert);
        }

        String getWebSocketHost()
        {
            return this->getPreference("websocketHost", "wss://open-community-safety.herokuapp.com/ws/device");
        }

        void setWebSocketHost(String websocketHost)
        {
            this->setPreference("websocketHost", websocketHost);
        }

        String getdeviceId()
        {
            return this->getPreference("deviceid", "00a0aa00-aa00-0000-0000-000000000000");
        }

        void setdeviceId(String deviceId)
        {
            this->setPreference("deviceid", deviceId);
        }

        void setup(String websocketHost, String deviceId, int gpio_in_01, int gpio_out_01, const char *ssl_ca_cert)
        {

            if (websocketHost.length() < 1)
            {
                websocketHost = this->getWebSocketHost();
            }
            else
            {
                this->setWebSocketHost(websocketHost);
            }

            if (deviceId.length() < 1)
            {
                deviceId = this->getdeviceId();
            }
            else
            {
                this->setdeviceId(deviceId);
            }

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
            this->wsclient.onEvent([&](WebsocketsEvent event, String data) -> void
                                   {
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        Serial.println("Connnection Opened");
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection Closed");
        Serial.print(this->wsclient.getCloseReason());
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
            //  Serial.printf("WebSocket available %d\n", this->wsclient.available());
            delay(500);
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
        Preferences pref;
        unsigned long intervalWsPing = 50000;
        unsigned long last_time_ws_ping = 0;

        String getPreference(String key, String default_value)
        {
            pref.begin(key.c_str(), true);
            String value = pref.getString(key.c_str(), default_value.c_str());
            pref.end();
            return value;
        }

        bool removePreference(String key)
        {
            pref.begin(key.c_str(), false);
            bool r = pref.remove(key.c_str());
            pref.end();
            return r;
        }

        void setPreference(String key, String value)
        {
            pref.begin(key.c_str(), false);
            pref.putString(key.c_str(), value);
            pref.end();
        }
    };
}
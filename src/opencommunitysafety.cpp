#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>

#include "Outputpin.cpp"

#ifdef ESP32
#include <Preferences.h>
#elif defined(ESP8266)
#include <EEPROM.h>
//#define EEPROM_SIZE 1024
#endif

using namespace websockets;

namespace ocs
{
    const byte MAX_SSID_WIFI = 4;
    const unsigned int EEPROM_SIZE_DEFAULT = 1024;
    const String default_websocketHost = "wss://open-community-safety.herokuapp.com/ws/device";
    const String default_deviceid = "00a0aa00-aa00-0000-0000-000000000000";
    const byte MAX_OUTPUTS = 1;

    struct WifiParams
    {
        String ssid;
        String pwd;
    };

    const WifiParams default_wifi = {.ssid = "accesspoint", .pwd = "123456@qwert"};

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

    class LocalStore
    {

    private:
        //      unsigned long last_time;
        //    unsigned long interval = 500;

    public:
        DynamicJsonDocument data;

        LocalStore() : data(ocs::EEPROM_SIZE_DEFAULT) {}

        static DynamicJsonDocument read()
        {

            char eeprom_data[ocs::EEPROM_SIZE_DEFAULT];
            EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT); // tamaño maximo 4096 bytes
                                                    //       Serial.print("3");
                                                    //     Serial.print("Read EEPROM " + String(ocs::EEPROM_SIZE_DEFAULT));
                                                    //    Serial.print("4");

            unsigned int i = 0;
            //   Serial.print("5");
            while (i < ocs::EEPROM_SIZE_DEFAULT)
            {
                eeprom_data[i] = EEPROM.read(i);
                // Serial.println(String(i) + " - " + String(eeprom_data[i]));
                i++;
            }
            // Serial.print("6");
            EEPROM.end();
            //  Serial.print("7");
            Serial.println(eeprom_data);
            DynamicJsonDocument doc(ocs::EEPROM_SIZE_DEFAULT);
            DeserializationError err = deserializeJson(doc, eeprom_data);
            // Serial.print("8");
            if (err)
            {
                Serial.print(F("read deserializeJson() failed: "));
                Serial.println(err.c_str());

                EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT); // tamaño maximo 4096 bytes

                unsigned int i = 0;
                while (i < ocs::EEPROM_SIZE_DEFAULT)
                {
                    EEPROM.write(i, 255);
                    i++;
                }
                EEPROM.commit();
                EEPROM.end();
            }

            return doc;
        }

        /*
                void loop()
                {

                    if (millis() - this->last_time > this->interval)
                    {
                        if (this->pendingChangesToSave)
                        {
                            this->save();
                        }
                    }
                }
        */

#ifdef ESP32

        bool removePreference(String key)
        {
            pref.begin(key.c_str(), false);
            bool r = pref.remove(key.c_str());
            pref.end();
            return r;
        }

#elif defined(ESP8266)

        bool removePreference(String key)
        {
            bool r = false;
            return r;
        }
#endif

#ifdef ESP32
        bool setPreference(String key, String value)
        {
            Serial.println("setPreference: " + key + " = " + value);
            //  Serial.println("setPreference: " + key);
            bool r = false;
            if (key.length() > 0)
            {
                pref.begin(key.c_str(), false);
                if (pref.putString(key.c_str(), value) > 0)
                {
                    r = true;
                }
                pref.end();
            }
            return r;
        }

#elif defined(ESP8266)

        void refreshData()
        {
            //            this->data = this->readData();
        }

        static bool save(DynamicJsonDocument doc)
        {
            bool r = false;
            Serial.println("--> Save - Cambios pendientes ");
            String data_serialized = "";
            serializeJson(doc, data_serialized);
            Serial.println(data_serialized);
            EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT);

            // Length (with one extra character for the null terminator)
            unsigned int str_len = data_serialized.length() + 1;

            // Prepare the character array (the buffer)
            char char_array[str_len];

            // Copy it over
            data_serialized.toCharArray(char_array, str_len);

            unsigned int i = 0;
            while (i < str_len)
            {
                EEPROM.write(i, char_array[i]);
                i++;
            }

            r = EEPROM.commit();
            EEPROM.end();

            /*
                        if (this->pendingChangesToSave)
                        {
                            Serial.println("--> Save - Cambios pendientes ");
                            String data_serialized = "";
                            serializeJson(this->data, data_serialized);
                            Serial.println(data_serialized);
                            EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT);

                            // Length (with one extra character for the null terminator)
                            unsigned int str_len = data_serialized.length() + 1;

                            // Prepare the character array (the buffer)
                            char char_array[str_len];

                            // Copy it over
                            data_serialized.toCharArray(char_array, str_len);

                            unsigned int i = 0;
                            while (i < str_len)
                            {
                                EEPROM.write(i, char_array[i]);
                                i++;
                            }

                            EEPROM.commit();
                            EEPROM.end();
                            //this->pendingChangesToSave = false;
                            Serial.println("--> Save - Fin cambios pendientes ");
                            this->refreshData();
                        }
                        */
            return r;
        }

        bool setPreference(String key, String value)
        {
            Serial.println("setPreference: " + key + " = " + value);
            //  Serial.println("setPreference: " + key);
            bool r = false;
            // https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/examples/eeprom_read/eeprom_read.ino

            if (key.length() > 0)
            {
                Serial.println("setPreference - 1");
                this->refreshData();
                Serial.println("setPreference - 2");
                /*
                if (this->data[key] != value)
                {
                    Serial.println("setPreference - 3");
                    r = true;
                    this->data[key] = value;
                    this->pendingChangesToSave = true;
                    this->save();
                }
                */
            }
            return r;
        }
#endif
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
            //        LocalStore storage
            unsigned int value;

            unsigned long last_time;
            unsigned long interval = 500;
            float zone_threshold = 40;
            Status last_status = Status::UNDEFINED;

        public:
            Status status = Status::UNDEFINED;
            String name = "Physical Button";
            byte gpio = 255;

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
                if (this->gpio != 255)
                {
                    this->value = analogRead(this->gpio); // read the input pin
                }
                else
                {
                    this->value = 0;
                }
                return this->value;
            }
            void setup(byte gpio_input, String name)
            {
                this->name = name;
                this->gpio = gpio_input;

                if (this->name == NULL || this->name.length() < 1)
                {
                    this->name = "Input " + String(this->gpio);
                }

                if (this->gpio != 255)
                {
                    pinMode(this->gpio, INPUT);
                }
            }
            /*
                        bool setName(String name)
                        {
                            this->name = name;
                            return this->setPreference("input_" + String(this->gpio), name);
                        }

                        String getName()
                        {
                            String n = "input_" + String(this->gpio);
                            this->name = this->getPreference(n, "Local GPIO " + String(this->gpio));
                            return this->name;
                        }
                        */

            bool changed()
            {
                bool Change = false;

                if (this->gpio != 255 && millis() - last_time > interval)
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

    //    const byte MAX_SSID = 3;

    struct InputConfig
    {
        String name;
        byte gpio;
    };

    struct outputConfig
    {
        //        edwinspire::OutputPin output;
        byte gpio;
    };

    class Config
    {
    private:
        void setDefault()
        {
            if (this->websocketHost == NULL || this->websocketHost.length() < 6)
            {
                this->websocketHost = ocs::default_websocketHost;
            }

            if (this->deviceId == NULL || this->deviceId.length() < 15)
            {
                this->deviceId = ocs::default_deviceid;
            }

            if (this->latitude == 0 && this->longitude == 0)
            {
                this->latitude = "37.44362";
                this->longitude = "-122.15719";
            }
            //  this->MAX_SSID_WIFI = ocs::MAX_SSID_WIFI;
            this->websocketHostRequest = this->websocketHost + "?device=" + this->deviceId;
        }

    public:
        Config()
        {
            setDefault();
        }

        bool saveLocalStorage()
        {
            this->setDefault();
            return ocs::LocalStore::save(this->toJson());
        }

        void fromLocalStore()
        {
            this->fromJson(LocalStore::read());
        }

        void fromJson(DynamicJsonDocument data)
        {
            this->websocketHost = data["websocketHost"].as<String>();

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {

                if (i == 0)
                {
                    // Setea en primera posición el wifi default
                    this->wifi[i].ssid = ocs::default_wifi.ssid;
                    this->wifi[i].pwd = ocs::default_wifi.pwd;
                }
                else if (!data["wifi"][i]["ssid"].isNull() && data["wifi"][i]["ssid"].as<String>().length() > 5)
                {
                    this->wifi[i].ssid = data["wifi"][i]["ssid"].as<String>();
                    this->wifi[i].pwd = data["wifi"][i]["pwd"].as<String>();
                }
            }

            this->deviceId = data["deviceId"].as<String>();
            this->caCert_fingerPrint = data["caCert_fingerPrint"].as<String>();

            this->input->gpio = data["input"]["gpio"].as<byte>();
            this->output->gpio = data["output"]["gpio"].as<byte>();
            this->setDefault();
        }

        DynamicJsonDocument toJson()
        {
            this->setDefault();
            DynamicJsonDocument doc(2048);
            doc["input01"] = this->input->gpio;
            doc["deviceId"] = this->deviceId;
            doc["websocketHost"] = this->websocketHost;
            doc["latitude"] = this->latitude;
            doc["longitude"] = this->longitude;
            doc["MAX_SSID_WIFI"] = ocs::MAX_SSID_WIFI;

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {
                if (i == 0)
                {
                    // Posición 0 siempre va la el SSID default
                    doc["wifi"][i]["ssid"] = ocs::default_wifi.ssid;
                    doc["wifi"][i]["pwd"] = ocs::default_wifi.ssid;
                }
                else
                {
                    doc["wifi"][i]["ssid"] = this->wifi[i].ssid;
                    doc["wifi"][i]["pwd"] = this->wifi[i].pwd;
                }
            }

            doc["caCert_fingerPrint"] = this->caCert_fingerPrint;

            return doc;
        }
        String websocketHost;
        WifiParams wifi[ocs::MAX_SSID_WIFI];
        InputConfig input[3];
        outputConfig output[ocs::MAX_OUTPUTS];
        String deviceId;
        String caCert_fingerPrint;
        String latitude;
        String longitude;
        // byte MAX_SSID_WIFI;
        String websocketHostRequest;
    };

    class OpenCommunitySafety
    {

    public:
        typedef Config (*delegateSetup)();
        typedef void (*delegateSaveConfig)(ocs::Config);

        Config ConfigParameter;
        delegateSaveConfig hsave;
        // String websocketHost;
        //  String deviceId;
        input::Input input01;

        void setAlarm(ocs::AlarmType at)
        {

            Serial.println("Entra en setAlarm");

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->outputs[i].low();

                switch (at)
                {
                case ocs::AlarmType::TEST:
                    this->outputs[i].blink(2000, 1500, 0, 4); // 1500 milliseconds ON, 2000 milliseconds OFF, start immidiately, blink 10 times (5 times OFF->ON, 5 times ON->OFF, interleavedly)
                    break;
                case ocs::AlarmType::EMERGENCY:
                    this->outputs[i].blink(500, 5 * 60 * 1000, 0, 2); // 5 minutes
                    break;
                case ocs::AlarmType::PULSE:
                    this->outputs[i].blink(3000, 4000, 0, 50); // 5 minutes
                    break;
                default:
                    this->outputs[i].low();
                    break;
                }
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

            //            this->out01.loop();

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->outputs[i].loop();
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

        bool setFromJson(DynamicJsonDocument json)
        {
            this->ConfigParameter.fromJson(json);
            return this->ConfigParameter.saveLocalStorage();
        }

        DynamicJsonDocument toJson()
        {
            return this->ConfigParameter.toJson();
        }

        void wssend(DynamicJsonDocument json_doc)
        {
            String outputJson = "";
            serializeJson(json_doc, outputJson);
            Serial.println(outputJson);
            this->wsclient.send(outputJson);
        }

        void setup(delegateSetup handlerSetup)
        {
            this->setup(handlerSetup());
        }

        void setup(delegateSetup handlerSetup, delegateSaveConfig handlerSave)
        {
            this->hsave = handlerSave;
            this->setup(handlerSetup);
        }

        void setup(ocs::Config config)
        {
            this->ConfigParameter = config;

#ifdef ESP32
            this->wsclient.setcaCert_fingerPrint_fingerPrint_fingerPrint(ssl_ca_cert);
#elif defined(ESP8266)
            // Serial.println(this->ConfigParameter.caCert_fingerPrint);
            if (this->ConfigParameter.websocketHost.startsWith("wss"))
            {
                this->wsclient.setFingerprint(this->ConfigParameter.caCert_fingerPrint.c_str());
            }
#endif

            this->input01.setup(this->ConfigParameter.input->gpio, this->ConfigParameter.input->name);

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {

                if (this->ConfigParameter.output[i].gpio != 255)
                {
                    this->outputs[i].setup(this->ConfigParameter.output[i].gpio);
                }
            }

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
                                             unsigned int command = doc["command"];

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
                                                 //String dev_Id = doc["deviceid"];
                                                 this->ConfigParameter.deviceId = doc["deviceId"].as<String>();
                                                 Serial.println("Reasignada UUID");
                                                 //this->setDefaultValues();
                                                 Serial.println("seteada UUID");

this->hsave(this->ConfigParameter);

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

            Serial.println(this->ConfigParameter.websocketHostRequest);
            delay(500);
            bool connected = this->wsclient.connect(this->ConfigParameter.websocketHostRequest);
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
        // unsigned long last_time_save_config;
        // unsigned long interval_save_config = 5000;

        WebsocketsClient wsclient;
        edwinspire::OutputPin outputs[ocs::MAX_OUTPUTS];
        unsigned long intervalWsPing = 50000;
        unsigned long last_time_ws_ping = 0;
    };
}
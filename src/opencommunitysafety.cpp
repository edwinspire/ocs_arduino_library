#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Outputpin.cpp"
#include <EEPROM.h>
#include "WebServer.cpp"
#include "AsyncJson.h"

using namespace websockets;

ocs::WebAdmin ocsWebAdmin(80);

namespace ocs
{

    struct InputConfig
    {
        String name;
        byte gpio = 255;
        bool enabled = false;
    };

    struct outputConfig
    {
        //        edwinspire::OutputPin output;
        String name;
        byte gpio = 255;
        bool enabled = false;
    };

    const byte MAX_OUTPUTS = 1;
    const byte MAX_INPUTS = 1;

    /*
    #ifdef ESP32

    #if defined(ESP_BOARD)

    #if ESP_BOARD == "ESP32_DEVKIT_V1_DOIT"
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;
        ocs::outputConfig default_outputs[MAX_OUTPUTS] = {{.gpio = 21, .name = "GPIO 21"}};
        ocs::outputConfig default_inputs[MAX_INPUTS] = {{.gpio = 23, .name = "GPIO 23"}};
    #elif ESP_BOARD == "ESP32_DEVKITC_V4"
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;
        ocs::outputConfig default_outputs[MAX_OUTPUTS] = {{.gpio = 21, .name = "GPIO 21"}};
        ocs::outputConfig default_inputs[MAX_INPUTS] = {{.gpio = 23, .name = "GPIO 23"}};
    #else
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;
        ocs::outputConfig default_outputs[MAX_OUTPUTS] = {{.gpio = 21, .name = "GPIO 21"}};
        ocs::outputConfig default_inputs[MAX_INPUTS] = {{.gpio = 23, .name = "GPIO 23"}};
    #endif

    #else
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;

        outputConfig *default_outputs()
        {
            static outputConfig r[MAX_OUTPUTS];
            r[0] = { .gpio = 21, .name = "GPIO 21"}

            return r;
        };
        InputConfig *default_inputs()
        {
            static InputConfig r[MAX_INPUTS];
            r[0] = {.gpio = 23, .name = "GPIO 23"};

            return r;
        };
    #endif

    #elif defined(ESP8266)

    #if defined(ESP_BOARD)

    #if ESP_BOARD == "ESP-01S"
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;

    #elif ESP_BOARD == "ESP8266"
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;
    #else
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;
    #endif

    #else
        // No ha definido el modelo de placa
        const byte MAX_OUTPUTS = 1;
        const byte MAX_INPUTS = 1;
    //    edwinspire::OutputPin outputs[ocs::MAX_OUTPUTS] = {{}, };
    #endif

    #endif
    */

    const byte MAX_SSID_WIFI = 4;
    const unsigned int EEPROM_SIZE_DEFAULT = 2560;
    const String default_websocketHost = "wss://open-community-safety.herokuapp.com/ws/device";
    const String default_deviceid = "00a0aa00-aa00-0000-0000-000000000000";

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
                // char default_json[2] = "{}";
                unsigned int i = 0;
                while (i < ocs::EEPROM_SIZE_DEFAULT)
                {
                    if (i == 0)
                    {
                        EEPROM.write(i, '{');
                    }
                    else if (i == 1)
                    {
                        EEPROM.write(i, '}');
                    }
                    else
                    {
                        EEPROM.write(i, 255);
                    }

                    i++;
                }
                EEPROM.commit();
                EEPROM.end();
            }

            return doc;
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

            return r;
        }
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
            bool enabled = false;
            Status status = Status::UNDEFINED;
            String name = "Physical Button";
            byte gpio = 255;

            DynamicJsonDocument toJson()
            {
                DynamicJsonDocument doc(256);
                doc["gpio"] = this->gpio;
                doc["status"] = this->status;
                doc["name"] = this->name;
                doc["enabled"] = this->enabled;
                return doc;
            }

            int getvalue()
            {
                this->value = 0;
                if (this->enabled)
                {
#ifdef ESP32
                    if (this->gpio != 0)
                    {
                        this->value = analogRead(this->gpio); // read the input pin
                    }
                    else
                    {
                        Serial.println(F("En ESP32 no está permitido usar el GPIO como entrada."));
                    }
#elif defined(ESP8266)
                    this->value = analogRead(this->gpio); // read the input pin
#endif
                }
                return this->value;
            }
            void setup(byte gpio_input, String name, bool enabled)
            {
                Serial.print(F("Setup Input => "));
                Serial.println(gpio_input);
                this->name = name;
                this->gpio = gpio_input;
                this->enabled = enabled;

                if (this->name == NULL || this->name.length() < 1)
                {
                    this->name = "Input " + String(this->gpio);
                }

                if (this->enabled)
                {

#ifdef ESP32
                    pinMode(0, OUTPUT);
                    if (this->gpio != 0)
                    {
                        pinMode(this->gpio, INPUT);
                    }
                    else
                    {
                        Serial.println(F("En ESP32 no está permitido usar el GPIO como entrada."));
                    }
#elif defined(ESP8266)
                    pinMode(this->gpio, INPUT);
#endif
                }
            }

            bool changed()
            {
                bool Change = false;

                if (this->enabled && millis() - last_time > interval)
                {

                    this->last_time = millis();
                    float center = 4096 / 2;
                    float upper_threshold = center + ((this->zone_threshold / 100) * center);
                    float lower_threshold = center - ((this->zone_threshold / 100) * center);
                    this->getvalue();

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

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                this->input[i].gpio = data["input"][i]["gpio"];
                this->input[i].name = data["input"][i]["name"].as<String>();
                this->input[i].enabled = data["input"][i]["enabled"].as<boolean>();
            }

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->output[i].gpio = data["output"][i]["gpio"];
                this->output[i].name = data["output"][i]["name"].as<String>();
                this->output[i].enabled = data["output"][i]["enabled"].as<boolean>();
            }

            this->deviceId = data["deviceId"].as<String>();
            this->caCert_fingerPrint = data["caCert_fingerPrint"].as<String>();

            // this->input->gpio = data["input"]["gpio"].as<byte>();
            // this->output->gpio = data["output"]["gpio"].as<byte>();
            this->setDefault();
        }

        DynamicJsonDocument toJson()
        {
            this->setDefault();
            DynamicJsonDocument doc(4096);
            // doc["input01"] = this->input->gpio;
            doc["deviceId"] = this->deviceId;
            doc["websocketHost"] = this->websocketHost;
            doc["latitude"] = this->latitude;
            doc["longitude"] = this->longitude;
            doc["MAX_SSID_WIFI"] = ocs::MAX_SSID_WIFI;
            doc["ChipModel"] = ESP.getChipModel();
            doc["EfuseMac"] = String(ESP.getEfuseMac(), HEX);
            doc["ChipRevision"] = ESP.getChipRevision();

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

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                doc["input"][i]["gpio"] = this->input[i].gpio;
                doc["input"][i]["name"] = this->input[i].name;
                doc["input"][i]["enabled"] = this->input[i].enabled;
            }

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                doc["output"][i]["gpio"] = this->output[i].gpio;
                doc["output"][i]["name"] = this->output[i].name;
                doc["output"][i]["enabled"] = this->output[i].enabled;
            }

            doc["caCert_fingerPrint"] = this->caCert_fingerPrint;

            return doc;
        }
        String websocketHost;
        WifiParams wifi[ocs::MAX_SSID_WIFI];
        InputConfig input[ocs::MAX_INPUTS];
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
        //    typedef Config (*delegateSetup)();
        //    typedef void (*delegateSaveConfig)(ocs::Config);

        Config ConfigParameter;
        //  delegateSaveConfig hsave;
        // String websocketHost;
        //  String deviceId;
        // input::Input input01;

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

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                if (this->inputs[i].changed())
                {

                    if (this->inputs[i].status == ocs::input::Status::ALARM)
                    {
                        this->setAlarm(ocs::AlarmType::EMERGENCY);
                    }

                    DynamicJsonDocument doc(256);
                    doc["data"] = this->inputs[i].toJson();
                    doc["event"] = "SZ"; // Change status GPIO
                    doc["latitude"] = this->ConfigParameter.latitude; 
                    doc["longitude"] = this->ConfigParameter.longitude; 
                    doc["deviceId"] = this->ConfigParameter.deviceId; 
                    this->wssend(doc);
                }
            }
        }

        DynamicJsonDocument setFromJson(DynamicJsonDocument json)
        {
            this->ConfigParameter.fromJson(json);
            this->ConfigParameter.saveLocalStorage();
            return this->ConfigParameter.toJson();
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

        void begin()
        {
            ocsWebAdmin.begin();
        }

        void setup()
        {
            ocs::Config c;
            c.fromLocalStore();
            this->setup(c);
        }

        void setup(ocs::Config config)
        {
            Serial.println(F("Setup OCS"));
            ocsWebAdmin.on("/getsettings", [&](AsyncWebServerRequest *request)
                           {
            String outputJson = "";
            serializeJson(this->toJson(), outputJson);
            Serial.println(outputJson);
            request->send(200, F("application/json"), outputJson); });

            ocsWebAdmin.on("/reboot", [&](AsyncWebServerRequest *request)
                           {  
            request->send(200, F("application/json"), "{\"ESP\": \"Restarting...\"}"); 
            ESP.restart(); });

            // ocsWebAdmin.on("/setsettings", HTTP_POST, onRequest, onUpload, setSettings);
            ocsWebAdmin.addHandler(this->handlerBody); // Para poder leer el body enviado en el request

            // ocsWebAdmin.onNotFound(notFound);
            ocsWebAdmin.setup();

            this->ConfigParameter = config;

            if (this->ConfigParameter.websocketHost.startsWith("wss"))
            {

#ifdef ESP32
                this->wsclient.setCACert(this->ConfigParameter.caCert_fingerPrint.c_str());
#elif defined(ESP8266)
                this->wsclient.setFingerprint(this->ConfigParameter.caCert_fingerPrint.c_str());
#endif
            }

            //            this->input01.setup(this->ConfigParameter.input->gpio, this->ConfigParameter.input->name);
            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                this->inputs[i].setup(this->ConfigParameter.input[i].gpio, this->ConfigParameter.input[i].name, this->ConfigParameter.input[i].enabled);
            }

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {

                if (this->ConfigParameter.output[i].gpio != 255)
                {
                    this->outputs[i].setup(this->ConfigParameter.output[i].gpio, this->ConfigParameter.output[i].enabled);
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
                                                Serial.println(F("SET ALARM..."));
                                                 Serial.println(doc["alarm_type"].as<const char *>());
                                                 ocs::AlarmType alarm_type = doc["alarm_type"];
                                                 this->setAlarm(alarm_type);
                                             }
                                             break;
                                             case 1000:
//                                                 Serial.println("Reasignar UUID");
                                                 //String dev_Id = doc["deviceid"];
                                                 this->ConfigParameter.deviceId = doc["deviceId"].as<String>();
 //                                                Serial.println("Reasignada UUID");
                                                 //this->setDefaultValues();
                                                 Serial.println("seteada UUID");

//this->hsave(this->ConfigParameter);
this->ConfigParameter.saveLocalStorage();

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
        ocs::input::Input inputs[ocs::MAX_INPUTS];
        unsigned long intervalWsPing = 50000;
        unsigned long last_time_ws_ping = 0;

        AsyncCallbackJsonWebHandler *handlerBody = new AsyncCallbackJsonWebHandler("/setsettings", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                   {
String r = "";
 serializeJson(this->setFromJson(json), r);

    request->send(200, F("application/json"), r); });
    };
}
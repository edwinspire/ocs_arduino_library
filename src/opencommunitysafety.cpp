#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Outputpin.cpp"
#include "WebServer.cpp"
#include "AsyncJson.h"
#include "Inputpin.cpp"
#include "LocalStore.cpp"

using namespace websockets;

ocs::WebAdmin ocsWebAdmin(80);

namespace ocs
{

    const byte MAX_SSID_WIFI = 4;
    const String default_websocketHost = "wss://open-community-safety.herokuapp.com/ws/device";
    const String default_deviceid = "00a0aa00-aa00-0000-0000-000000000000";
    //    const byte MAX_TELEGRAM_GROUPS = 3;
    const byte MAX_OUTPUTS = 1;
    const byte MAX_INPUTS = 1;

    struct telegramGroupConfig
    {
        String id = "";
        bool enabled = false;
        String name = "";
        DynamicJsonDocument toJson()
        {
            if (this->name == NULL)
            {
                this->name = "";
            }

            if (this->id == NULL)
            {
                this->id = "";
            }

            DynamicJsonDocument doc(128);
            doc["id"] = this->id;
            doc["enabled"] = this->enabled;
            doc["name"] = this->name;

            return doc;
        }
    };

    struct outputConfig
    {
        //        edwinspire::OutputPin output;
        String name = "";
        byte gpio = 255;
        bool enabled = false;

        void fromJson(DynamicJsonDocument data)
        {

            this->enabled = data["enabled"].as<boolean>();
            this->name = data["name"].as<String>();
            this->gpio = data["gpio"].as<byte>();
        }

        DynamicJsonDocument toJson()
        {
            DynamicJsonDocument doc(128);
            doc["gpio"] = this->gpio;
            doc["enabled"] = this->enabled;
            doc["name"] = this->name;
            return doc;
        }
    };

    struct WifiParams
    {
        String ssid;
        String pwd;

        DynamicJsonDocument toJson()
        {
            DynamicJsonDocument doc(32);
            doc["ssid"] = this->ssid;
            doc["pwd"] = this->pwd;
            return doc;
        }
    };

    const WifiParams default_wifi = {.ssid = "accesspoint", .pwd = "123456@qwert"};

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
            this->websocketHostRequest = this->websocketHost + "?deviceId=" + this->deviceId;
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

            // Serial.println("fromJson >>>> ");
            // serializeJsonPretty(data, Serial);

            this->websocketHost = data["wsHost"].as<String>();

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {

                if (i == 0)
                {
                    // Setea en primera posición el wifi default
                    this->wifi[i].ssid = ocs::default_wifi.ssid;
                    this->wifi[i].pwd = ocs::default_wifi.pwd;
                }
                else if (!data["wf"][i]["ssid"].isNull() && data["wf"][i]["ssid"].as<String>().length() > 5)
                {
                    this->wifi[i].ssid = data["wf"][i]["ssid"].as<String>();
                    this->wifi[i].pwd = data["wf"][i]["pwd"].as<String>();
                }
            }

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                this->input[i].fromJson(data["i"][i]);
            }

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->output[i].fromJson(data["o"][i]);
            }

            /*
                        for (byte i = 0; i < ocs::MAX_TELEGRAM_GROUPS; i = i + 1)
                        {
                            this->telegramGroups[i].enabled = data["tg"][i]["enabled"].as<boolean>();
                            this->telegramGroups[i].id = data["tg"][i]["id"].as<String>();
                            this->telegramGroups[i].name = data["tg"][i]["name"].as<String>();

                            if (this->telegramGroups[i].id == NULL)
                            {
                                this->telegramGroups[i].id = "";
                            }

                            if (this->telegramGroups[i].name == NULL)
                            {
                                this->telegramGroups[i].name = "";
                            }
                        }
                        */

            this->deviceId = data["deviceId"].as<String>();
            this->caCert_fingerPrint = data["cfp"].as<String>();
            this->allowActivationByGeolocation = data["acbgl"].as<boolean>();

            this->setDefault();
        }

        DynamicJsonDocument toJson()
        {
            this->setDefault();
            DynamicJsonDocument doc(4096);
            // doc["input01"] = this->input->gpio;
            doc["deviceId"] = this->deviceId;
            doc["wsHost"] = this->websocketHost;
            doc["latitude"] = this->latitude;
            doc["longitude"] = this->longitude;
            doc["MAX_SSID_WIFI"] = ocs::MAX_SSID_WIFI;

#ifdef ESP32
            doc["ChipModel"] = ESP.getChipModel();
            doc["EfuseMac"] = String(ESP.getEfuseMac(), HEX);
            doc["ChipRevision"] = ESP.getChipRevision();
#elif defined(ESP8266)
            doc["ChipModel"] = ESP.getChipId();
            doc["EfuseMac"] = "1a2b3c4d";
            doc["ChipRevision"] = ESP.getCoreVersion();

#endif

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {
                if (i == 0)
                {
                    // Posición 0 siempre va la el SSID default
                    doc["wf"][i]["ssid"] = ocs::default_wifi.ssid;
                    doc["wf"][i]["pwd"] = ocs::default_wifi.ssid;
                }
                else
                {
                    doc["wf"][i]["ssid"] = this->wifi[i].ssid;
                    doc["wf"][i]["pwd"] = this->wifi[i].pwd;
                }
            }

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                doc["i"][i] = this->input[i].toJson();
            }

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                doc["o"][i] = this->output[i].toJson();
            }

            /*
                        for (byte i = 0; i < ocs::MAX_TELEGRAM_GROUPS; i = i + 1)
                        {
                            doc["tg"][i] = this->telegramGroups[i].toJson();
                        }
                        */

            doc["cfp"] = this->caCert_fingerPrint;
            doc["acbgl"] = this->allowActivationByGeolocation;

            // Serial.println("Config toJson:");
            // serializeJsonPretty(doc, Serial);
            return doc;
        }
        String websocketHost;
        WifiParams wifi[ocs::MAX_SSID_WIFI];
        input::Configure input[ocs::MAX_INPUTS];
        outputConfig output[ocs::MAX_OUTPUTS];
        // telegramGroupConfig telegramGroups[ocs::MAX_TELEGRAM_GROUPS];
        String deviceId;
        String caCert_fingerPrint;
        String latitude;
        String longitude;
        bool allowActivationByGeolocation = false;
        // byte MAX_SSID_WIFI;
        String websocketHostRequest;
    };

    class OpenCommunitySafety
    {

    public:
        Config ConfigParameter;

        void setAlarm(ocs::input::SirenType at)
        {

            Serial.println("Entra en setAlarm");

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->outputs[i].low();

                switch (at)
                {
                case ocs::input::SirenType::TEST:
                    this->outputs[i].blink(2000, 1500, 0, 4); // 1500 milliseconds ON, 2000 milliseconds OFF, start immidiately, blink 10 times (5 times OFF->ON, 5 times ON->OFF, interleavedly)
                    break;
                case ocs::input::SirenType::CONTINUOUS:
                    this->outputs[i].blink(500, 5 * 60 * 1000, 0, 2); // 5 minutes
                    break;
                case ocs::input::SirenType::PULSING:
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
                        this->setAlarm(ocs::input::SirenType::CONTINUOUS);
                    }

                    DynamicJsonDocument doc(1024);

                    doc["event"]["deviceId"] = this->ConfigParameter.deviceId;
                    doc["event"]["input"] = this->inputs[i].toJson();
                    doc["event"]["latitude"] = this->ConfigParameter.latitude;
                    doc["event"]["longitude"] = this->ConfigParameter.longitude;
                    doc["event"]["allowActivationByGeolocation"] = this->ConfigParameter.allowActivationByGeolocation;

                   // Serial.println(F("this->inputs[i].changed() => "));
                   // serializeJsonPretty(doc, Serial);

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
          //  Serial.println(F("wssend >= "));
          //  serializeJsonPretty(json_doc, Serial);
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

            ocsWebAdmin.on("/getinputsstatus", [&](AsyncWebServerRequest *request)
                           {
            String outputJson = "";
            serializeJson(this->statusInputs(), outputJson);
           // Serial.println(outputJson);
            request->send(200, F("application/json"), outputJson); });

            ocsWebAdmin.on("/reboot", [&](AsyncWebServerRequest *request)
                           {  
            request->send(200, F("application/json"), "{\"ESP\": \"Restarting...\"}"); 
            ESP.restart(); });

            ocsWebAdmin.addHandler(this->handlerBody); // Para poder leer el body enviado en el request
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
                // this->inputs[i].setup(this->ConfigParameter.input[i].gpio, this->ConfigParameter.input[i].name, this->ConfigParameter.input[i].enabled);
                this->inputs[i].setup(this->ConfigParameter.input[i]);
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
                                         Serial.print(F("Got Message: "));
                                         Serial.println(message.data());

                                         DynamicJsonDocument doc(250);
                                         DeserializationError error = deserializeJson(doc, message.data());

                                         // Test if parsing succeeds.
                                         if (error)
                                         {
                                             Serial.print(F("deserializeJson() failed: "));
                                             Serial.println(error.f_str());
                                         }
                                         else
                                         {


if(!doc["command"].isNull()){
this->onwsCommand(doc);
}else if(!doc["request"].isNull()){
this->onwsRequest(doc);
}

                                         } });

            this->wsclient.onEvent([&](WebsocketsEvent event, String data) -> void
                                   {
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        Serial.println(F("Connnection Opened"));
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println(F("Connnection Closed"));
        Serial.print(this->wsclient.getCloseReason());
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        Serial.println(F("Got a Ping!"));
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        Serial.println(F("Got a Pong!"));
    } });
        }

        void connectWS()
        {
            delay(500);
            Serial.println(this->ConfigParameter.websocketHostRequest);
            bool connected = this->wsclient.connect(this->ConfigParameter.websocketHostRequest);

            if (connected)
            {
                Serial.println(F("WS Connected"));
                this->wsclient.send(F("{\"request\": 1000}"));
                this->wsclient.ping();
            }
            else
            {
                Serial.println(F("WS Not Connected!"));
                delay(1500);
            }
        }

        DynamicJsonDocument statusInputs()
        {
            DynamicJsonDocument doc(2048);

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                doc[i] = this->inputs[i].toJson();
            }

            return doc;
        }

    private:
        WebsocketsClient wsclient;
        edwinspire::OutputPin outputs[ocs::MAX_OUTPUTS];
        ocs::input::Input inputs[ocs::MAX_INPUTS];
        unsigned long intervalWsPing = 50000;
        unsigned long last_time_ws_ping = 0;

        void onwsCommand(DynamicJsonDocument doc)
        {
            unsigned int command = doc["command"];

            switch (command)
            {
            case 1: // Set Alarm
            {
                Serial.println(F("SET ALARM..."));
                Serial.println(doc["siren_type"].as<const char *>());
                ocs::input::SirenType siren_type = doc["siren_type"];
                this->setAlarm(siren_type);
            }
            break;
            case 1000: // Set deviceId
                this->ConfigParameter.deviceId = doc["deviceId"].as<String>();
                Serial.println(F("seteada UUID"));
                this->ConfigParameter.saveLocalStorage();
                break;
            }
        }

        void onwsRequest(DynamicJsonDocument doc)
        {
            unsigned int command = doc["request"];

            switch (command)
            {
            case 1000: // Requiere datos de configuración
            {
                Serial.println(F("Response configuration ..."));
                DynamicJsonDocument doc(4096);
                doc["response"] = 1000;
                doc["data"] = this->toJson();
                String outputJson = "";
                serializeJson(doc, outputJson);
                this->wsclient.send(outputJson);
            }
            break;
            }
        }

        AsyncCallbackJsonWebHandler *handlerBody = new AsyncCallbackJsonWebHandler("/setsettings", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                   {
String r = "";
 serializeJson(this->setFromJson(json), r);
    request->send(200, F("application/json"), r); });
    };
}
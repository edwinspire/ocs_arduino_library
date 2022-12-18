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
    const byte MAX_INPUTS = 6;

#ifdef ESP32
    const unsigned int JSON_MAX_SIZE = 4096;
#elif defined(ESP8266)
    const unsigned int JSON_MAX_SIZE = 2048;
#endif

    struct outputConfig
    {
        //        edwinspire::OutputPin output;
        String name = "";
        byte gpio = 255;
        bool enabled = false;

        void fromJson(const DynamicJsonDocument &data)
        {
            this->enabled = data[F("enabled")].as<boolean>();
            this->name = data[F("name")].as<String>();
            this->gpio = data[F("gpio")].as<byte>();
        }

        StaticJsonDocument<64> toJson()
        {
            StaticJsonDocument<64> doc;
            doc[F("gpio")] = this->gpio;
            doc[F("enabled")] = this->enabled;
            doc[F("name")] = this->name;
            return doc;
        }
    };

    struct WifiParams
    {
        String ssid;
        String pwd;

        StaticJsonDocument<32> toJson()
        {
            StaticJsonDocument<32> doc;
            doc[F("ssid")] = this->ssid;
            doc[F("pwd")] = this->pwd;
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
        void setConfigWifi(const DynamicJsonDocument & data)
        {

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {

                if (i == 0)
                {
                    // Setea en primera posición el wifi default
                    this->wifi[i].ssid = ocs::default_wifi.ssid;
                    this->wifi[i].pwd = ocs::default_wifi.pwd;
                }
                else if (!data[i][F("ssid")].isNull() && data[i][F("ssid")].as<String>().length() > 5)
                {
                    this->wifi[i].ssid = data[i][F("ssid")].as<String>();
                    this->wifi[i].pwd = data[i][F("pwd")].as<String>();
                }
            }
        }

        void setConfigInputs(const DynamicJsonDocument & data)
        {

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                this->input[i].fromJson(data[F("i")]);
            }
        }

        void setConfigoutputs(const DynamicJsonDocument & data)
        {
            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->output[i].fromJson(data[i]);
            }
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

        void fromJson(const DynamicJsonDocument & data)
        {

            Serial.println(F("----- Config fromJson -----"));
            // serializeJsonPretty(data, Serial);

            this->websocketHost = data[F("wsHost")].as<String>();
            this->led = data[F("led")].as<byte>();

            this->setConfigWifi(data[F("wf")]);
            this->setConfigInputs(data[F("i")]);
            this->setConfigoutputs(data[F("o")]);

            this->name = data[F("name")].as<String>();
            this->deviceId = data[F("deviceId")].as<String>();
            this->caCert_fingerPrint = data[F("cfp")].as<String>();
            this->allowActivationByGeolocation = data[F("acbgl")].as<boolean>();
            this->latitude = data[F("latitude")].as<String>();
            this->longitude = data[F("latitude")].as<String>();

            this->setDefault();
        }

        void printMemory()
        {

            Serial.print(F("getFreeHeap: "));
            Serial.println(ESP.getFreeHeap());

            // Serial.print(F("getFreeContStack: "));
            // Serial.println(ESP.resetHeap());

            // Serial.print(F("getFreeContStack: "));
            // Serial.println(ESP.getFreeContStack());

            Serial.print(F("getHeapFragmentation: "));
            Serial.println(ESP.getHeapFragmentation());
        }

        DynamicJsonDocument toJson()
        {
            this->setDefault();

            printMemory();
            ESP.resetHeap();
            DynamicJsonDocument doc(4096);
            printMemory();
            doc[F("deviceId")] = this->deviceId;
            doc[F("wsHost")] = this->websocketHost;
            doc[F("latitude")] = this->latitude;
            doc[F("longitude")] = this->longitude;
            doc[F("MAX_SSID_WIFI")] = ocs::MAX_SSID_WIFI;

#ifdef ESP32
            doc[F("ChipModel")] = ESP.getChipModel();
            doc[F("EfuseMac")] = String(ESP.getEfuseMac(), HEX);
            doc[F("ChipRevision")] = ESP.getChipRevision();
#elif defined(ESP8266)
            doc[F("ChipModel")] = String(ESP.getChipId(), HEX);
            doc[F("EfuseMac")] = String(ESP.getFlashChipId(), HEX);
            doc[F("ChipRevision")] = ESP.getCoreVersion();

#endif

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {
                if (i == 0)
                {
                    // Posición 0 siempre va la el SSID default
                    doc[F("wf")][i][F("ssid")] = ocs::default_wifi.ssid;
                    doc[F("wf")][i][F("pwd")] = ocs::default_wifi.ssid;
                }
                else
                {
                    doc[F("wf")][i][F("ssid")] = this->wifi[i].ssid;
                    doc[F("wf")][i][F("pwd")] = this->wifi[i].pwd;
                }
            }

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                doc[F("i")][i] = this->input[i].toJson();
            }

            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                doc[F("o")][i] = this->output[i].toJson();
            }

            doc[F("cfp")] = this->caCert_fingerPrint;
            doc[F("acbgl")] = this->allowActivationByGeolocation;
            doc[F("name")] = this->name;
            doc[F("led")] = this->led;

            //    Serial.println(F("Config toJson"));
            //    serializeJsonPretty(doc, Serial);

            return doc;
        }

        String websocketHost;
        WifiParams wifi[ocs::MAX_SSID_WIFI];
        input::Configure input[ocs::MAX_INPUTS];
        outputConfig output[ocs::MAX_OUTPUTS];
        byte led = 255; // Led GPIO
        String deviceId;
        String caCert_fingerPrint;
        String latitude;
        String longitude;
        String name;
        bool allowActivationByGeolocation = false;
        String websocketHostRequest;
        // StaticJsonDocument<4096> * json;
    };

    // char body_json_data_tmp[4096]  = {};

    class OpenCommunitySafety
    {

    public:
        Config ConfigParameter;

        void setAlarm(ocs::input::SirenType at)
        {

            Serial.println("Entra en setAlarm " + String(at));

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
                ESP.wdtFeed();
                this->wsclient.poll();
                if (millis() - this->last_time_ws_ping > this->intervalWsPing)
                {
                    this->wsclient.ping();
                    this->last_time_ws_ping = millis();
                    this->led.blink(1500, 1000, 500, 10);
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

            this->led.loop();

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                if (this->inputs[i].changed())
                {

                    if (this->inputs[i].status == ocs::input::Status::ALARM)
                    {
                        this->setAlarm(ocs::input::SirenType::CONTINUOUS);
                    }

                    StaticJsonDocument<1024> doc;

                    doc[F("event")][F("deviceId")] = this->ConfigParameter.deviceId;
                    doc[F("event")][F("input")] = this->inputs[i].toJson();
                    doc[F("event")][F("latitude")] = this->ConfigParameter.latitude;
                    doc[F("event")][F("longitude")] = this->ConfigParameter.longitude;
                    doc[F("event")][F("allowActivationByGeolocation")] = this->ConfigParameter.allowActivationByGeolocation;

                    // Serial.println(F("this->inputs[i].changed() => "));
                    // serializeJsonPretty(doc, Serial);

                    this->wssend(doc);
                }
            }
        }

        DynamicJsonDocument setFromJson(DynamicJsonDocument json)
        {

            // serializeJsonPretty(json, Serial);
            if (json != NULL)
            {
                Serial.println(F("Ingresa a setFromJson"));
                this->ConfigParameter.fromJson(json);
                this->ConfigParameter.saveLocalStorage();
            }

            return this->ConfigParameter.toJson();
        }

        DynamicJsonDocument toJson()
        {
            this->ConfigParameter.printMemory();
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

        void reboot()
        {

#ifdef ESP32
            ESP.restart();
#elif defined(ESP8266)
            ESP.wdtDisable();
            wdt_reset();
            ESP.restart();
#endif
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

            this->ConfigParameter = config;

            this->setUpInputs();
            this->setUpOutputs();

            this->led.setup(this->ConfigParameter.led, true);

            this->setUpWebAdmin();
            this->setUpwebSocket();

            this->led.high();
            delay(800);
            this->led.low();
            delay(400);
            this->led.high();
            delay(800);
            this->led.low();
            delay(400);
            this->led.high();
            delay(800);
            this->led.low();
        }

        void
        connectWS()
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
            DynamicJsonDocument doc(ocs::JSON_MAX_SIZE);

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                doc[i] = this->inputs[i].toJson();
            }

            return doc;
        }

    private:
        WebsocketsClient wsclient;
        edwinspire::OutputPin outputs[ocs::MAX_OUTPUTS];
        edwinspire::OutputPin led;
        ocs::input::Input inputs[ocs::MAX_INPUTS];
        unsigned long intervalWsPing = 50000;
        unsigned long last_time_ws_ping = 0;
        // char body_json_data_tmp[4096]  = {};
        // char variableName[4096]  = {};

        // bodyData bdata[3];
        String tmp_buffer_body;

        void setUpWebAdmin()
        {

            ocsWebAdmin.setup();

            ocsWebAdmin.on("/getsettings", [&](AsyncWebServerRequest *request)
                           {
            String outputJson = "";
            serializeJson(this->toJson(), outputJson);
            //Serial.println(outputJson);
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
                               this->reboot(); });

            ocsWebAdmin.on(
                "/setsettings", HTTP_POST, [&](AsyncWebServerRequest *request) {},
                NULL, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                {
  /*
                    Serial.println("Entra al nuevo setsettings ");
                    Serial.println(String(index));
                    Serial.println(String(len));
                    Serial.println(String(total));

                    Serial.println("-------------------------------");
*/

/*
Serial.println(F("<--- ------------ --->"));
Serial.println("Index: "+String(index));
Serial.println("size_t: "+String(len) );
Serial.println("total: "+total);
*/

                    if (index == 0)
                    {
                        /*
                        // Reemplazamos toda la matriz con espacios vacío
                        for (size_t i = 0; i < 4096; i++)
                        {
                            body_json_data_tmp[i] = 32;
                        }
                        */
  //                     Serial.println(F("<--- Es el primer bloque --->"));
                       //this->tmp_buffer_body.reserve(total+64);
                       this->tmp_buffer_body = "";
                    }


//Serial.println(F("++++++++++++++++++++++++++++"));
/*
  Serial.print(F("getFreeHeap: "));
  Serial.println(ESP.getFreeHeap());
*/
  //Serial.print(F("getFreeContStack: "));
  //Serial.println(ESP.getFreeContStack());
/*
  Serial.print(F("getHeapFragmentation: "));
  Serial.println(ESP.getHeapFragmentation());
*/
  //Serial.print(F("getMaxFreeBlockSize: "));
  //Serial.println(ESP.getMaxFreeBlockSize());



                    for (size_t i = 0; i < len; i++)
                    {
                        //body_json_data_tmp[i + index] = data[i];
                        //Serial.print(char(data[i]));
                        this->tmp_buffer_body.concat(char(data[i]));
                        //Serial.print(this->tmp_buffer_body.charAt(i + index));
                    }
                    
//                    Serial.println(F("   xxx"));

                    if (index + len == total)
                    {
                        Serial.println(F("--- Es el último bloque ---"));
                        Serial.println(this->tmp_buffer_body);

                        //DynamicJsonDocument json(this->tmp_buffer_body.length()+8);
                        StaticJsonDocument<ocs::JSON_MAX_SIZE> json;   

                        DeserializationError err = deserializeJson(json, this->tmp_buffer_body);
                        this->tmp_buffer_body.reserve(0);
                        if (err)
                        {
                            Serial.print(F("deserializeJson() failed: "));
                            Serial.println(err.c_str());
                            request->send(500, F("application/json"), "{\"error\":\"" + String(err.c_str()) + "\"}");
                        }
                        else
                        {
                            // serializeJsonPretty(json, Serial);
                            String r = "";
                            serializeJson(this->setFromJson(json), r);
                            request->send(200, F("application/json"), r);
                        }
                    } });

            // ocsWebAdmin.addHandler(this->handlerBody); // Para poder leer el body enviado en el request
        }

        void setUpwebSocket()
        {
            if (this->ConfigParameter.websocketHost.startsWith("wss"))
            {

#ifdef ESP32
                this->wsclient.setCACert(this->ConfigParameter.caCert_fingerPrint.c_str());
#elif defined(ESP8266)
                this->wsclient.setFingerprint(this->ConfigParameter.caCert_fingerPrint.c_str());
#endif
            }

            //   run callback when messages are received
            this->wsclient.onMessage([&](WebsocketsMessage message) -> void
                                     {
                                         Serial.print(F("Got Message: "));
                                         Serial.println(message.data());
                                         this->ledBlinkOnWs();
                                         StaticJsonDocument<250> doc;
                                         DeserializationError error = deserializeJson(doc, message.data());

                                         // Test if parsing succeeds.
                                         if (error)
                                         {
                                             Serial.print(F("deserializeJson() failed: "));
                                             Serial.println(error.f_str());
                                         }
                                         else
                                         {

                                             if (!doc["command"].isNull())
                                             {
                                                 this->onwsCommand(doc);
                                             }
                                             else if (!doc["request"].isNull())
                                             {
                                                 this->onwsRequest(doc);
                                             }
                                         } });

            this->wsclient.onEvent([&](WebsocketsEvent event, String data) -> void
                                   {
                                    this->ledBlinkOnWs();
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

        void setUpInputs()
        {
            //            this->input01.setup(this->ConfigParameter.input->gpio, this->ConfigParameter.input->name);
            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                // this->inputs[i].setup(this->ConfigParameter.input[i].gpio, this->ConfigParameter.input[i].name, this->ConfigParameter.input[i].enabled);
                this->inputs[i].setup(this->ConfigParameter.input[i]);
            }
        }

        void setUpOutputs()
        {
            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {

                if (this->ConfigParameter.output[i].gpio != 255)
                {
                    this->outputs[i].setup(this->ConfigParameter.output[i].gpio, this->ConfigParameter.output[i].enabled);
                    this->outputs[i].low();
                }
            }
        }

        void ledBlinkOnWs()
        {
            this->led.low();
            this->led.blink(1000, 500, 0, 6);
            // this->outputs[i].blink(2000, 1500, 0, 4); // 1500 milliseconds ON, 2000 milliseconds OFF, start immidiately, blink 10 times (5 times OFF->ON, 5 times ON->OFF, interleavedly)
        }

        void onwsCommand(DynamicJsonDocument doc)
        {
            unsigned int command = doc[F("command")];

            switch (command)
            {
            case 1: // Set Alarm
            {
                Serial.println(F("SET ALARM..."));
                // Serial.println(doc["siren_type"].as<const char *>());
                ocs::input::SirenType siren_type = doc[F("siren_type")];
                this->setAlarm(siren_type);
            }
            break;
            case 1000: // Set deviceId
                this->ConfigParameter.deviceId = doc[F("deviceId")].as<String>();
                Serial.println(F("seteada UUID"));
                this->ConfigParameter.saveLocalStorage();
                break;
            }
        }

        void onwsRequest(DynamicJsonDocument doc)
        {
            unsigned int command = doc[F("request")];

            switch (command)
            {
            case 1000: // Requiere datos de configuración
            {
                Serial.println(F("Response configuration ..."));
                StaticJsonDocument<4096> doc;
                doc[F("response")] = 1000;
                doc[F("data")] = this->toJson();
                String outputJson = "";
                serializeJson(doc, outputJson);
                this->wsclient.send(outputJson);
            }
            break;
            }
        }

        /*
                void ongetStatus(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
                {
                    {
                        String outputJson = "";
                        serializeJson(this->statusInputs(), outputJson);
                        // Serial.println(outputJson);
                        request->send(200, F("application/json"), outputJson);
                    }
                }

        */

        /*
                AsyncCallbackJsonWebHandler *handlerBody = new AsyncCallbackJsonWebHandler("/setsettingsX", [&](AsyncWebServerRequest *request, JsonVariant &json)

                                                                                           {
                serializeJsonPretty(json, Serial);
                        String r = "";
                        serializeJson(this->setFromJson(json), r);
                        request->send(200, F("application/json"), r); });
                        */
    };
}
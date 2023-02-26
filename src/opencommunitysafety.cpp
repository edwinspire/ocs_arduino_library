#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include "Outputpin.cpp"
#include "WebServer.cpp"
#include "AsyncJson.h"
#include "Inputpin.cpp"
#include "LocalStore.cpp"
#include <Interval.cpp>

using namespace websockets;

ocs::WebAdmin ocsWebAdmin(80);
AsyncWebSocket ws("/ws");

namespace ocs
{

    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        if (type == WS_EVT_CONNECT)
        {
            Serial.printf("ws => [%s][%u] connect\n", server->url(), client->id());
            client->printf("{\"ClientID\": %u}", client->id());
            client->ping();
        }
        else if (type == WS_EVT_DISCONNECT)
        {
            Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
        }
        else if (type == WS_EVT_ERROR)
        {
            Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
        }
        else if (type == WS_EVT_PONG)
        {
            Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
        }
        else if (type == WS_EVT_DATA)
        {
            AwsFrameInfo *info = (AwsFrameInfo *)arg;
            String msg = "";
            if (info->final && info->index == 0 && info->len == len)
            {
                // the whole message is in a single frame and we got all of it's data
                Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

                if (info->opcode == WS_TEXT)
                {
                    for (size_t i = 0; i < info->len; i++)
                    {
                        msg += (char)data[i];
                    }
                }
                else
                {
                    char buff[3];
                    for (size_t i = 0; i < info->len; i++)
                    {
                        sprintf(buff, "%02x ", (uint8_t)data[i]);
                        msg += buff;
                    }
                }
                Serial.printf("%s\n", msg.c_str());

                if (info->opcode == WS_TEXT)
                    client->text("I got your text message");
                else
                    client->binary("I got your binary message");
            }
            else
            {
                // message is comprised of multiple frames or the frame is split into multiple packets
                if (info->index == 0)
                {
                    if (info->num == 0)
                        Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
                }

                Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

                if (info->opcode == WS_TEXT)
                {
                    for (size_t i = 0; i < len; i++)
                    {
                        msg += (char)data[i];
                    }
                }
                else
                {
                    char buff[3];
                    for (size_t i = 0; i < len; i++)
                    {
                        sprintf(buff, "%02x ", (uint8_t)data[i]);
                        msg += buff;
                    }
                }
                Serial.printf("%s\n", msg.c_str());

                if ((info->index + len) == info->len)
                {
                    Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                    if (info->final)
                    {
                        Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                        if (info->message_opcode == WS_TEXT)
                            client->text("I got your text message");
                        else
                            client->binary("I got your binary message");
                    }
                }
            }
        }
    }

    const byte MAX_SSID_WIFI = 4;
    const String default_websocketHost = "wss://open-community-safety.herokuapp.com/ws/device";
    const String default_deviceid = "00a0aa00-aa00-0000-0000-000000000000";
    //    const byte MAX_TELEGRAM_GROUPS = 3;
    const byte MAX_OUTPUTS = 1;
    const byte MAX_INPUTS = 6;

    const char json_key_wf[3] = "wf";
    const char json_key_input[2] = "i";
    const char json_key_output[2] = "o";
    const char json_key_gpio[5] = "gpio";
    const char json_key_enabled[8] = "enabled";
    const char json_key_name[5] = "name";
    const char json_key_ssid[5] = "ssid";
    const char json_key_pwd[4] = "pwd";
    const char json_key_latitude[9] = "latitude";
    const char json_key_longitude[10] = "longitude";
    const char json_key_event[6] = "event";
    const char json_key_led[4] = "led";
    const char json_key_caCert_fingerPrint[4] = "cfp";
    const char json_key_deviceId[9] = "deviceId";
    const char json_key_mime_json[17] = "application/json";
    const char json_key_acbgl[6] = "acbgl";
    const char json_key_wshost[7] = "wsHost";
    const char json_key_status[7] = "status";
    const char json_key_value[6] = "value";
    const char json_key_geo[4] = "geo";
    const char json_key_info[5] = "info";
    const char json_key_function[3] = "fn";
    ; // json_key_info
    // const char json_key_outputs[8] = "outputs";
    // const char json_key_inputs[7] = "inputs";

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
            this->enabled = data[json_key_enabled].as<boolean>();
            this->name = data[json_key_name].as<String>();
            this->gpio = data[json_key_gpio].as<byte>();
        }

        StaticJsonDocument<64> toJson()
        {
            StaticJsonDocument<64> doc;
            doc[json_key_gpio] = this->gpio;
            doc[json_key_enabled] = this->enabled;
            doc[json_key_name] = this->name;
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
            doc[json_key_ssid] = this->ssid;
            doc[json_key_pwd] = this->pwd;
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

        void setConfigInputs(DynamicJsonDocument data)
        {

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                // serializeJson(data, Serial);
                this->input[i].fromJson(data[i]);
            }
        }

        DynamicJsonDocument getConfigInputs()
        {

            DynamicJsonDocument doc(1024);

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                doc[i] = this->input[i].toJson();
            }
            //  Serial.println("*******INPUTS**********");
            //  serializeJsonPretty(doc, Serial);
            return doc;
        }

        void setConfigoutputs(const DynamicJsonDocument &data)
        {
            this->led = data[json_key_led].as<byte>();
            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->output[i].fromJson(data[json_key_output][i]);
            }
        }

        DynamicJsonDocument getConfigoutputs()
        {
            DynamicJsonDocument doc(512);
            doc[json_key_led] = this->led;
            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                doc[json_key_output][i] = this->output[i].toJson();
            }
            return doc;
        }

        DynamicJsonDocument getConfigWifi()
        {

            DynamicJsonDocument doc(512);

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {
                if (i == 0)
                {
                    // Posición 0 siempre va la el SSID default
                    doc[i][json_key_ssid] = ocs::default_wifi.ssid;
                    doc[i][json_key_pwd] = ocs::default_wifi.pwd;
                }
                else
                {
                    doc[i][json_key_ssid] = this->wifi[i].ssid;
                    doc[i][json_key_pwd] = this->wifi[i].pwd;
                }
            }
            return doc;
        }
        void setConfigGeolocation(const DynamicJsonDocument &data)
        {
            this->latitude = data[json_key_latitude].as<String>();
            this->longitude = data[json_key_longitude].as<String>();
            this->allowActivationByGeolocation = data[json_key_acbgl].as<boolean>();
        }
        void setConfigInfo(const DynamicJsonDocument &data)
        {
            this->deviceId = data[json_key_deviceId].as<String>();
            this->name = data[json_key_name].as<String>();
            this->websocketHost = data[json_key_wshost].as<String>();
        }
        void setConfigWifi(const DynamicJsonDocument &data)
        {

            for (byte i = 0; i < ocs::MAX_SSID_WIFI; i = i + 1)
            {

                if (i == 0)
                {
                    // Setea en primera posición el wifi default
                    this->wifi[i].ssid = ocs::default_wifi.ssid;
                    this->wifi[i].pwd = ocs::default_wifi.pwd;
                }
                else if (!data[i][json_key_ssid].isNull() && data[i][json_key_ssid].as<String>().length() > 5)
                {
                    this->wifi[i].ssid = data[i][json_key_ssid].as<String>();
                    this->wifi[i].pwd = data[i][json_key_pwd].as<String>();
                }
            }
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

        void fromJson(const DynamicJsonDocument &data)
        {

            // Serial.println(F("----- Config fromJson -----"));
            this->setConfigInfo(data[json_key_info]);
            this->setConfigWifi(data[ocs::json_key_wf]);
            this->setConfigInputs(data[ocs::json_key_input]);
            this->setConfigoutputs(data[ocs::json_key_output]);
            this->setConfigGeolocation(data[json_key_geo]);
            this->caCert_fingerPrint = data[json_key_caCert_fingerPrint].as<String>();
            this->setDefault();
            // Serial.println("--------- CONFIG FROM JSON -------------");
            // serializeJsonPretty(data, Serial);
        }

        void printMemory()
        {

            Serial.print(F("getFreeHeap: "));
            Serial.println(ESP.getFreeHeap());

#if defined(ESP8266)
            Serial.print(F("getHeapFragmentation: "));
            Serial.println(ESP.getHeapFragmentation());
#endif
        }

        DynamicJsonDocument getInfo()
        {

            DynamicJsonDocument doc(512);
            doc[json_key_name] = this->name;
            doc[json_key_deviceId] = this->deviceId;
            doc[json_key_wshost] = this->websocketHost;
#ifdef ESP32
            doc[F("ChipModel")] = ESP.getChipModel();
            doc[F("EfuseMac")] = String(ESP.getEfuseMac(), HEX);
            doc[F("ChipRevision")] = ESP.getChipRevision();
#elif defined(ESP8266)
            doc[F("ChipModel")] = String(ESP.getChipId(), HEX);
            doc[F("EfuseMac")] = String(ESP.getFlashChipId(), HEX);
            doc[F("ChipRevision")] = ESP.getCoreVersion();
#endif
            return doc;
        }

        DynamicJsonDocument getGeolocation()
        {
            DynamicJsonDocument doc(128);
            doc[json_key_latitude] = (this->latitude == NULL) ? "0" : this->latitude;
            doc[json_key_longitude] = (this->longitude == NULL) ? "0" : this->longitude;
            doc[json_key_acbgl] = this->allowActivationByGeolocation;
            return doc;
        }

        DynamicJsonDocument toJson()
        {
            this->setDefault();

            // printMemory();

#if defined(ESP8266)
            ESP.resetHeap();
#endif

            DynamicJsonDocument doc(JSON_MAX_SIZE);

            doc[json_key_info] = this->getInfo();
            doc[json_key_geo] = this->getGeolocation();
            doc[F("MAX_SSID_WIFI")] = ocs::MAX_SSID_WIFI;
            doc[ocs::json_key_wf] = this->getConfigWifi();
            doc[ocs::json_key_input] = this->getConfigInputs();
            doc[ocs::json_key_output] = this->getConfigoutputs();
            doc[json_key_caCert_fingerPrint] = this->caCert_fingerPrint;
            // Serial.println("--------- CONFIG TO JSON -------------");
            // serializeJsonPretty(doc, Serial);
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
        String ip;
        String ssid;

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

            interval_ws_status_io.loop();
            interval_check_config_changed.loop();
            interval_ws_connect.loop();
            // let the websockets client check for incoming messages
            if (this->wsclient.available())
            {
                // ESP.wdtFeed();
                this->wsclient.poll();
                interval_ws_ping.loop();
            }

            //  loop for outputs
            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                this->outputs[i].loop();
            }
            // loop for led
            this->led.loop();
            // loop for inputs
            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                if (this->inputs[i].changed())
                {

                    if (this->inputs[i].status == ocs::input::Status::ALARM)
                    {
                        this->setAlarm(ocs::input::SirenType::CONTINUOUS);
                    }

                    DynamicJsonDocument doc(1024);

                    doc[json_key_event][json_key_deviceId] = this->ConfigParameter.deviceId;
                    doc[json_key_event][F("input")] = this->inputs[i].toJson();
                    doc[json_key_event][json_key_latitude] = this->ConfigParameter.latitude;
                    doc[json_key_event][json_key_longitude] = this->ConfigParameter.longitude;
                    doc[json_key_event][json_key_acbgl] = this->ConfigParameter.allowActivationByGeolocation;

                    this->wssend(doc);
                }
            }

            ws.cleanupClients();
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

        void wssend(DynamicJsonDocument &json_doc)
        {
            String outputJson = "";
            serializeJson(json_doc, outputJson);
            //  Serial.println(F("wssend >= "));
            //  serializeJsonPretty(json_doc, Serial);
            this->wsclient.send(outputJson);
        }

        void begin()
        {
            ws.onEvent(onWsEvent);
            ocsWebAdmin.addHandler(&ws);
            ocsWebAdmin.begin();
            this->led.blink(700, 800, 100, 5);
        }

        void connect_websocket()
        {

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
                this->wsclient.close();
            }
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

            interval_check_config_changed.setup(30000, [&]()
                                                {
 if (this->existsConfigChanged)
                {
                    this->existsConfigChanged = false;
                    this->ConfigParameter.saveLocalStorage();
                } });

            interval_ws_ping.setup(50000, [&]()
                                   {
                                    this->wsclient.ping();
                                    this->led.blink(1500, 1000, 500, 10); });

            interval_ws_connect.setup(10000, [&]()
                                      { if(!this->wsclient.available()){
                                        this->connect_websocket();
                                      } });

            interval_ws_status_io.setup(1000, [&]()
                                        {
// Serial.println(F("interval_ws_status_io ... "));
            DynamicJsonDocument docStatus(512);
            docStatus[json_key_status] = json_key_input;
            docStatus[json_key_value] = this->statusInputs();
            ws.textAll(DynamicJsonToString(docStatus));
            docStatus.clear();
            docStatus[json_key_status] = json_key_output;
            docStatus[json_key_value] = this->statusOutputs();
            ws.textAll(DynamicJsonToString(docStatus));
            docStatus.clear(); });
        }

        DynamicJsonDocument statusInputs()
        {
            DynamicJsonDocument doc(512);

            for (byte i = 0; i < ocs::MAX_INPUTS; i = i + 1)
            {
                // doc[i] = this->inputs[i].toJson();
                doc[i][json_key_gpio] = this->inputs[i].config.gpio;
                doc[i][json_key_status] = this->inputs[i].status;
                doc[i][json_key_value] = this->inputs[i].getvalue();
            }

            return doc;
        }

        DynamicJsonDocument statusOutputs()
        {
            DynamicJsonDocument doc(128);
            doc[json_key_led] = this->led.getState();
            for (byte i = 0; i < ocs::MAX_OUTPUTS; i = i + 1)
            {
                // doc[i] = this->inputs[i].toJson();
                doc[json_key_output][i][json_key_gpio] = this->outputs[i].getGPIO();
                doc[json_key_output][i][json_key_status] = this->outputs[i].getState();
                doc[json_key_output][i][json_key_enabled] = this->outputs[i].enabled;
            }

            return doc;
        }

    private:
        bool existsConfigChanged = false;
        WebsocketsClient wsclient;
        edwinspire::OutputPin outputs[ocs::MAX_OUTPUTS];
        edwinspire::OutputPin led;
        ocs::input::Input inputs[ocs::MAX_INPUTS];
        // unsigned long intervalWsPing = 50000;
        // unsigned long last_time_ws_ping = 0;
        edwinspire::Interval interval_ws_ping;

        // unsigned long last_time_check_config_changed = 0;
        // unsigned long interval_check_config_changed = 50000;
        edwinspire::Interval interval_check_config_changed;

        // unsigned long interval_send_status = 1000;
        // unsigned long interval_send_status_last = 0;
        edwinspire::Interval interval_ws_status_io;
        // char body_json_data_tmp[4096]  = {};
        // char variableName[4096]  = {};
        edwinspire::Interval interval_ws_connect;

        // bodyData bdata[3];
        String tmp_buffer_body;

        String DynamicJsonToString(DynamicJsonDocument doc)
        {
            String outputJson = "";
            serializeJson(doc, outputJson);
            return outputJson;
        }

        // set info device
        AsyncCallbackJsonWebHandler *handlerdevInfoPost = new AsyncCallbackJsonWebHandler("/device/info", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                          { 
                                                                                            
                                                                                         this->ConfigParameter.setConfigInfo(json);
                                                                                            this->existsConfigChanged = true;
                                                                                request->send(200, json_key_mime_json, "{}"); });

        // set geolocation
        AsyncCallbackJsonWebHandler *handlerdevgeoPost = new AsyncCallbackJsonWebHandler("/device/geolocation", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                         { 
                                                                                        //  serializeJsonPretty(json, Serial);
                                                                                          this->ConfigParameter.latitude = json[json_key_latitude].as<String>(); 
                                                                                          this->ConfigParameter.longitude = json[json_key_longitude].as<String>(); 
                                                                                          this->ConfigParameter.allowActivationByGeolocation = json[json_key_acbgl].as<boolean>(); 
                                                                                            this->existsConfigChanged = true;
                                                                                request->send(200, json_key_mime_json, "{}"); });

        // set wifi
        AsyncCallbackJsonWebHandler *handlerdevwifiPost = new AsyncCallbackJsonWebHandler("/device/wifi", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                          { 
                                                                                       //   serializeJsonPretty(json, Serial);
                                                                                        this->ConfigParameter.setConfigWifi(json);
                                                                                        this->existsConfigChanged = true;
                                                                                request->send(200, json_key_mime_json, "{}"); });
        // set inputs
        AsyncCallbackJsonWebHandler *handlerdevInputsPost = new AsyncCallbackJsonWebHandler("/device/inputs", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                            { 
                                                                             //             serializeJsonPretty(json, Serial);
                                                                                        this->ConfigParameter.setConfigInputs(json);
                                                                                        this->existsConfigChanged = true;
                                                                                request->send(200, json_key_mime_json, "{}"); });

        // set outputs
        AsyncCallbackJsonWebHandler *handlerdevOutputsPost = new AsyncCallbackJsonWebHandler("/device/outputs", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                             { 
                                                                                 //         serializeJsonPretty(json, Serial);
                                                                                        this->ConfigParameter.setConfigoutputs(json);
                                                                                        this->existsConfigChanged = true;
                                                                                request->send(200, json_key_mime_json, "{}"); });

        // set cert
        AsyncCallbackJsonWebHandler *handlerCertPost = new AsyncCallbackJsonWebHandler("/device/cert", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                       { 
                                                                            //              serializeJsonPretty(json, Serial);
                                                                                        this->ConfigParameter.caCert_fingerPrint = json[json_key_caCert_fingerPrint].as<String>();
                                                                                        this->existsConfigChanged = true;
                                                                                request->send(200, json_key_mime_json, "{}"); });

        void setUpWebAdmin()
        {

            ocsWebAdmin.setup();

            // get device info
            ocsWebAdmin.on("/device/info", HTTP_GET, [&](AsyncWebServerRequest *request)
                           { request->send(200, json_key_mime_json, DynamicJsonToString(this->ConfigParameter.getInfo())); });

            // set device info
            ocsWebAdmin.addHandler(handlerdevInfoPost);

            // get geolocation
            ocsWebAdmin.on("/device/geolocation", HTTP_GET, [&](AsyncWebServerRequest *request)
                           { request->send(200, json_key_mime_json, DynamicJsonToString(this->ConfigParameter.getGeolocation())); });

            // set geolocation
            ocsWebAdmin.addHandler(handlerdevgeoPost);

            // get wifi
            ocsWebAdmin.on("/device/wifi", HTTP_GET, [&](AsyncWebServerRequest *request)
                           { request->send(200, json_key_mime_json, DynamicJsonToString(this->ConfigParameter.getConfigWifi())); });

            // set wifi
            ocsWebAdmin.addHandler(handlerdevwifiPost);

            // get inputs
            ocsWebAdmin.on("/device/inputs", HTTP_GET, [&](AsyncWebServerRequest *request)
                           { request->send(200, json_key_mime_json, DynamicJsonToString(this->ConfigParameter.getConfigInputs())); });

            // set inputs
            ocsWebAdmin.addHandler(handlerdevInputsPost);

            // get outputs
            ocsWebAdmin.on("/device/outputs", HTTP_GET, [&](AsyncWebServerRequest *request)
                           { request->send(200, json_key_mime_json, DynamicJsonToString(this->ConfigParameter.getConfigoutputs())); });

            // set outputs
            ocsWebAdmin.addHandler(handlerdevOutputsPost);

            // get cert
            ocsWebAdmin.on("/device/cert", HTTP_GET, [&](AsyncWebServerRequest *request)
                           { 
                            DynamicJsonDocument doc(2048);
                            doc[json_key_caCert_fingerPrint] = this->ConfigParameter.caCert_fingerPrint;
                            request->send(200, json_key_mime_json, DynamicJsonToString(doc)); });

            // set cert
            ocsWebAdmin.addHandler(handlerCertPost);

            /// reboot
            ocsWebAdmin.on("/device/reboot", [&](AsyncWebServerRequest *request)
                           {
                               request->send(200, json_key_mime_json, "{\"ESP\": \"Restarting...\"}");
                               this->reboot(); });
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
        Serial.println(F("Connection Opened"));
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println(F("Connection Closed"));
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
                this->ConfigParameter.deviceId = doc[json_key_deviceId].as<String>();
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
                DynamicJsonDocument doc(JSON_MAX_SIZE);
                doc[F("data")] = this->toJson();
                doc[F("data")][json_key_wf] = (char *)0;                 // Quitamos wf
                doc[F("data")][json_key_caCert_fingerPrint] = (char *)0; // Quitamos cfp
                doc[F("data")][json_key_info][F("ip")] = this->ip;
                doc[F("data")][json_key_info][F("ssid")] = this->ssid;
                doc[F("response")] = 1000;
                String outputJson = "";
                serializeJson(doc, outputJson);
                doc.clear();
                // Serial.println(outputJson);

                this->wsclient.send(outputJson);
            }
            break;
            }
        }
    };
}
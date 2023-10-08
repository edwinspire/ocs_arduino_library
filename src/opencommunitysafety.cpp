#include <ArduinoJson.h>
#include <WebServer.cpp>
// #include <LocalStore.cpp>
#include <Inputpin.cpp>
#include <Outputpin.cpp>
#include <Interval.cpp>
#include <ArduinoWebsockets.h>
#include "AsyncJson.h"
#include "Configuration.cpp"

#ifdef ESP32
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266mDNS.h>
#endif

using namespace websockets;

ocs::HttpWebsocketServer ocsWebAdmin(80);

namespace ocs
{

    const byte MAX_SSID_WIFI = 4;
    const String default_websocketHost = "open-community-safety.herokuapp.com/ws/device";
    const String default_deviceid = "00a0aa00-aa00-0000-0000-000000000000";

#ifndef MAX_INPUTS
    MAX_INPUTS = 0
#endif

#ifndef MAX_OUTPUTS
        MAX_OUTPUTS = 1
#endif

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

    /*
        struct WifiParams
        {
            String ssid;
            String pwd;

            StaticJsonDocument<32> toJson()
            {
                StaticJsonDocument<32> doc;
                doc[json_key_ssid] = this->ssid;
                doc[json_key_password] = this->pwd;
                return doc;
            }
        };

        const WifiParams default_wifi = {.ssid = "accesspoint", .pwd = "123456@qwert"};
    */

    /*
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

                if (this->domainName == NULL || this->domainName.length() <= 10)
                {
                    this->domainName = "device.local";
                }

                if (this->latitude == 0 && this->longitude == 0)
                {
                    this->latitude = "37.44362";
                    this->longitude = "-122.15719";
                }

                if (this->password_user == NULL || this->password_user.length() < 6)
                {
                    this->password_user = "0000000000";
                }

                if (this->password_admin == NULL || this->password_admin.length() < 6)
                {
                    this->password_admin = "9999999999";
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

    #if MAX_INPUTS > 0
                for (byte i = 0; i < MAX_INPUTS; i = i + 1)
                {
                    // serializeJson(data, Serial);
                    this->input[i].fromJson(data[i]);
                }
    #endif
            }

            DynamicJsonDocument getConfigInputs()
            {

    #if MAX_INPUTS > 0
                DynamicJsonDocument doc(1024);
                JsonArray array = doc.to<JsonArray>();

                for (byte i = 0; i < MAX_INPUTS; i = i + 1)
                {
                    array.add(this->input[i].toJson());
                    // doc[i] = this->input[i].toJson();
                }
    #else
                DynamicJsonDocument doc(4);
                JsonArray array = doc.to<JsonArray>();
    #endif

                return doc;
            }

            void setConfigoutputs(const DynamicJsonDocument &data)
            {
                this->led = data[json_key_led].as<byte>();
                for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
                {
                    this->output[i].fromJson(data[json_key_output][i]);
                }
            }

            DynamicJsonDocument getConfigoutputs()
            {
                DynamicJsonDocument doc(1024);
                doc[json_key_led] = this->led;
                for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
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
                this->domainName = data[json_key_domainName].as<String>();
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
                return ocs::LocalStore::save(HttpWebsocketServer::DynamicJsonToString(this->toJson()));
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
                this->password_admin = data[json_key_password_admin].as<String>();
                this->password_user = data[json_key_password_user].as<String>();
                this->setDefault();
                Serial.println("--------- CONFIG FROM JSON -------------");
                serializeJsonPretty(data, Serial);
            }

            DynamicJsonDocument getInfo()
            {
                // Serial.println(F("getInfo 1"));
                DynamicJsonDocument doc(1024);
                doc[json_key_name] = this->name;
                doc[json_key_deviceId] = this->deviceId;
                doc[json_key_wshost] = this->websocketHost;
                doc[json_key_domainName] = this->domainName;
    #ifdef ESP32
                doc[F("ChipModel")] = ESP.getChipModel();
                doc[F("EfuseMac")] = String(ESP.getEfuseMac(), HEX);
                doc[F("ChipRevision")] = ESP.getChipRevision();
    #elif defined(ESP8266)
                doc[F("ChipModel")] = String(ESP.getChipId(), HEX);
                doc[F("EfuseMac")] = String(ESP.getFlashChipId(), HEX);
                doc[F("ChipRevision")] = ESP.getCoreVersion();
    #endif
                //  Serial.println(F("getInfo 2"));
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
                doc[json_key_password_admin] = this->password_admin;
                doc[json_key_password_user] = this->password_user;
                // Serial.println("--------- CONFIG TO JSON -------------");
                // serializeJsonPretty(doc, Serial);
                return doc;
            }

            String websocketHost;
            WifiParams wifi[ocs::MAX_SSID_WIFI];
            input::Configure input[MAX_INPUTS];
            outputConfig output[MAX_OUTPUTS];
            byte led = 255; // Led GPIO
            String deviceId;
            String caCert_fingerPrint;
            String latitude;
            String longitude;
            String name;
            String domainName = "device.local";
            // String username = "ocs";
            String password_admin = "";
            String password_user = "";
            bool allowActivationByGeolocation = false;
            String websocketHostRequest;
            // StaticJsonDocument<4096> * json;
        };
    */

    // char body_json_data_tmp[4096]  = {};

    class OpenCommunitySafety
    {

    public:
        Configuration ConfigParameter;
        String ip;
        String ssid;
        bool WifiConnected = false;

        /*
                void setAlarm(ocs::input::SirenType at)
                {

                    for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
                    {

                        Serial.print("Entra en setAlarm " + String(at));
                        Serial.print("\nGPIO " + String(this->outputs[i].getGPIO()));
                        Serial.print("\nEnabled " + String(this->outputs[i].enabled));

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
        */
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
            for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
            {
                this->outputs[i].loop();
            }
            // loop for led
            this->led.loop();
            // loop for inputs

#if MAX_INPUTS > 0
            for (byte i = 0; i < MAX_INPUTS; i = i + 1)
            {
                this->inputs[i].loop();
            }
#endif
        }

        /*
                DynamicJsonDocument setFromJson(DynamicJsonDocument json)
                {

                    // serializeJsonPretty(json, Serial);
                    if (json != NULL)
                    {
                        // Serial.println(F("Ingresa a setFromJson"));
                        this->ConfigParameter.fromJson(json);
                        this->ConfigParameter.saveLocalStorage();
                    }

                    return this->ConfigParameter.toJson();
                }
                */

        DynamicJsonDocument toJson()
        {
            // this->ConfigParameter.printMemory();
            return this->ConfigParameter.toJson();
        }

        void wssend(DynamicJsonDocument &json_doc)
        {
            this->wsclient.send(HttpWebsocketServer::DynamicJsonToString(json_doc));
        }

        void begin()
        {
            ocsWebAdmin.start();
            this->led.blink(700, 800, 100, 5);
        }

        void connect_websocket()
        {

            if (this->WifiConnected)
            {
                Serial.println(this->ConfigParameter.server.getUrlServer());

                HttpWebsocketServer::freeMemory();
                bool connected = this->wsclient.connect(this->ConfigParameter.server.getUrlServer());

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
            this->ConfigParameter.setup();
            this->setup(this->ConfigParameter);
        }

        void setup(Configuration config)
        {
            Serial.println(F("Setup OCS"));

            this->ConfigParameter = config;
            //this->ConfigParameter.setup();

            ocsWebAdmin.setup();
            ocsWebAdmin.setAdminPwd(this->ConfigParameter.device.getPwdAdm().c_str());
            ocsWebAdmin.setUserPwd(this->ConfigParameter.device.getPwdAdm().c_str());

            this->setUpInputs();

            this->setUpOutputs();

            this->led.setup(this->ConfigParameter.led, true);

            this->setUpWebAdmin();
            this->setUpwebSocket();

            /*
                        interval_check_config_changed.setup(30000, [&]()
                                                            {
             if (this->existsConfigChanged)
                            {
                                this->existsConfigChanged = false;
                                this->ConfigParameter.saveLocalStorage();
                            } });
                            */

            interval_ws_ping.setup(30000, [&]()
                                   {
                                       if (this->WifiConnected)
                                       {
                                           this->wsclient.ping();
                                           this->led.blink(1500, 1000, 500, 10);
                                       } });

            interval_ws_connect.setup(15000, [&]()
                                      { if(!this->wsclient.available()){
                                        this->connect_websocket();
                                      } });

            interval_ws_status_io.setup(1000, [&]()
                                        {
// Serial.println(F("interval_ws_status_io ... "));
            DynamicJsonDocument docStatus(512);
            docStatus[json_key_status] = json_key_input;
            docStatus[json_key_value] = this->statusInputs();
            ocsWebAdmin.wsTextAll(HttpWebsocketServer::DynamicJsonToString(docStatus).c_str());
            docStatus.clear();
            docStatus[json_key_status] = json_key_output;
            docStatus[json_key_value] = this->statusOutputs();
            ocsWebAdmin.wsTextAll(HttpWebsocketServer::DynamicJsonToString(docStatus).c_str());
            docStatus.clear(); });

            delay(3000);
        }

        DynamicJsonDocument statusInputs()
        {
#if MAX_INPUTS > 0
            DynamicJsonDocument doc(512);

            for (byte i = 0; i < MAX_INPUTS; i = i + 1)
            {
                // doc[i] = this->inputs[i].toJson();
                doc[i][json_key_gpio] = this->inputs[i].gpio;
                doc[i][json_key_status] = this->inputs[i].status;
                doc[i][json_key_value] = this->inputs[i].getvalue();
            }
#else
            DynamicJsonDocument doc(4);
#endif
            return doc;
        }

        DynamicJsonDocument statusOutputs()
        {
            DynamicJsonDocument doc(128);
            doc[json_key_led] = this->led.getState();
            for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
            {
                // doc[i] = this->inputs[i].toJson();
                doc[json_key_output][i][json_key_gpio] = this->outputs[i].getGPIO();
                doc[json_key_output][i][json_key_status] = this->outputs[i].getState();
                doc[json_key_output][i][json_key_enabled] = this->outputs[i].enabled;
            }

            return doc;
        }

    private:
        void alarm(byte position, ocs::input::Status status, ZoneType zone_type)
        {
            sendAlarmInputEvent(position, status);
            linkedOutputsAction(position, status, zone_type);
        }

        void linkedOutputsAction(byte position, ocs::input::Status status, ZoneType zone_type)
        {

            for (byte i = 0; i < MAX_OUTPUTS_LINKED; i = i + 1)
            {

                for (byte i1 = 0; i1 < MAX_OUTPUTS; i1 = i1 + 1)
                {

                    if (this->outputs[i1].enabled && this->outputs[i1].getGPIO() == this->ConfigParameter.inputs[position].getOuts()[i].getGpio())
                    {
                        // Aqui ejecutar la acción sobre la salida
                        switch (zone_type)
                        {
                        case ZoneType::BUTTON:
                            if (status == ocs::input::Status::ALARM)
                            {
                                this->outputs[i1].high();
                            }
                            else
                            {
                                this->outputs[i1].low();
                            }
                            break;
                        case ZoneType::TOGGLE:
                            if (status == ocs::input::Status::ALARM)
                            {
                                this->outputs[i1].toggle();
                            }
                            break;

                        default:
                            this->outputs[i1].blink_by_time(this->ConfigParameter.inputs[position].getOuts()[i].getLowTime(), this->ConfigParameter.inputs[position].getOuts()[i].geHighTime(), this->ConfigParameter.inputs[position].getOuts()[i].geTotalTime());
                            break;
                        }
                    }
                }
            }
        }

        void sendAlarmInputEvent(byte position, ocs::input::Status status)
        {
            DynamicJsonDocument doc(256);
            doc[json_key_event][json_key_device_id] = this->ConfigParameter.device.getDeviceID();
            doc[json_key_event][json_key_input][json_key_status] = status;
            doc[json_key_event][json_key_input][json_key_label] = this->ConfigParameter.inputs[position].getLabel();
            doc[json_key_event][json_key_input][json_key_report] = this->ConfigParameter.inputs[position].getReport();

            this->wssend(doc);
        }

        void onChangeStatusInput(byte position, ocs::input::Status status)
        {

            // Verifica el tipo de zona para procesar según el caso
            switch (this->ConfigParameter.inputs[position].getZoneType())
            {
            case ZoneType::ALWAYS:
                // Emite una alarma siempre
                this->alarm(position, status, ZoneType::ALWAYS);
                break;

            case ZoneType::ARM_DISARM:
                // Arma y desarma el sistema
                break;

            case ZoneType::DELAY:
                // Verifica si el sistema esta armado y si se terminó el tiempo de retardo para emitir una señal
                break;

            case ZoneType::INTERIOR:
                // Verifica el tipo de armado del sistema para emitir una señal

                if (this->ConfigParameter.getArmed() == ArmedType::INSTANT || this->ConfigParameter.getArmed() == ArmedType::WITH_DELAY)
                {
                    this->alarm(position, status, ZoneType::INTERIOR);
                }

                break;

            case ZoneType::NORMAL:
                // Verifica si el sistema está armado para emitir una señal de alarma
                if (this->ConfigParameter.getArmed() != ArmedType::DISARMED)
                {
                    this->alarm(position, status, ZoneType::NORMAL);
                }
                break;

            case ZoneType::BUTTON:
                // Independiente si el sistema está armado o no, comanda una o varias salidas
                linkedOutputsAction(position, status, ZoneType::BUTTON);
                break;

            case ZoneType::TOGGLE:
                // Independiente si el sistema está armado o no, comanda una o varias salidas
                linkedOutputsAction(position, status, ZoneType::TOGGLE);
                break;

            default:
                // Para otros casos
                break;
            }
        }

        bool changePassword(bool isAdmin, const char *old_pwd, const char *new_pwd)
        {

            bool r = false;

            if (ocsWebAdmin.checkPassword(isAdmin, old_pwd))
            {
                // Serial.println('Clave válida');
                if (isAdmin)
                {
                    this->ConfigParameter.device.setPwdAdm(new_pwd);
                    r = true;
                    this->existsConfigChanged = true;
                    ocsWebAdmin.setAdminPwd(new_pwd);
                }
                else
                {
                    this->ConfigParameter.device.setPwdUser(new_pwd);
                    r = true;
                    this->existsConfigChanged = true;
                    ocsWebAdmin.setUserPwd(new_pwd);
                }
            }

            return r;
        }

        WebsocketsClient wsclient;
        bool existsConfigChanged = false;
        edwinspire::OutputPin outputs[MAX_OUTPUTS];
        edwinspire::OutputPin led;
        ocs::input::Input inputs[MAX_INPUTS];
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
        // String tmp_buffer_body;

        /*
                String DynamicJsonToString(JsonVariantConst doc)
                {
                    this->ConfigParameter.freeMemory();
                    // Serial.println(F("DynamicJsonToString 1"));
                    String outputJson = "";
                    serializeJson(doc, outputJson);
                    // Serial.println(F("DynamicJsonToString 2"));
                    return outputJson;
                }
                */

        AsyncCallbackJsonWebHandler *handlerdevChangePwdPost = new AsyncCallbackJsonWebHandler("/device/pwd/change", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                               {
                            if (ocsWebAdmin.CheckToken(request))
                               {
                                       AsyncWebServerResponse *response = request->beginResponse(200, JSON_MIMETYPE, "{\"change\": \"" + String(this->changePassword(json[F("u")].as<byte>(), json[F("p")].as<String>().c_str(), json[F("np")].as<String>().c_str())) + "\"}");
                                       request->send(response);
                               } });

        // login
        AsyncCallbackJsonWebHandler *handlerdevLoginPost = new AsyncCallbackJsonWebHandler("/device/login", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                           { ocsWebAdmin.login(json[F("u")].as<bool>(), json[F("p")].as<String>().c_str(), request); }
                                                                                           //--
        );

        // set configuration
        AsyncCallbackJsonWebHandler *handlerdevInfoPost = new AsyncCallbackJsonWebHandler("/device/configuration", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                          {
                                                                                              if (ocsWebAdmin.CheckToken(request))
                                                                                              {
                                                                                                  this->ConfigParameter.fromJson(json);
                                                                                                  this->existsConfigChanged = true;
                                                                                                  request->send(200, JSON_MIMETYPE, "{}");
                                                                                              } });
        /*
                // set info device
                AsyncCallbackJsonWebHandler *handlerdevInfoPost = new AsyncCallbackJsonWebHandler("/device/info", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                                  {
                                                                                                      if (ocsWebAdmin.CheckToken(request))
                                                                                                      {
                                                                                                          this->ConfigParameter.setConfigInfo(json);
                                                                                                          this->existsConfigChanged = true;
                                                                                                          request->send(200, JSON_MIMETYPE, "{}");
                                                                                                      } });

                // set geolocation
                AsyncCallbackJsonWebHandler *handlerdevgeoPost = new AsyncCallbackJsonWebHandler("/device/geolocation", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                                 {
                                                                                                     if (ocsWebAdmin.CheckToken(request))
                                                                                                     {
                                                                                                         this->ConfigParameter.latitude = json[json_key_latitude].as<String>();
                                                                                                         this->ConfigParameter.longitude = json[json_key_longitude].as<String>();
                                                                                                         this->ConfigParameter.allowActivationByGeolocation = json[json_key_acbgl].as<boolean>();
                                                                                                         this->existsConfigChanged = true;
                                                                                                         request->send(200, JSON_MIMETYPE, "{}");
                                                                                                     }
                                                                                                     //  serializeJsonPretty(json, Serial);
                                                                                                 });

                // set wifi
                AsyncCallbackJsonWebHandler *handlerdevwifiPost = new AsyncCallbackJsonWebHandler("/device/wifi", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                                  {
                                                                                                      if (ocsWebAdmin.CheckToken(request))
                                                                                                      {
                                                                                                          this->ConfigParameter.setConfigWifi(json);
                                                                                                          this->existsConfigChanged = true;
                                                                                                          request->send(200, JSON_MIMETYPE, "{}");
                                                                                                      } });
                // set inputs
                AsyncCallbackJsonWebHandler *handlerdevInputsPost = new AsyncCallbackJsonWebHandler("/device/inputs", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                                    {
                                                        if (ocsWebAdmin.CheckToken(request)){
                                            this->ConfigParameter.setConfigInputs(json);
                                                                                                this->existsConfigChanged = true;
                                                                                        request->send(200, JSON_MIMETYPE, "{}");
                                                        } });

                // set outputs
                AsyncCallbackJsonWebHandler *handlerdevOutputsPost = new AsyncCallbackJsonWebHandler("/device/outputs", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                                     {
                                                                                         if (ocsWebAdmin.CheckToken(request)){
            this->ConfigParameter.setConfigoutputs(json);
                                                                                                this->existsConfigChanged = true;
                                                                                        request->send(200, JSON_MIMETYPE, "{}");
                                                                                         } });

                // set cert
                AsyncCallbackJsonWebHandler *handlerCertPost = new AsyncCallbackJsonWebHandler("/device/cert", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                               {
                                                                                    if (ocsWebAdmin.CheckToken(request)){
                this->ConfigParameter.caCert_fingerPrint = json[json_key_caCert_fingerPrint].as<String>();
                                                                                                this->existsConfigChanged = true;
                                                                                        request->send(200, JSON_MIMETYPE, "{}");
                                                                                    } });
                                                                                    */

        void setUpWebAdmin()
        {

            //  ocsWebAdmin.setup();
            // ocsWebAdmin.enableCORS(true);
            ocsWebAdmin.addHandler(handlerdevLoginPost);
            ocsWebAdmin.addHandler(handlerdevChangePwdPost);

            // get device info
            ocsWebAdmin.on("/device/configuration", HTTP_GET, [&](AsyncWebServerRequest *request)
                           {
                               if (ocsWebAdmin.CheckToken(request))
                               {
                                //Serial.println(F("/device/info para del checktoken"));
request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(this->ConfigParameter.toJson()));

                               } });

            // set device info
            ocsWebAdmin.addHandler(handlerdevInfoPost);

            /*
                        // get geolocation
                        ocsWebAdmin.on("/device/geolocation", HTTP_GET, [&](AsyncWebServerRequest *request)
                                       {
                                           if (ocsWebAdmin.CheckToken(request))
                                           {
                                               request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(this->ConfigParameter.getGeolocation()));
                                           } });

                        // set geolocation
                        ocsWebAdmin.addHandler(handlerdevgeoPost);

                        // get wifi
                        ocsWebAdmin.on("/device/wifi", HTTP_GET, [&](AsyncWebServerRequest *request)
                                       {
                                           if (ocsWebAdmin.CheckToken(request))
                                           {
                                               request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(this->ConfigParameter.getConfigWifi()));
                                           } });

                        // set wifi
                        ocsWebAdmin.addHandler(handlerdevwifiPost);

                        // get inputs
                        ocsWebAdmin.on("/device/inputs", HTTP_GET, [&](AsyncWebServerRequest *request)
                                       {
                                           if (ocsWebAdmin.CheckToken(request))
                                           {
                                               request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(this->ConfigParameter.getConfigInputs()));
                                           } });

                        // set inputs
                        ocsWebAdmin.addHandler(handlerdevInputsPost);

                        // get outputs
                        ocsWebAdmin.on("/device/outputs", HTTP_GET, [&](AsyncWebServerRequest *request)
                                       {
                                           if (ocsWebAdmin.CheckToken(request))
                                           {
                                               request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(this->ConfigParameter.getConfigoutputs()));
                                           } });

                        // set outputs
                        ocsWebAdmin.addHandler(handlerdevOutputsPost);

                        // get cert
                        ocsWebAdmin.on("/device/cert", HTTP_GET, [&](AsyncWebServerRequest *request)
                                       {
                                           if (ocsWebAdmin.CheckToken(request))
                                           {
                                               DynamicJsonDocument doc(2048);
                                               doc[json_key_caCert_fingerPrint] = this->ConfigParameter.caCert_fingerPrint;
                                               request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(doc));
                                           } });

                        // set cert
                        ocsWebAdmin.addHandler(handlerCertPost);
            */

            /// reboot
            ocsWebAdmin.on("/device/reboot", [&](AsyncWebServerRequest *request)
                           {
                               if (ocsWebAdmin.CheckToken(request))
                               {
                                   request->send(200, JSON_MIMETYPE, "{\"ESP\": \"Restarting...\"}");
                                   this->reboot();
                               } });

            // get inputs
            ocsWebAdmin.on("/device/isalive", HTTP_GET, [&](AsyncWebServerRequest *request)
                           {
                            DynamicJsonDocument doc(16);
                            doc[F("isalavive")] = true; 
                            request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(doc)); });
        }

        void setUpwebSocket()
        {
            if (this->ConfigParameter.server.getSecure())
            {

#ifdef ESP32
                this->wsclient.setCACert(this->ConfigParameter.server.getcertificate().c_str());
#elif defined(ESP8266)
                this->wsclient.setFingerprint(this->ConfigParameter.server.getcertificate().c_str());
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
            for (byte i = 0; i < MAX_INPUTS; i = i + 1)
            {
                // this->inputs[i].setup(this->ConfigParameter.input[i].gpio, this->ConfigParameter.input[i].name, this->ConfigParameter.input[i].enabled);
                this->inputs[i].setup(
                    i, this->ConfigParameter.inputs[i].getGpio(), this->ConfigParameter.inputs[i].getEnabled(), this->ConfigParameter.inputs[i].getZoneType(), this->ConfigParameter.inputs[i].getContactType());

                this->inputs[i].onChangeStatus([&](byte position, byte gpio, uint16_t value, ocs::input::Status status) -> void
                                               { this->onChangeStatusInput(position, status); });
            }
        }

        void setUpOutputs()
        {
            for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
            {

                if (this->ConfigParameter.outputs[i].getGpio() != 255)
                {
                    this->outputs[i].setup(this->ConfigParameter.outputs[i].getGpio(), this->ConfigParameter.outputs[i].getEnabled());
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
                // ocs::input::SirenType siren_type = doc[F("siren_type")];
                // this->setAlarm(siren_type);
            }
            break;
            case 1000: // Set deviceId
                this->ConfigParameter.device.setDeviceID(doc[json_key_device_id].as<String>());
                Serial.println(F("seteada UUID"));
                // this->ConfigParameter.saveLocalStorage();
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
                doc[json_key_data] = this->toJson();
                // doc[json_key_data][json_key_wf] = (char *)0;                 // Quitamos wf
                // doc[json_key_data][json_key_caCert_fingerPrint] = (char *)0; // Quitamos cfp
                doc[json_key_data][json_key_info][json_key_ip] = this->ip;
                doc[json_key_data][json_key_info][json_key_ssid] = this->ssid;
                doc[json_key_response] = 1000;

                // serializeJson(doc, outputJson);
                // doc.clear();
                // Serial.println(outputJson);

                this->wsclient.send(HttpWebsocketServer::DynamicJsonToString(doc));
                doc.clear();
            }
            break;
            }
        }
    };
}
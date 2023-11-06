#include <ArduinoJson.h>
#include <WebServer.cpp>
// #include <LocalStore.cpp>
#include <Inputpin.cpp>
#include <Outputpin.cpp>
#include <Interval.cpp>
#include <ArduinoWebsockets.h>
#include "AsyncJson.h"
#include "Configuration.cpp"

using namespace websockets;

ocs::HttpWebsocketServer ocsWebAdmin(80);

namespace ocs
{

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

    class OpenCommunitySafety
    {

    public:
        Configuration ConfigParameter;
        String ip;
        String ssid;
        typedef std::function<void(String)> messageCallbackWebsocket;
        typedef std::function<void()> pingCallbackWebsocket;

        void loop()
        {

            this->ConfigParameter.loop();
            interval_ws_status_io.loop();
            interval_check_config_changed.loop();
            interval_ws_ping.loop();

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

        DynamicJsonDocument toJson()
        {
            // this->ConfigParameter.printMemory();
            return this->ConfigParameter.toJson();
        }

        void begin()
        {
            ocsWebAdmin.start();
            this->led.blink_by_time(600, 400, 2000);
        }

        void sendCommand(CommunicationCommand request_type, const DynamicJsonDocument &data)
        {
            DynamicJsonDocument doc(2048);
            doc[json_key_command] = request_type;
            doc[json_key_data] = data;
            this->msgCallbackWs(HttpWebsocketServer::DynamicJsonToString(doc));
            doc.clear();
        }

        // Establecer el callback del evento
        void onMessageToWebsocket(messageCallbackWebsocket callback)
        {
            this->msgCallbackWs = callback;
        }

        void onPingIntervalWebSocket(pingCallbackWebsocket callback)
        {
            this->pingCallbackWs = callback;
        }

        String chip()
        {

#ifdef ESP8266
            return "ESP8266";
#elif defined(ESP32)
            return "ESP32";
#endif
        }

        String chip_model()
        {
#ifdef ESP32
            return ESP.getChipModel();
#elif defined(ESP8266)
            return String(ESP.getChipId(), HEX);
#endif
        }

        String chip_version()
        {
#ifdef ESP32
            return ESP.getChipRevision();
#elif defined(ESP8266)
            return ESP.getCoreVersion();
#endif
        }

        uint32_t chip_id()
        {
#ifdef ESP8266
            // Para ESP8266
            Serial.println("Modelo del chip (ESP8266): " + ESP.getChipId());
            return ESP.getChipId();
#endif

#ifdef ESP32
            // Para ESP32
            esp_chip_info_t chip_info;
            esp_chip_info(&chip_info);
            Serial.print("Modelo del chip (ESP32): ");
            Serial.println((char *)chip_info.model);
#endif
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

            ocsWebAdmin.setup();
            ocsWebAdmin.setAdminPwd(this->ConfigParameter.device.getPwdAdm().c_str());
            ocsWebAdmin.setUserPwd(this->ConfigParameter.device.getPwdAdm().c_str());

            this->setUpInputs();

            this->setUpOutputs();

            this->led.setup(this->ConfigParameter.getLed(), true);

            this->setUpWebAdmin();

            interval_ws_ping.setup(35000, [&]()
                                   {

                                    this->pingCallbackWs();
this->led.blink_by_time(250, 750, 2000); });

            interval_ws_status_io.setup(2000, [&]()
                                        {
            DynamicJsonDocument docStatus(512);
            docStatus[json_key_status] = json_key_input;
            docStatus[json_key_value] = this->statusInputs();
            ocsWebAdmin.wsTextAll(HttpWebsocketServer::DynamicJsonToString(docStatus).c_str());
            docStatus.clear();
            docStatus[json_key_status] = json_key_output;
            docStatus[json_key_value] = this->statusOutputs();
            ocsWebAdmin.wsTextAll(HttpWebsocketServer::DynamicJsonToString(docStatus).c_str());
            docStatus.clear(); });

            delay(1000);
        }

        void onMessageFromWebsocket(String message)
        {

            Serial.print(F("onMessageFromWebsocket: "));
            Serial.println(message);
            this->ledBlinkOnWs();
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, message);

            // Test if parsing succeeds.
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
            }
            else
            {

                if (!doc[json_key_command].isNull())
                {
                    this->onwsCommand(doc);
                }
                else
                {
                    Serial.println("Command tag not found");
                }
            }
        }

        void onConnect()
        {

            DynamicJsonDocument doc(256);
            if (this->ConfigParameter.device.getDeviceID().length() < 10)
            {
                this->ConfigParameter.device.setDeviceID(default_deviceid);
            }

            doc[json_key_device_id] = this->ConfigParameter.device.getDeviceID();

            if (strcmp(default_deviceid, this->ConfigParameter.device.getDeviceID().c_str()) != 0)
            {

                doc[json_key_name] = this->ConfigParameter.device.getName();
                doc[json_key_chip] = this->chip();
                doc[json_key_chip_model] = this->chip_model();
                doc[json_key_chip_version] = this->chip_version();
                doc[json_key_mac_address] = this->mac_address;
            }
            this->sendCommand(CommunicationCommand::REGISTER_DEVICE, doc);
        }

        String getBasicAuthorizationString()
        {
            return "Basic " + base64Encode(this->ConfigParameter.server.getUsername() + ":" + this->ConfigParameter.server.getPassword());
        }

        static String base64Encode(String text)
        {
            const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            String encoded = "";
            int val = 0, valb = -6;
            for (char c : text)
            {
                val = (val << 8) + c;
                valb += 8;
                while (valb >= 0)
                {
                    encoded += base64chars[(val >> valb) & 0x3F];
                    valb -= 6;
                }
            }

            if (valb > -6)
            {
                encoded += base64chars[((val << 8) >> (valb + 8)) & 0x3F];
            }
            while (encoded.length() % 4)
            {
                encoded += '=';
            }

            return encoded;
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

        void setMACAddress(String mac)
        {
            this->mac_address = mac;
        }

    private:
        messageCallbackWebsocket msgCallbackWs = nullptr;
        pingCallbackWebsocket pingCallbackWs = nullptr;

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

            // this->wssend(doc);
            this->sendCommand(CommunicationCommand::ALARM, doc);
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

        String mac_address = "";
        //        WebsocketsClient wsclient;
        bool existsConfigChanged = false;
        edwinspire::OutputPin outputs[MAX_OUTPUTS];
        edwinspire::OutputPin led;
        ocs::input::Input inputs[MAX_INPUTS];
        edwinspire::Interval interval_ws_ping;
        edwinspire::Interval interval_check_config_changed;
        edwinspire::Interval interval_ws_status_io;

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

        void setUpWebAdmin()
        {

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
                            doc[json_key_isalavive] = true; 
                            request->send(200, JSON_MIMETYPE, HttpWebsocketServer::DynamicJsonToString(doc)); });
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

            switch (doc[json_key_command].as<CommunicationCommand>())
            {
            case CommunicationCommand::ALARM: // Set Alarm
            {
                Serial.println(F("SET ALARM..."));

                unsigned long lowtime = doc[json_key_data][json_key_low_time].as<unsigned long>();
                unsigned long hightime = doc[json_key_data][json_key_high_time].as<unsigned long>();
                unsigned long totaltime = doc[json_key_data][json_key_total_time].as<unsigned long>();

                for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
                {
                    //  Serial.println(F("Blink out") + String(i));

                    if (this->ConfigParameter.outputs[i].getEnabled() && this->ConfigParameter.outputs[i].getGpio() != 255 && this->ConfigParameter.outputs[i].getName() == json_key_ocs)
                    {
                        if (totaltime > 0)
                        {
                            this->outputs[i].blink_by_time(lowtime, hightime, totaltime);
                            Serial.println(F("Blink out"));
                        }
                        else
                        {

                            this->outputs[i].low();
                            Serial.println(F("Off out"));
                        }
                    }
                }
            }
            break;
            case CommunicationCommand::SET_DEVICE_ID: // Set deviceId
                this->ConfigParameter.device.setDeviceID(doc[json_key_data][json_key_device_id].as<String>());
                Serial.println(F("seteada UUID"));
                // Cierra la conexión al websocket para que se vuelva a conectar con la nueva ID
                doc.clear();
                this->sendCommand(CommunicationCommand::DISCONNET, doc);
                break;
            case CommunicationCommand::GET_SETTINGS: // Requiere datos de configuración
            {
                Serial.println(F("Response configuration ..."));
                DynamicJsonDocument doc(JSON_MAX_SIZE);
                doc[json_key_info] = this->toJson();
                // doc[json_key_data][json_key_wf] = (char *)0;                 // Quitamos wf
                // doc[json_key_data][json_key_caCert_fingerPrint] = (char *)0; // Quitamos cfp
                doc[json_key_info][json_key_ip] = this->ip;
                doc[json_key_info][json_key_ssid] = this->ssid;

                this->sendCommand(CommunicationCommand::SET_SETTINGS, doc);

                doc.clear();
            }
            default:
                Serial.println("Comando desconocido: " + doc[json_key_command].as<String>());
                break;
            }
        }
    };
}
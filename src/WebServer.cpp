#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
// #include "SPIFFS.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#include "Interval.cpp"
#include <random>

namespace ocs
{

    const char MIMETYPE_JSON[17] = "application/json";
    const byte MAX_SESSIONS = 5;
    const byte MAX_LENGTH_TOKEN = 15;

    struct Session
    {
        char token[MAX_LENGTH_TOKEN];
        unsigned long expires;
        bool isAdmin;
    };

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

    class HttpWebsocketServer : public AsyncWebServer
    {

    public:
        HttpWebsocketServer(int port) : AsyncWebServer(port)
        {
            Serial.printf("HttpWebsocketServer Port: %i\n", port);
        }

        bool login(bool isAdmin, String pwd, AsyncWebServerRequest *request, char *token)
        {
            bool result = false;
            if (isAdmin)
            {
                if (strcmp(pwd.c_str(), this->admin_pwd) == 0)
                {
                    this->resetLock();

                    if (setToken(token, isAdmin))
                    {
                        this->responseLoginSuccess(request, String(token));
                        result = true;
                    }
                    else
                    {
                        this->responseLoginThereAreNoFreeSessions(request);
                    }
                }
                else
                {
                    this->responseLoginError(request);
                }
            }
            else
            {
                if (strcmp(pwd.c_str(), this->user_pwd) == 0)
                {
                    this->resetLock();

                    if (setToken(token, isAdmin))
                    {
                        this->responseLoginSuccess(request, String(token));
                        result = true;
                    }
                    else
                    {
                        this->responseLoginThereAreNoFreeSessions(request);
                    }
                }
                else
                {
                    this->responseLoginError(request);
                }
            }
            return result;
        }

        void setAdminPwd(String admin)
        {
            strncpy(this->admin_pwd, admin.c_str(), MAX_LENGTH_TOKEN);
        }

        void setUserPwd(String user)
        {
            strncpy(this->user_pwd, user.c_str(), MAX_LENGTH_TOKEN);
        }

        // Métodos Getter
        char *getAdminPwd()
        {
            return this->admin_pwd;
        }

        char *getUserPwd()
        {
            return this->user_pwd;
        }

        void setup()
        {
            this->ckeckToken.setup(60 * 1000, [&]()
                                   {
                                       for (size_t i = 0; i < MAX_SESSIONS; i++)
                                       {
                                           if (this->sessions[i].expires != 0 && this->sessions[i].expires < millis())
                                           {
                                               this->sessions[i].expires = 0;
                                               strncpy(this->sessions[i].token, "", MAX_LENGTH_TOKEN);
                                               this->sessions[i].isAdmin = false;
                                           }
                                       } });

            if (!LittleFS.begin())
            {
                Serial.println(F("An Error has occurred while mounting LittleFS"));
            }

            this->serveStatic("/", LittleFS, "/build").setDefaultFile("index.html").setCacheControl("max-age=31536000");
            this->onNotFound([&](AsyncWebServerRequest *request)
                             {
                                 if (request->method() == HTTP_OPTIONS)
                                 {
                                     request->send(200);
                                 }
                                 else
                                 {
                                     request->send(404);
                                 } });
        }

        void begin()
        {

            this->ws.onEvent(onWsEvent);
            this->addHandler(&ws);
            DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
            DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
            this->begin();
        }

        void loop()
        {
            this->ckeckToken.loop();
            this->ws.cleanupClients();
        }

        void wsTextAll(String message)
        {
            this->ws.textAll(message);
        }

        void resetLock()
        {
            this->unauthorized_counter = 0;
        }

        bool setToken(char *token, bool isAdmin)
        {
            bool result = false;
            for (int i = 0; i < MAX_SESSIONS; i++)
            {

                if (this->sessions[i].expires == 0)
                {
                    char token[MAX_LENGTH_TOKEN];
                    this->genToken(token);
                    this->sessions[i].expires = expiresTime + millis();
                    strncpy(this->sessions[i].token, token, MAX_LENGTH_TOKEN);
                    this->sessions[i].isAdmin = isAdmin;
                    result = true;
                    break;
                }
            }

            return result;
        }

        bool CheckToken(AsyncWebServerRequest *request)
        {

            if (this->unauthorized_counter > this->max_unauthorized_counter)
            {
                request->send(423, MIMETYPE_JSON, "{}");
                return false;
            }
            else if (request->hasHeader("token"))
            {
                AsyncWebHeader *h = request->getHeader("token");

                for (int i = 0; i < MAX_SESSIONS; i++)
                {
                    //  Serial.printf("Compara token: %s  >> %s\n", this->sessions[i], h->value().c_str());
                    if (this->sessions[i].token == h->value().c_str())
                    {
                        return true;
                    }
                }
                this->add_unauthorized_counter();
                request->send(403, MIMETYPE_JSON, "{}");
                return false;
            }
            else
            {
                this->add_unauthorized_counter();
                request->send(403, MIMETYPE_JSON, "{}");
            }
            return false;
        }

    private:
        unsigned long expiresTime = 324000000; // 90 minutos
        AsyncWebSocket ws = AsyncWebSocket("/ws");
        edwinspire::Interval ckeckToken;
        Session sessions[MAX_SESSIONS];
        byte unauthorized_counter = 0;
        byte max_unauthorized_counter = 10;
        char admin_pwd[20] = "";
        char user_pwd[20] = "";

        void responseLoginSuccess(AsyncWebServerRequest *request, String token)
        {
            AsyncWebServerResponse *response = request->beginResponse(200, MIMETYPE_JSON, "{\"token\": \"" + token + "\"}");
            response->addHeader(F("token"), token);
            request->send(response);
        }

        void responseLoginError(AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(401, MIMETYPE_JSON, F("{}"));
            response->addHeader(F("token"), F(""));
            request->send(response);
        }

        void responseLoginThereAreNoFreeSessions(AsyncWebServerRequest *request)
        {
            AsyncWebServerResponse *response = request->beginResponse(405, MIMETYPE_JSON, "{\"error\":\"Limit of active sessions has been exceeded\"}");
            response->addHeader(F("token"), F(""));
            request->send(response);
        }

        void add_unauthorized_counter()
        {
            if (this->unauthorized_counter < 200)
            {
                this->unauthorized_counter++;
            }
        }

        /*
        muestrame otra forma de hacer una petición a un API REST que está en otro dominio sobre HTTP desde una página que está en HTTPS?
        */

        void genToken(char *cadena)
        {
            const char caracteres[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_@#*.:|$%=+";

            for (int j = 0; j < MAX_LENGTH_TOKEN; j++)
            {
                cadena[j] = caracteres[random(sizeof(caracteres))];
            }
            cadena[MAX_LENGTH_TOKEN] = '\0';
        }
    };

}
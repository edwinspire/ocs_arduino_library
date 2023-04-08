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
#include <ArduinoJson.h>
#include "LittleFS.h"
#include <iostream>
#include <random>
#include <sstream>

namespace ocs
{

    const char json_key_mime_json[17] = "application/json";

    class WebAdmin : public AsyncWebServer
    {

    public:
        WebAdmin(int port) : AsyncWebServer(port)
        {
            Serial.printf("WebAdmin Port: %i\n", port);
        }

        void setup()
        {

            this->sessions[0] = "";
            this->sessions[1] = "";
            this->sessions[2] = "";
            this->sessions[3] = "";
            this->sessions[4] = "";

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
                                 }

                                 // request->send(404, F("text/plain"), F("Not found"));
                             });
        }

        void resetLock()
        {
            this->unauthorized_counter = 0;
        }

        String setToken()
        {

            String sid = String(this->GenSessionID());
            bool freespace = false;
            for (int i = 0; i < 5; i++)
            {

                //   Serial.printf("this->sessions: %d\n", this->sessions[i]);

                if (this->sessions[i].length() <= 1)
                {
                    this->sessions[i] = sid;
                    freespace = true;
                    // Serial.println("Se ha seteado");
                    break;
                }
            }

            if (!freespace)
            {
                this->sessions[0] = sid;
            }

            return String(sid);
        }

        bool CheckToken(AsyncWebServerRequest *request)
        {

            if (this->unauthorized_counter > this->max_unauthorized_counter)
            {
                request->send(423, json_key_mime_json, "{}");
                return false;
            }
            else if (request->hasHeader("token"))
            {
                AsyncWebHeader *h = request->getHeader("token");

                for (int i = 0; i < 5; i++)
                {
                    //  Serial.printf("Compara token: %s  >> %s\n", this->sessions[i], h->value().c_str());
                    if (this->sessions[i] == h->value().c_str())
                    {
                        return true;
                    }
                }
                this->add_unauthorized_counter();
                request->send(403, json_key_mime_json, "{}");
                return false;
            }
            else
            {
                this->add_unauthorized_counter();
                request->send(403, json_key_mime_json, "{}");
            }
            return false;
        }

    private:
        byte unauthorized_counter = 0;
        byte max_unauthorized_counter = 10;

        void add_unauthorized_counter()
        {
            if (this->unauthorized_counter < 200)
            {
                this->unauthorized_counter++;
            }
        }

        unsigned long GenSessionID()
        {
            std::random_device rd;
            std::mt19937_64 eng(rd());
            std::uniform_int_distribution<unsigned long long> distr;

            // Generar un número aleatorio de 64 bits
            return distr(eng);
        }

        String sessions[5];
    };
}
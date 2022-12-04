#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

namespace ocs
{

    class WebAdmin : public AsyncWebServer
    {

    public:
        WebAdmin(int port) : AsyncWebServer(port)
        {
            Serial.printf("WebAdmin Port: %i\n", port);
        }

#ifdef ESP32
        void setup()
        {

            // Initialize
            if (!SPIFFS.begin(true))
            {
                Serial.println("An Error has occurred while mounting SPIFFS");
                // return;
            }

            this->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(SPIFFS, F("/index.html"), F("text/html")); });

            this->on("/build/bundle.css", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(SPIFFS, F("/bundle.css"), F("text/css")); });

            this->on("/build/bundle.js", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(SPIFFS, F("/bundle.js"), F("application/javascript")); });
            this->onNotFound([&](AsyncWebServerRequest *request)
                             { request->send(404, F("text/plain"), "Not found"); });
        }

#elif defined(ESP8266)
        void setup()
        {

       if (!LittleFS.begin())
                {
                    Serial.println("An Error has occurred while mounting LittleFS");
                }
         

            this->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/index.html"), F("text/html")); });

            this->on("/build/bundle.css", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/bundle.css"), F("text/css")); });

            this->on("/build/bundle.js", HTTP_GET, [&](AsyncWebServerRequest *request)
                     { request->send(LittleFS, F("/bundle.js"), F("application/javascript")); });
            this->onNotFound([&](AsyncWebServerRequest *request)
                             { request->send(404, F("text/plain"), "Not found"); });
        }

#endif

    private:
    };
}
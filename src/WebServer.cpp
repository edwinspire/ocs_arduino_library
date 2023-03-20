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

namespace ocs
{

    class WebAdmin : public AsyncWebServer
    {

    public:
        WebAdmin(int port) : AsyncWebServer(port)
        {
            Serial.printf("WebAdmin Port: %i\n", port);
        }

        void setup()
        {
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

        /*
                void setCrossOrigin()
                {
                    this.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
                    this->.sendHeader(F("Access-Control-Max-Age"), F("600"));
                    server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
                    server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
                };
                */

    private:
    };
}
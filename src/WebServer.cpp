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
                             { request->send(404, F("text/plain"), F("Not found")); });
        }

    private:
    };
}
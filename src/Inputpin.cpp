#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <Configuration.cpp>

namespace ocs
{

    namespace input
    {

        enum Status : uint8_t
        {
            TROUBLE = 3,
            NORMAL = 1,
            ALARM = 2,
            UNDEFINED = 0
        };

        class Input
        {

        private:
            uint16_t value;
            float upper_threshold = 0;
            float lower_threshold = 0;
            unsigned long last_time;
            unsigned long interval = 500;
            float zone_threshold = 45;
            Status last_status = Status::UNDEFINED;
            typedef std::function<void(byte position, byte gpio, uint16_t value, Status status)> ChangeStatusCallback;
            ChangeStatusCallback eventCallback = nullptr;

        public:
            Status status = Status::UNDEFINED;
            // Configure config;
            bool enabled = false;
            byte gpio = 255;
            ZoneType type = ZoneType::UNUSED;
            ContactType contact_type = ContactType::NORMALLY_CLOSED;
            byte position = 0;

            DynamicJsonDocument toJson()
            {
                DynamicJsonDocument doc(256);
                doc[json_key_value] = this->getvalue();
                doc[json_key_gpio] = this->gpio;
                doc[json_key_status] = this->status;
                return doc;
            }

            int getvalue()
            {
                this->value = 0;
                if (this->enabled)
                {
#ifdef ESP32
                    if (this->gpio != 0)
                    {
                        this->value = analogRead(this->gpio); // read the input pin
                    }
                    else
                    {
                        Serial.println(F("En ESP32 no está permitido usar el GPIO como entrada."));
                        this->enabled = false;
                    }
#elif defined(ESP8266)
                    this->value = analogRead(this->gpio); // read the input pin
#endif
                }
                return this->value;
            }

            void setup(byte position, byte gpio_input, bool enabled, ZoneType type, ContactType contact_type)
            {
                Serial.print(F("Setup Input => "));
                Serial.println(gpio_input);
                this->position = position;

                float center = 4096 / 2;
                upper_threshold = center + ((this->zone_threshold / 100) * center);
                lower_threshold = center - ((this->zone_threshold / 100) * center);

                // this->config.name = name;
                this->gpio = gpio_input;
                this->enabled = enabled;
                this->type = type;
                // this->config.siren_type = siren_type;
                this->contact_type = contact_type;

                if (this->enabled)
                {

#ifdef ESP32
                    pinMode(0, OUTPUT);
                    if (this->gpio != 0)
                    {
                        pinMode(this->gpio, INPUT);
                    }
                    else
                    {
                        Serial.println(F("En ESP32 no está permitido usar el GPIO como entrada."));
                    }
#elif defined(ESP8266)
                    pinMode(this->gpio, INPUT);
#endif
                }
            }

            bool changed()
            {
                bool Change = false;

                if (millis() - last_time > interval)
                {

                    this->last_time = millis();

                    this->getvalue();

                    if (this->value <= lower_threshold)
                    {
                        this->status = Status::ALARM;
                    }
                    else if (this->value >= upper_threshold)
                    {
                        this->status = Status::TROUBLE;
                    }
                    else
                    {
                        this->status = Status::NORMAL;
                    }

                    if (last_status != status)
                    {
                        Change = true;
                        last_status = this->status;
                        this->eventCallback(this->position, this->gpio, this->value, this->status);
                    }
                }

                return Change;
            }

            void loop()
            {
                if (this->enabled && this->type != ZoneType::UNUSED && this->gpio != 255)
                {
                    this->changed();
                }
            }

            // Establecer el callback del evento
            void onChangeStatus(ChangeStatusCallback callback)
            {
                this->eventCallback = callback;
            }
        };
    };
}
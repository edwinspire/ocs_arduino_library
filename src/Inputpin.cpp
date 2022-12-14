#include <Arduino.h>
#include <ArduinoJson.h>

namespace ocs
{

    namespace input
    {

        const char json_key_enabled[8] = "enabled";
        const char json_key_name[5] = "name";
        const char json_key_gpio[5] = "gpio";

        enum SirenType
        {
            UNABLED = 0,
            SILENT = 1,
            CONTINUOUS = 2,
            PULSING = 3,
            TEST = 4
        };

        enum Type
        {
            NONE = 0,
            ALARM_MEDICAL = 100,
            ALARM_FIRE = 101,
            ALARM_PANIC = 102,
            ALARM_BURGLARY = 103,
            ALARM_GENERAL = 104,
            ALARM_24H = 105
        };

        enum Status
        {
            TROUBLE = 3,
            NORMAL = 1,
            ALARM = 2,
            UNDEFINED = 0
        };

        enum ContactType
        {
            NORMALLY_CLOSED = 1,
            NORMALLY_OPENED = 2
        };

        struct Configure
        {
            Type type = Type::NONE;
            SirenType siren_type = SirenType::UNABLED;
            boolean enabled = false;

            String name = "Input";
            byte gpio = 255;
            ContactType contact_type = ContactType::NORMALLY_CLOSED;

            DynamicJsonDocument toJson()
            {
                this->set_default();
                DynamicJsonDocument doc(256);
                doc[input::json_key_gpio] = this->gpio;
                doc[input::json_key_enabled] = this->enabled;
                doc[F("type")] = this->type;
                doc[F("siren_type")] = this->siren_type;
                doc[F("contact_type")] = this->contact_type;                
                doc[input::json_key_name] = this->name;                
               
              //  Serial.println(F("Input Configure toJson"));

             //  Serial.println(" __>>>>> NAME  " + String(input::json_key_name)+ " ->>> " +String(this->name));

            //    serializeJsonPretty(doc, Serial);
                return doc;
            }

            void set_default()
            {

                if (this->name == NULL)
                {
                    this->name = "";
                }
            }

            void fromJson(DynamicJsonDocument data)
            {

                //Serial.println(">>>< Input Configure from JSON");

                //              serializeJsonPretty(data, Serial);

                this->set_default();
                this->gpio = data[input::json_key_gpio].as<byte>();
                this->type = data[F("type")].as<Type>();
                this->siren_type = data[F("siren_type")].as<SirenType>();
                this->contact_type = data[F("contact_type")].as<ContactType>();
                this->enabled = data[input::json_key_enabled].as<boolean>();
                this->name = data[input::json_key_name].as<String>();
                

               // Serial.println(" name " + this->name);
               // Serial.println(" json_key_gpio " + String(this->gpio));
               // Serial.println(" siren_type " + String(this->siren_type));

            }
        };

        class Input
        {

        private:
            unsigned int value;
            float upper_threshold = 0;
            float lower_threshold = 0;
            unsigned long last_time;
            unsigned long interval = 500;
            float zone_threshold = 45;
            Status last_status = Status::UNDEFINED;

        public:
            // bool enabled = false;
            Status status = Status::UNDEFINED;
            // String name = "Physical Button";
            // byte gpio = 255;
            Configure config;

            StaticJsonDocument<250> toJson()
            {
                StaticJsonDocument<250> doc;
                doc[F("config")] = this->config.toJson();
                doc[F("status")] = this->status;
                doc[F("value")] = this->getvalue();
                // Serial.println(F("Input toJson => "));
                // serializeJsonPretty(doc, Serial);
                return doc;
            }

            int getvalue()
            {
                this->value = 0;
                if (this->config.enabled)
                {
#ifdef ESP32
                    if (this->config.gpio != 0)
                    {
                        this->value = analogRead(this->config.gpio); // read the input pin
                    }
                    else
                    {
                        Serial.println(F("En ESP32 no est?? permitido usar el GPIO como entrada."));
                        this->config.enabled = false;
                    }
#elif defined(ESP8266)
                    this->value = analogRead(this->config.gpio); // read the input pin
#endif
                }
                return this->value;
            }

            void setup(Configure conf)
            {
                this->setup(conf.gpio, conf.name, conf.enabled, conf.type, conf.siren_type, conf.contact_type);
            }

            void setup(byte gpio_input, String name, bool enabled, Type type, SirenType siren_type, ContactType contact_type)
            {
                Serial.print(F("Setup Input => "));
                Serial.println(gpio_input);

                float center = 4096 / 2;
                upper_threshold = center + ((this->zone_threshold / 100) * center);
                lower_threshold = center - ((this->zone_threshold / 100) * center);

                this->config.name = name;
                this->config.gpio = gpio_input;
                this->config.enabled = enabled;
                this->config.type = type;
                this->config.siren_type = siren_type;
                this->config.contact_type = contact_type;

                if (this->config.name == NULL || this->config.name.length() < 1)
                {
                    this->config.name = "Input " + String(this->config.gpio);
                }

                if (this->config.enabled)
                {

#ifdef ESP32
                    pinMode(0, OUTPUT);
                    if (this->config.gpio != 0)
                    {
                        pinMode(this->config.gpio, INPUT);
                    }
                    else
                    {
                        Serial.println(F("En ESP32 no est?? permitido usar el GPIO como entrada."));
                    }
#elif defined(ESP8266)
                    pinMode(this->config.gpio, INPUT);
#endif
                }
            }

            bool changed()
            {
                bool Change = false;

                if (this->config.enabled && millis() - last_time > interval)
                {

                    this->last_time = millis();

                    this->getvalue();

                    if (this->value <= lower_threshold)
                    {
                        this->status = Status::ALARM;
                        // Serial.println("ALARMA");
                    }
                    else if (this->value >= upper_threshold)
                    {
                        this->status = Status::TROUBLE;
                        // Serial.println("PROBLEMA");
                    }
                    else
                    {
                        this->status = Status::NORMAL;
                        // Serial.println("NORMAL");
                    }

                    if (last_status != status)
                    {
                        Change = true;
                        last_status = this->status;
                    }
                }

                return Change;
            }
        };
    };
}
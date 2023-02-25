#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

namespace ocs
{

#ifdef ESP32
    const unsigned int EEPROM_SIZE_DEFAULT = 4096;
#elif defined(ESP8266)
    const unsigned int EEPROM_SIZE_DEFAULT = 2048;
#endif

    class LocalStore
    {

    private:
    public:
        DynamicJsonDocument data;

        LocalStore() : data(ocs::EEPROM_SIZE_DEFAULT) {}

        static bool validateJson(const char *json)
        {
            StaticJsonDocument<0> doc, filter;
            return deserializeJson(doc, json, DeserializationOption::Filter(filter)) == DeserializationError::Ok;
        }

        static bool validateJson(char json[])
        {
            StaticJsonDocument<0> doc, filter;
            return deserializeJson(doc, json, DeserializationOption::Filter(filter)) == DeserializationError::Ok;
        }

        static String json()
        {

            char eeprom_data[ocs::EEPROM_SIZE_DEFAULT];
            EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT); // tamaño maximo 4096 bytes

            unsigned int i = 0;
            //   Serial.print("5");
            while (i < ocs::EEPROM_SIZE_DEFAULT)
            {
                eeprom_data[i] = EEPROM.read(i);
                i++;
            }
            // Serial.print("6");
            EEPROM.end();
            Serial.print(F("EEPROM Data: "));
//            Serial.println(eeprom_data);

            if (!validateJson(eeprom_data))
            {

                eeprom_data[0] = '{';
                eeprom_data[1] = '}';
                eeprom_data[2] = '\0';
            }
            else
            {
                Serial.println(eeprom_data);
            }

            return eeprom_data;
        }

        static DynamicJsonDocument read()
        {
/*
            char eeprom_data[ocs::EEPROM_SIZE_DEFAULT];
            EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT); // tamaño maximo 4096 bytes

            unsigned int i = 0;
            //   Serial.print("5");
            while (i < ocs::EEPROM_SIZE_DEFAULT)
            {
                eeprom_data[i] = EEPROM.read(i);
                i++;
            }
            // Serial.print("6");
            EEPROM.end();
            Serial.print(F("EEPROM Data: "));
            Serial.println(eeprom_data);
*/


            DynamicJsonDocument doc(ocs::EEPROM_SIZE_DEFAULT);
            DeserializationError err = deserializeJson(doc, json());
            // Serial.print("8");
            if (err)
            {
                Serial.print(F("read deserializeJson() failed: "));
                Serial.println(err.c_str());

                EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT); // tamaño maximo 4096 bytes
                // char default_json[2] = "{}";
                unsigned int i = 0;
                while (i < ocs::EEPROM_SIZE_DEFAULT)
                {
                    if (i == 0)
                    {
                        EEPROM.write(i, '{');
                    }
                    else if (i == 1)
                    {
                        EEPROM.write(i, '}');
                    }
                    else
                    {
                        EEPROM.write(i, 255);
                    }

                    i++;
                }
                EEPROM.commit();
                EEPROM.end();
            }
            else
            {
                Serial.println(F("LOCALSTORAGE :==> Read"));
                //serializeJsonPretty(doc, Serial);
            }

            return doc;
        }

        static bool save(const DynamicJsonDocument & doc)
        {
            bool r = false;
            Serial.println(F("--> Save - Cambios pendientes "));
            String data_serialized = "";
            serializeJson(doc, data_serialized);

            Serial.println(data_serialized);
            EEPROM.begin(ocs::EEPROM_SIZE_DEFAULT);

            // Length (with one extra character for the null terminator)
            unsigned int str_len = data_serialized.length() + 1;

            // Prepare the character array (the buffer)
            char char_array[str_len];

            // Copy it over
            data_serialized.toCharArray(char_array, str_len);

            unsigned int i = 0;
            while (i < str_len)
            {
                EEPROM.write(i, char_array[i]);
                i++;
            }

            r = EEPROM.commit();
            EEPROM.end();

            if (!r)
            {
                Serial.println(F("Save EEPROM Fail!!"));
            }

            return r;
        }
    };

}

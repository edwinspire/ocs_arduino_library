#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

namespace ocs
{
    const unsigned int EEPROM_SIZE_DEFAULT = 2816;
    class LocalStore
    {

    private:
    public:
        DynamicJsonDocument data;

        LocalStore() : data(ocs::EEPROM_SIZE_DEFAULT) {}

        static DynamicJsonDocument read()
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
            //  Serial.print("7");
            Serial.println(eeprom_data);
            DynamicJsonDocument doc(ocs::EEPROM_SIZE_DEFAULT);
            DeserializationError err = deserializeJson(doc, eeprom_data);
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

            return doc;
        }

        static bool save(DynamicJsonDocument doc)
        {
            bool r = false;
            Serial.println("--> Save - Cambios pendientes ");
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

            return r;
        }
    };

}

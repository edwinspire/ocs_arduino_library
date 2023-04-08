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
        static char *json()
        {
            static char json[ocs::EEPROM_SIZE_DEFAULT];
            serializeJson(read(), json, ocs::EEPROM_SIZE_DEFAULT);
            return json;
        }

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

            // eeprom_data[ocs::EEPROM_SIZE_DEFAULT] = '\0';

            Serial.print(F("EEPROM Data: "));
            Serial.println(eeprom_data);

            DynamicJsonDocument doc(ocs::EEPROM_SIZE_DEFAULT);
            DeserializationError err = deserializeJson(doc, eeprom_data);

            if (err)
            {
                Serial.print(F("read deserializeJson() failed: "));
                Serial.println(err.c_str());
                DynamicJsonDocument doc_default(8);
                doc_default[F("ocs")] = "empty";
                save(doc_default);
                doc_default.clear();
            }

            return doc;
        }

        static bool save(const DynamicJsonDocument &doc)
        {
            bool r = false;
            Serial.println(F("--> Save - Cambios pendientes "));
            String data_serialized = "";
            serializeJson(doc, data_serialized);

            Serial.println(data_serialized);

            // Serial.println(data_serialized);
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

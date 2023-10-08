#include <Arduino.h>
#include <ArduinoJson.h>
#include <Interval.cpp>
#include <LocalStore.cpp>

#pragma once

#ifndef MAX_OUTPUTS
MAX_OUTPUTS = 1
#endif

#ifndef MAX_SSID
    MAX_SSID = 5
#endif

#ifndef MAX_OUTPUTS_LINKED
    MAX_OUTPUTS_LINKED = 2
#endif

    const char json_key_name[5] = "name";
const char json_key_device_id[10] = "device_id";
const char json_key_model[6] = "model";
const char json_key_version[8] = "version";
const char json_key_pwd_adm[8] = "pwd_adm";
const char json_key_pwd_user[9] = "pwd_user";
const char json_key_delay[6] = "delay";

const char json_key_websocket_server[9] = "wsserver";
const char json_key_username[9] = "username";
const char json_key_password[4] = "pwd";
const char json_key_certificate[5] = "cert";

// const char json_key_up[3] = "up";
// const char json_key_down[5] = "down";
// const char json_key_time[5] = "time";

const char json_key_gpio[5] = "gpio";
const char json_key_enabled[8] = "enabled";
const char json_key_value[4] = "val";

const char json_key_contact_type[3] = "ct";
const char json_key_zone_type[3] = "zt";
const char json_key_report[7] = "report";
const char json_key_interval[4] = "int";
const char json_key_is_secure[4] = "sec";

const char json_key_device[4] = "dev";
const char json_key_server[4] = "srv";
const char json_key_input[2] = "i";
const char json_key_output[2] = "o";

const char json_key_status[7] = "status";
const char json_key_info[5] = "info";

const char json_key_ssid[5] = "ssid";

const char json_key_armed[6] = "armed";

const char json_key_event[6] = "event";

const char json_key_data[5] = "data";
const char json_key_label[6] = "label";
const char json_key_ip[3] = "ip";
const char json_key_response[5] = "resp";

const char json_key_led[4] = "led";

const char json_key_low_time[8] = "lowtime";
const char json_key_high_time[9] = "hightime";
const char json_key_total_time[10] = "totaltime";

class SSID
{
private:
    String ssid = "user";
    String password = "pwd";

public:
    bool isChanged = false;

    DynamicJsonDocument toJson()
    {
        DynamicJsonDocument jsonDocument(64);

        // Agrega las propiedades al objeto JSON
        jsonDocument[json_key_ssid] = ssid;
        jsonDocument[json_key_password] = password;

        return jsonDocument;
    }

    void fromJson(const DynamicJsonDocument &jsonDocument)
    {
        // Verifica si las propiedades existen en el JSON antes de asignarlas
        if (jsonDocument.containsKey(json_key_ssid))
        {
            ssid = jsonDocument[json_key_ssid].as<String>();
        }
        if (jsonDocument.containsKey(json_key_password))
        {
            password = jsonDocument[json_key_password].as<String>();
        }
        this->isChanged = true;
    }

    // Getter para up
    String getSSID() const
    {
        return ssid;
    }

    // Setter para up
    void setSSID(const String newValue)
    {
        ssid = newValue;
        isChanged = true;
    }

    // Getter para down
    String getPassword() const
    {
        return password;
    }

    // Setter para down
    void setPassword(const String newValue)
    {
        password = newValue;
        isChanged = true;
    }
};

// Definir una estructura para las salidas
class Output
{
private:
    byte gpio = 255;
    bool enabled = false;
    String name = "";
    String label = "";

public:
    bool isChanged = false;

    DynamicJsonDocument toJson()
    {
        DynamicJsonDocument jsonDocument(64);
        // Agrega las propiedades al objeto JSON
        jsonDocument[json_key_gpio] = gpio;
        jsonDocument[json_key_enabled] = enabled;
        jsonDocument[json_key_name] = name;
        jsonDocument[json_key_label] = label;
        return jsonDocument;
    }

    void fromJson(const DynamicJsonDocument &jsonDocument)
    {
        // Verifica si las propiedades existen en el JSON antes de asignarlas
        if (jsonDocument.containsKey(json_key_gpio))
        {
            gpio = jsonDocument[json_key_gpio].as<byte>();
        }
        if (jsonDocument.containsKey(json_key_enabled))
        {
            enabled = jsonDocument[json_key_enabled].as<bool>();
        }
        if (jsonDocument.containsKey(json_key_name))
        {
            name = jsonDocument[json_key_name].as<String>();
        }
        if (jsonDocument.containsKey(json_key_label))
        {
            label = jsonDocument[json_key_label].as<String>();
        }
        this->isChanged = true;
    }

    // Getter para gpio
    byte getGpio() const
    {
        return gpio;
    }

    // Setter para gpio
    void setGpio(byte newValue)
    {
        gpio = newValue;
        isChanged = true;
    }

    // Getter para enabled
    bool getEnabled() const
    {
        return enabled;
    }

    // Setter para enabled
    void setEnabled(bool newValue)
    {
        enabled = newValue;
        isChanged = true;
    }

    // Getter para name
    String getName() const
    {
        return name;
    }

    // Setter para name
    void setName(const String &newValue)
    {
        name = newValue;
        isChanged = true;
    }

    // Getter para label
    String getLabel() const
    {
        return label;
    }

    // Setter para label
    void setLabel(const String &newValue)
    {
        label = newValue;
        isChanged = true;
    }
};

// Contact Type Enum 1: Normal Open, 2: Normal Close, 0: No used,
enum ContactType : uint8_t
{
    NORMALLY_CLOSED = 1,
    NORMALLY_OPENED = 2
};

// Zone Type
//  Enum 0: Normal, 1: Delay, 2: Always, 3: Interior, 6: Switch, 7: Toggle, 8: Arma/Disarm
enum ZoneType : uint8_t
{
    UNUSED = 0,
    NORMAL = 1,
    DELAY = 2,
    ALWAYS = 3,
    INTERIOR = 4,
    BUTTON = 5,
    TOGGLE = 6,
    ARM_DISARM = 7
};

enum ArmedType : uint8_t
{
    DISARMED = 0,
    INSTANT = 1,    // Cuando se desea armar todo el sistema de forma inmediata
    WITH_DELAY = 2, // Cuando se arma el sistema con las zonas de retardo habilitadas
    PERIMETER = 3   // Arma toda la zona perimetral y no toma en cuenta las zonas interiores
};

class LinkedOutputs
{
private:
    byte gpio;
    unsigned long lowTime;
    unsigned long highTime;
    unsigned long totalTime;

public:
    bool isChanged = false;

    unsigned long getLowTime() const
    {
        return lowTime;
    }

    void setLowTime(unsigned long newValue)
    {
        lowTime = newValue;
        isChanged = true;
    }

    unsigned long geHighTime() const
    {
        return highTime;
    }

    void setHighTime(unsigned long newValue)
    {
        highTime = newValue;
        isChanged = true;
    }

    unsigned long geTotalTime() const
    {
        return totalTime;
    }

    void setTotalTime(unsigned long newValue)
    {
        totalTime = newValue;
        isChanged = true;
    }

    // Getter para gpio
    byte getGpio() const
    {
        return gpio;
    }

    // Setter para gpio
    void setGpio(byte newGpio)
    {
        gpio = newGpio;
        isChanged = true;
    }

    // Método toJson para convertir a JSON
    DynamicJsonDocument toJson() const
    {
        DynamicJsonDocument jsonDocument(16);
        jsonDocument[json_key_gpio] = gpio;
        jsonDocument[json_key_low_time] = lowTime;
        jsonDocument[json_key_high_time] = highTime;
        jsonDocument[json_key_total_time] = totalTime;
        return jsonDocument;
    }

    // Método fromJson para asignar valores desde JSON
    void fromJson(const DynamicJsonDocument &jsonDocument)
    {
        if (jsonDocument.containsKey(json_key_gpio))
        {
            gpio = jsonDocument[json_key_gpio].as<byte>();
        }
        /*
        if (jsonDocument.containsKey(json_key_interval))
        {
            interval = jsonDocument[json_key_interval].as<byte>();
        }
        */
        isChanged = true;
    }
};

// Definir una estructura para las entradas
class Input
{
private:
    byte gpio = 255;
    String name = "";
    String label = "";
    bool enabled = false;
    ContactType contact_type;
    ZoneType zone_type;
    bool report = false; // Reportar evento a grupo telegram - Sim embargo siempre reportará de forma individual a los usuarios que hayan registrado el dispositivo como propio
    LinkedOutputs outs[MAX_OUTPUTS_LINKED];
    // byte interval = 0;

public:
    bool isChanged = false;

    DynamicJsonDocument toJson()
    {
        DynamicJsonDocument jsonDocument(64);

        // Agrega las propiedades al objeto JSON
        jsonDocument[json_key_gpio] = gpio;
        jsonDocument[json_key_name] = name;
        jsonDocument[json_key_label] = label;
        jsonDocument[json_key_enabled] = enabled;
        jsonDocument[contact_type] = static_cast<byte>(contact_type);
        jsonDocument[zone_type] = static_cast<byte>(zone_type);
        jsonDocument[json_key_report] = report;
        // jsonDocument[json_key_interval] = interval;

        for (int i = 0; i < MAX_OUTPUTS_LINKED; i++)
        {
            jsonDocument[json_key_output][i] = outs[i].toJson();
        }

        return jsonDocument;
    }

    void fromJson(const DynamicJsonDocument &jsonDocument)
    {
        if (jsonDocument.containsKey(json_key_gpio))
        {
            gpio = jsonDocument[json_key_gpio].as<byte>();
        }
        if (jsonDocument.containsKey(json_key_name))
        {
            name = jsonDocument[json_key_name].as<String>();
        }
        if (jsonDocument.containsKey(json_key_label))
        {
            label = jsonDocument[json_key_label].as<String>();
        }
        if (jsonDocument.containsKey(json_key_enabled))
        {
            enabled = jsonDocument[json_key_enabled].as<bool>();
        }
        if (jsonDocument.containsKey(json_key_contact_type))
        {
            contact_type = static_cast<ContactType>(jsonDocument[json_key_contact_type].as<byte>());
        }
        if (jsonDocument.containsKey(json_key_zone_type))
        {
            zone_type = static_cast<ZoneType>(jsonDocument[json_key_zone_type].as<byte>());
        }
        if (jsonDocument.containsKey(json_key_report))
        {
            report = jsonDocument[json_key_report].as<bool>();
        }
        /*
        if (jsonDocument.containsKey(json_key_interval))
        {
            interval = jsonDocument[json_key_interval].as<byte>();
        }
        */

        if (jsonDocument.containsKey(json_key_output))
        {
            //            JsonArray outsArray = jsonDocument[json_key_output].as<JsonArray>();
            for (int i = 0; i < MAX_OUTPUTS_LINKED; i++)
            {
                outs[i].fromJson(jsonDocument[json_key_output][i]);
            }
        }

        // Establece isChanged en true
        isChanged = true;
    }

    // Getter para outs
    LinkedOutputs *getOuts()
    {
        return outs;
    }

    // Setter para outs
    void setOuts(const LinkedOutputs *newOuts)
    {
        for (int i = 0; i < MAX_OUTPUTS_LINKED; i++)
        {
            outs[i] = newOuts[i];
        }
        isChanged = true;
    }

    // Getter para obtener el valor de gpio
    byte getGpio() const
    {
        return gpio;
    }

    // Setter para establecer el valor de gpio
    void setGpio(byte newGpio)
    {
        gpio = newGpio;
        isChanged = true;
    }

    // Getter para obtener el valor de name
    String getName() const
    {
        return name;
    }

    // Setter para establecer el valor de name
    void setName(const String &newName)
    {
        name = newName;
        isChanged = true;
    }

    // Getter para obtener el valor de label
    String getLabel() const
    {
        return label;
    }

    // Setter para establecer el valor de label
    void setLabel(const String &newlabel)
    {
        label = newlabel;
        isChanged = true;
    }

    // Getter para obtener el valor de enabled
    bool getEnabled() const
    {
        return enabled;
    }

    // Setter para establecer el valor de enabled
    void setEnabled(bool newEnabled)
    {
        enabled = newEnabled;
        isChanged = true;
    }

    // Getter para obtener el valor de contact_type
    ContactType getContactType() const
    {
        return contact_type;
    }

    // Setter para establecer el valor de contact_type
    void setContactType(ContactType newContactType)
    {
        contact_type = newContactType;
        isChanged = true;
    }

    // Getter para obtener el valor de zone_type
    ZoneType getZoneType() const
    {
        return zone_type;
    }

    // Setter para establecer el valor de zone_type
    void setZoneType(ZoneType newZoneType)
    {
        zone_type = newZoneType;
        isChanged = true;
    }

    // Getter para obtener el valor de report
    bool getReport() const
    {
        return report;
    }

    // Setter para establecer el valor de report
    void setReport(bool newReport)
    {
        report = newReport;
        isChanged = true;
    }

    /*
        // Getter para obtener el valor de interval
        byte getInterval() const
        {
            return interval;
        }

        // Setter para establecer el valor de interval
        void setInterval(byte newInterval)
        {
            interval = newInterval;
            isChanged = true;
        }
        */
};

class Device
{
private:
    String name = "device";
    String device_id = "000000";
    String model = "";
    String version = "";
    String pwd_adm = "9999999999";
    String pwd_user = "0000000000";
    byte entry_delay = 60;

public:
    bool isChanged = false;
    // Getter para obtener el valor de name
    String getName() const
    {
        return name;
    }

    // Setter para establecer el valor de name
    void setName(const String &newName)
    {
        name = newName;
        this->isChanged = true;
    }

    // Getter para obtener el valor de device_id
    String getDeviceID() const
    {
        return device_id;
    }

    // Setter para establecer el valor de device_id
    void setDeviceID(const String &newDeviceID)
    {
        device_id = newDeviceID;
        this->isChanged = true;
    }

    // Getter para obtener el valor de model
    String getModel() const
    {
        return model;
    }

    // Setter para establecer el valor de model
    void setModel(const String &newModel)
    {
        model = newModel;
        this->isChanged = true;
    }

    // Getter para obtener el valor de version
    String getVersion() const
    {
        return version;
    }

    // Setter para establecer el valor de version
    void setVersion(const String &newVersion)
    {
        version = newVersion;
        this->isChanged = true;
    }

    // Getter para obtener el valor de pwd_adm
    String getPwdAdm() const
    {
        return pwd_adm;
    }

    // Setter para establecer el valor de pwd_adm
    void setPwdAdm(const String &newPwdAdm)
    {
        pwd_adm = newPwdAdm;
        this->isChanged = true;
    }

    // Getter para obtener el valor de pwd_user
    String getPwdUser() const
    {
        return pwd_user;
    }

    // Setter para establecer el valor de pwd_user
    void setPwdUser(const String &newPwdUser)
    {
        pwd_user = newPwdUser;
        this->isChanged = true;
    }

    // Getter para obtener el valor de entry_delay
    byte getEntryDelay() const
    {
        return entry_delay;
    }

    // Setter para establecer el valor de entry_delay
    void setEntryDelay(byte newEntryDelay)
    {
        entry_delay = newEntryDelay;
        this->isChanged = true;
    }

    void fromJson(const DynamicJsonDocument &jsonDocument)
    {
        if (jsonDocument.containsKey(json_key_name))
        {
            name = jsonDocument[json_key_name].as<String>();
        }
        if (jsonDocument.containsKey(json_key_device_id))
        {
            device_id = jsonDocument[json_key_device_id].as<String>();
        }
        if (jsonDocument.containsKey(json_key_model))
        {
            model = jsonDocument[json_key_model].as<String>();
        }
        if (jsonDocument.containsKey(json_key_version))
        {
            version = jsonDocument[json_key_version].as<String>();
        }
        if (jsonDocument.containsKey(json_key_pwd_adm))
        {
            pwd_adm = jsonDocument[json_key_pwd_adm].as<String>();
        }
        if (jsonDocument.containsKey(json_key_pwd_user))
        {
            pwd_user = jsonDocument[json_key_pwd_user].as<String>();
        }
        if (jsonDocument.containsKey(json_key_delay))
        {
            entry_delay = jsonDocument[json_key_delay].as<byte>();
        }

        // Establece isChanged en true
        isChanged = true;
    }

    DynamicJsonDocument toJson()
    {
        // Crea un objeto JSON
        DynamicJsonDocument doc(256);

        // Agrega las propiedades al objeto JSON
        doc[json_key_name] = name;
        doc[json_key_device_id] = device_id;
        doc[json_key_model] = model;
        doc[json_key_version] = version;
        doc[json_key_pwd_adm] = pwd_adm;
        doc[json_key_pwd_user] = pwd_user;
        doc[json_key_delay] = entry_delay;

        return doc;
    }
};

class Srv
{
private:
    String server = "";
    String username = "superuser";
    String password = "superuser";
    String certificate = "HGHJJHGHJRTT";
    bool is_secure = false;

public:
    bool isChanged = false;

    void fromJson(const DynamicJsonDocument &jsonDocument)
    {
        if (jsonDocument.containsKey(json_key_websocket_server))
        {
            server = jsonDocument[json_key_websocket_server].as<String>();
        }
        if (jsonDocument.containsKey(json_key_username))
        {
            username = jsonDocument[json_key_username].as<String>();
        }
        if (jsonDocument.containsKey(json_key_password))
        {
            password = jsonDocument[json_key_password].as<String>();
        }
        if (jsonDocument.containsKey(json_key_certificate))
        {
            certificate = jsonDocument[json_key_certificate].as<String>();
        }

        if (jsonDocument.containsKey(json_key_is_secure))
        {
            is_secure = jsonDocument[json_key_is_secure].as<bool>();
        }
        // Establece isChanged en true
        isChanged = true;
    }

    DynamicJsonDocument toJson()
    {
        // Crea un objeto JSON
        DynamicJsonDocument doc(256);

        // Agrega las propiedades al objeto JSON
        doc[json_key_websocket_server] = server;
        doc[json_key_username] = username;
        doc[json_key_password] = password;
        doc[json_key_certificate] = certificate;
        doc[json_key_is_secure] = this->is_secure;

        return doc;
    }

    // Getter para obtener el valor de websocket_server
    String getServer() const
    {
        return server;
    }

    // Setter para establecer el valor de websocket_server
    void setServer(const String &newwebsocket_server)
    {
        server = newwebsocket_server;
        this->isChanged = true;
    }

    String getUrlServer() const
    {
        if (this->is_secure)
        {
            return "wss://" + username + ":" + password + "@" + this->server;
        }
        else
        {
            return "ws://" + username + ":" + password + "@" + this->server;
        }
    }

    // Getter para obtener el valor de websocket_server
    bool getSecure() const
    {
        return is_secure;
    }

    // Setter para establecer el valor de websocket_server
    void setSecure(bool secure)
    {
        is_secure = secure;
        this->isChanged = true;
    }

    // Getter para obtener el valor de username
    String getUsername() const
    {
        return username;
    }

    // Setter para establecer el valor de username
    void setUsername(const String &newUsername)
    {
        username = newUsername;
        this->isChanged = true;
    }

    // Getter para obtener el valor de password
    String getPassword() const
    {
        return password;
    }

    // Setter para establecer el valor de password
    void setPassword(const String &newPassword)
    {
        password = newPassword;
        this->isChanged = true;
    }

    // Getter para obtener el valor
    String getcertificate() const
    {
        return certificate;
    }

    // Setter para establecer el valor
    void setCertificate(const String &cert)
    {
        this->certificate = cert;
        this->isChanged = true;
    }
};

// Definir la estructura principal que representa el JSON completo
class Configuration
{
public:
    Device device;
    Srv server;
    // Interval intervals[MAX_INTERVALS];
    Output outputs[MAX_OUTPUTS];
    Input inputs[MAX_INPUTS];
    SSID ssids[MAX_SSID];
    byte led = 255;

    Configuration()
    {
        interval_store.setup(5000, [&]()
                             {
                                 // Cada 5 segundos verifica si hubo cambios
                                 this->store(); });
    }

    void setup()
    {
        fromJson(ocs::LocalStore::read());
    }

    void setup(const DynamicJsonDocument &jsonDocument)
    {
        fromJson(jsonDocument);
    }

    void fromJson(const DynamicJsonDocument &jsonDocument)
    {

        if (jsonDocument.containsKey(json_key_led))
        {
            this->led = jsonDocument[json_key_led].as<byte>();
        }

        if (jsonDocument.containsKey(json_key_armed))
        {
            this->armed = jsonDocument[json_key_armed].as<ArmedType>();
        }

        if (jsonDocument.containsKey(json_key_device))
        {
            this->device.fromJson(jsonDocument[json_key_device]);
        }

        if (jsonDocument.containsKey(json_key_server))
        {
            this->server.fromJson(jsonDocument[json_key_server]);
        }

        /*
                if (jsonDocument.containsKey(json_key_interval))
                {
                    for (int i = 0; i < MAX_INTERVALS; i++)
                    {
                        intervals[i].fromJson(jsonDocument[json_key_interval][i]);
                    }
                }
                */

        if (jsonDocument.containsKey(json_key_output))
        {
            for (int i = 0; i < MAX_OUTPUTS; i++)
            {
                outputs[i].fromJson(jsonDocument[json_key_output][i]);
            }
        }

        if (jsonDocument.containsKey(json_key_input))
        {
            for (int i = 0; i < MAX_INPUTS; i++)
            {
                inputs[i].fromJson(jsonDocument[json_key_input][i]);
            }
        }

        if (jsonDocument.containsKey(json_key_ssid))
        {
            for (int i = 0; i < MAX_SSID; i++)
            {
                ssids[i].fromJson(jsonDocument[json_key_ssid][i]);
            }
        }

        isChanged = true;
    }

    DynamicJsonDocument toJson()
    {

        DynamicJsonDocument doc(2048);
        doc[json_key_led] = this->led;
        doc[json_key_device] = this->device.toJson();
        doc[json_key_server] = this->server.toJson();
        doc[json_key_armed] = armed;

        /*
                for (byte i = 0; i < MAX_INTERVALS; i = i + 1)
                {
                    doc[json_key_interval][i] = this->intervals[i].toJson();
                }
        */
        for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
        {
            doc[json_key_output][i] = this->outputs[i].toJson();
        }

        for (byte i = 0; i < MAX_INPUTS; i = i + 1)
        {
            doc[json_key_input][i] = this->inputs[i].toJson();
        }

        for (byte i = 0; i < MAX_SSID; i = i + 1)
        {
            doc[json_key_ssid][i] = this->ssids[i].toJson();
        }

        return doc;
    }

    void loop()
    {
        interval_store.loop();
    }

    // Getter para label
    byte getLed() const
    {
        return led;
    }

    // Setter para label
    void setLed(const byte &newValue)
    {
        led = newValue;
        isChanged = true;
    }

    ArmedType getArmed() const
    {
        return armed;
    }

    // Setter para establecer el valor de password
    void setArmed(const ArmedType &newArmed)
    {
        armed = newArmed;
        this->isChanged = true;
    }

private:
    edwinspire::Interval interval_store;
    ArmedType armed = ArmedType::INSTANT;
    bool isChanged = false;

    void store()
    {
        if (!isChanged)
        {
            this->isChanged = device.isChanged || server.isChanged;

            /*
                        if (!this->isChanged)
                        {

                            for (byte i = 0; i < MAX_INTERVALS; i = i + 1)
                            {
                                if (this->intervals[i].isChanged)
                                {
                                    this->isChanged = true;
                                    break;
                                }
                            }
                        }
                        */

            if (!this->isChanged)
            {

                for (byte i = 0; i < MAX_INPUT; i = i + 1)
                {
                    if (this->inputs[i].isChanged)
                    {
                        this->isChanged = true;
                        break;
                    }
                }
            }

            if (!this->isChanged)
            {

                for (byte i = 0; i < MAX_OUTPUTS; i = i + 1)
                {
                    if (this->outputs[i].isChanged)
                    {
                        this->isChanged = true;
                        break;
                    }
                }
            }

            if (!this->isChanged)
            {

                for (byte i = 0; i < MAX_SSID; i = i + 1)
                {
                    if (this->ssids[i].isChanged)
                    {
                        this->isChanged = true;
                        break;
                    }
                }
            }
        }

        if (this->isChanged)
        {
            //
            Serial.println("Hubo cambios.... se guardarán... ");
            ocs::LocalStore::save(toJson());
        }
    }
};

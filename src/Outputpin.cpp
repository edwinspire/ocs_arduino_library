#include <Arduino.h>

namespace edwinspire
{

    enum BlinkState : uint8_t
    {
        DISABLE = 0,
        DELAY = 1,
        BLINK = 2
    };

    class OutputPin
    {

    public:
        bool enabled = false;

        OutputPin()
        {
            // Without inicialized
            this->_outputPin = 255;
            this->enabled = false;
        }
        byte getGPIO()
        {
            return this->_outputPin;
        }
        void setup(byte gpio, bool enabled)
        {
            Serial.print(F("Setup Output pin: "));
            Serial.println(gpio);

            if (this->_outputPin == 255)
            {
                this->enabled = false;
            }

            this->enabled = enabled;
            this->_outputPin = gpio;
            this->_outputState = LOW;
            this->_blinkState = BlinkState::DISABLE;

            this->_highTime = 0;
            this->_lowTime = 0;
            this->_blinkTimes = -1;
            this->_lastBlinkTime = 0;
            if (this->enabled)
            {
                pinMode(this->_outputPin, OUTPUT);
                //  Serial.print(F("OUTPUT => "));
                //  Serial.println(OUTPUT);
            }
        }

        void high(void)
        {
            if (this->enabled)
            {
                this->_blinkState = BlinkState::DISABLE;
                this->_outputState = HIGH;
                // Serial.println("HIGH");
                // Serial.println(HIGH);
                digitalWrite(this->_outputPin, this->_outputState);
            }
        }

        int getValue()
        {
            return digitalRead(this->_outputPin);
        }

        void toggle()
        {
            if (this->getValue() == HIGH)
            {
                this->low();
            }
            else
            {
                this->high();
            }
        }

        void low(void)
        {
            if (this->enabled)
            {
                this->_blinkState = BlinkState::DISABLE;
                this->_outputState = LOW;
                digitalWrite(this->_outputPin, this->_outputState);
            }
        }

        void blink_by_time(unsigned long lowTime, unsigned long highTime, unsigned long total_time_ms)
        {
            long times = 1;
            unsigned long timePulse = lowTime + highTime;
            if (timePulse <= total_time_ms)
            {
                times = total_time_ms / timePulse;
                Serial.print("Veces que se va a ejecutar el pulso: ");
                Serial.println(times * 2);

                if (times <= 0)
                {
                    times = 1;
                }
            }

            this->blink(lowTime, highTime, 0, times * 2);
        }

        void blink(unsigned long lowTime, unsigned long highTime, unsigned long delayTime, long blinkTimes)
        {
            //this->low();
            this->_highTime = highTime;
            this->_lowTime = lowTime;
            this->_startTime = delayTime;
            this->_blinkTimes = blinkTimes;

            if (this->_blinkState == BlinkState::DISABLE)
            {
                this->_blinkState = BlinkState::DELAY;
                this->_lastBlinkTime = millis();
            }
            // Serial.print(F("Set BLINK Output pin: "));
            // Serial.println(this->_outputPin);
        }

        void blink(unsigned long lowTime, unsigned long highTime, unsigned long delayTime)
        {
            this->blink(lowTime, highTime, delayTime, -1);
        }

        void blink(unsigned long lowTime, unsigned long highTime)
        {
            this->blink(lowTime, highTime, 0, -1);
        }

        int getState(void)
        {
            return this->_outputState;
        }

        void loop(void)
        {
            // Serial.println("Enabled: " + String(this->enabled));
            //  Serial.println("_blinkState: " + String(this->_blinkState));
            //    Serial.println("_blinkTimes: " + String(this->_blinkTimes));
            if (this->enabled)
            {
                bool isBlink = false;

                if (this->_blinkTimes == 0)
                {
                    this->_blinkState = BlinkState::DISABLE;
                }

                switch (this->_blinkState)
                {
                case BlinkState::DISABLE:
                    this->low();
                    return;

                case BlinkState::DELAY:
                    if ((unsigned long)(millis() - this->_lastBlinkTime) >= this->_startTime)
                    {
                        isBlink = true;
                        this->_blinkState = BlinkState::BLINK;
                    }

                    break;

                case BlinkState::BLINK:
                    if (this->_outputState == LOW && (unsigned long)(millis() - this->_lastBlinkTime) >= this->_lowTime)
                        isBlink = true;
                    else if (this->_outputState == HIGH && (unsigned long)(millis() - this->_lastBlinkTime) >= this->_highTime)
                        isBlink = true;

                    break;

                default:
                    return;
                }

                if (isBlink)
                {
                    this->_outputState = (this->_outputState == LOW) ? HIGH : LOW;
                    digitalWrite(this->_outputPin, this->_outputState);
                    this->_lastBlinkTime = millis();

                    if (this->_blinkTimes > 0)
                    {
                        this->_blinkTimes--;
                    }
                }
            }
        }

    private:
        byte _outputPin;
        int _outputState;
        BlinkState _blinkState;

        unsigned long _highTime;
        unsigned long _lowTime;
        unsigned long _startTime;
        unsigned long _blinkTimes;
        unsigned long _lastBlinkTime; // the last time the output pin was blinked
    };

}
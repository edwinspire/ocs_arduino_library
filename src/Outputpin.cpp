#include <Arduino.h>

namespace edwinspire
{

    enum BlinkState
    {
        DISABLE = 0,
        DELAY = 1,
        BLINK = 2
    };

    class OutputPin
    {

    public:
        void setup(int pin)
        {
            Serial.println("Setup Output pin ");
            this->_outputPin = pin;
            this->_outputState = LOW;
            this->_blinkState = BlinkState::DISABLE;

            this->_highTime = 0;
            this->_lowTime = 0;
            this->_blinkTimes = -1;
            this->_lastBlinkTime = 0;

            pinMode(this->_outputPin, OUTPUT);
            Serial.println("OUTPUT");
            Serial.println(OUTPUT);
        }

        void high(void)
        {
            this->_blinkState = BlinkState::DISABLE;
            this->_outputState = HIGH;
            Serial.println("HIGH");
            Serial.println(HIGH);
            digitalWrite(this->_outputPin, this->_outputState);
        }

        void low(void)
        {
            this->_blinkState = BlinkState::DISABLE;
            this->_outputState = LOW;
            digitalWrite(this->_outputPin, this->_outputState);
        }

        void blink(unsigned long lowTime, unsigned long highTime, unsigned long delayTime, long blinkTimes)
        {
            this->_highTime = highTime;
            this->_lowTime = lowTime;
            this->_startTime = delayTime;
            this->_blinkTimes = blinkTimes;

            if (this->_blinkState == BlinkState::DISABLE)
            {
                this->_blinkState = BlinkState::DELAY;
                this->_lastBlinkTime = millis();
            }
            Serial.println("Set BLINK Output pin ");
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
            bool isBlink = false;

            if (this->_blinkTimes == 0)
                this->_blinkState = BlinkState::DISABLE;

            switch (this->_blinkState)
            {
            case BlinkState::DISABLE:
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
                    this->_blinkTimes--;
            }
        }

    private:
        int _outputPin;
        int _outputState;
        BlinkState _blinkState;

        unsigned long _highTime;
        unsigned long _lowTime;
        unsigned long _startTime;
        unsigned long _blinkTimes;
        unsigned long _lastBlinkTime; // the last time the output pin was blinked
    };

}
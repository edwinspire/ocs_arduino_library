#include <Arduino.h>
#include <functional>

namespace edwinspire
{

#ifndef EDWINSPIRE_INTERVAL_CLASS
#define EDWINSPIRE_INTERVAL_CLASS

    class Interval
    {

    private:
        unsigned long last_time = 0;
        unsigned long interval = 5000; // Miliseconds
        std::function<void()> callback;

    public:
        void setup(ulong interval, std::function<void()> callback)
        {
            this->interval = interval;
            this->callback = callback;
        }

        void loop()
        {

            if (millis() - this->last_time > this->interval)
            {
                this->last_time = millis();
                if (this->callback != NULL)
                {
                    this->callback();
                }
            }
        }
    };
#endif
};

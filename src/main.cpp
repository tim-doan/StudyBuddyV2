#include <Arduino.h>

void setup()
{
Serial.begin(115200);
}

void loop() 
{
    // Define user's state:
    enum class state {WORK, BREAK};
    state current = state::WORK;
    // Switch between modes:
    if (current != state::WORK)
        state current = state::BREAK;

    // Define work timer:
    for (int work_seconds = 59; work_seconds > 0; work_seconds--)
    {
        delay(1000);
        Serial.println(work_seconds);
    }
}
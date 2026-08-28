// Tim Doan
// Study Buddy v2
// Main Code

// Libraries:
#include <Arduino.h>

// Variables:
int work_minutes = 25;
int break_minutes = 5;

// User States:
enum class state {WORK, BREAK};
state current = state::WORK;

// Timer Logic:
int total_seconds = work_minutes * 60;
unsigned long last_tick  = 0;

// Work Mode:
void work_mode()
{
    current = state::WORK;
    total_seconds = work_minutes * 60;
}

// Break Mode:
void break_mode()
{
    current = state::BREAK;
    total_seconds = break_minutes * 60;
}

void tick()
{
    total_seconds --;

    Serial.printf("%s %d:%02d\n",   
        current == state::WORK ? "Work: " : "Break:",
        total_seconds / 60,
        total_seconds % 60);

    if (total_seconds == 0)
    {
        if (current == state::WORK)
        {
            break_mode();
        }

        else
        {
            work_mode();
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
 
    last_tick = millis();
    work_mode();    
}

void loop() 
{
    if (millis() - last_tick >= 1000)
    {
        last_tick += 1000;
        tick();
    }
}


  
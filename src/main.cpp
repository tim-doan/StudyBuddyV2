// Tim Doan
// Study Buddy v2
// Main Code
// This file runs the main operations of the project.

// Libraries:
#include "dialogue.h"
#include "buzzer.h"
#include "button.h"
#include "display.h"
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

// Announce an event on every output the project has.
// The terminal, the buzzer, and the screen all get told at once, and the screen is
// handed the exact line the terminal printed so the two never disagree.
void on_event(Event e)
{
    const char* line = say(e);
    play(e);
    display_event(e, line, total_seconds);
}

// Work Mode:
void work_mode()
{
    // The mode and timer are set first so the screen has the new countdown to draw
    current = state::WORK;
    total_seconds = work_minutes * 60;
    on_event(Event::WORK_START);
}

// Break Mode:
void break_mode()
{
    // The mode and timer are set first so the screen has the new countdown to draw
    current = state::BREAK;
    total_seconds = break_minutes * 60;
    on_event(Event::BREAK_START);
}

// Move the project into whichever mode comes next.
// Both the timer running out and the button being pressed come through here,
// so an early switch behaves exactly like one the timer made on its own.
void next_mode()
{
    if (current == state::WORK)
    {
        on_event(Event::WORK_DONE);
        break_mode();
    }

    else
    {  
        on_event(Event::BREAK_DONE);
        work_mode();
    }

    // Start the new mode on a fresh second so its first tick is a whole one
    last_tick = millis();
}
void setup()
{
    // Inputs:
    button_begin();

    // Outputs:
    buzzer_begin();
    display_begin();

    Serial.begin(115200);
    delay(1000);

    // Greet the user, then hold here until the opening jingle finishes so that the
    // work ding does not cut it short. The timer has not started yet, so waiting costs nothing.
    on_event(Event::SESSION_START);
    while (buzzer_busy())
    {
        buzzer_update();
    }
 
    last_tick = millis();
    work_mode();    

}

void tick()
{
    total_seconds --;

    Serial.printf("%s %d:%02d\n",   
        current == state::WORK ? "Work: " : "Break:",
        total_seconds / 60,
        total_seconds % 60);

    display_time(total_seconds);

    if (total_seconds == 0)
    {
        next_mode();
    }
}

void loop() 
{
    // Keep whatever jingle is playing moving from note to note
    buzzer_update();

    // Let the user jump ahead to the next mode early
    if (button_pressed())
    {
        next_mode();
    }

    if (millis() - last_tick >= 1000)
    {
        last_tick += 1000;
        tick();
    }
}



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

// Opening Message Logic:
const unsigned long MESSAGE_MS = 3000;   // how long a mode's message holds the screen
unsigned long message_until = 0;         // when to trade it for the timer, or 0 if none is up

// Pause Logic:
bool paused = false;
unsigned long paused_at = 0;             // the moment the pause button froze the countdown

// Announce an event on every output the project has.
// The terminal, the buzzer, and the screen all get told at once, and the screen is
// handed the exact line the terminal printed so the two never disagree.
void on_event(Event e)
{
    const char* line = say(e);
    play(e);
    display_message(e, line);
}

// Work Mode:
void work_mode()
{
    // The mode and timer are set first so the screen has the new countdown to draw
    current = state::WORK;
    total_seconds = work_minutes * 60;
    on_event(Event::WORK_START);

    // The countdown waits while the opening message has the screen to itself
    message_until = millis() + MESSAGE_MS;
}

// Break Mode:
void break_mode()
{
    // The mode and timer are set first so the screen has the new countdown to draw
    current = state::BREAK;
    total_seconds = break_minutes * 60;
    on_event(Event::BREAK_START);

    // The countdown waits while the opening message has the screen to itself
    message_until = millis() + MESSAGE_MS;
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

    // Greet the user and hold the screen until they press start. The jingle keeps
    // moving while it sits there, and the timer has not started yet, so waiting is free.
    on_event(Event::SESSION_START);
    while (!start_pressed())
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

    // Check every button on every pass so none of them misses its settling clock
    bool skip  = skip_pressed();
    bool start = start_pressed();
    bool pause = pause_pressed();

    // Freeze the countdown where it stands
    if (pause && !paused)
    {
        paused = true;
        paused_at = millis();
        Serial.println("Paused");
    }

    // Pick back up from the exact point the countdown stopped. Everything waiting on the
    // clock is pushed back by the time spent paused, so no part of a second is lost.

    if (start && paused)
    {
        unsigned long paused_for = millis() - paused_at;
        last_tick += paused_for;
        if (message_until != 0)
        {
            message_until += paused_for;
        }

        paused = false;
        Serial.println("Resumed");
    }

    // Let the user jump ahead to the next mode early.
    // A skip starts the next mode running even if the countdown was paused.
    if (skip)
    {
        paused = false;
        next_mode();
        return;
    }

    if (paused)
    {
        return;
    }

    // The opening message holds both the screen and the countdown until its time is up,
    // then the timer takes over and the first whole second starts from there
    if (message_until != 0)
    {
        if (millis() >= message_until)
        {
            message_until = 0;
            display_timer(total_seconds);
            last_tick = millis();
        }

        return;
    }

    if (millis() - last_tick >= 1000)
    {
        last_tick += 1000;
        tick();
    }
}



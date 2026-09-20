/// Tim Doan
// Study Buddy v2
// Button

// This file defines the button's pin and the logic that turns a noisy press into one clean signal.

#include "button.h"
#include <Arduino.h>

// Pin Definition:
static const int BUTTON_1 = 4;

// How long the pin has to hold still before a reading counts, in milliseconds.
static const unsigned long DEBOUNCE_MS = 25;

// Button Logic:
static bool stable_state = false;       // the reading trusted as real, true while held down
static bool last_raw = false;           // the raw reading from the last check
static unsigned long last_change = 0;   // the moment the raw reading last flipped

// Give the button pin its pull-up resistor:
void button_begin()
{
    pinMode(BUTTON_1, INPUT_PULLUP);
}

// Report a press once, at the moment the button settles into being held down.
bool button_pressed()
{
    // The pull-up holds the pin high, so the pin reads low while the button is held down
    bool raw = (digitalRead(BUTTON_1) == LOW);

    // The reading just flipped, so start the settling clock over
    if (raw != last_raw)
    {
        last_raw = raw;
        last_change = millis();
        return false;
    }

    // The reading has not held still long enough to be trusted yet
    if (millis() - last_change < DEBOUNCE_MS)
    {
        return false;
    }

    // The reading held steady, so take it as the button's real state.
    // Returning stable_state reports true only when it settled into being down.
    if (raw != stable_state)
    {
        stable_state = raw;
        return stable_state;
    }

    return false;
}

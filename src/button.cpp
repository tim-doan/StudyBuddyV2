/// Tim Doan
// Study Buddy v2
// Button

// This file defines the buttons' pins and the logic that turns a noisy press into one clean signal.

#include "button.h"
#include <Arduino.h>

// Pin Definitions:
static const int SKIP_BUTTON  = 4;
static const int START_BUTTON = 32;
static const int PAUSE_BUTTON = 33;

// How long the pin has to hold still before a reading counts, in milliseconds.
static const unsigned long DEBOUNCE_MS = 25;

// Button Logic:
// Every button keeps its own settling clock so one bouncing button can't disturb another.
struct Button
{
    int pin;
    bool stable_state = false;       // the reading trusted as real, true while held down
    bool last_raw = false;           // the raw reading from the last check
    unsigned long last_change = 0;   // the moment the raw reading last flipped
};

static Button skip_button  = {SKIP_BUTTON};
static Button start_button = {START_BUTTON};
static Button pause_button = {PAUSE_BUTTON};

// Give each button pin its pull-up resistor.
// These pins all have one built into the chip, so the buttons only need a wire to GND.
void button_begin()
{
    pinMode(SKIP_BUTTON, INPUT_PULLUP);
    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(PAUSE_BUTTON, INPUT_PULLUP);
}

// Report a press once, at the moment the button settles into being held down.
static bool pressed(Button& b)
{
    // The pull-up holds the pin high, so the pin reads low while the button is held down
    bool raw = (digitalRead(b.pin) == LOW);

    // The reading just flipped, so start the settling clock over
    if (raw != b.last_raw)
    {
        b.last_raw = raw;
        b.last_change = millis();
        return false;
    }

    // The reading has not held still long enough to be trusted yet
    if (millis() - b.last_change < DEBOUNCE_MS)
    {
        return false;
    }

    // The reading held steady, so take it as the button's real state.
    // Returning stable_state reports true only when it settled into being down.
    if (raw != b.stable_state)
    {
        b.stable_state = raw;
        return b.stable_state;
    }

    return false;
}

bool skip_pressed()
{
    return pressed(skip_button);
}

bool start_pressed()
{
    return pressed(start_button);
}

bool pause_pressed()
{
    return pressed(pause_button);
}

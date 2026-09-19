// Tim Doan
// Study Buddy v2
// Buzzer

// This file defines the jingle for each event and the logic to play a jingle one note at a time.

#include "buzzer.h"
#include <Arduino.h>

// Buzzer pin, wired to D2 through a 220 ohm resistor:
static const int BUZZER_PIN = 2;

// Settings for the ESP32's tone hardware:
static const int BUZZER_START_FREQ = 2000;  // starting frequency, replaced by every note that plays
static const int BUZZER_RESOLUTION = 10;    // how finely the square wave can be shaped, in bits

// One note of a jingle. Frequency is in hertz and length is in milliseconds.
// A frequency of 0 makes no sound, which is how a rest gets written.
struct Note
{
    uint16_t freq;
    uint16_t ms;
};

// Define jingles for events:
// Session Start: notes climbing upward to get the user motivated
static const Note SESSION_START_NOTES[] =
{
    {523, 120},
    {659, 120},
    {784, 120},
    {1047, 260},
};

// Starting Work Session: one high, loud ding, like the shot before track runners race
static const Note WORK_START_NOTES[] =
{
    {1568, 180},
};

// Starting Break Session: a quick note into a longer one, like getting a coin in Mario
static const Note BREAK_START_NOTES[] =
{
    {988, 90},
    {1319, 220},
};

// Session End: a run up the scale, a beat of silence, then a held note to close the stage out
static const Note SESSION_DONE_NOTES[] =
{
    {784, 140},
    {880, 140},
    {988, 140},
    {1047, 140},
    {0, 60},
    {1319, 420},
};

// Bundle all the notes of a jingle into one value:
struct Melody
{
    const Note* notes;
    uint8_t count;
};

// Macro for text substution:
#define SET(arr) {arr, sizeof(arr) / sizeof(arr[0])}

// Lookup table:
// WORK_DONE and BREAK_DONE are left silent on purpose. Every mode change announces its
// DONE event and its START event back to back, so only the START event carries a jingle.
// That way one change of modes makes one sound instead of two.
static const Melody TABLE[(int)Event::COUNT] =
{
    SET(SESSION_START_NOTES),   // SESSION_START
    SET(WORK_START_NOTES),      // WORK_START
    {nullptr, 0},               // WORK_DONE
    SET(BREAK_START_NOTES),     // BREAK_START
    {nullptr, 0},               // BREAK_DONE
    SET(SESSION_DONE_NOTES),    // SESSION_DONE
};

// Jingle Logic:
static const Note* melody = nullptr;    // the jingle playing now, or nullptr when the buzzer is quiet
static uint8_t note_count = 0;          // how many notes that jingle holds
static uint8_t note_index = 0;          // which note of the jingle is sounding
static unsigned long note_started = 0;  // the moment the current note began

// Send the current note's frequency out to the buzzer:
static void start_note()
{
    ledcWriteTone(BUZZER_PIN, melody[note_index].freq);
}

// Hand the buzzer pin over to the ESP32's tone hardware:
void buzzer_begin()
{
    ledcAttach(BUZZER_PIN, BUZZER_START_FREQ, BUZZER_RESOLUTION);
}

// Look up the jingle that belongs to an event and sound its first note:
void play(Event e)
{
    const Melody& set = TABLE[(int)e];

    // A silent event has no notes, so there is nothing to start
    if (set.count == 0)
    {
        return;
    }

    melody = set.notes;
    note_count = set.count;
    note_index = 0;
    note_started = millis();
    start_note();
}

// Move a jingle onto its next note once the current note has run its full length.
// This never waits around, so the timer keeps counting while a jingle plays.
void buzzer_update()
{
    // Nothing is playing right now
    if (melody == nullptr)
    {
        return;
    }

    // The current note still has time left on it
    if (millis() - note_started < melody[note_index].ms)
    {
        return;
    }

    // Carry any leftover time into the next note so the jingle keeps its rhythm
    note_started += melody[note_index].ms;
    note_index ++;

    // The jingle ran out of notes, so go quiet
    if (note_index >= note_count)
    {
        ledcWriteTone(BUZZER_PIN, 0);
        melody = nullptr;
        return;
    }

    start_note();
}

// Report whether a jingle is still playing:
bool buzzer_busy()
{
    return melody != nullptr;
}

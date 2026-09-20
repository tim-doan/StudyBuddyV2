// Tim Doan
// Study Buddy v2
// Dialogue

// This file defines the dialogue lines for each event and the logic to bring up each line.

#include "dialogue.h"
#include <Arduino.h>

// Define lines for events:
// Session Start:
static const char* const SESSION_START_LINES[] = 
{
    "Have you cleared yourself of all distractions?",
    "Take a deep breath. Be mindful and ready to focus.",
    "Do you have all your tasks planned out?",
    "Before working, make sure you plan out all your tasks.",
};

// Starting Work Session:
static const char* const WORK_START_LINES[] = 
{
    "Let's get to work!",
    "Time to get it done!",
    "Work time!",
    "Starting the work session..."
    "Time to work!",
    "Let's get it done!"
};

// Ending Work Session:
static const char* const WORK_DONE_LINES[] = 
{

};

// Starting Break Session:
static const char* const BREAK_START_LINES[] = 
{
    "Time to take a break!",
    "Let's take a quick break now.",
    "Break time!!!",
    "Work period done!",
    "Timer's up!",
    "Good job!",
};

// Ending Break Session:
static const char* const BREAK_DONE_LINES[] = 
{

};

// Session End:
static const char* const SESSION_DONE_LINES[] = 
{
    "Hooray to another productive session!",
    "Great work today! I'll see you soon.",
    "I got to say. That was fantastic!",
};

// Bundle all the lines into one value:
struct DialogueSet
{
    const char* const* lines;
    uint8_t count;
};

// Macro for text substution:
#define SET(arr) {arr, sizeof(arr) / sizeof(arr[0])}

// Lookup table:
static const DialogueSet TABLE[(int)Event::COUNT] = 
{   
    SET(SESSION_START_LINES),
    SET(WORK_START_LINES), 
    SET(WORK_DONE_LINES),
    SET(BREAK_START_LINES),
    SET(BREAK_DONE_LINES),
    SET(SESSION_DONE_LINES), 
};

// Choose and say random lines based off each event, then hand that line back:
const char* say(Event e)
{
    const DialogueSet& set = 
    TABLE[(int)e];
    const char* line = set.lines[random(set.count)];
    Serial.println(line);
    return line;
}
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
    "Take a deep breath! Be mindful and ready to focus",
    "Make sure you have all your tasks planned out!",
};

// Starting Work Session:
static const char* const WORK_START_LINES[] = 
{
    "Let's get to work!",
    "Time to get it done!",
    "Work time!!!",
};
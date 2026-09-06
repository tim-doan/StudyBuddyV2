// Tim Doan
// Study Buddy v2
// Dialogue

// This file definds the user events and the function that summons dialogue.

// Define the file once
#pragma once

// User States:
enum class Event
{
    SESSION_START,
    WORK_START,
    WORK_DONE,
    BREAK_START,
    BREAK_DONE,
    SESSION_DONE,
    COUNT
};

// Function to summon dialogue when an event changes
void say(Event e);
// Tim Doan
// Study Buddy v2
// Display

// This file defines the functions that set up the LCD display and redraw each part of the screen.

// Define the file once
#pragma once

// Bring in the list of user events so the screen can change along with the mode
#include "dialogue.h"

// Function to wake the display up and paint the first screen
void display_begin();

// Function to fill the whole screen with the message an event brings
void display_message(Event e, const char* message);

// Function to trade the message out for the mode label and the countdown
void display_timer(int total_seconds);

// Function to redraw only the countdown
void display_time(int total_seconds);

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

// Function to hand the screen to the buddy, who acts out an event and says its line
void display_message(Event e, const char* message);

// Function to put the buddy to sleep on screen while the countdown is paused
void display_paused(int total_seconds);

// Function that keeps the buddy moving, one frame at a time, while it has the screen
void display_update();

// Function to trade the buddy out for the mode label and the countdown
void display_timer(int total_seconds);

// Function to redraw only the countdown
void display_time(int total_seconds);

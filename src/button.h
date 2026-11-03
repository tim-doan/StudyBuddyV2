// Tim Doan
// Study Buddy v2
// Button

// This file defines functions with buttons

// Define the file once
#pragma once

// Bring in the list of user events so that a button press knows which event to move to
#include "dialogue.h"

// Function: Give the buttons their pullup resistors
void button_begin();

// Function: Tell when the skip button is pressed
bool skip_pressed();

// Function: Tell when the start button is pressed
bool start_pressed();

// Function: Tell when the pause button is pressed
bool pause_pressed();

// Tim Doan
// Study Buddy v2
// Button

// This file defines functions with buttons

// Define the file once
#pragma once

// Bring in the list of user events so that a button press knows which event to move to
#include "dialogue.h"

// Function: Give button its pullup resistor
void button_begin();

// Function: Tell when button is pressed
bool button_pressed();
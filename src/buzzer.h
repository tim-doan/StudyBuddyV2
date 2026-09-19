// Tim Doan
// Study Buddy v2
// Buzzer

// This file defines the functions that set up the buzzer, start a jingle, and keep that jingle playing.

// Define the file once
#pragma once

// Bring in the list of user events so every jingle can be tied to one
#include "dialogue.h"

// Function to hand the buzzer pin over to the ESP32's tone hardware
void buzzer_begin();

// Function to summon a jingle when an event happens
void play(Event e);

// Function that walks a jingle from one note to the next
void buzzer_update();

// Function that reports whether a jingle is still playing
bool buzzer_busy();

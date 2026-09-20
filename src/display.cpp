// Tim Doan
// Study Buddy v2
// Display

// This file defines where everything sits on the screen and the logic that draws each part of it.

#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>

// Control pins. SCL and SDA are missing on purpose, because they run on the ESP32's
// hardware SPI, which already knows to use GPIO18 and GPIO23.
static const int TFT_CS = 5;
static const int TFT_DC = 16;
static const int TFT_RST = 17;

// Screen size in pixels:
static const int SCREEN_W = 128;
static const int SCREEN_H = 160;

// Where each part of the screen begins, measured down from the top:
static const int MODE_Y = 18;
static const int TIME_Y = 58;
static const int MESSAGE_Y = 104;

// How tall the countdown band is, so it can be wiped clean without disturbing anything else:
static const int TIME_H = 26;

// The library keeps the panel offset setter to itself, so this thin wrapper around it
// reaches in and hands that one setting out. Everything else is inherited untouched.
class Panel : public Adafruit_ST7735
{
public:
    Panel(int8_t cs, int8_t dc, int8_t rst) : Adafruit_ST7735(cs, dc, rst) {}

    // Tell the library where the visible glass begins inside the controller's memory
    void set_offset(int8_t col, int8_t row)
    {
        setColRowStart(col, row);
    }
};

// The display itself. The library toggles CS, DC, and RES by hand and pushes
// everything else out over hardware SPI.
static Panel tft = Panel(TFT_CS, TFT_DC, TFT_RST);

// Background colors, mixed once the display is awake:
static uint16_t work_bg = 0;
static uint16_t break_bg = 0;
static uint16_t background = 0;   // whichever of the two belongs to the mode running now

// Draw a line of text centered across the screen:
static void draw_centered(const char* text, int y, uint8_t size)
{
    // Every character in this font is six pixels wide before it gets scaled up
    int width = strlen(text) * 6 * size;

    tft.setTextSize(size);
    tft.setCursor((SCREEN_W - width) / 2, y);
    tft.print(text);
}

// Wipe the countdown band and write the time back into it:
static void draw_time(int total_seconds)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d:%02d", total_seconds / 60, total_seconds % 60);

    tft.fillRect(0, TIME_Y, SCREEN_W, TIME_H, background);
    draw_centered(buffer, TIME_Y, 3);
}

// Wipe the message area and write a new line into it:
static void draw_message(const char* message)
{
    tft.fillRect(0, MESSAGE_Y, SCREEN_W, SCREEN_H - MESSAGE_Y, background);
    tft.setTextSize(1);
    tft.setCursor(3, MESSAGE_Y);
    tft.print(message);
}

// Wake the display up and paint the first background:
void display_begin()
{
    // BLACKTAB matches this 1.8 inch 128x160 panel's colors
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(0);

    // The visible glass on this panel begins two columns in and one row down from where
    // the controller's memory begins. Without telling the library that, every drawing
    // lands two columns left and one row high of where it belongs, so the far right
    // columns and the bottom row never get written and show leftover memory instead.
    tft.set_offset(2, 1);

    work_bg = tft.color565(0, 45, 190);
    break_bg = tft.color565(0, 140, 55);
    background = work_bg;

    // Messages run longer than the screen is wide, so let them fall onto the next line
    tft.setTextWrap(true);
    tft.setTextColor(ST77XX_WHITE);

    tft.fillScreen(background);
}

// Redraw the screen for an event:
void display_event(Event e, const char* message, int total_seconds)
{
    bool work_start = (e == Event::WORK_START);
    bool break_start = (e == Event::BREAK_START);

    // A mode change repaints the whole screen, since the background color changes with it
    if (work_start || break_start)
    {
        background = work_start ? work_bg : break_bg;
        tft.fillScreen(background);

        draw_centered(work_start ? "WORK" : "BREAK", MODE_Y, 2);
        draw_time(total_seconds);
        draw_message(message);
        return;
    }

    // The DONE events fire in the split second before a mode begins, so letting them
    // draw only flashes a line too briefly to read. The message a user actually sees is
    // the one the new mode puts up, the way "Go!" is what matters at the start of a race.
    if (e == Event::WORK_DONE || e == Event::BREAK_DONE)
    {
        return;
    }

    // What is left is the opening and closing of a whole session, which still get a message
    draw_message(message);
}

// Redraw only the countdown, which is the one thing that changes second to second:
void display_time(int total_seconds)
{
    draw_time(total_seconds);
}

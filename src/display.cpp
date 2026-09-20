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

// The controller carries more memory than the glass can show, and the visible area sits
// somewhere inside it. These say how big that memory is and where the glass begins in it.
static const int PANEL_W = 132;
static const int PANEL_H = 162;
static const int COL_OFFSET = 2;
static const int ROW_OFFSET = 1;

// Where each part of the timer screen begins, measured down from the top:
static const int MODE_Y = 48;
static const int TIME_Y = 84;

// How tall the countdown band is, so it can be wiped clean without disturbing anything else:
static const int TIME_H = 26;

// The most lines a message can break into, and the most characters one line can hold:
static const int MAX_LINES = 10;
static const int MAX_LINE = 32;

// The library keeps a couple of its settings to itself, so this thin wrapper around it
// reaches in and hands those out. Everything else is inherited untouched.
class Panel : public Adafruit_ST7735
{
public:
    Panel(int8_t cs, int8_t dc, int8_t rst) : Adafruit_ST7735(cs, dc, rst) {}

    // Tell the library where the visible glass begins inside the controller's memory.
    // This has to be set before setRotation, because setRotation is what copies it into
    // the values the drawing code actually reads.
    void set_offset(int8_t col, int8_t row)
    {
        setColRowStart(col, row);
    }

    // Paint every pixel the controller holds, including the rows and columns that fall
    // outside the glass. Memory left unwritten out there is what shows up as junk along
    // an edge, so this makes sure none of it is ever left unwritten.
    void clear_all(uint16_t color)
    {
        int16_t keep_x = _xstart;
        int16_t keep_y = _ystart;

        _xstart = 0;
        _ystart = 0;

        startWrite();
        setAddrWindow(0, 0, PANEL_W, PANEL_H);
        writeColor(color, (uint32_t)PANEL_W * PANEL_H);
        endWrite();

        _xstart = keep_x;
        _ystart = keep_y;
    }
};

// The display itself. The library toggles CS, DC, and RES by hand and pushes
// everything else out over hardware SPI.
static Panel tft = Panel(TFT_CS, TFT_DC, TFT_RST);

// Background colors, mixed once the display is awake:
static uint16_t work_bg = 0;
static uint16_t break_bg = 0;
static uint16_t background = 0;    // whichever of the two belongs to the mode running now

// The word the timer screen puts above the countdown:
static const char* mode_label = "WORK";

// Draw a line of text centered across the screen:
static void draw_centered(const char* text, int y, uint8_t size)
{
    // Every character in this font is six pixels wide before it gets scaled up
    int width = strlen(text) * 6 * size;

    tft.setTextSize(size);
    tft.setCursor((SCREEN_W - width) / 2, y);
    tft.print(text);
}

// How many characters fit across the screen at a given text size:
static int chars_across(uint8_t size)
{
    return SCREEN_W / (6 * size);
}

// Check that every word in a message is short enough to sit on one line:
static bool words_fit(const char* message, int width)
{
    int run = 0;

    for (const char* c = message; ; c ++)
    {
        if (*c == ' ' || *c == 0)
        {
            if (run > width)
            {
                return false;
            }

            run = 0;

            if (*c == 0)
            {
                return true;
            }
        }

        else
        {
            run ++;
        }
    }
}

// Break a message onto lines that fit the screen, and report how many it took:
static int wrap_message(const char* message, int width, char lines[MAX_LINES][MAX_LINE])
{
    int count = 0;
    int len = 0;
    lines[0][0] = 0;

    const char* word = message;

    while (count < MAX_LINES)
    {
        // Walk to the end of the next word
        const char* end = word;
        while (*end != ' ' && *end != 0)
        {
            end ++;
        }

        int word_len = end - word;

        // Start a new line when this word will not fit on the one being built
        if (len > 0 && len + 1 + word_len > width)
        {
            count ++;

            if (count >= MAX_LINES)
            {
                break;
            }

            len = 0;
            lines[count][0] = 0;
        }

        // Every word but the first on a line needs a space in front of it
        if (len > 0)
        {
            lines[count][len] = ' ';
            len ++;
        }

        // Copy the word over, clipping anything that would run past the end of the line
        for (int i = 0; i < word_len && len < MAX_LINE - 1; i ++)
        {
            lines[count][len] = word[i];
            len ++;
        }

        lines[count][len] = 0;

        // The message ended, so the line being built is the last one
        if (*end == 0)
        {
            count ++;
            break;
        }

        word = end + 1;
    }

    return count;
}

// Fill the whole screen with a message, written as large as it can be without
// splitting a word across two lines:
static void draw_full_message(const char* message)
{
    uint8_t size = words_fit(message, chars_across(2)) ? 2 : 1;

    char lines[MAX_LINES][MAX_LINE];
    int count = wrap_message(message, chars_across(size), lines);

    // Leave a few pixels between lines, then push the whole block far enough down
    // the screen to sit in the middle of it
    int line_h = 8 * size + 4;
    int top = (SCREEN_H - count * line_h) / 2;

    tft.clear_all(background);

    for (int i = 0; i < count; i ++)
    {
        draw_centered(lines[i], top + i * line_h, size);
    }
}

// Wipe the countdown band and write the time back into it:
static void draw_time(int total_seconds)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d:%02d", total_seconds / 60, total_seconds % 60);

    tft.fillRect(0, TIME_Y, SCREEN_W, TIME_H, background);
    draw_centered(buffer, TIME_Y, 3);
}

// Wake the display up and paint the first background:
void display_begin()
{
    // BLACKTAB matches this 1.8 inch 128x160 panel's colors
    tft.initR(INITR_BLACKTAB);

    // The offset goes in before setRotation, since setRotation is what carries it over
    // into the values the drawing code reads. Setting it afterwards does nothing at all.
    tft.set_offset(COL_OFFSET, ROW_OFFSET);
    tft.setRotation(0);

    work_bg = tft.color565(0, 140, 55);
    break_bg = tft.color565(0, 45, 190);
    background = work_bg;

    // Lines get broken up by hand here, so the library should leave them alone
    tft.setTextWrap(false);
    tft.setTextColor(ST77XX_WHITE);

    tft.clear_all(background);
}

// Fill the whole screen with the message an event brings:
void display_message(Event e, const char* message)
{
    // A mode change carries its own background color and label in with it
    if (e == Event::WORK_START)
    {
        background = work_bg;
        mode_label = "WORK";
    }

    else if (e == Event::BREAK_START)
    {
        background = break_bg;
        mode_label = "BREAK";
    }

    // The DONE events land in the split second before a mode begins, so letting them
    // draw only flashes a line too briefly to read. The message a user actually sees is
    // the one the new mode puts up, the way "Go!" is what matters at the start of a race.
    else if (e == Event::WORK_DONE || e == Event::BREAK_DONE)
    {
        return;
    }

    draw_full_message(message);
}

// Trade the message out for the mode label and the countdown:
void display_timer(int total_seconds)
{
    tft.clear_all(background);

    draw_centered(mode_label, MODE_Y, 2);
    draw_time(total_seconds);
}

// Redraw only the countdown, which is the one thing that changes second to second:
void display_time(int total_seconds)
{
    draw_time(total_seconds);
}

// Tim Doan
// Study Buddy v2
// Display

// This file defines where everything sits on the screen and the logic that draws each part of it,
// including the buddy whose face takes over the screen whenever something happens.

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

// The Buddy:
// The face owns the top of the screen and a speech bubble sits underneath it.
// Only the face moves, so only the rows above FACE_H get redrawn while it animates.
static const int FACE_H = 100;

// Eyes, measured from their centers:
static const int EYE_W = 30;
static const int EYE_H = 38;
static const int EYE_R = 10;          // how round the corners are
static const int FOCUSED_EYE_H = 32;  // narrowed a little when it's time to concentrate
static const int EYE_Y = 46;
static const int LEFT_EYE_X = 39;
static const int RIGHT_EYE_X = 89;
static const int LOOK_PX = 8;         // how far the eyes slide when the buddy glances to one side

// Mouth, measured from its center:
static const int MOUTH_X = 64;
static const int MOUTH_Y = 74;

// Speech bubble, with a small tail that points up at the mouth:
static const int BUBBLE_X = 4;
static const int BUBBLE_Y = 110;
static const int BUBBLE_W = 120;
static const int BUBBLE_H = 46;
static const int BUBBLE_R = 8;
static const int BUBBLE_PAD = 4;
static const int TAIL_W = 12;
static const int TAIL_H = 8;
static const int LINE_GAP = 2;        // space between lines of words, in pixels

// How long one hello hop takes, in milliseconds:
static const unsigned long HOP_MS = 260;

// How often the face may redraw, in milliseconds. About 30 frames a second is smooth
// enough for a blink, and leaves plenty of time between frames for the buzzer and buttons.
static const unsigned long FRAME_MS = 33;

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

// The buddy gets drawn here in memory first and then copied to the glass in one burst,
// so the screen never shows a half-finished face in the middle of a blink.
static GFXcanvas16 canvas(SCREEN_W, SCREEN_H);

// Background colors, mixed once the display is awake:
static uint16_t work_bg = 0;
static uint16_t break_bg = 0;
static uint16_t work_dim = 0;      // the same two colors turned down, for while the buddy sleeps
static uint16_t break_dim = 0;
static uint16_t background = 0;    // whichever of the two belongs to the mode running now
static uint16_t dimmed = 0;        // the turned-down version of that same color

// Colors the buddy is drawn in:
static uint16_t ink = 0;           // the dark color words are written in inside the bubble
static uint16_t drowsy = 0;        // the softer white the buddy fades to while it sleeps

// The word the timer screen puts above the countdown:
static const char* mode_label = "WORK";

// The buddy's moods, one for each moment it takes over the screen:
enum class Mood : uint8_t
{
    NONE,       // the timer has the screen, so the buddy is away
    CHEERFUL,   // saying hello at the start of a session
    FOCUSED,    // getting ready to work
    JOYFUL,     // celebrating the start of a break
    SLEEPY,     // dozing while the countdown is paused
};

// Everything that can move on the face, captured at one moment. Two frames that match
// look identical, so the screen only gets redrawn when a new frame differs from the last.
struct Frame
{
    Mood mood;
    int8_t open;     // how tall the eyes are, in pixels
    int8_t look;     // how far the eyes have turned, in pixels, negative to the left
    int8_t lift;     // how far the whole face has moved, in pixels, negative upward
    uint8_t step;    // which beat the extras, like sparkles and z's, are on
};

// Buddy Logic:
static Mood mood = Mood::NONE;
static uint16_t face_bg = 0;           // the colors of the scene on screen now
static uint16_t face_fg = 0;
static unsigned long scene_start = 0;  // the moment the buddy took over the screen
static unsigned long last_frame = 0;   // the moment the face was last checked for changes
static unsigned long next_blink = 0;   // when the next blink begins
static unsigned long next_glance = 0;  // when the eyes next pick somewhere new to look
static int8_t look_target = 0;         // where the eyes are headed
static int8_t look_now = 0;            // where the eyes are pointed this frame
static Frame shown = {};               // the frame on the glass right now

// Draw a line of text centered across the screen, onto either the glass or the canvas:
static void draw_centered(Adafruit_GFX& gfx, const char* text, int y, uint8_t size)
{
    // Every character in this font is six pixels wide before it gets scaled up
    int width = strlen(text) * 6 * size;

    gfx.setTextSize(size);
    gfx.setCursor((SCREEN_W - width) / 2, y);
    gfx.print(text);
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

// How tall a block of lines stands at a given text size, in pixels:
static int block_height(int count, uint8_t size)
{
    return count * 8 * size + (count - 1) * LINE_GAP;
}

// Wipe the countdown band and write the time back into it:
static void draw_time(int total_seconds)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d:%02d", total_seconds / 60, total_seconds % 60);

    tft.fillRect(0, TIME_Y, SCREEN_W, TIME_H, background);
    draw_centered(tft, buffer, TIME_Y, 3);
}

// Draw the speech bubble, tail and all:
static void draw_bubble(uint16_t fill)
{
    canvas.fillRoundRect(BUBBLE_X, BUBBLE_Y, BUBBLE_W, BUBBLE_H, BUBBLE_R, fill);
    canvas.fillTriangle(MOUTH_X - TAIL_W / 2, BUBBLE_Y,
                        MOUTH_X + TAIL_W / 2, BUBBLE_Y,
                        MOUTH_X, BUBBLE_Y - TAIL_H, fill);
}

// Write a message inside the bubble, as large as it can be while still fitting:
static void draw_speech(const char* message)
{
    const int inner_w = BUBBLE_W - 2 * BUBBLE_PAD;
    const int inner_h = BUBBLE_H - 2 * BUBBLE_PAD;

    char lines[MAX_LINES][MAX_LINE];
    uint8_t size = 2;
    int count = 0;

    // Big letters only when no word has to split and the whole message still fits
    if (words_fit(message, inner_w / 12))
    {
        count = wrap_message(message, inner_w / 12, lines);
    }

    if (count == 0 || block_height(count, 2) > inner_h)
    {
        size = 1;
        count = wrap_message(message, inner_w / 6, lines);
    }

    // A message too long even for the small letters loses its last lines instead of spilling out
    while (count > 1 && block_height(count, size) > inner_h)
    {
        count --;
    }

    int top = BUBBLE_Y + (BUBBLE_H - block_height(count, size)) / 2;

    canvas.setTextColor(ink);
    for (int i = 0; i < count; i ++)
    {
        draw_centered(canvas, lines[i], top + i * (8 * size + LINE_GAP), size);
    }
}

// Face Parts:
// An open eye, shrunk down to a sliver when the buddy blinks:
static void draw_eye(int cx, int cy, int h)
{
    int r = min(EYE_R, h / 2);
    canvas.fillRoundRect(cx - EYE_W / 2, cy - h / 2, EYE_W, h, r, face_fg);
}

// Slice the inner top corner off an eye so the brow slants down toward the middle,
// which is what makes the buddy look determined. Inner is +1 for the left eye and -1 for the right.
static void draw_brow(int cx, int cy, int h, int inner)
{
    int top = cy - h / 2;
    int depth = 10 * h / EYE_H;
    int outer_x = cx - inner * (EYE_W / 2 + 1);
    int inner_x = cx + inner * (EYE_W / 2 + 1);

    canvas.fillTriangle(outer_x, top - 1, inner_x, top - 1, inner_x, top + depth, face_bg);
}

// An eye squeezed shut with happiness, shaped like a ^. One circle gets most of
// its bottom carved away by a second circle, which leaves a curved band on top.
static void draw_happy_eye(int cx, int cy)
{
    canvas.fillCircle(cx, cy, 14, face_fg);
    canvas.fillCircle(cx, cy + 7, 14, face_bg);
}

// An eye with a heavy lid drawn over it. Only the bottom of the eye shows, flat across the top.
static void draw_lidded_eye(int cx, int cy, int open)
{
    int top = cy - EYE_H / 2;
    canvas.fillRoundRect(cx - EYE_W / 2, top, EYE_W, EYE_H, EYE_R, face_fg);
    canvas.fillRect(cx - EYE_W / 2, top, EYE_W, EYE_H - open, face_bg);
}

// A closed smile, the same carved-circle trick as the happy eye flipped upside down:
static void draw_smile(int cx, int cy, int r, int thickness)
{
    canvas.fillCircle(cx, cy, r, face_fg);
    canvas.fillCircle(cx, cy - thickness, r, face_bg);
}

// A wide open grin, which is the bottom half of a circle:
static void draw_grin(int cx, int cy, int r)
{
    canvas.fillCircle(cx, cy, r, face_fg);
    canvas.fillRect(cx - r, cy - r, 2 * r + 1, r, face_bg);
}

// A small round mouth, the kind that snores:
static void draw_snore(int cx, int cy)
{
    canvas.drawCircle(cx, cy, 4, face_fg);
    canvas.drawCircle(cx, cy, 3, face_fg);
}

// A four-pointed sparkle:
static void draw_sparkle(int x, int y, int s)
{
    canvas.drawFastHLine(x - s, y, 2 * s + 1, face_fg);
    canvas.drawFastVLine(x, y - s, 2 * s + 1, face_fg);
    canvas.fillRect(x - 1, y - 1, 3, 3, face_fg);
}

// Sparkles around the buddy's head. They take turns growing so they look like they twinkle.
static void draw_sparkles(uint8_t step)
{
    draw_sparkle(12, 22, step ? 2 : 5);
    draw_sparkle(116, 16, step ? 5 : 2);
    draw_sparkle(64, 10, step ? 3 : 1);
}

// Z's floating up off the buddy's head, one more on each beat until it starts over:
static void draw_zs(uint8_t step)
{
    canvas.setTextColor(face_fg);

    if (step >= 1)
    {
        canvas.setTextSize(1);
        canvas.setCursor(90, 38);
        canvas.print('z');
    }

    if (step >= 2)
    {
        canvas.setTextSize(2);
        canvas.setCursor(97, 21);
        canvas.print('z');
    }

    if (step >= 3)
    {
        canvas.setTextSize(3);
        canvas.setCursor(107, 3);
        canvas.print('Z');
    }
}

// Paint one frame of the face into the canvas, over the top of the last one:
static void draw_face(const Frame& f)
{
    canvas.fillRect(0, 0, SCREEN_W, FACE_H, face_bg);

    int eye_y = EYE_Y + f.lift;
    int left_x = LEFT_EYE_X + f.look;
    int right_x = RIGHT_EYE_X + f.look;
    int mouth_x = MOUTH_X + f.look / 3;
    int mouth_y = MOUTH_Y + f.lift;

    // The mouth always goes down before the eyes, since a few of these shapes carve into
    // the background color and should never cut a notch out of an eye
    switch (f.mood)
    {
        case Mood::CHEERFUL:
            draw_smile(mouth_x, mouth_y, 14, 5);
            draw_eye(left_x, eye_y, f.open);
            draw_eye(right_x, eye_y, f.open);
            break;

        case Mood::FOCUSED:
            draw_smile(mouth_x, mouth_y + 2, 9, 3);
            draw_eye(left_x, eye_y, f.open);
            draw_brow(left_x, eye_y, f.open, 1);
            draw_eye(right_x, eye_y, f.open);
            draw_brow(right_x, eye_y, f.open, -1);
            break;

        case Mood::JOYFUL:
            draw_grin(mouth_x, mouth_y, 14);
            draw_happy_eye(left_x, eye_y);
            draw_happy_eye(right_x, eye_y);
            draw_sparkles(f.step);
            break;

        case Mood::SLEEPY:
            draw_snore(mouth_x, mouth_y + 6);
            draw_lidded_eye(left_x, eye_y, f.open);
            draw_lidded_eye(right_x, eye_y, f.open);
            draw_zs(f.step);
            break;

        case Mood::NONE:
            break;
    }
}

// Face Motion:
// How far a hop has lifted the face at a moment, in pixels. Each hop rises and falls
// along half a sine wave, so it slows at the top the way a real jump does.
static int8_t hop(unsigned long t, unsigned long period, int height)
{
    float phase = (float)(t % period) / period;
    return (int8_t)(-lroundf(height * sinf(PI * phase)));
}

// How open the eyes are at a moment, closing over the first half of a blink and opening
// over the second. Once a blink finishes, the next one gets booked for a random time
// later, so the buddy never blinks like clockwork.
static int8_t blink(unsigned long now, int open, unsigned long length, long gap_min, long gap_max)
{
    if (now < next_blink)
    {
        return open;
    }

    unsigned long into = now - next_blink;

    if (into >= length)
    {
        next_blink = now + random(gap_min, gap_max);
        return open;
    }

    long half = length / 2;
    long from_edge = (into < (unsigned long)half) ? into : length - into;

    return open - (open - 2) * from_edge / half;
}

// Where the eyes are pointed at a moment. Every so often they peek off to one side and
// then come back to the middle, sliding a few pixels each frame so the turn reads as motion.
static int8_t glance(unsigned long now)
{
    if (now >= next_glance)
    {
        if (look_target == 0)
        {
            look_target = random(2) ? LOOK_PX : -LOOK_PX;
            next_glance = now + random(600, 1400);
        }

        else
        {
            look_target = 0;
            next_glance = now + random(2000, 4500);
        }
    }

    int step = look_target - look_now;
    step = constrain(step, -3, 3);
    look_now += step;

    return look_now;
}

// Work out how the face should look at a moment, based on its mood and how long it has been up:
static Frame next_frame(unsigned long now)
{
    unsigned long t = now - scene_start;
    Frame f = {mood, EYE_H, 0, 0, 0};

    switch (mood)
    {
        // Saying hello: two quick hops, then it waits for start, blinking and peeking around
        case Mood::CHEERFUL:
            f.lift = (t < 2 * HOP_MS) ? hop(t, HOP_MS, 5) : 0;
            f.open = blink(now, EYE_H, 160, 2000, 4500);
            f.look = glance(now);
            break;

        // Back to work: one firm nod, like saying "let's do this", then steady eyes on the task
        case Mood::FOCUSED:
            f.lift = (t < 400) ? -hop(t, 400, 4) : 0;
            f.open = blink(now, FOCUSED_EYE_H, 160, 2000, 4500);
            break;

        // Break time: bouncing the whole time it's up, eyes squeezed shut, sparkles twinkling
        case Mood::JOYFUL:
            f.lift = hop(t, 420, 6);
            f.step = (t / 250) % 2;
            break;

        // Paused: the lids drift heavier and lighter with each slow breath, it nods off
        // every so often, and z's float up one at a time
        case Mood::SLEEPY:
        {
            int open = 10 + lroundf(3 * sinf(2 * PI * (t % 3200) / 3200.0f));
            f.open = blink(now, open, 900, 4000, 8000);
            f.step = (t / 600) % 4;
            break;
        }

        case Mood::NONE:
            break;
    }

    return f;
}

static bool same(const Frame& a, const Frame& b)
{
    return a.mood == b.mood && a.open == b.open && a.look == b.look &&
           a.lift == b.lift && a.step == b.step;
}

// Hand the screen to the buddy in a new mood, painted in the colors given. Whatever goes
// under the face gets drawn next, and then show_scene puts the whole thing up at once.
static void start_scene(Mood next, uint16_t bg, uint16_t fg)
{
    unsigned long now = millis();

    mood = next;
    face_bg = bg;
    face_fg = fg;
    scene_start = now;
    next_blink = now + (next == Mood::SLEEPY ? random(3000, 6000) : random(1500, 3500));
    next_glance = now + random(1500, 3000);
    look_target = 0;
    look_now = 0;

    canvas.fillScreen(bg);
}

// Draw the first frame of the face and copy the whole scene onto the glass:
static void show_scene()
{
    shown = next_frame(millis());
    draw_face(shown);

    tft.clear_all(face_bg);
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
    last_frame = millis();
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
    work_dim = tft.color565(0, 55, 22);
    break_dim = tft.color565(0, 18, 80);
    background = work_bg;
    dimmed = work_dim;

    ink = tft.color565(25, 30, 35);
    drowsy = tft.color565(185, 195, 205);

    // Lines get broken up by hand here, so the library should leave them alone
    tft.setTextWrap(false);
    tft.setTextColor(ST77XX_WHITE);
    canvas.setTextWrap(false);

    tft.clear_all(background);
}

// Hand the screen to the buddy, who acts out the event and says its line in the bubble:
void display_message(Event e, const char* message)
{
    Mood next = Mood::CHEERFUL;

    // A mode change carries its own background color, label, and mood in with it
    if (e == Event::WORK_START)
    {
        background = work_bg;
        dimmed = work_dim;
        mode_label = "WORK";
        next = Mood::FOCUSED;
    }

    else if (e == Event::BREAK_START)
    {
        background = break_bg;
        dimmed = break_dim;
        mode_label = "BREAK";
        next = Mood::JOYFUL;
    }

    // The DONE events land in the split second before a mode begins, so letting them
    // draw only flashes a line too briefly to read. The message a user actually sees is
    // the one the new mode puts up, the way "Go!" is what matters at the start of a race.
    else if (e == Event::WORK_DONE || e == Event::BREAK_DONE)
    {
        return;
    }

    else if (e == Event::SESSION_DONE)
    {
        next = Mood::JOYFUL;
    }

    start_scene(next, background, ST77XX_WHITE);
    draw_bubble(ST77XX_WHITE);
    draw_speech(message);
    show_scene();
}

// Dim the lights and let the buddy doze until the countdown starts again. The bubble
// keeps the time that was left, so nobody has to guess where things stood.
void display_paused(int total_seconds)
{
    start_scene(Mood::SLEEPY, dimmed, drowsy);
    draw_bubble(drowsy);

    char left[16];
    snprintf(left, sizeof(left), "%d:%02d left", total_seconds / 60, total_seconds % 60);

    // "Paused" in big letters with the time in small ones under it, centered as a pair
    int gap = 6;
    int top = BUBBLE_Y + (BUBBLE_H - (16 + gap + 8)) / 2;

    canvas.setTextColor(ink);
    draw_centered(canvas, "Paused", top, 2);
    draw_centered(canvas, left, top + 16 + gap, 1);

    show_scene();
}

// Move the buddy on to its next frame, but only redraw when something actually changed:
void display_update()
{
    if (mood == Mood::NONE)
    {
        return;
    }

    unsigned long now = millis();

    if (now - last_frame < FRAME_MS)
    {
        return;
    }

    last_frame = now;

    Frame f = next_frame(now);

    if (same(f, shown))
    {
        return;
    }

    // The canvas is a full screen wide, so its first FACE_H rows sit side by side in
    // memory and can go out in one piece without touching the bubble underneath
    draw_face(f);
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, FACE_H);
    shown = f;
}

// Trade the buddy out for the mode label and the countdown:
void display_timer(int total_seconds)
{
    mood = Mood::NONE;

    tft.clear_all(background);

    draw_centered(tft, mode_label, MODE_Y, 2);
    draw_time(total_seconds);
}

// Redraw only the countdown, which is the one thing that changes second to second:
void display_time(int total_seconds)
{
    draw_time(total_seconds);
}

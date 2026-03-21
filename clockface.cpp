// clockface.cpp — Clock face renderers for the 32×8 LED matrix.
// See clockface.h for the face index table.
#include "clockface.h"
#include "globals.h"
#include "display.h"   // setLED(), drawChar(), playFullscreenGifFrame()

// ── Big-digit bitmaps (ported from AWTRIX3 Apps.cpp) ─────────────────────────
// 6 pixels wide × 7 pixels tall. MSB-first encoding, 1 byte per row.
// Rendering: bit=0 → draw color (digit pixel), bit=1 → leave black (background).
// Entry 10 = ':', entry 11 = ' ' (blank, for blinking colon).

static const uint8_t BIGDIGITS[12][7] = {
  {132,  48,  48,  48,  48,  48, 132},  // 0
  {204, 140, 204, 204, 204, 204,   0},  // 1
  {132,  48, 240, 196, 156,  48,   0},  // 2
  {132,  48, 240, 196, 240,  48, 132},  // 3
  {228, 196, 132,  36,   0, 228, 192},  // 4
  {  0,  60,   4, 240, 240,  48, 132},  // 5
  {196, 156,  60,   4,  48,  48, 132},  // 6
  {  0,  48, 240, 228, 204, 204, 204},  // 7
  {132,  48,  48, 132,  48,  48, 132},  // 8
  {132,  48,  48, 128, 240, 228, 140},  // 9
  {252, 204, 204, 252, 204, 204, 252},  // : (colon)
  {252, 252, 252, 252, 252, 252, 252},  // ' ' (blank — used for blinking)
};

// Draw one big digit at pixel position (x, y).
// Digit index: 0-9 for numerals, 10 for ':', 11 for blank.
static void drawBigDigit(int x, int y, int d, CRGB color) {
  for (int row = 0; row < 7; row++) {
    uint8_t m = BIGDIGITS[d][row];
    for (int col = 0; col < 6; col++) {
      if (!((m >> (7 - col)) & 1))
        setLED(x + col, y + row, color);
    }
  }
}

// ── Face 0 — Classic HH:MM:SS ─────────────────────────────────────────────────

static void drawFaceClassic(const struct tm& t) {
  const int xo = (MATRIX_WIDTH  - 27 + 1) / 2;  // = 3
  const int yo = (MATRIX_HEIGHT -  5)     / 2;  // = 1
  int h = t.tm_hour, m = t.tm_min, s = t.tm_sec;
  drawChar(xo +  0, yo, '0' + h / 10, colorClock);
  drawChar(xo +  4, yo, '0' + h % 10, colorClock);
  setLED(xo +  8, yo + 1, colorClock); setLED(xo +  8, yo + 3, colorClock);
  drawChar(xo + 10, yo, '0' + m / 10, colorClock);
  drawChar(xo + 14, yo, '0' + m % 10, colorClock);
  setLED(xo + 18, yo + 1, colorClock); setLED(xo + 18, yo + 3, colorClock);
  drawChar(xo + 20, yo, '0' + s / 10, colorClock);
  drawChar(xo + 24, yo, '0' + s % 10, colorClock);
}

// ── Face 1 — Big HH:MM ────────────────────────────────────────────────────────
// Position formula from AWTRIX3: xx = i*7 - (i>2?2:0) - (i==2)
// Fits exactly in 32 pixels: digits at 0-5, 7-12, 13-18, 19-24, 26-31.
// Colon blinks every second (odd=visible, even=blank).

static void drawFaceBig(const struct tm& t) {
  int h = t.tm_hour, m = t.tm_min, s = t.tm_sec;
  int colonIdx  = (s % 2) ? 10 : 11;  // 10=colon visible, 11=blank
  int digits[5] = { h / 10, h % 10, colonIdx, m / 10, m % 10 };
  for (int i = 0; i < 5; i++) {
    int xx = i * 7 - (i > 2 ? 2 : 0) - (i == 2 ? 1 : 0);
    drawBigDigit(xx, 0, digits[i], colorClock);
  }
}

// ── Face 2 — Rolodex calendar + HH:MM + weekday dots ─────────────────────────
// Left 9×7 px (y=0..6):
//   - Blue header bar (y=0..1) with two black ring holes → rolodex look
//   - Grey body with date number centred below the header
// Right x=10..31: HH:MM centred in the remaining 22 px.
// Bottom row y=7: weekday tick marks across full width (Monday-first).

static void drawFaceCalendar(const struct tm& t) {
  // ── Calendar box body (9 wide × 7 tall, leaves y=7 for weekday dots) ──────
  CRGB body(90, 90, 90);
  for (int cx = 0; cx < 9; cx++)
    for (int cy = 0; cy < MATRIX_HEIGHT; cy++)
      setLED(cx, cy, body);

  // Blue header bar (y=0..1)
  CRGB hdr(0, 80, 220);
  for (int cx = 0; cx < 9; cx++) {
    setLED(cx, 0, hdr);
    setLED(cx, 1, hdr);
  }

  // Rolodex ring holes: two 2-px black marks at top of header
  setLED(1, 0, CRGB::Black); setLED(2, 0, CRGB::Black);
  setLED(6, 0, CRGB::Black); setLED(7, 0, CRGB::Black);

  // Date number — centred in 9 px, just below header (y=2, font is 5 rows tall → y=2..6)
  int day = t.tm_mday;
  char buf[3];
  snprintf(buf, sizeof(buf), "%d", day);
  int dayLen = strlen(buf);
  int dayW   = dayLen * 4 - 1;
  int dayX   = (9 - dayW) / 2;
  for (int i = 0; i < dayLen; i++)
    drawChar(dayX + i * 4, 2, buf[i], CRGB(10, 10, 10));

  // ── HH:MM on the right (x=10..31, 22 px available, centred) ──────────────
  // Colon blinks every second so the user can see the clock is running.
  const int yo = (MATRIX_HEIGHT - 5) / 2;  // = 1
  int h = t.tm_hour, m = t.tm_min;
  int xi = 11;
  drawChar(xi,      yo, '0' + h / 10, colorClock); xi += 4;
  drawChar(xi,      yo, '0' + h % 10, colorClock); xi += 4;
  if (t.tm_sec % 2) {
    setLED(xi, yo + 1, colorClock);
    setLED(xi, yo + 3, colorClock);
  }
  xi += 2;
  drawChar(xi,      yo, '0' + m / 10, colorClock); xi += 4;
  drawChar(xi,      yo, '0' + m % 10, colorClock);

  // ── Weekday dots at y=7 — 7×1 dot + 6×2 gap = 19 px ────────────────────────
  // Available after calendar box (x=9..31 = 23 px): margin = (23-19)/2 = 2
  // → start at x=11, step=3: Mo=11 Tu=14 We=17 Th=20 Fr=23 Sa=26 Su=29
  int today = (t.tm_wday + 6) % 7;  // Monday-first
  CRGB inactive(35, 35, 35);
  for (int i = 0; i < 7; i++) {
    CRGB c = (i == today) ? colorClock : inactive;
    setLED(10 + i * 3, 7, c);
  }
}

// ── Face 3 — Weekday bar ──────────────────────────────────────────────────────
// HH:MM:SS centred (identical to Face 0) plus 7 three-pixel weekday ticks at
// the bottom row (y=7). Monday-first order; active day uses colorClock.

static void drawFaceWeekday(const struct tm& t) {
  drawFaceClassic(t);

  // tm_wday: Sun=0, Mon=1 … Sat=6  →  convert to Monday-first index 0-6
  int today = (t.tm_wday + 6) % 7;
  CRGB inactive(35, 35, 35);
  for (int i = 0; i < 7; i++) {
    CRGB c = (i == today) ? colorClock : inactive;
    int x0 = 2 + i * 4;  // LINE_START=2, (LINE_WIDTH=3)+(LINE_SPACING=1)=4
    setLED(x0,     7, c);
    setLED(x0 + 1, 7, c);
    setLED(x0 + 2, 7, c);
  }
}

// ── Face 4 — Binary clock ─────────────────────────────────────────────────────
// Three rows of 6 bits each, rendered as 2×2 pixel blocks.
//   y=0..1  Hours   (red ON,   dim OFF)
//   y=3..4  Minutes (green ON, dim OFF)
//   y=6..7  Seconds (blue ON,  dim OFF)
// Bit order: MSB (value 32) leftmost at x=5, LSB (value 1) at x=25.

static void drawFaceBinary(const struct tm& t) {
  int values[3]    = { t.tm_hour, t.tm_min, t.tm_sec };
  CRGB onColor[3]  = { CRGB(200,   0,   0),   // hours   — red
                       CRGB(  0, 200,   0),   // minutes — green
                       CRGB(  0, 100, 255) }; // seconds — blue
  CRGB offColor(18, 18, 18);
  int  yRow[3]     = { 0, 3, 6 };

  for (int row = 0; row < 3; row++) {
    for (int i = 0; i < 6; i++) {
      bool on = (values[row] >> (5 - i)) & 1;
      CRGB c  = on ? onColor[row] : offColor;
      int xb  = 5 + i * 4;
      int yb  = yRow[row];
      setLED(xb,     yb,     c);
      setLED(xb + 1, yb,     c);
      setLED(xb,     yb + 1, c);
      setLED(xb + 1, yb + 1, c);
    }
  }
}

// ── Face 4 — Big HH:MM with scrolling rainbow inside the digits ───────────────
// Renders a smooth horizontal HSV rainbow that scrolls left across the digit
// shapes. Each digit pixel's hue is derived from its x position + a scroll
// offset that advances every frame — always smooth, no GIF file needed.
// Update rate: ~50 ms (≈20 fps) for fluid motion.

static int drawFaceBigGif(const struct tm& t) {
  int h = t.tm_hour, m = t.tm_min;
  int digits[5] = { h / 10, h % 10, 10, m / 10, m % 10 };  // 10 = colon, always on

  // Build digit pixel mask (shared by both GIF and rainbow paths)
  uint32_t digitMask[MATRIX_HEIGHT] = {};
  for (int i = 0; i < 5; i++) {
    int xx = i * 7 - (i > 2 ? 2 : 0) - (i == 2 ? 1 : 0);
    for (int row = 0; row < 7; row++) {
      uint8_t mask = BIGDIGITS[digits[i]][row];
      for (int col = 0; col < 6; col++)
        if (!((mask >> (7 - col)) & 1) && xx + col >= 0 && xx + col < MATRIX_WIDTH)
          digitMask[row] |= (1u << (xx + col));
    }
  }

  // ── GIF path: /bigtime.gif exists ────────────────────────────────────────
  int gifDelay = 80;
  if (playFullscreenGifFrame("/bigtime.gif", &gifDelay)) {
    // Black out all non-digit pixels; digit pixels keep the GIF color
    for (int y = 0; y < MATRIX_HEIGHT; y++)
      for (int x = 0; x < MATRIX_WIDTH; x++)
        if (!(digitMask[y] & (1u << x)))
          setLED(x, y, CRGB::Black);
    return max(gifDelay, 80);  // never faster than 80 ms to avoid WS2812B glitches
  }

  // ── Fallback: built-in scrolling rainbow ─────────────────────────────────
  static uint8_t scroll = 0;
  scroll += 2;
  for (int y = 0; y < MATRIX_HEIGHT; y++)
    for (int x = 0; x < MATRIX_WIDTH; x++)
      if (digitMask[y] & (1u << x))
        setLED(x, y, CHSV((uint8_t)(scroll + x * 8), 255, 255));
      else
        setLED(x, y, CRGB::Black);
  return 80;
}

// ── Dispatcher ────────────────────────────────────────────────────────────────

int drawClockFace(const struct tm& t) {
  switch (clockFace % CLOCK_FACE_COUNT) {
    case 1:  drawFaceClassic(t);  return 1000;
    case 2:  drawFaceWeekday(t);  return 1000;
    case 3:  drawFaceBig(t);      return 1000;
    case 4:  return drawFaceBigGif(t);
    case 5:  drawFaceBinary(t);   return 1000;
    default: drawFaceCalendar(t); return 1000;  // 0 = default
  }
}

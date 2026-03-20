#pragma once
// clockface.h — Five clock face renderers for the 32×8 LED matrix.
//
// Face 0  Classic   — HH:MM:SS centred (3×5 font)
// Face 1  Big       — HH:MM in large 6×7 AWTRIX-style bitmap digits
// Face 2  Calendar  — 9×8 calendar box (date) on left + HH:MM on right
// Face 3  Weekday   — HH:MM:SS + 7 weekday tick marks at bottom row
// Face 4  Binary    — H / M / S as 6-bit rows (2×2 blocks, R/G/B)
//
// Call drawClockFace(t) once per second from loopDisplay() after FastLED.clear().
// clockFace is persisted to NVS key "clk_face".

#include <Arduino.h>
#include <time.h>

#define CLOCK_FACE_COUNT  5

void drawClockFace(const struct tm& t);

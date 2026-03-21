#pragma once
// clockface.h — Six clock face renderers for the 32×8 LED matrix.
//
// Face 0  Calendar  — 9×8 calendar box (date) on left + HH:MM on right
// Face 1  Classic   — HH:MM:SS centred (3×5 font)
// Face 2  Weekday   — HH:MM:SS + 7 weekday tick marks at bottom row
// Face 3  Big       — HH:MM in large 6×7 AWTRIX-style bitmap digits
// Face 4  BigGIF    — HH:MM big digits as black cutouts over /bigtime.gif
// Face 5  Binary    — H / M / S as 6-bit rows (2×2 blocks, R/G/B)
//
// drawClockFace() returns the suggested redraw delay in ms (33 for GIF faces,
// 1000 for static faces). loopDisplay() uses this to pace redraws.
// clockFace is persisted to NVS key "clk_face".

#include <Arduino.h>
#include <time.h>

#define CLOCK_FACE_COUNT  6

int drawClockFace(const struct tm& t);

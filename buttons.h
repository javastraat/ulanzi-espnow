#pragma once
#include <Arduino.h>

#define BTN_LONGPRESS_MS 600   // hold duration to trigger long press

void loopButtons();
void triggerButton(int i, bool longPress);  // 0=left, 1=middle, 2=right

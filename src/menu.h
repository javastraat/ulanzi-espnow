#pragma once
#include <Arduino.h>

// Menu state machine for on-device settings navigation.
//
// Enter:       Middle long press
// Next item:   Middle short press
// Adjust:      Left (decrease/off) or Right (increase/on)
// Exit + save: Middle long press (from within menu)

bool isMenuActive();
void enterMenu();
void loopMenu();

// Button callbacks (called by buttons.cpp when menu is active)
void menuButtonLeft();
void menuButtonMiddle(bool longPress);
void menuButtonRight();

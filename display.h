#pragma once
#include <FastLED.h>

// Sentinel returned by drawIcon on failure (must be more negative than any valid x)
#define ICON_DRAW_FAILED  (-9999)

// Unified icon draw: routes to GIF or JPEG decoder. x0 = left edge.
// Returns text start x (= x0 + iconWidth + 1), or ICON_DRAW_FAILED on failure.
int drawIcon(const char* path, int* delayMs, int x0 = 0);

void setupDisplay();
void loopDisplay();
void loopBrightness();
void loopAutoRotate();
void drawBootScreen();
void drawUpdate();
void drawProgress(int barW);
void drawDone();
void drawError();
void drawReboot();
void drawClickOK();
void drawChar(int x, int y, char c, CRGB color);
void setLED(int x, int y, CRGB color);
void _gifCloseIfOpen();
bool playFullscreenGifFrame(const char* path, int* delayMs);
const char* getScreenName();
void screensaverApplyBrightness();
void resetScreensaverIdle();
void drawIndicators();
void drawBrightnessOverlay();

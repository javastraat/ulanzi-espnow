// buttons.cpp — Debounced button handler with short/long-press detection.
#include "buttons.h"
#include "globals.h"
#include "buzzer.h"
#include "display.h"
#include "menu.h"
#include "mqtt.h"
#include "nvs_settings.h"

// ── Brightness cycle helpers ──────────────────────────────────────────────────

static const uint8_t BMODE_LEVELS[BMODE_COUNT] = { 0, 5, 20, 80, 255 };  // AUTO unused (LDR)

static void applyBrightnessMode(BrightnessMode m) {
  brightnessMode = m;
  if (m == BMODE_AUTO) {
    autoBrightnessEnabled = true;
  } else {
    autoBrightnessEnabled = false;
    currentBrightness     = BMODE_LEVELS[m];
    FastLED.setBrightness(currentBrightness);
  }
  // Show overlay for 5 seconds
  brightnessOverlayActive = true;
  brightnessOverlayUntil  = millis() + 5000;
  saveSettings();
}

// ── Notification replay ───────────────────────────────────────────────────────

#if RECV_POCSAG
static void replayLastNotification() {
  if (pocsagMsgLen == 0) return;   // nothing received yet
  bool longMsg = (pocsagMsgLen > 5);
  pocsagMsgActive      = true;
  pocsagIsScrolling    = longMsg;
  pocsagScrollX        = MATRIX_WIDTH;
  pocsagScrollPass     = 0;
  pocsagScrollLast     = 0;
  pocsagStaticUntil    = millis() + POCSAG_STATIC_MS;
  pocsagStaticLastDraw = 0;
  resetScreensaverIdle();
}
#endif

// ── Button actions ────────────────────────────────────────────────────────────

void triggerButton(int i, bool longPress) {
  // While OTA confirm is pending, intercept all buttons before normal handling
  if (otaAwaitingConfirm) {
    if (i == 1 && !longPress) {
      // Middle short = confirm → reboot
      otaAwaitingConfirm = false;
      LOG("[BTN] Middle — OTA confirmed, rebooting\n");
      drawReboot();
      delay(1500);
      ESP.restart();
    } else {
      // Any other button or long press = cancel
      otaAwaitingConfirm = false;
      LOG("[BTN] Button %d — OTA cancelled\n", i);
      buzzerClick();
      drawError();
      delay(5000);
      FastLED.clear(); FastLED.show();
      otaInProgress = false;
    }
    return;
  }

  // While waiting for OTA upload — any button cancels back to clock
  if (otaReadyMode) {
    otaReadyMode = false;
    FastLED.clear();
    FastLED.show();
    LOG("[OTA] Ready mode cancelled by button\n");
    buzzerClick();
    return;
  }

  // While menu is active, route to menu handlers
  if (isMenuActive()) {
    resetScreensaverIdle();
    switch (i) {
      case 0: menuButtonLeft();               break;
      case 1: menuButtonMiddle(longPress);    break;
      case 2: menuButtonRight();              break;
    }
    buzzerClick();
    return;
  }

  resetScreensaverIdle();

  switch (i) {
    // ── Left: cycle brightness mode ─────────────────────────────────────────
    case 0:
      if (!longPress) {
        BrightnessMode next = (BrightnessMode)((brightnessMode + 1) % BMODE_COUNT);
        applyBrightnessMode(next);
        LOG("[BTN] Left — brightness mode: %d\n", next);
        buzzerClick();
      }
      break;

    // ── Middle short: replay last notification / long: enter menu ──────────
    case 1:
      if (longPress) {
        LOG("[BTN] Middle long — enter menu\n");
        enterMenu();
        buzzerClick();
      } else {
        LOG("[BTN] Middle short — replay notification\n");
#if RECV_POCSAG
        replayLastNotification();
#endif
        buzzerClick();
      }
      break;

    // ── Right: cycle display mode ───────────────────────────────────────────
    case 2:
      if (!longPress) {
        if (sht31Available) {
          displayMode = (DisplayMode)((displayMode + 1) % MODE_COUNT);
          modeActiveUntil = (displayMode != MODE_CLOCK) ? millis() + MODE_TIMEOUT_MS : 0;
          LOG("[BTN] Right — mode: %s\n",
            displayMode == MODE_TEMP     ? "temp" :
            displayMode == MODE_HUMIDITY ? "humidity" :
            displayMode == MODE_BATTERY  ? "battery" : "clock");
        } else {
          // No SHT31 — cycle clock/battery only
          displayMode = (displayMode == MODE_CLOCK) ? MODE_BATTERY : MODE_CLOCK;
          modeActiveUntil = (displayMode != MODE_CLOCK) ? millis() + MODE_TIMEOUT_MS : 0;
          LOG("[BTN] Right — mode: %s\n",
            displayMode == MODE_BATTERY ? "battery" : "clock");
        }
        buzzerClick();
      }
      break;
  }

  mqttNotifyState();
}

// ── Loop: debounce + short/long-press detection ───────────────────────────────

void loopButtons() {
  static bool          lastState[3]     = {HIGH, HIGH, HIGH};
  static unsigned long pressStart[3]    = {0, 0, 0};
  static bool          longFired[3]     = {false, false, false};
  static bool          shortEligible[3] = {false, false, false};
  static const uint8_t pins[3]          = {BTN_LEFT, BTN_MIDDLE, BTN_RIGHT};

  for (int i = 0; i < 3; i++) {
    bool state = digitalRead(pins[i]);

    if (state != lastState[i]) {
      if (state == LOW) {
        // Button just pressed — start timing
        pressStart[i]    = millis();
        longFired[i]     = false;
        shortEligible[i] = false;
      } else {
        // Button just released — fire short press if debounced and long not already fired
        if (!longFired[i] && shortEligible[i]) {
          triggerButton(i, false);
        }
        longFired[i]     = false;
        shortEligible[i] = false;
      }
      lastState[i] = state;
    } else if (state == LOW) {
      unsigned long held = millis() - pressStart[i];
      // Mark eligible for short press once debounce window passed
      if (!shortEligible[i] && held >= BTN_DEBOUNCE_MS) {
        shortEligible[i] = true;
      }
      // Fire long press at threshold (once per press)
      if (!longFired[i] && held >= BTN_LONGPRESS_MS) {
        longFired[i] = true;
        triggerButton(i, true);
      }
    }
  }
}

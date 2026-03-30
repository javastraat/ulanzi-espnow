#include <Arduino.h>
// menu.cpp — On-device settings menu (middle long-press to enter/exit).
//
// Display layout (32×8 LED matrix, 3×5 font):
//   Row 0-6: item label (4 chars) + space + value (2-3 chars), centered
//   Row 7:   progress dots (one per item, current item lit)
//
// Navigation:
//   Middle short → next item
//   Left / Right → toggle OFF / ON  (or trigger action for action items)
//   Middle long  → exit
//
// Menu items:
//   ROTA  auto-rotate        ON / OFF
//   SCRN  screensaver        ON / OFF
//   POCG  POCSAG receive     ON / OFF
//   DOTS  indicators         ON / OFF
//   BEEP  click sound        ON / OFF
//   IP    show IP address    → triggers scroll, exits menu

#include "menu.h"
#include "globals.h"
#include "display.h"
#include "nvs_settings.h"
#include "web_handlers_system.h"

// ── Menu item definitions ─────────────────────────────────────────────────────

enum ItemType { ITEM_BOOL, ITEM_ACTION };

struct MenuItem {
  const char* label;     // 4 chars
  ItemType    type;
  bool*       value;     // ITEM_BOOL only — nullptr for ITEM_ACTION
};

static void triggerIpScroll();
static void triggerOtaReady();

static MenuItem items[] = {
  { "ROTA", ITEM_BOOL,   &autoRotateEnabled  },
  { "SCRN", ITEM_BOOL,   &screensaverEnabled },
  { "MSGS", ITEM_BOOL,   &recvPocsagEnabled  },
  { "DOTS", ITEM_BOOL,   &indicatorsEnabled  },
  { "BEEP", ITEM_BOOL,   &buzzerClickEnabled },
  { "SHOW IP", ITEM_ACTION, nullptr          },
  { "UPDATE", ITEM_ACTION, nullptr           },
};
static const int ITEM_COUNT = sizeof(items) / sizeof(items[0]);

// ── State ─────────────────────────────────────────────────────────────────────

static bool menuActive     = false;
static int  currentItem    = 0;
static bool needsRedraw    = false;
static bool confirmPending = false;  // true when UPDT confirm step is active

// ── IP scroll trigger ─────────────────────────────────────────────────────────

static void triggerIpScroll() {
  // Build the IP string the same way the WiFi connection code does
  String ip = WiFi.localIP().toString();
  if (ip == "0.0.0.0" || !WiFi.isConnected()) {
    // Not connected — show "NO IP" instead
    strncpy(ipScrollMsg, "NO IP", sizeof(ipScrollMsg) - 1);
    ipScrollMsg[sizeof(ipScrollMsg) - 1] = '\0';
  } else {
    String msg = "IP:" + ip;
    strncpy(ipScrollMsg, msg.c_str(), sizeof(ipScrollMsg) - 1);
    ipScrollMsg[sizeof(ipScrollMsg) - 1] = '\0';
  }
  ipScrollLen  = strlen(ipScrollMsg);
  ipScrollX    = MATRIX_WIDTH;
  ipScrollPass = 0;
  ipScrollLast = 0;
  ipScrollActive = true;
  displayMode    = MODE_CLOCK;   // return to clock when scroll finishes

  // Exit menu so the IP scroll can take over the display
  menuActive = false;
  FastLED.clear();
  FastLED.show();
  LOG("[MENU] IP scroll: %s\n", ipScrollMsg);
}

// ── OTA ready trigger ─────────────────────────────────────────────────────────

static void triggerOtaReady() {
  menuActive     = false;
  confirmPending = false;
  String err;
  int written = 0;
  if (!startOtaDownloadFromGithub("stable", &err, &written)) {
    LOG("[MENU] OTA download failed: %s\n", err.c_str());
    return;
  }
  LOG("[MENU] OTA download complete: %d bytes\n", written);
}

// ── Drawing ───────────────────────────────────────────────────────────────────

static void drawConfirm() {
  // "SURE?" — 5 chars, amber warning color, centered
  const char* word  = "SURE?";
  int         len   = 5;
  int         xo    = (MATRIX_WIDTH - (len * 4 - 1)) / 2;
  int         yo    = (MATRIX_HEIGHT - 5) / 2;
  CRGB        color = CRGB(255, 140, 0);
  FastLED.clear();
  for (int i = 0; i < len; i++)
    drawChar(xo + i * 4, yo, word[i], color);
  FastLED.show();
}

static void drawMenu() {
  FastLED.clear();

  const MenuItem& item = items[currentItem];
  int yo = 1;  // vertically center 5-row glyph in 8 rows

  if (item.type == ITEM_ACTION) {
    // Action item: show label centered, cyan (no ON/OFF value)
    // "IP  " = 4 chars → center in 32px
    int labelLen = strlen(item.label);
    int totalW   = labelLen * 4 - 1;
    int xo       = (MATRIX_WIDTH - totalW) / 2;
    for (int i = 0; i < labelLen; i++)
      drawChar(xo + i * 4, yo, item.label[i], CRGB(0, 200, 255));
  } else {
    // Bool item: "XXXX ON" or "XXXX OFF"
    bool        val      = *item.value;
    const char* valStr   = val ? "ON" : "OFF";
    CRGB        valColor = val ? CRGB(0, 220, 50) : CRGB(220, 40, 0);
    int labelLen = 4;
    int valLen   = val ? 2 : 3;
    int totalLen = labelLen + 1 + valLen;
    int totalW   = totalLen * 4 - 1;
    int xo       = (MATRIX_WIDTH - totalW) / 2;

    for (int i = 0; i < labelLen; i++)
      drawChar(xo + i * 4, yo, item.label[i], CRGB::White);
    int valX = xo + (labelLen + 1) * 4;
    for (int i = 0; i < valLen; i++)
      drawChar(valX + i * 4, yo, valStr[i], valColor);
  }

  // Bottom row: progress dots
  int spacing  = MATRIX_WIDTH / ITEM_COUNT;
  int dotStart = (MATRIX_WIDTH - (ITEM_COUNT - 1) * spacing) / 2;
  for (int i = 0; i < ITEM_COUNT; i++) {
    int x = dotStart + i * spacing;
    setLED(x, 7, (i == currentItem) ? CRGB::White : CRGB(40, 40, 40));
  }

  FastLED.show();
}

// ── Public API ────────────────────────────────────────────────────────────────

bool isMenuActive() { return menuActive; }

void enterMenu() {
  menuActive     = true;
  currentItem    = 0;
  confirmPending = false;
  needsRedraw    = true;
}

void loopMenu() {
  if (!menuActive) return;
  if (needsRedraw) {
    if (confirmPending)
      drawConfirm();
    else
      drawMenu();
    needsRedraw = false;
  }
}

void menuButtonLeft() {
  if (confirmPending) {
    confirmPending = false;
    needsRedraw    = true;
    LOG("[MENU] UPDT cancelled\n");
    return;
  }
  if (items[currentItem].type == ITEM_ACTION) {
    if (strcmp(items[currentItem].label, "UPDATE") == 0)
      { confirmPending = true; needsRedraw = true; }
    else
      triggerIpScroll();
  } else {
    *items[currentItem].value = false;
    saveSettings();
    needsRedraw = true;
  }
}

void menuButtonRight() {
  if (confirmPending) {
    confirmPending = false;
    needsRedraw    = true;
    LOG("[MENU] UPDT cancelled\n");
    return;
  }
  if (items[currentItem].type == ITEM_ACTION) {
    if (strcmp(items[currentItem].label, "UPDATE") == 0)
      { confirmPending = true; needsRedraw = true; }
    else
      triggerIpScroll();
  } else {
    *items[currentItem].value = true;
    saveSettings();
    needsRedraw = true;
  }
}

void menuButtonMiddle(bool longPress) {
  if (confirmPending) {
    if (longPress) {
      // Long press also cancels confirm
      confirmPending = false;
      needsRedraw    = true;
      LOG("[MENU] UPDT cancelled\n");
    } else {
      // Short press confirms → start OTA ready mode
      triggerOtaReady();
    }
    return;
  }

  if (longPress) {
    menuActive     = false;
    confirmPending = false;
    FastLED.clear();
    FastLED.show();
    LOG("[MENU] Exit\n");
  } else {
    currentItem = (currentItem + 1) % ITEM_COUNT;
    needsRedraw = true;
    LOG("[MENU] Item %d: %s\n", currentItem, items[currentItem].label);
  }
}

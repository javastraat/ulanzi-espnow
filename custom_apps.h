#pragma once
#include <Arduino.h>
#include <FastLED.h>

// ── Custom Apps (AWTRIX3-compatible app slots) ────────────────────────────────
// Apps are stored in LittleFS /customapps/{name}.json and loaded at boot.
// MQTT: publish JSON to {mqttNodeId}/custom_app/{name}/set
// HTTP: GET/POST/DELETE /api/custom_apps

#define CUSTOM_APP_SLOTS   8    // max simultaneous apps
#define CA_NAME_LEN       16    // name buffer (incl. null)
#define CA_TEXT_LEN       64    // text buffer (incl. null)
#define CA_ICON_LEN       32    // icon path (incl. null)

struct CustomApp {
  bool          enabled;                // slot in use
  char          name[CA_NAME_LEN];      // unique key
  char          text[CA_TEXT_LEN];      // display text
  char          icon[CA_ICON_LEN];      // icon file path (empty = none)
  CRGB          color;                  // text color
  uint16_t      duration;               // seconds to show (0 = autoRotateIntervalSec)
  bool          center;                 // true = static centered, false = scrolling
  uint8_t       scrollSpeed;            // ms/step (0 = POCSAG_SCROLL_SPEED_MS)
  bool          repeat;                 // false = one scroll pass then expire
  unsigned long expiresAt;              // millis() of lifetime expiry (0 = never)
  // Runtime state (not persisted)
  int           scrollX;
  int           scrollPass;
  unsigned long scrollLast;
  bool          scrollFirst;
};

extern CustomApp customApps[CUSTOM_APP_SLOTS];
extern uint8_t   customAppCount;        // number of enabled slots

void        initCustomApps();
bool        loopCustomApp();            // render active app; returns true if painting
bool        customAppIsActive();        // true if an app is showing
void        customAppAdvance();         // called from loopAutoRotate
void        customAppSetFromJson(const char* name, const String& json);
void        customAppDelete(const char* name);
void        customAppListJson(char* buf, int len);
const char* customAppScreenName();      // for getScreenName()

// Interleave state (used by loopAutoRotate in display.cpp)
bool customAppShownThisCycle();
void customAppSetShownFlag(bool v);

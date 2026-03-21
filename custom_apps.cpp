// custom_apps.cpp — AWTRIX3-compatible custom app slots.
//
// Apps are persisted in LittleFS /customapps/{name}.json.
// Display rendering reuses the POCSAG scroll/static infrastructure.
// Rotation interleaves apps between built-in screens (clock/temp/hum/bat).
//
// MQTT payload format (all fields optional except text):
//   {"text":"Hello","color":"#FF0000","icon":"/icons/x.gif",
//    "duration":10,"lifetime":300,"center":false,"scrollSpeed":0,"repeat":true}
//
#include "custom_apps.h"
#include "globals.h"
#include "display.h"
#include "transition.h"
#include <LittleFS.h>

// ── Global state ──────────────────────────────────────────────────────────────

CustomApp customApps[CUSTOM_APP_SLOTS] = {};
uint8_t   customAppCount = 0;

// ── File-private state ────────────────────────────────────────────────────────

static int           _activeSlot          = -1;   // currently displaying (-1 = none)
static unsigned long _slotUntil           = 0;    // when duration expires
static int           _rr                  = 0;    // round-robin index
static int           _phaseRemaining      = 0;    // apps left in current phase (set by display.cpp)

// Redraw timing for static and scroll modes
static unsigned long _nextStaticDraw      = 0;
static int           _staticGifDelay      = 100;
static unsigned long _scrollNextGifMs     = 0;
static bool          _scrollHasIcon       = false;

static char          _screenNameBuf[24]   = "app";

// ── JSON helpers ──────────────────────────────────────────────────────────────

static String _jStr(const String& json, const char* key, const char* def = "") {
  String needle = String("\"") + key + "\"";
  int k = json.indexOf(needle);
  if (k < 0) return String(def);
  int c = json.indexOf(':', k + needle.length());
  if (c < 0) return String(def);
  int q1 = json.indexOf('"', c + 1);
  if (q1 < 0) return String(def);
  int q2 = json.indexOf('"', q1 + 1);
  if (q2 < 0) return String(def);
  return json.substring(q1 + 1, q2);
}

static int _jInt(const String& json, const char* key, int def = 0) {
  String needle = String("\"") + key + "\"";
  int k = json.indexOf(needle);
  if (k < 0) return def;
  int c = json.indexOf(':', k + needle.length());
  if (c < 0) return def;
  String tail = json.substring(c + 1);
  tail.trim();
  if (tail.length() == 0) return def;
  if (tail[0] == '[' || tail[0] == '{') return def;
  // Handle quoted number: "5"  →  5
  if (tail[0] == '"') {
    int q2 = tail.indexOf('"', 1);
    String inner = (q2 > 0) ? tail.substring(1, q2) : tail.substring(1);
    inner.trim();
    return inner.toInt();
  }
  return tail.toInt();
}

static bool _jBool(const String& json, const char* key, bool def = false) {
  String needle = String("\"") + key + "\"";
  int k = json.indexOf(needle);
  if (k < 0) return def;
  int c = json.indexOf(':', k + needle.length());
  if (c < 0) return def;
  String tail = json.substring(c + 1);
  tail.trim();
  // Handle quoted value: "false", "true", "0", "2"
  if (tail.length() > 0 && tail[0] == '"') {
    int q2 = tail.indexOf('"', 1);
    String inner = (q2 > 0) ? tail.substring(1, q2) : tail.substring(1);
    inner.trim(); inner.toLowerCase();
    if (inner == "true"  || inner == "yes" || inner == "1") return true;
    if (inner == "false" || inner == "no"  || inner == "0") return false;
    return inner.toInt() != 0;  // "2" → true
  }
  tail.toLowerCase();
  if (tail.startsWith("true"))  return true;
  if (tail.startsWith("false")) return false;
  return tail.toInt() != 0;    // numeric 0 = false, anything else = true
}

static CRGB _jColor(const String& json, const char* key, CRGB def = CRGB::White) {
  String s = _jStr(json, key, "");
  if (s.length() == 7 && s[0] == '#') {
    long n = strtol(s.c_str() + 1, nullptr, 16);
    return CRGB((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
  }
  return def;
}

// ── Placeholder + icon helpers ────────────────────────────────────────────────

// Resolve {{topic}} placeholders in tmpl → out, substituting MQTT cached values.
// Unresolved placeholders (topic not yet received) are replaced with "N/A".
static void _resolvePlaceholders(const char* tmpl, char* out, int outLen) {
  int oi = 0;
  const char* p = tmpl;
  while (*p && oi < outLen - 1) {
    if (p[0] == '{' && p[1] == '{') {
      const char* close = strstr(p + 2, "}}");
      if (close) {
        char topicBuf[64] = {};
        int tlen = min((int)(close - p - 2), 63);
        strncpy(topicBuf, p + 2, tlen);
        char valBuf[48] = {};
        const char* sub = mqttCacheGet(topicBuf, valBuf, sizeof(valBuf)) ? valBuf : "N/A";
        for (const char* v = sub; *v && oi < outLen - 1; v++)
          out[oi++] = *v;
        p = close + 2;
        continue;
      }
    }
    out[oi++] = *p++;
  }
  out[oi] = '\0';
}

// Parse all {{topic}} in text and subscribe to each via MQTT.
static void _subscribePlaceholders(const char* text) {
  const char* p = text;
  while (*p) {
    if (p[0] == '{' && p[1] == '{') {
      const char* close = strstr(p + 2, "}}");
      if (close) {
        char topic[64] = {};
        int tlen = min((int)(close - p - 2), 63);
        strncpy(topic, p + 2, tlen);
        if (topic[0]) mqttSubscribeTopic(topic);
        p = close + 2;
        continue;
      }
    }
    p++;
  }
}

// Resolve a bare icon ID or name to a full LittleFS path.
// Already-absolute paths (start with '/') are returned unchanged.
// Bare IDs like "55505" → tries /icons/55505.gif, then /icons/55505.jpg.
static void _resolveIconPath(const char* raw, char* out, int outLen) {
  out[0] = '\0';
  if (!raw || raw[0] == '\0') return;
  if (raw[0] == '/') { strncpy(out, raw, outLen - 1); out[outLen-1] = '\0'; return; }
  // Bare name/ID — look in /icons/
  if (!fsAvailable) return;
  char path[CA_ICON_LEN];
  snprintf(path, sizeof(path), "/icons/%s.gif", raw);
  if (LittleFS.exists(path)) { strncpy(out, path, outLen - 1); out[outLen-1] = '\0'; return; }
  snprintf(path, sizeof(path), "/icons/%s.jpg", raw);
  if (LittleFS.exists(path)) { strncpy(out, path, outLen - 1); out[outLen-1] = '\0'; return; }
  // Not found — icon will be skipped
}

// ── LittleFS helpers ──────────────────────────────────────────────────────────

static void _saveToFile(int slot) {
  if (!fsAvailable) return;
  const CustomApp& a = customApps[slot];
  char path[56];
  snprintf(path, sizeof(path), "/customapps/%s.json", a.name);
  char colStr[8];
  snprintf(colStr, sizeof(colStr), "#%02X%02X%02X", a.color.r, a.color.g, a.color.b);
  char buf[320];
  snprintf(buf, sizeof(buf),
    "{\"name\":\"%s\",\"text\":\"%s\",\"color\":\"%s\",\"icon\":\"%s\","
    "\"duration\":%u,\"center\":%s,\"scrollSpeed\":%u,\"repeat\":%s,\"show\":%s,\"lifetime\":0}",
    a.name, a.text, colStr, a.icon,
    a.duration, a.center ? "true" : "false",
    a.scrollSpeed, a.repeat ? "true" : "false",
    a.show ? "true" : "false");
  File f = LittleFS.open(path, "w");
  if (f) { f.print(buf); f.close(); }
}

static void _loadFromFile(const char* path) {
  File f = LittleFS.open(path);
  if (!f) return;
  String json = f.readString();
  f.close();
  String name = _jStr(json, "name", "");
  // Fallback: derive name from filename (strip path prefix and .json suffix)
  if (name.length() == 0) {
    const char* slash = strrchr(path, '/');
    const char* fname = slash ? slash + 1 : path;
    char nameBuf[CA_NAME_LEN];
    strncpy(nameBuf, fname, CA_NAME_LEN - 1);
    nameBuf[CA_NAME_LEN - 1] = '\0';
    char* dot = strrchr(nameBuf, '.');
    if (dot) *dot = '\0';
    name = String(nameBuf);
  }
  if (name.length() == 0 || name.length() >= CA_NAME_LEN) return;
  customAppSetFromJson(name.c_str(), json);
}

// ── Init ──────────────────────────────────────────────────────────────────────

void initCustomApps() {
  if (!fsAvailable) return;
  if (!LittleFS.exists("/customapps")) LittleFS.mkdir("/customapps");
  File dir = LittleFS.open("/customapps");
  if (!dir || !dir.isDirectory()) return;
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      const char* nm = f.name();
      int nmlen = strlen(nm);
      if (nmlen > 5 && strcasecmp(nm + nmlen - 5, ".json") == 0) {
        char fullpath[64];
        snprintf(fullpath, sizeof(fullpath), "/customapps/%s", nm);
        f.close();
        _loadFromFile(fullpath);
      } else {
        f.close();
      }
    } else {
      f.close();
    }
    f = dir.openNextFile();
  }
  dir.close();
  LOG("[CA] Loaded %d custom app(s)\n", customAppCount);
}

// ── Public API ────────────────────────────────────────────────────────────────

void customAppSetFromJson(const char* name, const String& json) {
  // Find existing slot by name, or first free slot
  int slot = -1;
  for (int i = 0; i < CUSTOM_APP_SLOTS; i++)
    if (customApps[i].enabled && strcmp(customApps[i].name, name) == 0) { slot = i; break; }
  if (slot < 0)
    for (int i = 0; i < CUSTOM_APP_SLOTS; i++)
      if (!customApps[i].enabled) { slot = i; break; }
  if (slot < 0) { LOG("[CA] No free slot for '%s'\n", name); return; }

  CustomApp& a    = customApps[slot];
  bool wasEnabled = a.enabled;

  strncpy(a.name, name, CA_NAME_LEN - 1);  a.name[CA_NAME_LEN - 1] = '\0';

  String text = _jStr(json, "text", wasEnabled ? a.text : "");
  strncpy(a.text, text.c_str(), CA_TEXT_LEN - 1);  a.text[CA_TEXT_LEN - 1] = '\0';
  _subscribePlaceholders(a.text);  // subscribe to all {{topic}} found in text

  String icon = _jStr(json, "icon", wasEnabled ? a.icon : "");
  strncpy(a.icon, icon.c_str(), CA_ICON_LEN - 1);  a.icon[CA_ICON_LEN - 1] = '\0';
  _resolveIconPath(a.icon, a.iconResolved, CA_ICON_LEN);  // cache resolved path

  a.color       = _jColor(json, "color",       wasEnabled ? a.color       : CRGB::White);
  a.duration    = (uint16_t) _jInt(json,  "duration",    wasEnabled ? a.duration    : 0);
  a.center      = _jBool(json, "center",       wasEnabled ? a.center      : false);
  a.scrollSpeed = (uint8_t)  _jInt(json,  "scrollSpeed", wasEnabled ? (int)a.scrollSpeed : 0);
  a.pushIcon    = (uint8_t)  _jInt(json,  "pushIcon",    wasEnabled ? (int)a.pushIcon    : 0);
  a.repeat      = _jBool(json, "repeat",       wasEnabled ? a.repeat      : true);
  a.show        = _jBool(json, "show",         wasEnabled ? a.show        : true);

  int lifetime  = _jInt(json, "lifetime", 0);  // seconds; 0 = never
  a.expiresAt   = (lifetime > 0) ? millis() + (unsigned long)lifetime * 1000UL : 0;

  a.enabled     = true;
  a.scrollFirst = true;
  a.scrollX     = MATRIX_WIDTH;
  a.scrollPass  = 0;
  a.scrollLast  = 0;

  if (!wasEnabled) customAppCount++;

  // Refresh state if this slot is currently active
  if (_activeSlot == slot) { _nextStaticDraw = 0; _scrollNextGifMs = 0; }

  _saveToFile(slot);
  LOG("[CA] Set '%s' text='%s' dur=%us center=%d\n", name, a.text, a.duration, a.center);
}

void customAppDelete(const char* name) {
  for (int i = 0; i < CUSTOM_APP_SLOTS; i++) {
    if (!customApps[i].enabled || strcmp(customApps[i].name, name) != 0) continue;
    if (_activeSlot == i) _activeSlot = -1;
    if (fsAvailable) {
      char path[56];
      snprintf(path, sizeof(path), "/customapps/%s.json", name);
      LittleFS.remove(path);
    }
    customApps[i].enabled = false;
    customApps[i].name[0] = '\0';
    customApps[i].text[0] = '\0';
    if (customAppCount > 0) customAppCount--;
    LOG("[CA] Deleted '%s'\n", name);
    return;
  }
}

void customAppToggleShow(const char* name) {
  for (int i = 0; i < CUSTOM_APP_SLOTS; i++) {
    if (!customApps[i].enabled || strcmp(customApps[i].name, name) != 0) continue;
    customApps[i].show = !customApps[i].show;
    // If hiding the currently active app, stop it
    if (!customApps[i].show && _activeSlot == i) _activeSlot = -1;
    _saveToFile(i);
    LOG("[CA] '%s' show=%d\n", name, customApps[i].show);
    return;
  }
}

void customAppListJson(char* buf, int len) {
  int n = 0;
  n += snprintf(buf + n, len - n, "[");
  bool first = true;
  for (int i = 0; i < CUSTOM_APP_SLOTS; i++) {
    if (!customApps[i].enabled) continue;
    if (!first) n += snprintf(buf + n, len - n, ",");
    first = false;
    char colStr[8];
    snprintf(colStr, sizeof(colStr), "#%02X%02X%02X",
      customApps[i].color.r, customApps[i].color.g, customApps[i].color.b);
    unsigned long rem = (customApps[i].expiresAt > 0 && millis() < customApps[i].expiresAt)
                        ? (customApps[i].expiresAt - millis()) / 1000 : 0;
    n += snprintf(buf + n, len - n,
      "{\"name\":\"%s\",\"text\":\"%s\",\"color\":\"%s\",\"icon\":\"%s\","
      "\"duration\":%u,\"center\":%s,\"repeat\":%s,\"show\":%s,\"lifetime_rem\":%lu}",
      customApps[i].name, customApps[i].text, colStr, customApps[i].icon,
      customApps[i].duration,
      customApps[i].center ? "true" : "false",
      customApps[i].repeat ? "true" : "false",
      customApps[i].show ? "true" : "false",
      rem);
  }
  snprintf(buf + n, len - n, "]");
}

bool customAppIsActive() { return _activeSlot >= 0; }

void customAppSetPhaseRemaining(int n) { _phaseRemaining = n; }

const char* customAppScreenName() {
  if (_activeSlot < 0) return "app";
  snprintf(_screenNameBuf, sizeof(_screenNameBuf), "app:%s", customApps[_activeSlot].name);
  return _screenNameBuf;
}

void customAppAdvance() {
  if (customAppCount == 0) return;
  for (int guard = 0; guard < CUSTOM_APP_SLOTS; guard++) {
    int idx = (_rr + guard) % CUSTOM_APP_SLOTS;
    if (!customApps[idx].enabled) continue;
    if (!customApps[idx].show)    continue;
    if (customApps[idx].text[0] == '\0') continue;
    // Auto-expire by lifetime
    if (customApps[idx].expiresAt > 0 && millis() >= customApps[idx].expiresAt) {
      const char* nm = customApps[idx].name;  // stash before delete clears it
      char nameBuf[CA_NAME_LEN]; strncpy(nameBuf, nm, CA_NAME_LEN - 1); nameBuf[CA_NAME_LEN-1]='\0';
      customAppDelete(nameBuf);
      continue;
    }
    // Activate slot — re-resolve icon in case LittleFS wasn't ready at load time
    _resolveIconPath(customApps[idx].icon, customApps[idx].iconResolved, CA_ICON_LEN);
    _activeSlot  = idx;
    _rr          = (idx + 1) % CUSTOM_APP_SLOTS;
    customApps[idx].scrollX     = MATRIX_WIDTH;
    customApps[idx].scrollPass  = 0;
    customApps[idx].scrollFirst = true;
    customApps[idx].scrollLast  = 0;
    _nextStaticDraw  = 0;
    _scrollNextGifMs = 0;
    _scrollHasIcon   = false;
    uint16_t dur = customApps[idx].duration > 0 ? customApps[idx].duration : autoRotateIntervalSec;
    _slotUntil = millis() + (unsigned long)dur * 1000UL;
    LOG("[CA] Show '%s' for %us\n", customApps[idx].name, dur);
    return;
  }
}

// ── Display rendering ─────────────────────────────────────────────────────────

bool loopCustomApp() {
  if (_activeSlot < 0) return false;
  CustomApp& app = customApps[_activeSlot];

  // Lifetime expiry
  if (app.expiresAt > 0 && millis() >= app.expiresAt) {
    char nameBuf[CA_NAME_LEN]; strncpy(nameBuf, app.name, CA_NAME_LEN - 1); nameBuf[CA_NAME_LEN-1]='\0';
    customAppDelete(nameBuf);
    _activeSlot = -1;
    if (_phaseRemaining > 0) { _phaseRemaining--; customAppAdvance(); return _activeSlot >= 0; }
    transitionBegin();  // last app → back to built-in
    return false;
  }
  // Duration expiry — if more apps are queued in this phase, advance immediately (no flash)
  if (_slotUntil > 0 && millis() >= _slotUntil) {
    _activeSlot = -1;
    if (_phaseRemaining > 0) { _phaseRemaining--; transitionBegin(); customAppAdvance(); return _activeSlot >= 0; }
    transitionBegin();  // last app → back to built-in
    return false;
  }

  // Resolve {{topic}} placeholders at render time (no IO — uses MQTT cache)
  char resolvedText[CA_TEXT_LEN];
  _resolvePlaceholders(app.text, resolvedText, sizeof(resolvedText));
  // Icon path was resolved once in customAppAdvance() / customAppSetFromJson()
  const char* resolvedIcon = app.iconResolved;

  const int yo      = (MATRIX_HEIGHT - 5) / 2;
  int       textLen = strlen(resolvedText);

  // Auto-static: if text fits in the available space next to the icon, don't scroll
  int availW   = resolvedIcon[0] ? (MATRIX_WIDTH - POCSAG_ICON_RESERVED_PX) : MATRIX_WIDTH;
  int textW    = textLen > 0 ? textLen * 4 - 1 : 0;
  bool doStatic = app.center || textLen == 0 || textW <= availW;

  if (doStatic) {
    // ── Static / centered ────────────────────────────────────────────────────
    if (millis() < _nextStaticDraw) return true;
    FastLED.clear();
    int gifDelay = 500;
    int textX    = 0;
    if (resolvedIcon[0]) {
      int tx = drawIcon(resolvedIcon, &gifDelay, 0);
      if (tx != ICON_DRAW_FAILED) textX = tx; else gifDelay = 500;
    }
    _staticGifDelay = max(gifDelay, 50);
    if (textLen > 0) {
      int tW  = textLen * 4 - 1;
      int aW  = MATRIX_WIDTH - textX;
      int xo  = textX + max(0, (aW - tW) / 2);
      for (int i = 0; i < textLen; i++)
        drawChar(xo + i * 4, yo, resolvedText[i], app.color);
    }
    drawIndicators();
    FastLED.show();
    _nextStaticDraw = millis() + (unsigned long)_staticGifDelay;

  } else {
    // ── Scrolling (text too wide to fit statically) ───────────────────────────
    uint8_t sp = app.scrollSpeed ? app.scrollSpeed : POCSAG_SCROLL_SPEED_MS;
    if (millis() - app.scrollLast < sp) return true;
    app.scrollLast = millis();

    if (app.scrollFirst) {
      app.scrollX      = MATRIX_WIDTH;
      app.scrollFirst  = false;
      _scrollNextGifMs = 0;
      _scrollHasIcon   = false;
    }

    // Advance icon at GIF frame rate
    if (millis() >= _scrollNextGifMs) {
      int gifDelay = 100;
      if (resolvedIcon[0]) {
        int tx = drawIcon(resolvedIcon, &gifDelay, 0);
        _scrollHasIcon = (tx != ICON_DRAW_FAILED);
      } else {
        _scrollHasIcon = false;
      }
      _scrollNextGifMs = millis() + max(gifDelay, 33);
    }

    // Clear text area (preserve icon area)
    int clearFrom = _scrollHasIcon ? POCSAG_ICON_RESERVED_PX - 1 : 0;
    for (int x = clearFrom; x < MATRIX_WIDTH; x++)
      for (int y = 0; y < MATRIX_HEIGHT; y++)
        setLED(x, y, CRGB::Black);

    // Draw text at current scroll position
    int clipFrom = _scrollHasIcon ? POCSAG_ICON_RESERVED_PX : 0;
    for (int i = 0; i < textLen; i++) {
      int cx = app.scrollX + i * 4;
      if (cx >= clipFrom) drawChar(cx, yo, resolvedText[i], app.color);
    }

    drawIndicators();
    FastLED.show();

    app.scrollX--;
    if (app.scrollX < -(textLen * 4)) {
      app.scrollPass++;
      if (!app.repeat) {
        // One pass completed — expire this slot
        _activeSlot = -1;
        if (_phaseRemaining > 0) { _phaseRemaining--; transitionBegin(); customAppAdvance(); return _activeSlot >= 0; }
        transitionBegin();  // last app → back to built-in
        return false;
      }
      app.scrollX = MATRIX_WIDTH;
    }
  }
  return true;
}


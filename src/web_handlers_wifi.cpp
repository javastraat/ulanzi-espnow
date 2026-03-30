#include <Arduino.h>
// web_handlers_wifi.cpp — WiFi multi-slot config and SoftAP API handlers.
#include "web_handlers_wifi.h"
#include "globals.h"
#include "nvs_settings.h"
#include <WiFi.h>

// Simple JSON string escaping (handles " and \)
static String jsonStr(const String& s) {
  String out;
  out.reserve(s.length() + 4);
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if      (c == '"')  out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else                out += c;
  }
  return out;
}

void registerWifiHandlers() {

  // GET /api/wifi-slot?slot=N  — return label/ssid/password for one slot
  webServer.on("/api/wifi-slot", HTTP_GET, []() {
    int slot = webServer.arg("slot").toInt();
    if (slot < 0 || slot >= WIFI_SLOT_COUNT) {
      webServer.send(400, "text/plain", "Bad slot");
      return;
    }
    String json = "{\"label\":\"" + jsonStr(wifiSlotLabel[slot]) +
                  "\",\"ssid\":\""     + jsonStr(wifiSlotSsid[slot]) +
                  "\",\"password\":\"" + jsonStr(wifiSlotPass[slot]) + "\"}";
    webServer.send(200, "application/json", json);
  });

  // GET /api/wifi-slots  — return label+ssid summary for all slots (no passwords)
  webServer.on("/api/wifi-slots", HTTP_GET, []() {
    String json = "{\"slots\":[";
    for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
      if (i > 0) json += ",";
      json += "{\"label\":\"" + jsonStr(wifiSlotLabel[i]) +
              "\",\"ssid\":\""  + jsonStr(wifiSlotSsid[i]) + "\"}";
    }
    json += "]}";
    webServer.send(200, "application/json", json);
  });

  // POST /api/save-wifi-slot  — save label/ssid/password for one slot
  webServer.on("/api/save-wifi-slot", HTTP_POST, []() {
    int    slot  = webServer.arg("slot").toInt();
    String label = webServer.arg("label");
    String ssid  = webServer.arg("ssid");
    String pass  = webServer.arg("password");
    label.trim(); ssid.trim(); pass.trim();
    if (slot < 0 || slot >= WIFI_SLOT_COUNT) {
      webServer.send(400, "text/plain", "Bad slot");
      return;
    }
    saveWifiSlot(slot, label, ssid, pass);
    LOG("[WiFi] Slot %d saved: %s\n", slot + 1, ssid.c_str());
    webServer.send(200, "text/plain", "Slot " + String(slot + 1) + " saved.");
  });

  // POST /api/reset-wifi-slot  — reset one slot to config.h defaults
  webServer.on("/api/reset-wifi-slot", HTTP_POST, []() {
    int slot = webServer.arg("slot").toInt();
    if (slot < 0 || slot >= WIFI_SLOT_COUNT) {
      webServer.send(400, "text/plain", "Bad slot");
      return;
    }
    resetWifiSlot(slot);
    String json = "{\"label\":\"" + jsonStr(wifiSlotLabel[slot]) +
                  "\",\"ssid\":\""     + jsonStr(wifiSlotSsid[slot]) +
                  "\",\"password\":\"" + jsonStr(wifiSlotPass[slot]) + "\"}";
    webServer.send(200, "application/json", json);
  });

  // POST /api/save-wifi-ap  — save AP SSID/password/channel/retries
  webServer.on("/api/save-wifi-ap", HTTP_POST, []() {
    String ssid = webServer.arg("ssid");
    String pass = webServer.arg("password");
    int    ch   = webServer.arg("channel").toInt();
    int    ret  = webServer.arg("retries").toInt();
    ssid.trim(); pass.trim();
    if (ssid.length() < 3) { webServer.send(400, "text/plain", "SSID too short"); return; }
    if (pass.length() < 8) { webServer.send(400, "text/plain", "Password too short (min 8)"); return; }
    if (ch  < 1 || ch  > 13) ch  = 1;
    if (ret < 1 || ret > 40) ret = 10;
    wifiApSsid     = ssid;
    wifiApPassword = pass;
    wifiApChannel  = (uint8_t)ch;
    wifiMaxRetries = (uint8_t)ret;
    saveWifiApSettings();
    LOG("[WiFi] AP settings saved: %s ch%d ret%d\n", ssid.c_str(), ch, ret);
    webServer.send(200, "text/plain", "AP settings saved.");
  });

  // POST /api/reset-wifi-ap  — reset AP settings to defaults
  webServer.on("/api/reset-wifi-ap", HTTP_POST, []() {
    resetWifiApSettings();
    LOG("[WiFi] AP settings reset to defaults\n");
    webServer.send(200, "text/plain", "AP settings reset to defaults.");
  });

  // GET /api/wifiscan  — scan nearby networks (blocks ~3 s)
  webServer.on("/api/wifiscan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    String json = "{\"networks\":[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      String enc = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "WPA";
      json += "{\"ssid\":\""  + jsonStr(WiFi.SSID(i)) +
              "\",\"rssi\":"  + String(WiFi.RSSI(i)) +
              ",\"encryption\":\"" + enc + "\"}";
    }
    json += "]}";
    WiFi.scanDelete();
    webServer.send(200, "application/json", json);
  });
}

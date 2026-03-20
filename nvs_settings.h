#pragma once
#include <Arduino.h>

void loadSettings();
void saveSettings();

// WiFi multi-slot + AP settings (separate NVS namespace "wifi")
void loadWifiSlots();
void saveWifiSlot(int slot, const String& label, const String& ssid, const String& pass);
void resetWifiSlot(int slot);
void saveWifiApSettings();
void resetWifiApSettings();

// ArduinoOTA settings (separate save so callers don't have to save everything)
void saveOtaSettings();
void resetOtaSettings();

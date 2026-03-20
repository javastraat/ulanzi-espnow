// nvs_settings.cpp — NVS Preferences: load and save all user-configurable settings.
#include "nvs_settings.h"
#include "globals.h"
#include <Preferences.h>

// ── WiFi slots + AP (namespace "wifi") ───────────────────────────────────────

void loadWifiSlots() {
  Preferences p;
  p.begin("wifi", true);
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
    char lk[4], sk[4], pk[4];
    snprintf(lk, sizeof(lk), "l%d", i);
    snprintf(sk, sizeof(sk), "s%d", i);
    snprintf(pk, sizeof(pk), "p%d", i);
    // Slot 0 defaults to config.h credentials; others default empty
    wifiSlotLabel[i] = p.getString(lk, i == 0 ? "Home" : "");
    wifiSlotSsid[i]  = p.getString(sk, i == 0 ? WIFI_SSID : "");
    wifiSlotPass[i]  = p.getString(pk, i == 0 ? WIFI_PASSWORD : "");
  }
  wifiApSsid     = p.getString("ap_ssid", "Ulanzi-AP");
  wifiApPassword = p.getString("ap_pass",  "ulanzi1234");
  wifiApChannel  = p.getUChar ("ap_ch",    1);
  wifiMaxRetries = p.getUChar ("retries",  10);
  p.end();
}

void saveWifiSlot(int slot, const String& label, const String& ssid, const String& pass) {
  if (slot < 0 || slot >= WIFI_SLOT_COUNT) return;
  wifiSlotLabel[slot] = label;
  wifiSlotSsid[slot]  = ssid;
  wifiSlotPass[slot]  = pass;
  Preferences p;
  p.begin("wifi", false);
  char lk[4], sk[4], pk[4];
  snprintf(lk, sizeof(lk), "l%d", slot);
  snprintf(sk, sizeof(sk), "s%d", slot);
  snprintf(pk, sizeof(pk), "p%d", slot);
  p.putString(lk, label);
  p.putString(sk, ssid);
  p.putString(pk, pass);
  p.end();
}

void resetWifiSlot(int slot) {
  String defLabel = (slot == 0) ? "Home" : "";
  String defSsid  = (slot == 0) ? WIFI_SSID : "";
  String defPass  = (slot == 0) ? WIFI_PASSWORD : "";
  saveWifiSlot(slot, defLabel, defSsid, defPass);
}

void saveWifiApSettings() {
  Preferences p;
  p.begin("wifi", false);
  p.putString("ap_ssid", wifiApSsid);
  p.putString("ap_pass",  wifiApPassword);
  p.putUChar ("ap_ch",    wifiApChannel);
  p.putUChar ("retries",  wifiMaxRetries);
  p.end();
}

void resetWifiApSettings() {
  wifiApSsid     = "Ulanzi-AP";
  wifiApPassword = "ulanzi1234";
  wifiApChannel  = 1;
  wifiMaxRetries = 10;
  saveWifiApSettings();
}

void saveOtaSettings() {
  Preferences p;
  p.begin("ulanzi", false);
  p.putBool  ("ota_en",   otaEnabled);
  p.putInt   ("ota_port", otaPort);
  p.putString("ota_pass", otaPassword);
  p.end();
}

void resetOtaSettings() {
  otaEnabled = true;
  otaPort    = OTA_PORT;
  strncpy(otaPassword, OTA_PASSWORD, 63);
  otaPassword[63] = '\0';
  saveOtaSettings();
}

static inline uint32_t packCRGB(CRGB c)       { return ((uint32_t)c.r<<16)|((uint32_t)c.g<<8)|c.b; }
static inline CRGB     unpackCRGB(uint32_t v) { return CRGB((v>>16)&0xFF,(v>>8)&0xFF,v&0xFF); }

void loadSettings() {
  Preferences p;
  p.begin("ulanzi", true);  // read-only
  // Brightness
  autoBrightnessEnabled = p.getBool  ("auto_br",    true);
  currentBrightness     = p.getUChar ("brightness", LED_BRIGHTNESS);
  brightnessMode        = (BrightnessMode)p.getUChar("br_mode", BMODE_AUTO);
  // Buzzer
  buzzerBootEnabled     = p.getBool ("buz_boot_en",  true);
  buzzerBootVolume      = p.getUChar("buz_boot_vol", BUZZER_VOL_BOOT);
  buzzerPocsagEnabled   = p.getBool ("buz_poc_en",   true);
  buzzerPocsagVolume    = p.getUChar("buz_poc_vol",  BUZZER_VOL_POCSAG);
  buzzerClickEnabled    = p.getBool ("buz_clk_en",   true);
  buzzerClickVolume     = p.getUChar("buz_clk_vol",  BUZZER_VOL_CLICK);
  // Display rotation
  autoRotateEnabled     = p.getBool ("rot_en",  false);
  autoRotateIntervalSec = p.getUChar("rot_sec", 5);
  // Device name + mDNS hostname
  // ArduinoOTA runtime settings
  otaEnabled = p.getBool("ota_en", true);
  otaPort    = p.getInt ("ota_port", OTA_PORT);
  { String s = p.getString("ota_pass", OTA_PASSWORD); strncpy(otaPassword, s.c_str(), 63); otaPassword[63] = '\0'; }
  // Debug logging
  debugLogEnabled = p.getBool("debug_log", false);
  // MQTT
  mqttEnabled   = p.getBool  ("mqtt_en",     false);
  mqttPort      = p.getUShort("mqtt_port",   1883);
  mqttDiscovery = p.getBool  ("mqtt_disc",   true);
  { String s = p.getString("mqtt_broker", ""); s.trim(); strncpy(mqttBroker, s.c_str(), 63); mqttBroker[63] = '\0'; }
  { String s = p.getString("mqtt_user",   ""); s.trim(); strncpy(mqttUser,   s.c_str(), 31); mqttUser[31]   = '\0'; }
  { String s = p.getString("mqtt_pass",   ""); s.trim(); strncpy(mqttPass,   s.c_str(), 63); mqttPass[63]   = '\0'; }
  { String s = p.getString("mqtt_prefix", "homeassistant"); s.trim(); strncpy(mqttPrefix, s.c_str(), 31); mqttPrefix[31] = '\0'; }
  { String s = p.getString("mqtt_node",   "ulanzi");        s.trim(); strncpy(mqttNodeId, s.c_str(), 31); mqttNodeId[31] = '\0'; }
  { String s = p.getString("mqtt_ha_name", ""); s.trim(); strncpy(mqttHaName, s.c_str(), 31); mqttHaName[31] = '\0'; }
  { String s = p.getString("boot_name", "ULANZI"); s.trim(); s.toUpperCase(); strncpy(bootName, s.c_str(), 8); bootName[8] = '\0'; }
  { String s = p.getString("mdns_name", MDNS_HOSTNAME); s.trim(); s.toLowerCase(); strncpy(mdnsName, s.c_str(), 31); mdnsName[31] = '\0'; }
  // Indicators
  indicatorsEnabled = p.getBool("ind_en", true);
  // Protocol mode
  recvPocsagEnabled = p.getBool("recv_pocsag", true);
  // POCSAG RIC settings
  timePocRic  = p.getUInt("ric_time",  TIME_POCSAG_RIC);
  callsignRic = p.getUInt("ric_call",  CALLSIGN_RIC);
  {
    String s = p.getString("ric_excl", EXCLUDED_RICS_DEFAULT);
    excludedRicsCount = 0;
    int start = 0;
    for (int i = 0; i <= (int)s.length() && excludedRicsCount < EXCLUDED_RICS_MAX; i++) {
      if (i == (int)s.length() || s[i] == ',') {
        String tok = s.substring(start, i); tok.trim();
        if (tok.length() > 0) excludedRics[excludedRicsCount++] = (uint32_t)tok.toInt();
        start = i + 1;
      }
    }
  }
  // Icon filenames — trim whitespace to fix any accidentally saved leading/trailing spaces
  { String s = p.getString("icon_temp", "");  s.trim(); strncpy(iconTempFile,   s.c_str(), 31); iconTempFile[31]   = '\0'; }
  { String s = p.getString("icon_hum",  "");  s.trim(); strncpy(iconHumFile,    s.c_str(), 31); iconHumFile[31]    = '\0'; }
  { String s = p.getString("icon_bat",  "");  s.trim(); strncpy(iconBatFile,    s.c_str(), 31); iconBatFile[31]    = '\0'; }
  { String s = p.getString("icon_poc",  "");  s.trim(); strncpy(iconPocsagFile, s.c_str(), 31); iconPocsagFile[31] = '\0'; }
  // Screensaver
  screensaverEnabled    = p.getBool   ("scr_en",      false);
  screensaverTimeoutSec = p.getUShort ("scr_timeout", 60);
  screensaverBrightness = (int16_t)p.getInt("scr_bright", 50);
  { String s = p.getString("scr_file", ""); s.trim(); strncpy(screensaverFile, s.c_str(), 63); screensaverFile[63] = '\0'; }
  // Display colors
  colorClock   = unpackCRGB(p.getUInt("clk_col",     packCRGB(CRGB(255,255,255))));
  colorPocsag  = unpackCRGB(p.getUInt("poc_col",     packCRGB(CRGB(255,160,  0))));
  tempThreshLo = p.getFloat ("tmp_thr_lo", 16.0f);
  tempThreshHi = p.getFloat ("tmp_thr_hi", 20.0f);
  colorTempLo  = unpackCRGB(p.getUInt("tmp_col_lo",  packCRGB(CRGB(  0,160,255))));
  colorTempMid = unpackCRGB(p.getUInt("tmp_col_mid", packCRGB(CRGB(  0,200, 50))));
  colorTempHi  = unpackCRGB(p.getUInt("tmp_col_hi",  packCRGB(CRGB(255, 80,  0))));
  humThreshLo  = p.getFloat ("hum_thr_lo", 30.0f);
  humThreshHi  = p.getFloat ("hum_thr_hi", 50.0f);
  colorHumLo   = unpackCRGB(p.getUInt("hum_col_lo",  packCRGB(CRGB(255,160,  0))));
  colorHumMid  = unpackCRGB(p.getUInt("hum_col_mid", packCRGB(CRGB(  0,200, 50))));
  colorHumHi   = unpackCRGB(p.getUInt("hum_col_hi",  packCRGB(CRGB(  0,160,255))));
  batThreshLo  = p.getUChar("bat_thr_lo", 30);
  batThreshHi  = p.getUChar("bat_thr_hi", 60);
  colorBatLo   = unpackCRGB(p.getUInt("bat_col_lo",  packCRGB(CRGB(220, 40,  0))));
  colorBatMid  = unpackCRGB(p.getUInt("bat_col_mid", packCRGB(CRGB(220,180,  0))));
  colorBatHi   = unpackCRGB(p.getUInt("bat_col_hi",  packCRGB(CRGB(  0,200, 50))));
  p.end();
}

void saveSettings() {
  Preferences p;
  p.begin("ulanzi", false);  // read-write
  // Brightness
  p.putBool  ("auto_br",    autoBrightnessEnabled);
  p.putUChar ("brightness", currentBrightness);
  p.putUChar ("br_mode",    (uint8_t)brightnessMode);
  // Buzzer
  p.putBool ("buz_boot_en",  buzzerBootEnabled);
  p.putUChar("buz_boot_vol", buzzerBootVolume);
  p.putBool ("buz_poc_en",   buzzerPocsagEnabled);
  p.putUChar("buz_poc_vol",  buzzerPocsagVolume);
  p.putBool ("buz_clk_en",   buzzerClickEnabled);
  p.putUChar("buz_clk_vol",  buzzerClickVolume);
  // Display rotation
  p.putBool ("rot_en",  autoRotateEnabled);
  p.putUChar("rot_sec", autoRotateIntervalSec);
  // ArduinoOTA runtime settings
  p.putBool  ("ota_en",   otaEnabled);
  p.putInt   ("ota_port", otaPort);
  p.putString("ota_pass", otaPassword);
  // Debug logging
  p.putBool("debug_log", debugLogEnabled);
  // MQTT
  p.putBool  ("mqtt_en",     mqttEnabled);
  p.putUShort("mqtt_port",   mqttPort);
  p.putBool  ("mqtt_disc",   mqttDiscovery);
  p.putString("mqtt_broker", mqttBroker);
  p.putString("mqtt_user",   mqttUser);
  p.putString("mqtt_pass",   mqttPass);
  p.putString("mqtt_prefix", mqttPrefix);
  p.putString("mqtt_node",   mqttNodeId);
  p.putString("mqtt_ha_name", mqttHaName);
  // Device name + mDNS hostname
  p.putString("boot_name", bootName);
  p.putString("mdns_name", mdnsName);
  // Indicators
  p.putBool("ind_en", indicatorsEnabled);
  // Protocol mode
  p.putBool("recv_pocsag", recvPocsagEnabled);
  // POCSAG RIC settings
  p.putUInt("ric_time",  timePocRic);
  p.putUInt("ric_call",  callsignRic);
  {
    String s = "";
    for (int i = 0; i < excludedRicsCount; i++) { if (i) s += ","; s += String(excludedRics[i]); }
    p.putString("ric_excl", s);
  }
  // Icon filenames
  p.putString("icon_temp", iconTempFile);
  p.putString("icon_hum",  iconHumFile);
  p.putString("icon_bat",  iconBatFile);
  p.putString("icon_poc",  iconPocsagFile);
  // Screensaver
  p.putBool   ("scr_en",      screensaverEnabled);
  p.putUShort ("scr_timeout", screensaverTimeoutSec);
  p.putInt    ("scr_bright",  screensaverBrightness);
  p.putString ("scr_file",    screensaverFile);
  // Display colors
  p.putUInt("clk_col",     packCRGB(colorClock));
  p.putUInt("poc_col",     packCRGB(colorPocsag));
  p.putFloat("tmp_thr_lo", tempThreshLo);
  p.putFloat("tmp_thr_hi", tempThreshHi);
  p.putUInt("tmp_col_lo",  packCRGB(colorTempLo));
  p.putUInt("tmp_col_mid", packCRGB(colorTempMid));
  p.putUInt("tmp_col_hi",  packCRGB(colorTempHi));
  p.putFloat("hum_thr_lo", humThreshLo);
  p.putFloat("hum_thr_hi", humThreshHi);
  p.putUInt("hum_col_lo",  packCRGB(colorHumLo));
  p.putUInt("hum_col_mid", packCRGB(colorHumMid));
  p.putUInt("hum_col_hi",  packCRGB(colorHumHi));
  p.putUChar("bat_thr_lo", batThreshLo);
  p.putUChar("bat_thr_hi", batThreshHi);
  p.putUInt("bat_col_lo",  packCRGB(colorBatLo));
  p.putUInt("bat_col_mid", packCRGB(colorBatMid));
  p.putUInt("bat_col_hi",  packCRGB(colorBatHi));
  p.end();
}

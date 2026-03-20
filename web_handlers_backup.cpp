// web_handlers_backup.cpp — Config export/import and LittleFS snapshot management.
#include "web_handlers_backup.h"
#include "globals.h"
#include "nvs_settings.h"
#include <LittleFS.h>
#include "web/backup.h"

// ── Helpers ──────────────────────────────────────────────────────────────────

static String colorToHex(CRGB c) {
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
  return String(buf);
}

static CRGB hexToColor(const String& s) {
  if (s.length() == 7 && s[0] == '#') {
    long n = strtol(s.c_str() + 1, nullptr, 16);
    return CRGB((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
  }
  return CRGB(0, 0, 0);
}

// ── Export ────────────────────────────────────────────────────────────────────

static String buildExportText() {
  String out;
  out.reserve(2200);

  out += "# Ulanzi Config Backup\n";
  out += "# Device: "; out += mdnsName; out += "\n";
  out += "# Firmware: "; out += FIRMWARE_VERSION; out += "\n";
  out += "# (keep this file safe — contains WiFi credentials)\n\n";

  out += "[ulanzi]\n";
  out += "auto_br=";          out += autoBrightnessEnabled ? "true" : "false"; out += "\n";
  out += "brightness=";       out += currentBrightness;                        out += "\n";
  out += "br_mode=";          out += (uint8_t)brightnessMode;                  out += "\n";
  out += "buz_boot_en=";      out += buzzerBootEnabled   ? "true" : "false";   out += "\n";
  out += "buz_boot_vol=";     out += buzzerBootVolume;                         out += "\n";
  out += "buz_poc_en=";       out += buzzerPocsagEnabled ? "true" : "false";   out += "\n";
  out += "buz_poc_vol=";      out += buzzerPocsagVolume;                       out += "\n";
  out += "buz_clk_en=";       out += buzzerClickEnabled  ? "true" : "false";   out += "\n";
  out += "buz_clk_vol=";      out += buzzerClickVolume;                        out += "\n";
  out += "rot_en=";           out += autoRotateEnabled   ? "true" : "false";   out += "\n";
  out += "rot_sec=";          out += autoRotateIntervalSec;                    out += "\n";
  out += "ota_en=";           out += otaEnabled          ? "true" : "false";   out += "\n";
  out += "ota_port=";         out += otaPort;                                  out += "\n";
  out += "debug_log=";        out += debugLogEnabled     ? "true" : "false";   out += "\n";
  out += "mqtt_en=";          out += mqttEnabled         ? "true" : "false";   out += "\n";
  out += "mqtt_port=";        out += mqttPort;                                 out += "\n";
  out += "mqtt_disc=";        out += mqttDiscovery       ? "true" : "false";   out += "\n";
  out += "mqtt_broker=";      out += mqttBroker;                               out += "\n";
  out += "mqtt_user=";        out += mqttUser;                                 out += "\n";
  out += "mqtt_pass=";        out += mqttPass;                                 out += "\n";
  out += "mqtt_prefix=";      out += mqttPrefix;                               out += "\n";
  out += "mqtt_node=";        out += mqttNodeId;                               out += "\n";
  out += "mqtt_ha_name=";     out += mqttHaName;                               out += "\n";
  out += "boot_name=";        out += bootName;                                 out += "\n";
  out += "mdns_name=";        out += mdnsName;                                 out += "\n";
  out += "ind_en=";           out += indicatorsEnabled   ? "true" : "false";   out += "\n";
  out += "recv_pocsag=";      out += recvPocsagEnabled   ? "true" : "false";   out += "\n";
  out += "ric_time=";         out += timePocRic;                               out += "\n";
  out += "ric_call=";         out += callsignRic;                              out += "\n";
  {
    out += "ric_excl=";
    for (int i = 0; i < excludedRicsCount; i++) { if (i) out += ","; out += excludedRics[i]; }
    out += "\n";
  }
  out += "icon_temp=";        out += iconTempFile;   out += "\n";
  out += "icon_hum=";         out += iconHumFile;    out += "\n";
  out += "icon_bat=";         out += iconBatFile;    out += "\n";
  out += "icon_poc=";         out += iconPocsagFile; out += "\n";
  out += "icon_hass=";        out += iconHassFile;   out += "\n";
  out += "icon_web=";         out += iconWebFile;    out += "\n";
  out += "scr_en=";           out += screensaverEnabled ? "true" : "false";    out += "\n";
  out += "scr_timeout=";      out += screensaverTimeoutSec;                    out += "\n";
  out += "scr_bright=";       out += screensaverBrightness;                    out += "\n";
  out += "scr_file=";         out += screensaverFile;                          out += "\n";
  out += "clk_col=";          out += colorToHex(colorClock);                   out += "\n";
  out += "poc_col=";          out += colorToHex(colorPocsag);                  out += "\n";
  out += "tmp_thr_lo=";       out += tempThreshLo;                             out += "\n";
  out += "tmp_thr_hi=";       out += tempThreshHi;                             out += "\n";
  out += "tmp_col_lo=";       out += colorToHex(colorTempLo);                  out += "\n";
  out += "tmp_col_mid=";      out += colorToHex(colorTempMid);                 out += "\n";
  out += "tmp_col_hi=";       out += colorToHex(colorTempHi);                  out += "\n";
  out += "hum_thr_lo=";       out += humThreshLo;                              out += "\n";
  out += "hum_thr_hi=";       out += humThreshHi;                              out += "\n";
  out += "hum_col_lo=";       out += colorToHex(colorHumLo);                   out += "\n";
  out += "hum_col_mid=";      out += colorToHex(colorHumMid);                  out += "\n";
  out += "hum_col_hi=";       out += colorToHex(colorHumHi);                   out += "\n";
  out += "bat_thr_lo=";       out += batThreshLo;                              out += "\n";
  out += "bat_thr_hi=";       out += batThreshHi;                              out += "\n";
  out += "bat_col_lo=";       out += colorToHex(colorBatLo);                   out += "\n";
  out += "bat_col_mid=";      out += colorToHex(colorBatMid);                  out += "\n";
  out += "bat_col_hi=";       out += colorToHex(colorBatHi);                   out += "\n";

  out += "\n[wifi]\n";
  out += "ap_ssid=";   out += wifiApSsid;     out += "\n";
  out += "ap_pass=";   out += wifiApPassword; out += "\n";
  out += "ap_ch=";     out += wifiApChannel;  out += "\n";
  out += "retries=";   out += wifiMaxRetries; out += "\n";
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
    char lk[4], sk[4], pk[4];
    snprintf(lk, sizeof(lk), "l%d", i);
    snprintf(sk, sizeof(sk), "s%d", i);
    snprintf(pk, sizeof(pk), "p%d", i);
    out += lk; out += "="; out += wifiSlotLabel[i]; out += "\n";
    out += sk; out += "="; out += wifiSlotSsid[i];  out += "\n";
    out += pk; out += "="; out += wifiSlotPass[i];  out += "\n";
  }

  return out;
}

// ── Import ────────────────────────────────────────────────────────────────────

static void applyImportText(const String& text) {
  // Rebuild wifi slot arrays; apply at the end
  String slotLabel[WIFI_SLOT_COUNT], slotSsid[WIFI_SLOT_COUNT], slotPass[WIFI_SLOT_COUNT];
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
    slotLabel[i] = wifiSlotLabel[i];
    slotSsid[i]  = wifiSlotSsid[i];
    slotPass[i]  = wifiSlotPass[i];
  }

  int pos = 0, len = text.length();
  while (pos < len) {
    int nl = text.indexOf('\n', pos);
    if (nl < 0) nl = len;
    String line = text.substring(pos, nl);
    line.trim();
    pos = nl + 1;

    if (line.startsWith("#") || line.startsWith("[") || line.length() == 0) continue;

    int eq = line.indexOf('=');
    if (eq < 1) continue;
    String key = line.substring(0, eq);   key.trim();
    String val = line.substring(eq + 1);  // don't trim — passwords may have leading space

    // ── ulanzi namespace ──────────────────────────────────────────────
    if      (key == "auto_br")      autoBrightnessEnabled = (val == "true" || val == "1");
    else if (key == "brightness")   currentBrightness     = (uint8_t)constrain(val.toInt(), 0, 255);
    else if (key == "buz_boot_en")  buzzerBootEnabled     = (val == "true" || val == "1");
    else if (key == "buz_boot_vol") buzzerBootVolume      = (uint8_t)constrain(val.toInt(), 0, 255);
    else if (key == "buz_poc_en")   buzzerPocsagEnabled   = (val == "true" || val == "1");
    else if (key == "buz_poc_vol")  buzzerPocsagVolume    = (uint8_t)constrain(val.toInt(), 0, 255);
    else if (key == "buz_clk_en")   buzzerClickEnabled    = (val == "true" || val == "1");
    else if (key == "buz_clk_vol")  buzzerClickVolume     = (uint8_t)constrain(val.toInt(), 0, 255);
    else if (key == "rot_en")       autoRotateEnabled     = (val == "true" || val == "1");
    else if (key == "rot_sec")      autoRotateIntervalSec = (uint8_t)constrain(val.toInt(), 1, 60);
    else if (key == "ota_en")       otaEnabled            = (val == "true" || val == "1");
    else if (key == "ota_port")     otaPort               = constrain(val.toInt(), 1, 65535);
    else if (key == "debug_log")    debugLogEnabled       = (val == "true" || val == "1");
    else if (key == "mqtt_en")      mqttEnabled           = (val == "true" || val == "1");
    else if (key == "mqtt_port")    mqttPort              = (uint16_t)constrain(val.toInt(), 1, 65535);
    else if (key == "mqtt_disc")    mqttDiscovery         = (val == "true" || val == "1");
    else if (key == "mqtt_broker")  { val.trim(); strncpy(mqttBroker, val.c_str(), 63); mqttBroker[63] = '\0'; }
    else if (key == "mqtt_user")    { val.trim(); strncpy(mqttUser,   val.c_str(), 31); mqttUser[31]   = '\0'; }
    else if (key == "mqtt_pass")    { val.trim(); strncpy(mqttPass,   val.c_str(), 63); mqttPass[63]   = '\0'; }
    else if (key == "mqtt_prefix")  { val.trim(); strncpy(mqttPrefix, val.c_str(), 31); mqttPrefix[31] = '\0'; }
    else if (key == "mqtt_node")    { val.trim(); strncpy(mqttNodeId, val.c_str(), 31); mqttNodeId[31] = '\0'; }
    else if (key == "mqtt_ha_name") { val.trim(); strncpy(mqttHaName, val.c_str(), 31); mqttHaName[31] = '\0'; }
    else if (key == "boot_name")    { val.trim(); val.toUpperCase(); strncpy(bootName, val.c_str(), 8); bootName[8] = '\0'; }
    else if (key == "mdns_name")    { val.trim(); val.toLowerCase(); strncpy(mdnsName, val.c_str(), 31); mdnsName[31] = '\0'; }
    else if (key == "ind_en")       indicatorsEnabled = (val == "true" || val == "1");
    else if (key == "recv_pocsag")  recvPocsagEnabled = (val == "true" || val == "1");
    else if (key == "ric_time")     timePocRic  = (uint32_t)val.toInt();
    else if (key == "ric_call")     callsignRic = (uint32_t)val.toInt();
    else if (key == "ric_excl") {
      excludedRicsCount = 0;
      int s2 = 0;
      for (int i = 0; i <= (int)val.length() && excludedRicsCount < EXCLUDED_RICS_MAX; i++) {
        if (i == (int)val.length() || val[i] == ',') {
          String tok = val.substring(s2, i); tok.trim();
          if (tok.length()) excludedRics[excludedRicsCount++] = (uint32_t)tok.toInt();
          s2 = i + 1;
        }
      }
    }
    else if (key == "icon_temp")    { val.trim(); strncpy(iconTempFile,   val.c_str(), 31); iconTempFile[31]   = '\0'; }
    else if (key == "icon_hum")     { val.trim(); strncpy(iconHumFile,    val.c_str(), 31); iconHumFile[31]    = '\0'; }
    else if (key == "icon_bat")     { val.trim(); strncpy(iconBatFile,    val.c_str(), 31); iconBatFile[31]    = '\0'; }
    else if (key == "icon_poc")     { val.trim(); strncpy(iconPocsagFile, val.c_str(), 31); iconPocsagFile[31] = '\0'; }
    else if (key == "icon_hass")    { val.trim(); strncpy(iconHassFile,   val.c_str(), 31); iconHassFile[31]   = '\0'; }
    else if (key == "icon_web")     { val.trim(); strncpy(iconWebFile,    val.c_str(), 31); iconWebFile[31]    = '\0'; }
    else if (key == "scr_en")       screensaverEnabled    = (val == "true" || val == "1");
    else if (key == "scr_timeout")  screensaverTimeoutSec = (uint16_t)constrain(val.toInt(), 1, 3600);
    else if (key == "scr_bright")   screensaverBrightness = (int16_t)constrain(val.toInt(), -2, 255);
    else if (key == "scr_file")     { val.trim(); strncpy(screensaverFile, val.c_str(), 63); screensaverFile[63] = '\0'; }
    else if (key == "clk_col")      colorClock    = hexToColor(val);
    else if (key == "poc_col")      colorPocsag   = hexToColor(val);
    else if (key == "tmp_thr_lo")   tempThreshLo  = val.toFloat();
    else if (key == "tmp_thr_hi")   tempThreshHi  = val.toFloat();
    else if (key == "tmp_col_lo")   colorTempLo   = hexToColor(val);
    else if (key == "tmp_col_mid")  colorTempMid  = hexToColor(val);
    else if (key == "tmp_col_hi")   colorTempHi   = hexToColor(val);
    else if (key == "hum_thr_lo")   humThreshLo   = val.toFloat();
    else if (key == "hum_thr_hi")   humThreshHi   = val.toFloat();
    else if (key == "hum_col_lo")   colorHumLo    = hexToColor(val);
    else if (key == "hum_col_mid")  colorHumMid   = hexToColor(val);
    else if (key == "hum_col_hi")   colorHumHi    = hexToColor(val);
    else if (key == "bat_thr_lo")   batThreshLo   = (uint8_t)constrain(val.toInt(), 0, 100);
    else if (key == "bat_thr_hi")   batThreshHi   = (uint8_t)constrain(val.toInt(), 0, 100);
    else if (key == "bat_col_lo")   colorBatLo    = hexToColor(val);
    else if (key == "bat_col_mid")  colorBatMid   = hexToColor(val);
    else if (key == "bat_col_hi")   colorBatHi    = hexToColor(val);
    // ── wifi namespace ─────────────────────────────────────────────────
    else if (key == "ap_ssid")  { val.trim(); wifiApSsid     = val; }
    else if (key == "ap_pass")  { val.trim(); wifiApPassword = val; }
    else if (key == "ap_ch")    wifiApChannel  = (uint8_t)constrain(val.toInt(), 1, 13);
    else if (key == "retries")  wifiMaxRetries = (uint8_t)constrain(val.toInt(), 1, 30);
    else {
      // WiFi slots: l0..l5, s0..s5, p0..p5
      if (key.length() == 2 && key[1] >= '0' && key[1] < '0' + WIFI_SLOT_COUNT) {
        int idx = key[1] - '0';
        if      (key[0] == 'l') slotLabel[idx] = val;
        else if (key[0] == 's') slotSsid[idx]  = val;
        else if (key[0] == 'p') slotPass[idx]  = val;
      }
    }
  }

  // Persist everything
  saveSettings();
  saveWifiApSettings();
  for (int i = 0; i < WIFI_SLOT_COUNT; i++)
    saveWifiSlot(i, slotLabel[i], slotSsid[i], slotPass[i]);
}

// ── Snapshot helpers ──────────────────────────────────────────────────────────

static String sanitizeSnapName(const String& name) {
  String out;
  for (int i = 0; i < (int)name.length() && out.length() < 32; i++) {
    char c = name[i];
    if (isAlphaNumeric(c) || c == '-' || c == '_') out += c;
    else if (c == ' ') out += '-';
  }
  return out;
}

static String snapPath(const String& name) {
  return "/backups/" + name + ".txt";
}

// ── Handler registration ──────────────────────────────────────────────────────

void registerBackupHandlers() {

  webServer.on("/backup", HTTP_GET, []() {
    webServer.send_P(200, "text/html", PAGE_BACKUP);
  });

  // ── Export config ─────────────────────────────────────────────────────────
  webServer.on("/api/backup/export", HTTP_GET, []() {
    String text = buildExportText();
    String filename = String("ulanzi-") + mdnsName + ".txt";
    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    webServer.send(200, "text/plain", text);
  });

  // ── Import config ─────────────────────────────────────────────────────────
  webServer.on("/api/backup/import", HTTP_POST, []() {
    if (!webServer.hasArg("plain") || webServer.arg("plain").length() == 0) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"empty body\"}");
      return;
    }
    applyImportText(webServer.arg("plain"));
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    ESP.restart();
  });

  // ── List snapshots ────────────────────────────────────────────────────────
  webServer.on("/api/backup/snapshots", HTTP_GET, []() {
    if (!fsAvailable) {
      webServer.send(200, "application/json", "[]");
      return;
    }
    // Ensure /backups/ directory exists
    if (!LittleFS.exists("/backups")) LittleFS.mkdir("/backups");
    File dir = LittleFS.open("/backups");
    String json = "[";
    bool first = true;
    if (dir) {
      File f = dir.openNextFile();
      while (f) {
        if (!f.isDirectory()) {
          String nm = String(f.name());
          // f.name() may return full path or just filename depending on LittleFS version
          int sl = nm.lastIndexOf('/');
          if (sl >= 0) nm = nm.substring(sl + 1);
          // Strip extension so JS always passes bare name back to the backend
          int dot = nm.lastIndexOf('.');
          if (dot > 0) nm = nm.substring(0, dot);
          if (!first) json += ",";
          json += "{\"name\":\"" + nm + "\",\"size\":" + String(f.size()) + "}";
          first = false;
        }
        f = dir.openNextFile();
      }
      dir.close();
    }
    json += "]";
    webServer.send(200, "application/json", json);
  });

  // ── Save snapshot ─────────────────────────────────────────────────────────
  webServer.on("/api/backup/snapshot/save", HTTP_POST, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"ok\":false,\"error\":\"filesystem unavailable\"}");
      return;
    }
    String name = sanitizeSnapName(webServer.arg("name"));
    if (name.length() == 0) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid name\"}");
      return;
    }
    if (!LittleFS.exists("/backups")) LittleFS.mkdir("/backups");
    String text = buildExportText();
    File f = LittleFS.open(snapPath(name), "w");
    if (!f) {
      webServer.send(500, "application/json", "{\"ok\":false,\"error\":\"write failed\"}");
      return;
    }
    f.print(text);
    f.close();
    webServer.send(200, "application/json", "{\"ok\":true}");
  });

  // ── Download snapshot ─────────────────────────────────────────────────────
  webServer.on("/api/backup/snapshot/download", HTTP_GET, []() {
    if (!fsAvailable) {
      webServer.send(503, "text/plain", "filesystem unavailable");
      return;
    }
    String name = sanitizeSnapName(webServer.arg("name"));
    if (name.length() == 0 || !LittleFS.exists(snapPath(name))) {
      webServer.send(404, "text/plain", "not found");
      return;
    }
    File f = LittleFS.open(snapPath(name), "r");
    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + name + ".txt\"");
    webServer.streamFile(f, "text/plain");
    f.close();
  });

  // ── Load snapshot ─────────────────────────────────────────────────────────
  webServer.on("/api/backup/snapshot/load", HTTP_POST, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"ok\":false,\"error\":\"filesystem unavailable\"}");
      return;
    }
    String name = sanitizeSnapName(webServer.arg("name"));
    if (name.length() == 0 || !LittleFS.exists(snapPath(name))) {
      webServer.send(404, "application/json", "{\"ok\":false,\"error\":\"snapshot not found\"}");
      return;
    }
    File f = LittleFS.open(snapPath(name), "r");
    String text = f.readString();
    f.close();
    applyImportText(text);
    webServer.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    ESP.restart();
  });

  // ── Delete snapshot ───────────────────────────────────────────────────────
  webServer.on("/api/backup/snapshot/delete", HTTP_POST, []() {
    if (!fsAvailable) {
      webServer.send(503, "application/json", "{\"ok\":false,\"error\":\"filesystem unavailable\"}");
      return;
    }
    String name = sanitizeSnapName(webServer.arg("name"));
    if (name.length() == 0) {
      webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid name\"}");
      return;
    }
    bool ok = LittleFS.remove(snapPath(name));
    webServer.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
  });
}

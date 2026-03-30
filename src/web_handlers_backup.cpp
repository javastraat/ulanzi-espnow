#include <Arduino.h>
// web_handlers_backup.cpp — Config export/import and LittleFS snapshot management.
//
// Export: iterates the "ulanzi" and "wifi" NVS namespaces directly, capturing
// every key regardless of type, so new settings are automatically included.
// Format: key=TYPE:value  (u8/i8/u16/i16/u32/i32/str/blob)
// Import: writes each key back to NVS via Preferences using the stored type,
// then restarts — no need to parse into globals.
#include "web_handlers_backup.h"
#include "globals.h"
#include "nvs_settings.h"
#include <LittleFS.h>
#include <Preferences.h>
#include <nvs.h>
#include "web/backup.h"
#include "esp_idf_version.h"

// ── NVS namespace dump ────────────────────────────────────────────────────────
// Iterates every key in one NVS namespace and appends "key=TYPE:value\n" lines.
// Compatible with ESP-IDF 4.x (Arduino ESP32 2.x) and ESP-IDF 5.x (3.x).

static void dumpNvsNamespace(const char* ns, String& out) {
  out += "["; out += ns; out += "]\n";

  nvs_handle_t handle;
  if (nvs_open(ns, NVS_READONLY, &handle) != ESP_OK) return;

  nvs_iterator_t it = nullptr;

#if ESP_IDF_VERSION_MAJOR >= 5
  esp_err_t err = nvs_entry_find("nvs", ns, NVS_TYPE_ANY, &it);
  while (err == ESP_OK && it) {
#else
  it = nvs_entry_find("nvs", ns, NVS_TYPE_ANY);
  while (it) {
#endif
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);

    out += info.key;
    out += "=";

    switch (info.type) {
      case NVS_TYPE_U8: {
        uint8_t v = 0; nvs_get_u8(handle, info.key, &v);
        out += "u8:"; out += v;
      } break;
      case NVS_TYPE_I8: {
        int8_t v = 0; nvs_get_i8(handle, info.key, &v);
        out += "i8:"; out += (int)v;
      } break;
      case NVS_TYPE_U16: {
        uint16_t v = 0; nvs_get_u16(handle, info.key, &v);
        out += "u16:"; out += v;
      } break;
      case NVS_TYPE_I16: {
        int16_t v = 0; nvs_get_i16(handle, info.key, &v);
        out += "i16:"; out += (int)v;
      } break;
      case NVS_TYPE_U32: {
        uint32_t v = 0; nvs_get_u32(handle, info.key, &v);
        out += "u32:"; out += v;
      } break;
      case NVS_TYPE_I32: {
        int32_t v = 0; nvs_get_i32(handle, info.key, &v);
        out += "i32:"; out += v;
      } break;
      case NVS_TYPE_U64: {
        uint64_t v = 0; nvs_get_u64(handle, info.key, &v);
        out += "u64:";
        out += String((uint32_t)(v >> 32));
        out += String((uint32_t)v);
      } break;
      case NVS_TYPE_I64: {
        int64_t v = 0; nvs_get_i64(handle, info.key, &v);
        out += "i64:"; out += String((long)v);
      } break;
      case NVS_TYPE_STR: {
        size_t len = 0;
        nvs_get_str(handle, info.key, nullptr, &len);
        out += "str:";
        if (len > 0) {
          char* buf = new char[len];
          nvs_get_str(handle, info.key, buf, &len);
          out += buf;
          delete[] buf;
        }
      } break;
      case NVS_TYPE_BLOB: {
        size_t len = 0;
        nvs_get_blob(handle, info.key, nullptr, &len);
        out += "blob:";
        if (len > 0) {
          uint8_t* buf = new uint8_t[len];
          nvs_get_blob(handle, info.key, buf, &len);
          for (size_t i = 0; i < len; i++) {
            char h[3]; snprintf(h, 3, "%02X", buf[i]); out += h;
          }
          delete[] buf;
        }
      } break;
      default:
        out += "skip:";
        break;
    }
    out += "\n";

#if ESP_IDF_VERSION_MAJOR >= 5
    err = nvs_entry_next(&it);
#else
    it = nvs_entry_next(it);
#endif
  }
  nvs_release_iterator(it);
  nvs_close(handle);
}

// ── Build export text ─────────────────────────────────────────────────────────

static String buildExportText() {
  String out;
  out.reserve(3000);

  out += "# Ulanzi Config Backup\n";
  out += "# Device: "; out += mdnsName; out += "\n";
  out += "# Firmware: "; out += FIRMWARE_VERSION; out += "\n";
  out += "# Format: key=TYPE:value  (bool=u8, colors=u32 packed RGB, floats=blob hex)\n";
  out += "# (keep this file safe -- contains WiFi credentials)\n\n";

  dumpNvsNamespace("ulanzi", out);
  out += "\n";
  dumpNvsNamespace("wifi", out);

  return out;
}

// ── Apply import text ─────────────────────────────────────────────────────────
// Writes keys directly to NVS via Preferences — no need to touch globals since
// we restart immediately after. Unknown type tags are silently skipped.

static void applyImportText(const String& text) {
  char currentNs[16] = "";
  Preferences p;
  bool pOpen = false;

  int pos = 0, len = text.length();
  while (pos < len) {
    int nl = text.indexOf('\n', pos);
    if (nl < 0) nl = len;
    String line = text.substring(pos, nl);
    line.trim();
    pos = nl + 1;

    if (line.length() == 0 || line.startsWith("#")) continue;

    // Section header [namespace]
    if (line.startsWith("[") && line.endsWith("]")) {
      if (pOpen) { p.end(); pOpen = false; }
      String ns = line.substring(1, line.length() - 1);
      ns.trim();
      if (ns.length() > 0 && ns.length() < (int)sizeof(currentNs)) {
        strncpy(currentNs, ns.c_str(), sizeof(currentNs) - 1);
        currentNs[sizeof(currentNs) - 1] = '\0';
        p.begin(currentNs, false);
        pOpen = true;
      }
      continue;
    }

    if (!pOpen) continue;

    int eq = line.indexOf('=');
    if (eq < 1) continue;
    String key = line.substring(0, eq);  key.trim();
    String rest = line.substring(eq + 1);   // "TYPE:value"

    // Split "TYPE:value" on first colon
    int colon = rest.indexOf(':');
    if (colon < 0) continue;
    String type = rest.substring(0, colon);  type.trim();
    String val  = rest.substring(colon + 1); // value (may contain ':')

    if      (type == "u8")   p.putUChar  (key.c_str(), (uint8_t)val.toInt());
    else if (type == "i8")   p.putChar   (key.c_str(), (int8_t)val.toInt());
    else if (type == "u16")  p.putUShort (key.c_str(), (uint16_t)val.toInt());
    else if (type == "i16")  p.putShort  (key.c_str(), (int16_t)val.toInt());
    else if (type == "u32")  p.putUInt   (key.c_str(), (uint32_t)strtoul(val.c_str(), nullptr, 10));
    else if (type == "i32")  p.putInt    (key.c_str(), (int32_t)val.toInt());
    else if (type == "str")  p.putString (key.c_str(), val);
    else if (type == "blob") {
      int blen = val.length() / 2;
      if (blen > 0) {
        uint8_t* buf = new uint8_t[blen];
        for (int i = 0; i < blen; i++) {
          char h[3] = { val[i * 2], val[i * 2 + 1], '\0' };
          buf[i] = (uint8_t)strtol(h, nullptr, 16);
        }
        p.putBytes(key.c_str(), buf, blen);
        delete[] buf;
      }
    }
    // "u64", "i64", "skip" — ignored
  }

  if (pOpen) p.end();
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
    if (!LittleFS.exists("/backups")) LittleFS.mkdir("/backups");
    File dir = LittleFS.open("/backups");
    String json = "[";
    bool first = true;
    if (dir) {
      File f = dir.openNextFile();
      while (f) {
        if (!f.isDirectory()) {
          String nm = String(f.name());
          int sl = nm.lastIndexOf('/');
          if (sl >= 0) nm = nm.substring(sl + 1);
          // Strip extension so JS passes bare name back; backend always appends .txt
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

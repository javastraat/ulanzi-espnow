#include <Arduino.h>
// receiver.cpp — ESP-NOW receive callback, POCSAG processing, and receiver setup.
#include "receiver.h"
#include "globals.h"
#include "buzzer.h"
#include "sensor.h"
#include "web_server.h"
#include "display.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <time.h>

#if RECV_ESPNOW2
#include <UniversalMesh.h>
#include <ArduinoJson.h>
static UniversalMesh mesh;
#endif

// ── POCSAG helpers ────────────────────────────────────────────────────────────

#if RECV_POCSAG

static void applyDisplayMessageState(const char* msg, const char* iconFile, bool doBeep) {
  if (screensaverActive) resetScreensaverIdle();  // restores brightness + clears GIF state

  strncpy(pocsagMsg, msg, POCSAG_MSG_MAX_LEN);
  pocsagMsg[POCSAG_MSG_MAX_LEN] = '\0';
  pocsagMsgLen    = strlen(pocsagMsg);
  pocsagMsgActive = (pocsagMsgLen > 0);
  if (pocsagMsgActive && doBeep) buzzerBeep();

  if (iconFile && iconFile[0]) {
    strncpy(pocsagMsgCustomIconFile, iconFile, sizeof(pocsagMsgCustomIconFile) - 1);
    pocsagMsgCustomIconFile[sizeof(pocsagMsgCustomIconFile) - 1] = '\0';
    pocsagMsgUseCustomIcon = true;
  } else {
    pocsagMsgUseCustomIcon = false;
    pocsagMsgCustomIconFile[0] = '\0';
  }

  // Keep same sizing behavior as native POCSAG display path.
  pocsagIsScrolling = (POCSAG_ICON_RESERVED_PX + pocsagMsgLen * 4 - 1 > MATRIX_WIDTH);
  if (pocsagIsScrolling) {
    pocsagScrollX    = MATRIX_WIDTH;
    pocsagScrollPass = 0;
    pocsagScrollLast = millis();
  } else {
    pocsagStaticUntil    = millis() + POCSAG_STATIC_MS;
    pocsagStaticLastDraw = 0;
  }
}

bool injectDisplayMessage(const char* text, const char* iconFile, bool beep, uint32_t ric) {
  if (!text) return false;

  char trimmed[POCSAG_MSG_MAX_LEN + 1] = {};
  strncpy(trimmed, text, POCSAG_MSG_MAX_LEN);
  trimmed[POCSAG_MSG_MAX_LEN] = '\0';
  int len = strlen(trimmed);
  while (len > 0 && (trimmed[len - 1] == ' ' || trimmed[len - 1] == '\t' || trimmed[len - 1] == '\r' || trimmed[len - 1] == '\n'))
    trimmed[--len] = '\0';
  int start = 0;
  while (trimmed[start] == ' ' || trimmed[start] == '\t' || trimmed[start] == '\r' || trimmed[start] == '\n') start++;
  if (trimmed[start] == '\0') return false;

  applyDisplayMessageState(trimmed + start, iconFile, beep);

  wsCountPocsag++;
  wsPocsagLog[wsPocsagHead].ric = ric;
  strncpy(wsPocsagLog[wsPocsagHead].msg, pocsagMsg, POCSAG_MSG_MAX_LEN);
  wsPocsagLog[wsPocsagHead].msg[POCSAG_MSG_MAX_LEN] = '\0';
  { struct tm _t; if (getLocalTime(&_t)) snprintf(wsPocsagLog[wsPocsagHead].ts, 9, "%02d:%02d:%02d", _t.tm_hour, _t.tm_min, _t.tm_sec); else wsPocsagLog[wsPocsagHead].ts[0] = '\0'; }
  wsPocsagHead = (wsPocsagHead + 1) % POCSAG_LOG_SIZE;
  if (wsPocsagFill < POCSAG_LOG_SIZE) wsPocsagFill++;

  mqttNotifyPocsag();
  LOG("[MSG] Injected message: '%s'\n", pocsagMsg);
  return true;
}

static void _logMsgEntry(WsMsgEntry* log, uint8_t& head, uint8_t& fill, uint32_t& count, const char* msg) {
  strncpy(log[head].msg, msg, POCSAG_MSG_MAX_LEN);
  log[head].msg[POCSAG_MSG_MAX_LEN] = '\0';
  { struct tm t; if (getLocalTime(&t)) snprintf(log[head].ts, 9, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec); else log[head].ts[0] = '\0'; }
  head = (head + 1) % POCSAG_LOG_SIZE;
  if (fill < POCSAG_LOG_SIZE) fill++;
  count++;
}

bool injectWebMessage(const char* text, const char* iconFile, bool beep) {
  if (!text || text[0] == '\0') return false;
  applyDisplayMessageState(text, iconFile, beep);
  _logMsgEntry(wsWebLog, wsWebHead, wsWebFill, wsCountWeb, pocsagMsg);
  mqttNotifyWebMsg();
  LOG("[WEB] Message: '%s'\n", pocsagMsg);
  return true;
}

bool injectMqttMessage(const char* text, const char* iconFile, bool beep) {
  if (!text || text[0] == '\0') return false;
  applyDisplayMessageState(text, iconFile, beep);
  _logMsgEntry(wsMqttLog, wsMqttHead, wsMqttFill, wsCountMqtt, pocsagMsg);
  mqttNotifyHassMsg();
  LOG("[MQTT] Message: '%s'\n", pocsagMsg);
  return true;
}

static const char* functionalNameRx(uint8_t f) {
  switch (f) {
    case 0: return "NUMERIC";
    case 1: return "ALERT1";
    case 2: return "ALERT2";
    case 3: return "ALPHA";
    default: return "?";
  }
}

// Parse time beacon from RIC 224.
// Message format: "YYYYMMDDHHMMSS" + "YYMMDDHHmmSS"
// Calls settimeofday() to set the system clock.
static void applyPocsagTime(const char* msg) {
  size_t len = strlen(msg);
  if (len < 26) {
    LOG("[TIME] RIC 224 message too short (%u chars), expected >=26\n", len);
    return;
  }
  const char* d = msg + 14;  // skip the "YYYYMMDDHHMMSS" format label

  char tmp[3] = {};
  struct tm t = {};
  memcpy(tmp, d +  0, 2); t.tm_year = 100 + atoi(tmp); // YY → years since 1900
  memcpy(tmp, d +  2, 2); t.tm_mon  = atoi(tmp) - 1;   // 1-based → 0-based
  memcpy(tmp, d +  4, 2); t.tm_mday = atoi(tmp);
  memcpy(tmp, d +  6, 2); t.tm_hour = atoi(tmp);
  memcpy(tmp, d +  8, 2); t.tm_min  = atoi(tmp);
  memcpy(tmp, d + 10, 2); t.tm_sec  = atoi(tmp) + 1;  // +1 sec to compensate for processing delay

  time_t epoch = mktime(&t);
  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  timeSynced   = true;
  pocsagSynced = true;

  if (rtcAvailable)
    ds1307Write(t);

  LOG("[TIME] Set from POCSAG RIC %lu: %04d-%02d-%02d %02d:%02d:%02d%s\n",
    (unsigned long)timePocRic,
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec,
    rtcAvailable ? " [RTC updated]" : "");
}

// Runs on Core 1 (loop()) after being dequeued from pocsagRxQueue.
void processPocsagPacket(const EspNowPocsagPacket& pkt) {
  if (!recvPocsagEnabled) return;

  if (pkt.ric == timePocRic)
    applyPocsagTime(pkt.message);

  bool excluded = false;
  for (int i = 0; i < excludedRicsCount; i++)
    if (pkt.ric == excludedRics[i]) { excluded = true; break; }

  if (!excluded) {
    char msgBuf[POCSAG_MSG_MAX_LEN + 1] = {};
    strncpy(msgBuf, pkt.message, POCSAG_MSG_MAX_LEN);
    msgBuf[POCSAG_MSG_MAX_LEN] = '\0';
    if (pkt.ric == callsignRic) {
      int len = strlen(msgBuf);
      while (len > 0 && msgBuf[len - 1] >= '0' && msgBuf[len - 1] <= '9')
        msgBuf[--len] = '\0';
    }
    applyDisplayMessageState(msgBuf, nullptr, true);
  }
}

#endif  // RECV_POCSAG

// ── ESP-NOW v2 processor ──────────────────────────────────────────────────────

#if RECV_ESPNOW2

#define MESH_HEARTBEAT_INTERVAL  60000
#define MESH_SENSOR_INTERVAL     30000

static unsigned long meshLastHeartbeat = 0;
static unsigned long meshLastSensor    = 0;
static uint8_t       meshBroadcast[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void injectEspNow2Message(const char* msg, uint32_t msgId, uint8_t appId, uint8_t ttl, const uint8_t* relay) {
  if (!msg || msg[0] == '\0') return;

  WsEspNow2Entry& e = wsEspNow2Log[wsEspNow2Head];
  strncpy(e.msg, msg, POCSAG_MSG_MAX_LEN);
  e.msg[POCSAG_MSG_MAX_LEN] = '\0';
  e.msgId = msgId;
  e.appId = appId;
  e.ttl   = ttl;
  if (relay)
    snprintf(e.relay, sizeof(e.relay), "%02X:%02X:%02X:%02X:%02X:%02X",
      relay[0], relay[1], relay[2], relay[3], relay[4], relay[5]);
  else
    e.relay[0] = '\0';
  { struct tm t; if (getLocalTime(&t)) snprintf(e.ts, 9, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec); else e.ts[0] = '\0'; }

  wsEspNow2Head = (wsEspNow2Head + 1) % POCSAG_LOG_SIZE;
  if (wsEspNow2Fill < POCSAG_LOG_SIZE) wsEspNow2Fill++;
  wsCountEspNow2++;

  mqttNotifyEspNow2();
}

static void processMeshPacket(const uint8_t* data, int len) {
  if (len < (int)sizeof(MeshPacket)) { LOG("[MESH] Packet too short (%d bytes)\n", len); return; }
  const MeshPacket* p = (const MeshPacket*)data;

  LOG("[MESH] type=0x%02X msgId=%u ttl=%d appId=0x%02X src=%02X:%02X:%02X:%02X:%02X:%02X payloadLen=%d\n",
    p->type, p->msgId, p->ttl, p->appId,
    p->srcMac[0], p->srcMac[1], p->srcMac[2],
    p->srcMac[3], p->srcMac[4], p->srcMac[5],
    p->payloadLen);

  // PONG with appId 0xFF = coordinator replied to our PING — save its MAC and announce
  if (p->type == MESH_TYPE_PONG && p->appId == 0xFF) {
    mesh.setCoordinatorMac((uint8_t*)p->srcMac);
    LOG("[MESH] Coordinator found: %02X:%02X:%02X:%02X:%02X:%02X\n",
      p->srcMac[0], p->srcMac[1], p->srcMac[2],
      p->srcMac[3], p->srcMac[4], p->srcMac[5]);
    mesh.send((uint8_t*)p->srcMac, MESH_TYPE_DATA, 0x06,
              (const uint8_t*)mdnsName, strlen(mdnsName), 4);
    return;
  }

  if (p->type != MESH_TYPE_DATA) return;

  uint8_t msgLen = p->payloadLen;
  if (msgLen == 0 || !recvEspnow2Enabled) return;

  char msg[201] = {};
  uint8_t copyLen = (msgLen > 200) ? 200 : msgLen;
  memcpy(msg, p->payload, copyLen);

  LOG("[MESH] appId=0x%02X msg=\"%s\"\n", p->appId, msg);

  // Drop excluded appIds before touching the log
  for (int i = 0; i < excludedAppIdsCount; i++) {
    if (p->appId == excludedAppIds[i]) {
      LOG("[MESH] appId=0x%02X excluded — not logged\n", p->appId);
      return;
    }
  }

  // 0x05 = heartbeat — log to web, no display
  if (p->appId == 0x05) {
    LOG("[MESH] Heartbeat from %02X:%02X:%02X:%02X:%02X:%02X\n",
      p->srcMac[0], p->srcMac[1], p->srcMac[2],
      p->srcMac[3], p->srcMac[4], p->srcMac[5]);
    injectEspNow2Message("heartbeat", p->msgId, p->appId, p->ttl, p->srcMac);
    return;
  }

  // 0x06 = node name registration — log to web, no display
  if (p->appId == 0x06) {
    LOG("[MESH] Node registered: \"%s\" (%02X:%02X:%02X:%02X:%02X:%02X)\n",
      msg,
      p->srcMac[0], p->srcMac[1], p->srcMac[2],
      p->srcMac[3], p->srcMac[4], p->srcMac[5]);
    injectEspNow2Message(msg, p->msgId, p->appId, p->ttl, p->srcMac);
    return;
  }

  // 0x01 = sensor/data payload — could be forwarded POCSAG or sensor readings
  if (p->appId == 0x01) {
    JsonDocument doc;
    if (deserializeJson(doc, msg) == DeserializationError::Ok) {

      // Forwarded POCSAG: {"ric":200,"func":3,"msg":"..."}
      if (doc["msg"].is<const char*>()) {
        uint32_t    ric       = doc["ric"] | 0;
        const char* pocsagMsg = doc["msg"];

        // Time beacon via mesh — sync clock even if POCSAG protocol is off
        if (ric == timePocRic) {
          applyPocsagTime(pocsagMsg);
          injectEspNow2Message(msg, p->msgId, p->appId, p->ttl, p->srcMac);
          return;
        }

        bool excluded = false;
        for (int i = 0; i < excludedRicsCount; i++)
          if (ric == (uint32_t)excludedRics[i]) { excluded = true; break; }
        if (excluded) {
          LOG("[MESH→POCSAG] RIC=%lu excluded — skipping display\n", (unsigned long)ric);
          injectEspNow2Message(msg, p->msgId, p->appId, p->ttl, p->srcMac);
          return;
        }
        LOG("[MESH→POCSAG] RIC=%lu msg='%s'\n", (unsigned long)ric, pocsagMsg);
        injectEspNow2Message(msg, p->msgId, p->appId, p->ttl, p->srcMac);
#if RECV_POCSAG
        char pocBuf[POCSAG_MSG_MAX_LEN + 1] = {};
        strncpy(pocBuf, pocsagMsg, POCSAG_MSG_MAX_LEN);
        if (ric == callsignRic) {
          int len = strlen(pocBuf);
          while (len > 0 && pocBuf[len - 1] >= '0' && pocBuf[len - 1] <= '9')
            pocBuf[--len] = '\0';
        }
        injectDisplayMessage(pocBuf, nullptr, true, ric);
#endif
        return;
      }

      // Sensor node reading: {"name":"node","temp":"23.5"} — log, no display
      if (doc["temp"].is<const char*>() || doc["temp"].is<float>()) {
        const char* name = doc["name"] | "unknown";
        float temp = doc["temp"] | 0.0f;
        LOG("[MESH] Sensor \"%s\" temp=%.1f\n", name, temp);
        injectEspNow2Message(msg, p->msgId, p->appId, p->ttl, p->srcMac);
        return;
      }
    }
  }

  // Everything else goes to the ESP-NOW2 log
  injectEspNow2Message(msg, p->msgId, p->appId, p->ttl, p->srcMac);
}

void loopMesh() {
  unsigned long now = millis();

  // Not connected yet — keep pinging to find coordinator
  if (!mesh.isCoordinatorFound()) {
    static unsigned long meshLastPing = 0;
    if (now - meshLastPing >= 10000) {
      meshLastPing = now;
      mesh.send(meshBroadcast, MESH_TYPE_PING, 0x00,
                (const uint8_t*)mdnsName, strlen(mdnsName), 4);
      LOG("[MESH] Scanning for coordinator...\n");
    }
    return;
  }

  // Heartbeat + name re-registration every 60s
  if (now - meshLastHeartbeat >= MESH_HEARTBEAT_INTERVAL) {
    meshLastHeartbeat = now;
    uint8_t hb = 0x01;
    mesh.sendToCoordinator(0x05, &hb, 1);
    mesh.sendToCoordinator(0x06, (uint8_t*)mdnsName, strlen(mdnsName));
    LOG("[MESH] Heartbeat sent\n");
  }

  // Sensor data every 30s
  if (now - meshLastSensor >= MESH_SENSOR_INTERVAL) {
    meshLastSensor = now;
    if (sht31Available) {
      JsonDocument doc;
      doc["name"] = mdnsName;
      doc["temp"] = serialized(String(sht31Temp, 1));
      doc["hum"]  = serialized(String(sht31Hum,  1));
      String payload;
      serializeJson(doc, payload);
      mesh.sendToCoordinator(0x01, payload);
      LOG("[MESH] Sensor: %s\n", payload.c_str());
    }
  }
}

#endif  // RECV_ESPNOW2

// ── Receive callback (Core 0 WiFi stack) ──────────────────────────────────────

void onReceive(const esp_now_recv_info_t* info, const uint8_t* inData, int inLen) {
  if (inLen < 1) return;
  uint8_t type = inData[0];

  // ── DMR packet ──────────────────────────────────────────────────────────────
#if RECV_DMR
  if (type == ESPNOW_TYPE_DMR_NET && recvDmrEnabled) {
    EspNowDmrNetPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));

    if (pkt.len < 21 || memcmp(pkt.data, "DMRD", 4) != 0) {
      LOG("[RX-DMR] Bad DMRD payload (len=%d)\n", pkt.len);
      return;
    }

    rxTotalDmr++;
    wsCountDmr++;
    uint32_t srcId   = ((uint32_t)pkt.data[5] << 16) | ((uint32_t)pkt.data[6] << 8) | pkt.data[7];
    uint32_t dstId   = ((uint32_t)pkt.data[8] << 16) | ((uint32_t)pkt.data[9] << 8) | pkt.data[10];
    uint8_t  flags   = pkt.data[15];
    uint8_t  slot    = (flags & 0x80) ? 2 : 1;
    bool     isGroup = (flags & 0x40) != 0;
    uint8_t  seq     = pkt.data[4];

    bool isNewCall = (srcId != callSrc || dstId != callDst || slot != callSlot);
    if (isNewCall) {
      if (callFrames > 0) {
        unsigned long dur = (millis() - callStart) / 1000;
        LOG("[RX-DMR] ── END   src=%-8lu  dst=TG%-6lu  slot=%d  frames=%lu  dur=%lus\n\n",
          callSrc, callDst, callSlot, callFrames, dur);
      }
      callSrc    = srcId;
      callDst    = dstId;
      callSlot   = slot;
      callFrames = 0;
      callStart  = millis();
      LOG("[RX-DMR] ══ NEW   src=%-8lu  dst=%s%-6lu  slot=%d  pkt#%lu\n",
        srcId, isGroup ? "TG" : "", dstId, slot, rxTotalDmr);
    }
    callFrames++;

#if ESPNOW_DEBUG
    char streamHex[9];
    snprintf(streamHex, sizeof(streamHex), "%02X%02X%02X%02X",
      pkt.data[16], pkt.data[17], pkt.data[18], pkt.data[19]);
    LOG("  [#%lu] seq=%3d  stream=%s  frame: %02X %02X %02X %02X %02X %02X %02X %02X\n",
      callFrames, seq, streamHex,
      pkt.data[20], pkt.data[21], pkt.data[22], pkt.data[23],
      pkt.data[24], pkt.data[25], pkt.data[26], pkt.data[27]);
#else
    (void)seq;
#endif
    return;
  }
#endif  // RECV_DMR

  // ── POCSAG packet ────────────────────────────────────────────────────────────
#if RECV_POCSAG
  if (type == ESPNOW_TYPE_POCSAG) {
    EspNowPocsagPacket pkt = {};
    memcpy(&pkt, inData, (inLen < (int)sizeof(pkt)) ? inLen : sizeof(pkt));
    pkt.message[POCSAG_MSG_MAX_LEN] = '\0';

    rxTotalPocsag++;
    wsCountPocsag++;
    wsPocsagLog[wsPocsagHead].ric = pkt.ric;
    strncpy(wsPocsagLog[wsPocsagHead].msg, pkt.message, POCSAG_MSG_MAX_LEN);
    wsPocsagLog[wsPocsagHead].msg[POCSAG_MSG_MAX_LEN] = '\0';
    { struct tm _t; if (getLocalTime(&_t)) snprintf(wsPocsagLog[wsPocsagHead].ts, 9, "%02d:%02d:%02d", _t.tm_hour, _t.tm_min, _t.tm_sec); else wsPocsagLog[wsPocsagHead].ts[0] = '\0'; }
    wsPocsagHead = (wsPocsagHead + 1) % POCSAG_LOG_SIZE;
    if (wsPocsagFill < POCSAG_LOG_SIZE) wsPocsagFill++;
    LOG("[RX-POCSAG #%lu] RIC=%-10lu  enc=%-7s  msg='%s'\n",
      rxTotalPocsag, (unsigned long)pkt.ric,
      functionalNameRx(pkt.functional), pkt.message);

    // Immediate MQTT push
    mqttNotifyPocsag();
    // Hand off display state update to Core 1 via queue
    xQueueSendFromISR(pocsagRxQueue, &pkt, nullptr);
    return;
  }
#endif  // RECV_POCSAG

#if RECV_ESPNOW2
  // UniversalMesh packet types: PING(0x12) PONG(0x13) ACK(0x14) DATA(0x15) and secure variants
  if (type >= MESH_TYPE_PING && type <= MESH_TYPE_PARANOID_DATA) {
    processMeshPacket(inData, inLen);
    return;
  }
#endif

  LOG("[RX] Unknown type 0x%02X (%d bytes)\n", type, inLen);
}

// ── WiFi + ESP-NOW setup ──────────────────────────────────────────────────────

static void setupReceiverNetwork() {
  WiFi.mode(WIFI_STA);
  bool connected = false;

  for (int slot = 0; slot < WIFI_SLOT_COUNT && !connected; slot++) {
    if (wifiSlotSsid[slot].length() == 0) continue;
    WiFi.setHostname(mdnsName);  // must be set before begin() for DHCP + mDNS
    WiFi.begin(wifiSlotSsid[slot].c_str(), wifiSlotPass[slot].c_str());
    LOG("[WiFi] Slot %d (%s) ", slot + 1, wifiSlotSsid[slot].c_str());
    unsigned long t       = millis();
    unsigned long timeout = (unsigned long)wifiMaxRetries * 500UL;
    while (WiFi.status() != WL_CONNECTED && millis() - t < timeout) {
      delay(250); LOG(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      WiFi.setSleep(false);  // prevent WiFi power-save pauses from glitching RMT/WS2812B
      LOG("\n[WiFi] Connected: %s  channel: %d\n",
        WiFi.localIP().toString().c_str(), WiFi.channel());
    } else {
      LOG("\n[WiFi] Slot %d failed\n", slot + 1);
      WiFi.disconnect(true);
      delay(200);
    }
  }

  if (connected) {
    softAPActive = false;
    setupOTA();    // ArduinoOTA + mDNS + WebServer (in web_server.cpp)
  } else {
    LOG("[WiFi] All slots failed — starting SoftAP: %s\n", wifiApSsid.c_str());
    WiFi.mode(WIFI_AP_STA);  // AP_STA keeps STA interface alive for ESP-NOW
    WiFi.softAP(wifiApSsid.c_str(), wifiApPassword.c_str(), wifiApChannel);
    softAPActive = true;
    LOG("[WiFi] SoftAP started — IP: %s\n", WiFi.softAPIP().toString().c_str());
    setupWebAP();  // WebServer only, no OTA/mDNS (in web_server.cpp)
  }
}

void setupReceiver() {
  LOG("[ROLE] RECEIVER — modes:"
#if RECV_DMR
  " DMR"
#endif
#if RECV_POCSAG
  " POCSAG"
#endif
#if RECV_ESPNOW2
  " ESPNOW2"
#endif
  "\n");

#if RECV_DMR && ESPNOW_DEBUG
  LOG("[MODE] DMR debug: ON\n");
#elif RECV_DMR
  LOG("[MODE] DMR debug: OFF (set ESPNOW_DEBUG true for full frames)\n");
#endif

  setupReceiverNetwork();

#if RECV_ESPNOW2
  // Init as a mesh node on the current WiFi channel
  uint8_t wifiChannel = softAPActive ? wifiApChannel : (uint8_t)WiFi.channel();
  if (!mesh.begin(wifiChannel, MESH_NODE)) {
    LOG("[ESP-NOW] Init FAILED — halting.\n");
    while (true) delay(1000);
  }
  LOG("[MESH] Node started on channel %d — scanning for coordinator\n", wifiChannel);
  // Re-register our own callback so POCSAG/DMR raw packets still work
  esp_now_register_recv_cb(onReceive);
#else
  if (esp_now_init() != ESP_OK) {
    LOG("[ESP-NOW] Init FAILED — halting.\n");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onReceive);
#endif

  uint8_t macBytes[6];
  esp_wifi_get_mac(WIFI_IF_STA, macBytes);
  LOG("[INFO] My MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
    macBytes[0], macBytes[1], macBytes[2],
    macBytes[3], macBytes[4], macBytes[5]);

  LOG("[RECEIVER] Listening — clock will sync on first RIC %lu beacon\n",
    (unsigned long)timePocRic);

#if RECV_ESPNOW2
  meshLastHeartbeat = millis() - MESH_HEARTBEAT_INTERVAL;
  meshLastSensor    = millis() - MESH_SENSOR_INTERVAL;
  // Send initial PING — loopMesh() will keep retrying until coordinator replies
  mesh.send(meshBroadcast, MESH_TYPE_PING, 0x00,
            (const uint8_t*)mdnsName, strlen(mdnsName), 4);
#endif
}

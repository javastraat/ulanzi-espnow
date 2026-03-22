#pragma once
#include "globals.h"

void setupReceiver();

#if RECV_POCSAG
void processPocsagPacket(const EspNowPocsagPacket& pkt);
bool injectDisplayMessage(const char* text, const char* iconFile = nullptr, bool beep = true, uint32_t ric = 0);
bool injectWebMessage(const char* text, const char* iconFile = nullptr, bool beep = true);
bool injectMqttMessage(const char* text, const char* iconFile = nullptr, bool beep = true);
#endif

#if RECV_ESPNOW2
void injectEspNow2Message(const char* msg, uint32_t msgId = 0, uint8_t appId = 0, uint8_t ttl = 0, const uint8_t* relay = nullptr);
#endif

#pragma once
#include "globals.h"

void setupReceiver();

#if RECV_POCSAG
void processPocsagPacket(const EspNowPocsagPacket& pkt);
bool injectDisplayMessage(const char* text, const char* iconFile = nullptr, bool beep = true, uint32_t ric = 0);
#endif

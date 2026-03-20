#pragma once
#include <Arduino.h>

// setupOTA() — called from receiver.cpp once WiFi connects.
// Initialises ArduinoOTA and starts the web server.
void setupOTA();

// setupWebAP() — called from receiver.cpp when no WiFi connects and SoftAP is started.
// Starts the web server only (no OTA/mDNS) so the user can configure WiFi at 192.168.4.1.
void setupWebAP();

// initWebTask() — called from setup() in ulanzi-espnow.ino.
// Starts the FreeRTOS task that drives ArduinoOTA.handle() + webServer.handleClient().
void initWebTask();

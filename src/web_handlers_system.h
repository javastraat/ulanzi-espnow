#pragma once
#include <Arduino.h>

void registerSystemHandlers();
bool startOtaDownloadFromGithub(const char* version, String* errorOut = nullptr, int* writtenOut = nullptr);

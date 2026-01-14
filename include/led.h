#pragma once

#include <stdint.h>

void initLed();
void applyLedConfig();
void setLedScanning(bool scanning);
void updateLed(uint32_t nowMs);

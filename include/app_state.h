#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <map>
#include <vector>

#include "app_types.h"
#include "ble_stack.h"

extern Preferences prefs;
extern WebServer server;
extern BleScan *bleScan;

extern std::vector<DeviceInfo> lastScan;
extern std::map<String, SuspectEntry> suspects;
extern std::map<String, uint32_t> whitelist;

extern String apSsid;
extern String apPass;
extern bool useNimble;
extern uint8_t ledMode;
extern uint8_t ledPin;
extern uint8_t ledPinR;
extern uint8_t ledPinG;
extern uint8_t ledPinB;
extern bool ledActiveLow;
extern bool ssidHidden;

extern uint32_t lastScanStartMs;

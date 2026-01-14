#include "app_state.h"

Preferences prefs;
WebServer server(80);
BleScan *bleScan = nullptr;

std::vector<DeviceInfo> lastScan;
std::map<String, SuspectEntry> suspects;
std::map<String, uint32_t> whitelist;

String apSsid;
String apPass;
bool useNimble = false;
uint8_t ledMode = 0;
uint8_t ledPin = 8;
uint8_t ledPinR = 4;
uint8_t ledPinG = 5;
uint8_t ledPinB = 6;
bool ledActiveLow = false;
bool ssidHidden = false;

uint32_t lastScanStartMs = 0;

#include <Arduino.h>
#include <algorithm>

#include "app_config.h"
#include "app_state.h"
#include "ble_scan.h"
#include "button.h"
#include "led.h"
#include "settings.h"
#include "storage.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("[BOOT] Suspectre starting...");
  pinMode(AppConfig::kButtonPin, INPUT_PULLUP);

  prefs.begin(AppConfig::kPrefsNamespace, false);
  loadSettings();
  loadWhitelist();
  loadSuspects();
  applyApSettings();
  initLed();

  initBleScan();
  setupWebServer();
}

void loop() {
  server.handleClient();
  handleButton();

  uint32_t now = millis();
  if (now - lastScanStartMs > AppConfig::kScanIntervalMs) {
    runBleScan();
    now = millis();
  }

  updateLed(now);
  if (lastScanStartMs != 0 && now >= lastScanStartMs &&
      (now - lastScanStartMs) > AppConfig::kStaleScanMs) {
    lastScan.clear();
  }
}

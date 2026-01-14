#include "led.h"

#include <Arduino.h>
#include "app_state.h"
#include "storage.h"

namespace {

uint32_t lastBlinkToggleMs = 0;
bool blinkOn = false;

void setOn(bool on) {
  digitalWrite(ledPin, ledActiveLow ? !on : on);
}

uint32_t intervalFromRssi(int rssi) {
  int clamped = rssi;
  if (clamped > -30) {
    clamped = -30;
  }
  if (clamped < -90) {
    clamped = -90;
  }
  int range = -30 - (-90);
  int offset = clamped - (-90);
  int interval = 800 - (offset * (800 - 150) / range);
  return static_cast<uint32_t>(interval);
}

void blinkSingle(uint32_t now, int rssi) {
  uint32_t interval = intervalFromRssi(rssi);
  uint32_t half = interval / 2;
  if (half == 0) {
    half = 1;
  }
  if ((now - lastBlinkToggleMs) >= half) {
    blinkOn = !blinkOn;
    lastBlinkToggleMs = now;
  }
  setOn(blinkOn);
}

void blinkTriple(uint32_t now, int rssi) {
  uint32_t interval = intervalFromRssi(rssi);
  uint32_t pulse = interval / 4;
  if (pulse < 50) {
    pulse = 50;
  }
  uint32_t cycle = pulse * 8;
  uint32_t t = now % cycle;
  bool on = (t < pulse) || (t >= 2 * pulse && t < 3 * pulse) || (t >= 4 * pulse && t < 5 * pulse);
  setOn(on);
}

} // namespace

void initLed() {
  applyLedConfig();
}

void applyLedConfig() {
  pinMode(ledPin, OUTPUT);
  setOn(false);
}

void setLedScanning(bool scanning) {
  (void)scanning;
}

void updateLed(uint32_t nowMs) {
  int strongestSuspect = -1000;
  int strongestConfirmed = -1000;
  for (const auto &dev : lastScan) {
    auto it = suspects.find(dev.addr);
    if (it == suspects.end()) {
      continue;
    }
    if (it->second.confirmed) {
      strongestConfirmed = max(strongestConfirmed, dev.rssi);
    } else {
      strongestSuspect = max(strongestSuspect, dev.rssi);
    }
  }

  if (strongestConfirmed > -1000) {
    blinkTriple(nowMs, strongestConfirmed);
    return;
  }

  if (strongestSuspect > -1000) {
    blinkSingle(nowMs, strongestSuspect);
    return;
  }

  setOn(false);
}

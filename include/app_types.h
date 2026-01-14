#pragma once

#include <Arduino.h>

struct DeviceInfo {
  String addr;
  String name;
  String advData;
  int rssi = 0;
  uint32_t lastSeenMs = 0;
};

struct SuspectEntry {
  DeviceInfo device;
  uint32_t count = 0;
  bool confirmed = false;
  String notes;
};

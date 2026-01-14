#include "ble_scan.h"

#include "app_config.h"
#include "app_state.h"
#include "led.h"
#include "utils.h"

namespace {

String parseAdvName(const uint8_t *payload, size_t length) {
  if (!payload || length == 0) {
    return "";
  }
  String shortName;
  size_t index = 0;
  while (index + 1 < length) {
    uint8_t fieldLen = payload[index++];
    if (fieldLen == 0 || index + fieldLen > length) {
      break;
    }
    uint8_t type = payload[index++];
    uint8_t dataLen = fieldLen - 1;
    if ((type == 0x09 || type == 0x08) && dataLen > 0) {
      String value;
      value.reserve(dataLen);
      for (uint8_t i = 0; i < dataLen && (index + i) < length; ++i) {
        value += static_cast<char>(payload[index + i]);
      }
      if (type == 0x09) {
        return value;
      }
      if (shortName.isEmpty()) {
        shortName = value;
      }
    }
    index += dataLen;
  }
  return shortName;
}

} // namespace

void initBleScan() {
  BleDevice::init("Suspectre");
#ifdef USE_NIMBLE
  BleDevice::setPower(ESP_PWR_LVL_P9);
#endif
  bleScan = BleDevice::getScan();
  bleScan->setActiveScan(true);
  bleScan->setInterval(160);
  bleScan->setWindow(80);
#ifdef USE_NIMBLE
  bleScan->setFilterPolicy(0);
#endif
  Serial.println("[BLE] Scan initialized");
}

void runBleScan() {
  if (!bleScan) {
    return;
  }
  setLedScanning(true);
  lastScanStartMs = millis();
  BleScanResults results;
#ifdef USE_NIMBLE
  bool started = bleScan->start(5, false, false);
  if (!started) {
    Serial.println("[BLE] NimBLE scan start failed");
  }
  results = bleScan->getResults();
#else
  results = bleScan->start(3, false);
#endif
  lastScan.clear();
  int count = results.getCount();
  Serial.print("[BLE] Scan results: ");
  Serial.println(count);
  lastScan.reserve(count);
  for (int i = 0; i < count && i < AppConfig::kMaxDevices; ++i) {
    BleAdvertisedDevice dev =
#ifdef USE_NIMBLE
        *results.getDevice(i);
#else
        results.getDevice(i);
#endif
    DeviceInfo info;
    info.addr = dev.getAddress().toString().c_str();
    info.name = dev.getName().c_str();
    if (info.name.isEmpty()) {
#ifdef USE_NIMBLE
      info.name = parseAdvName(dev.getPayload(), dev.getPayloadLength());
#else
      info.name = parseAdvName(dev.getPayload(), dev.getPayloadLength());
#endif
    }
    info.rssi = dev.getRSSI();
    if (dev.haveManufacturerData()) {
      info.advData = toHexString(dev.getManufacturerData());
    } else if (dev.haveServiceData()) {
      info.advData = toHexString(dev.getServiceData());
    }
    info.lastSeenMs = millis();
    lastScan.push_back(info);
  }
  bleScan->clearResults();
  setLedScanning(false);
}

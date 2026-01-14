#pragma once

#include <stdint.h>

namespace AppConfig {
constexpr uint8_t kButtonPin = 9; // BOOT button on ESP32-C3
constexpr uint32_t kScanIntervalMs = 5000;
constexpr uint32_t kStaleScanMs = 15000;
constexpr uint16_t kMaxDevices = 200;
constexpr uint8_t kApChannel = 6;
constexpr uint8_t kApMaxConn = 4;
constexpr const char *kPrefsNamespace = "suspectre";
constexpr const char *kPrefsSsidKey = "ssid";
constexpr const char *kPrefsPassKey = "pass";
constexpr const char *kPrefsUseNimbleKey = "use_nimble";
constexpr const char *kPrefsWhitelistKey = "whitelist";
constexpr const char *kPrefsSuspectsKey = "suspects";
constexpr const char *kPrefsLedModeKey = "led_mode";
constexpr const char *kPrefsLedPinKey = "led_pin";
constexpr const char *kPrefsLedPinRKey = "led_pin_r";
constexpr const char *kPrefsLedPinGKey = "led_pin_g";
constexpr const char *kPrefsLedPinBKey = "led_pin_b";
constexpr const char *kPrefsLedActiveLowKey = "led_active_low";
constexpr const char *kPrefsSsidHiddenKey = "ssid_hidden";
} // namespace AppConfig

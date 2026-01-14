#include "settings.h"

#include <WiFi.h>

#include "app_config.h"
#include "app_state.h"

String defaultSsid() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String suffix = mac.substring(mac.length() - 4);
  suffix.toUpperCase();
  return String("Suspectre-") + suffix;
}

void applyApSettings() {
  if (apSsid.length() == 0) {
    apSsid = defaultSsid();
  }
  if (apPass.length() < 8) {
    apPass = "suspectre";
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str(), apPass.c_str(), AppConfig::kApChannel, ssidHidden, AppConfig::kApMaxConn);
}

void loadSettings() {
  apSsid = prefs.getString(AppConfig::kPrefsSsidKey, "");
  apPass = prefs.getString(AppConfig::kPrefsPassKey, "");
  useNimble = prefs.getBool(AppConfig::kPrefsUseNimbleKey, false);
  ledMode = prefs.getUChar(AppConfig::kPrefsLedModeKey, 0);
  ledPin = prefs.getUChar(AppConfig::kPrefsLedPinKey, 8);
  ledPinR = prefs.getUChar(AppConfig::kPrefsLedPinRKey, 4);
  ledPinG = prefs.getUChar(AppConfig::kPrefsLedPinGKey, 5);
  ledPinB = prefs.getUChar(AppConfig::kPrefsLedPinBKey, 6);
  ledActiveLow = prefs.getBool(AppConfig::kPrefsLedActiveLowKey, false);
  ssidHidden = prefs.getBool(AppConfig::kPrefsSsidHiddenKey, false);
  ledMode = 2;
  prefs.putUChar(AppConfig::kPrefsLedModeKey, ledMode);
  if (apSsid.length() == 0) {
    apSsid = defaultSsid();
    prefs.putString(AppConfig::kPrefsSsidKey, apSsid);
  }
  if (apPass.length() < 8) {
    apPass = "suspectre";
    prefs.putString(AppConfig::kPrefsPassKey, apPass);
  }
}

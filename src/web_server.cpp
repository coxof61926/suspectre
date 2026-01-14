#include "web_server.h"

#include <Arduino.h>
#include <algorithm>
#include <vector>
#include <pgmspace.h>

#include "app_config.h"
#include "app_state.h"
#include "led.h"
#include "settings.h"
#include "storage.h"
#include "suspects.h"
#include "utils.h"
#include "web_ui_gz.h"

namespace {

void handleRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.setContentLength(kWebUiGzLen);
  server.send(200, "text/html", "");
  WiFiClient client = server.client();
  client.write_P(reinterpret_cast<const char *>(kWebUiGz), kWebUiGzLen);
}

void handleScanApi() {
  uint32_t now = millis();
  std::vector<DeviceInfo> sorted = lastScan;
  std::sort(sorted.begin(), sorted.end(), [](const DeviceInfo &a, const DeviceInfo &b) {
    return a.rssi > b.rssi;
  });
  String out = "{\"lastScan\":" + String((now - lastScanStartMs) / 1000) +
               ",\"count\":" + String(static_cast<int>(sorted.size())) +
               ",\"devices\":[";
  bool first = true;
  for (const auto &dev : sorted) {
    if (!first) {
      out += ',';
    }
    first = false;
    bool isSuspect = suspects.find(dev.addr) != suspects.end();
    bool isConfirmed = false;
    if (isSuspect) {
      isConfirmed = suspects[dev.addr].confirmed;
    }
    out += "{\"addr\":\"" + jsonEscape(dev.addr) + "\",\"name\":\"" + jsonEscape(dev.name) +
           "\",\"rssi\":" + String(dev.rssi) + ",\"advData\":\"" + jsonEscape(dev.advData) +
           "\",\"suspect\":" + String(isSuspect ? "true" : "false") +
           ",\"confirmed\":" + String(isConfirmed ? "true" : "false") + "}";
  }
  out += "]}";
  server.send(200, "application/json", out);
}

void handleSuspectsApi() {
  std::vector<SuspectEntry> sorted;
  sorted.reserve(suspects.size());
  for (const auto &pair : suspects) {
    sorted.push_back(pair.second);
  }
  std::sort(sorted.begin(), sorted.end(), [](const SuspectEntry &a, const SuspectEntry &b) {
    if (a.device.lastSeenMs != b.device.lastSeenMs) {
      return a.device.lastSeenMs > b.device.lastSeenMs;
    }
    return a.device.rssi > b.device.rssi;
  });

  String out = "{\"suspects\":[";
  bool first = true;
  for (const auto &entry : sorted) {
    if (!first) {
      out += ',';
    }
    first = false;
    out += "{\"addr\":\"" + jsonEscape(entry.device.addr) + "\",\"name\":\"" +
           jsonEscape(entry.device.name) + "\",\"count\":" + String(entry.count) +
           ",\"confirmed\":" + String(entry.confirmed ? "true" : "false") + ",\"notes\":\"" +
           jsonEscape(entry.notes) + "\"}";
  }
  out += "]}";
  server.send(200, "application/json", out);
}

void handleFlagAll() {
  flagAllInRange();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleFlagOne() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }
  String body = server.arg("plain");
  auto extract = [&](const String &key) -> String {
    int idx = body.indexOf("\"" + key + "\"");
    if (idx == -1) {
      return "";
    }
    int colon = body.indexOf(':', idx);
    if (colon == -1) {
      return "";
    }
    int start = body.indexOf('\"', colon + 1);
    if (start == -1) {
      return "";
    }
    int end = body.indexOf('\"', start + 1);
    if (end == -1) {
      return "";
    }
    return body.substring(start + 1, end);
  };

  String addr = extract("addr");
  if (addr.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"addr required\"}");
    return;
  }
  if (isWhitelisted(addr)) {
    server.send(200, "application/json", "{\"status\":\"whitelisted\"}");
    return;
  }

  DeviceInfo *found = nullptr;
  for (auto &dev : lastScan) {
    if (dev.addr == addr) {
      found = &dev;
      break;
    }
  }
  if (!found) {
    server.send(404, "application/json", "{\"error\":\"device not in scan\"}");
    return;
  }

  auto it = suspects.find(addr);
  if (it == suspects.end()) {
    SuspectEntry entry;
    entry.device = *found;
    entry.count = 1;
    suspects[addr] = entry;
  } else {
    it->second.count++;
    it->second.device = *found;
  }
  saveSuspects();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSuspectAction() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }
  String body = server.arg("plain");
  String addr;
  String action;
  String notes;

  auto extract = [&](const String &key) -> String {
    int idx = body.indexOf("\"" + key + "\"");
    if (idx == -1) {
      return "";
    }
    int colon = body.indexOf(':', idx);
    if (colon == -1) {
      return "";
    }
    int start = body.indexOf('"', colon + 1);
    if (start == -1) {
      return "";
    }
    int end = body.indexOf('"', start + 1);
    if (end == -1) {
      return "";
    }
    return body.substring(start + 1, end);
  };

  addr = extract("addr");
  action = extract("action");
  notes = extract("notes");

  if (addr.length() == 0 || action.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"invalid payload\"}");
    return;
  }

  auto it = suspects.find(addr);
  if (action == "confirm") {
    if (it != suspects.end()) {
      it->second.confirmed = true;
    }
  } else if (action == "remove") {
    if (it != suspects.end()) {
      suspects.erase(it);
    }
  } else if (action == "whitelist") {
    if (it != suspects.end()) {
      suspects.erase(it);
    }
    whitelist[addr] = millis();
    saveWhitelist();
  } else if (action == "notes") {
    if (it != suspects.end()) {
      it->second.notes = notes;
    }
  }

  saveSuspects();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

std::vector<String> parseCsvLine(const String &line) {
  std::vector<String> fields;
  String current;
  bool inQuotes = false;
  for (size_t i = 0; i < line.length(); ++i) {
    char c = line[i];
    if (inQuotes) {
      if (c == '"' && i + 1 < line.length() && line[i + 1] == '"') {
        current += '"';
        i++;
      } else if (c == '"') {
        inQuotes = false;
      } else {
        current += c;
      }
    } else {
      if (c == '"') {
        inQuotes = true;
      } else if (c == ',') {
        fields.push_back(current);
        current = "";
      } else {
        current += c;
      }
    }
  }
  fields.push_back(current);
  return fields;
}

void handleImportCsv() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }
  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"empty csv\"}");
    return;
  }

  uint32_t merged = 0;
  uint32_t added = 0;
  uint32_t skipped = 0;

  int lineStart = 0;
  bool firstLine = true;
  while (lineStart < body.length()) {
    int lineEnd = body.indexOf('\n', lineStart);
    if (lineEnd == -1) {
      lineEnd = body.length();
    }
    String line = body.substring(lineStart, lineEnd);
    line.trim();
    lineStart = lineEnd + 1;
    if (line.length() == 0) {
      continue;
    }

    if (firstLine) {
      firstLine = false;
      String lower = line;
      lower.toLowerCase();
      if (lower.startsWith("addr,")) {
        continue;
      }
    }

    std::vector<String> fields = parseCsvLine(line);
    if (fields.size() < 7) {
      skipped++;
      continue;
    }

    String addr = fields[0];
    addr.trim();
    if (addr.length() == 0 || isWhitelisted(addr)) {
      skipped++;
      continue;
    }

    String name = fields[1];
    int rssi = fields[2].toInt();
    String adv = fields[3];
    uint32_t count = static_cast<uint32_t>(fields[4].toInt());
    bool confirmed = fields[5].equalsIgnoreCase("true") || fields[5] == "1";
    String notes = fields[6];

    auto it = suspects.find(addr);
    if (it == suspects.end()) {
      SuspectEntry entry;
      entry.device.addr = addr;
      entry.device.name = name;
      entry.device.rssi = rssi;
      entry.device.advData = adv;
      entry.device.lastSeenMs = 0;
      entry.count = count;
      entry.confirmed = confirmed;
      entry.notes = notes;
      suspects[addr] = entry;
      added++;
    } else {
      SuspectEntry &entry = it->second;
      entry.confirmed = entry.confirmed || confirmed;
      entry.count += count;
      if (entry.device.name.length() == 0 && name.length()) {
        entry.device.name = name;
      }
      if (entry.device.advData.length() == 0 && adv.length()) {
        entry.device.advData = adv;
      }
      if (rssi != 0) {
        entry.device.rssi = max(entry.device.rssi, rssi);
      }
      if (notes.length()) {
        if (entry.notes.length()) {
          entry.notes += "\n";
        }
        entry.notes += notes;
      }
      merged++;
    }
  }

  saveSuspects();
  String message = "Import complete. Added " + String(added) + ", merged " + String(merged) +
                   ", skipped " + String(skipped) + ".";
  server.send(200, "application/json", "{\"message\":\"" + jsonEscape(message) + "\"}");
}

void handleSettingsGet() {
  String out = "{\"ssid\":\"" + jsonEscape(apSsid) + "\",\"pass\":\"" + jsonEscape(apPass) +
               ",\"ledMode\":" + String(ledMode) +
               ",\"ledPin\":" + String(ledPin) +
               ",\"ledPinR\":" + String(ledPinR) +
               ",\"ledPinG\":" + String(ledPinG) +
               ",\"ledPinB\":" + String(ledPinB) +
               ",\"ledActiveLow\":" + String(ledActiveLow ? "true" : "false") +
               ",\"ssidHidden\":" + String(ssidHidden ? "true" : "false") +
               "}";
  server.send(200, "application/json", out);
}

void handleSettingsPost() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }
  String body = server.arg("plain");
  uint8_t prevLedMode = ledMode;
  uint8_t prevLedPin = ledPin;
  uint8_t prevLedPinR = ledPinR;
  uint8_t prevLedPinG = ledPinG;
  uint8_t prevLedPinB = ledPinB;
  bool prevLedActiveLow = ledActiveLow;
  bool prevSsidHidden = ssidHidden;

  auto extract = [&](const String &key) -> String {
    int idx = body.indexOf("\"" + key + "\"");
    if (idx == -1) {
      return "";
    }
    int colon = body.indexOf(':', idx);
    if (colon == -1) {
      return "";
    }
    int start = body.indexOf('"', colon + 1);
    if (start == -1) {
      return "";
    }
    int end = body.indexOf('"', start + 1);
    if (end == -1) {
      return "";
    }
    return body.substring(start + 1, end);
  };

  auto extractInt = [&](const String &key, int fallback) -> int {
    int idx = body.indexOf("\"" + key + "\"");
    if (idx == -1) {
      return fallback;
    }
    int colon = body.indexOf(':', idx);
    if (colon == -1) {
      return fallback;
    }
    int start = colon + 1;
    while (start < body.length() && (body[start] == ' ')) {
      start++;
    }
    int end = start;
    while (end < body.length() && (body[end] == '-' || (body[end] >= '0' && body[end] <= '9'))) {
      end++;
    }
    return body.substring(start, end).toInt();
  };

  String newSsid = extract("ssid");
  String newPass = extract("pass");
  int newLedMode = extractInt("ledMode", ledMode);
  int newLedPin = extractInt("ledPin", ledPin);
  int newLedPinR = extractInt("ledPinR", ledPinR);
  int newLedPinG = extractInt("ledPinG", ledPinG);
  int newLedPinB = extractInt("ledPinB", ledPinB);
  bool newLedActiveLow = body.indexOf("\"ledActiveLow\":true") != -1;
  bool newSsidHidden = body.indexOf("\"ssidHidden\":true") != -1;
  if (newSsid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"ssid required\"}");
    return;
  }
  if (newPass.length() < 8) {
    server.send(400, "application/json", "{\"error\":\"password must be 8+ chars\"}");
    return;
  }

  apSsid = newSsid;
  apPass = newPass;
  ledMode = static_cast<uint8_t>(newLedMode);
  ledPin = static_cast<uint8_t>(newLedPin);
  ledPinR = static_cast<uint8_t>(newLedPinR);
  ledPinG = static_cast<uint8_t>(newLedPinG);
  ledPinB = static_cast<uint8_t>(newLedPinB);
  ledActiveLow = newLedActiveLow;
  ssidHidden = newSsidHidden;
  prefs.putString(AppConfig::kPrefsSsidKey, apSsid);
  prefs.putString(AppConfig::kPrefsPassKey, apPass);
  prefs.putUChar(AppConfig::kPrefsLedModeKey, ledMode);
  prefs.putUChar(AppConfig::kPrefsLedPinKey, ledPin);
  prefs.putUChar(AppConfig::kPrefsLedPinRKey, ledPinR);
  prefs.putUChar(AppConfig::kPrefsLedPinGKey, ledPinG);
  prefs.putUChar(AppConfig::kPrefsLedPinBKey, ledPinB);
  prefs.putBool(AppConfig::kPrefsLedActiveLowKey, ledActiveLow);
  prefs.putBool(AppConfig::kPrefsSsidHiddenKey, ssidHidden);
  applyApSettings();
  if (prevLedMode != ledMode || prevLedPin != ledPin || prevLedPinR != ledPinR ||
      prevLedPinG != ledPinG || prevLedPinB != ledPinB || prevLedActiveLow != ledActiveLow) {
    applyLedConfig();
  }

  String message = "Settings saved. Reconnect to the new SSID.";
  server.send(200, "application/json", "{\"message\":\"" + jsonEscape(message) + "\"}");
}

void handleReboot() {
  server.send(200, "application/json", "{\"message\":\"Rebooting...\"}");
  delay(250);
  ESP.restart();
}

void handleCsvDownload() {
  String out = "addr,name,rssi,adv_data,count,confirmed,notes\n";
  for (const auto &pair : suspects) {
    const auto &entry = pair.second;
    out += csvEscape(entry.device.addr) + ",";
    out += csvEscape(entry.device.name) + ",";
    out += String(entry.device.rssi) + ",";
    out += csvEscape(entry.device.advData) + ",";
    out += String(entry.count) + ",";
    out += (entry.confirmed ? "true" : "false");
    out += ",";
    out += csvEscape(entry.notes);
    out += "\n";
  }
  server.send(200, "text/csv", out);
}

} // namespace

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/scan", HTTP_GET, handleScanApi);
  server.on("/api/suspects", HTTP_GET, handleSuspectsApi);
  server.on("/api/flag", HTTP_POST, handleFlagAll);
  server.on("/api/flag_one", HTTP_POST, handleFlagOne);
  server.on("/api/suspect/action", HTTP_POST, handleSuspectAction);
  server.on("/api/import", HTTP_POST, handleImportCsv);
  server.on("/api/settings", HTTP_GET, handleSettingsGet);
  server.on("/api/settings", HTTP_POST, handleSettingsPost);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/download/suspects.csv", HTTP_GET, handleCsvDownload);
  server.begin();
}

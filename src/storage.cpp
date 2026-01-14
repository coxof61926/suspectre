#include "storage.h"

#include <vector>

#include "app_config.h"
#include "app_state.h"
#include "utils.h"

bool isWhitelisted(const String &addr) {
  return whitelist.find(addr) != whitelist.end();
}

void loadWhitelist() {
  whitelist.clear();
  String serialized = prefs.getString(AppConfig::kPrefsWhitelistKey, "");
  if (serialized.length() == 0) {
    return;
  }
  int start = 0;
  while (start < serialized.length()) {
    int comma = serialized.indexOf(',', start);
    if (comma == -1) {
      comma = serialized.length();
    }
    String addr = serialized.substring(start, comma);
    addr.trim();
    if (addr.length()) {
      whitelist[addr] = millis();
    }
    start = comma + 1;
  }
}

void saveWhitelist() {
  String serialized;
  for (const auto &entry : whitelist) {
    if (serialized.length()) {
      serialized += ',';
    }
    serialized += entry.first;
  }
  prefs.putString(AppConfig::kPrefsWhitelistKey, serialized);
}

void saveSuspects() {
  String serialized;
  for (const auto &pair : suspects) {
    const auto &entry = pair.second;
    if (serialized.length()) {
      serialized += '\n';
    }
    serialized += percentEscape(entry.device.addr);
    serialized += '|';
    serialized += percentEscape(entry.device.name);
    serialized += '|';
    serialized += String(entry.device.rssi);
    serialized += '|';
    serialized += percentEscape(entry.device.advData);
    serialized += '|';
    serialized += String(entry.count);
    serialized += '|';
    serialized += entry.confirmed ? "1" : "0";
    serialized += '|';
    serialized += percentEscape(entry.notes);
  }
  prefs.putString(AppConfig::kPrefsSuspectsKey, serialized);
}

void loadSuspects() {
  suspects.clear();
  String serialized = prefs.getString(AppConfig::kPrefsSuspectsKey, "");
  if (serialized.length() == 0) {
    return;
  }
  int lineStart = 0;
  while (lineStart < serialized.length()) {
    int lineEnd = serialized.indexOf('\n', lineStart);
    if (lineEnd == -1) {
      lineEnd = serialized.length();
    }
    String line = serialized.substring(lineStart, lineEnd);
    lineStart = lineEnd + 1;
    if (line.length() == 0) {
      continue;
    }

    std::vector<String> fields;
    int fieldStart = 0;
    while (fieldStart <= line.length()) {
      int sep = line.indexOf('|', fieldStart);
      if (sep == -1) {
        sep = line.length();
      }
      fields.push_back(line.substring(fieldStart, sep));
      fieldStart = sep + 1;
      if (sep >= line.length()) {
        break;
      }
    }
    if (fields.size() < 7) {
      continue;
    }

    DeviceInfo info;
    info.addr = percentUnescape(fields[0]);
    if (info.addr.length() == 0) {
      continue;
    }
    if (isWhitelisted(info.addr)) {
      continue;
    }
    info.name = percentUnescape(fields[1]);
    info.rssi = fields[2].toInt();
    info.advData = percentUnescape(fields[3]);
    info.lastSeenMs = 0;

    SuspectEntry entry;
    entry.device = info;
    entry.count = fields[4].toInt();
    entry.confirmed = fields[5] == "1";
    entry.notes = percentUnescape(fields[6]);
    suspects[info.addr] = entry;
  }
}

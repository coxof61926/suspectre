#include "suspects.h"

#include "app_state.h"
#include "storage.h"

void flagAllInRange() {
  for (const auto &dev : lastScan) {
    if (isWhitelisted(dev.addr)) {
      continue;
    }
    auto it = suspects.find(dev.addr);
    if (it == suspects.end()) {
      SuspectEntry entry;
      entry.device = dev;
      entry.count = 1;
      suspects[dev.addr] = entry;
    } else {
      it->second.count++;
      it->second.device = dev;
    }
  }
  saveSuspects();
}

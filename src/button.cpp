#include "button.h"

#include <Arduino.h>

#include "app_config.h"
#include "suspects.h"

namespace {
bool lastButtonState = true;
uint32_t lastButtonChangeMs = 0;
} // namespace

void handleButton() {
  bool state = digitalRead(AppConfig::kButtonPin);
  uint32_t now = millis();
  if (state != lastButtonState && (now - lastButtonChangeMs) > 40) {
    lastButtonChangeMs = now;
    lastButtonState = state;
    if (!state) {
      flagAllInRange();
    }
  }
}

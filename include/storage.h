#pragma once

#include <Arduino.h>

bool isWhitelisted(const String &addr);
void loadWhitelist();
void saveWhitelist();
void loadSuspects();
void saveSuspects();

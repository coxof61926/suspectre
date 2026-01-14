#pragma once

#include <Arduino.h>

String toHexString(const std::string &data);
String jsonEscape(const String &in);
String csvEscape(const String &in);
String percentEscape(const String &in);
String percentUnescape(const String &in);

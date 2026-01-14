#include "utils.h"

String toHexString(const std::string &data) {
  static const char kHex[] = "0123456789ABCDEF";
  String out;
  out.reserve(data.size() * 2);
  for (uint8_t c : data) {
    out += kHex[(c >> 4) & 0x0F];
    out += kHex[c & 0x0F];
  }
  return out;
}

String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

String csvEscape(const String &in) {
  String out = "\"";
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '\"') {
      out += "\"\"";
    } else {
      out += c;
    }
  }
  out += "\"";
  return out;
}

String percentEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '%' || c == '|' || c == '\n' || c == '\r') {
      char buf[4];
      snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned char>(c));
      out += '%';
      out += buf;
    } else {
      out += c;
    }
  }
  return out;
}

String percentUnescape(const String &in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == '%' && i + 2 < in.length()) {
      char hex[3] = { in[i + 1], in[i + 2], 0 };
      char *end = nullptr;
      long val = strtol(hex, &end, 16);
      if (end && *end == 0) {
        out += static_cast<char>(val);
        i += 2;
        continue;
      }
    }
    out += c;
  }
  return out;
}

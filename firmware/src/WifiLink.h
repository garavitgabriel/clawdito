#pragma once
#include <Arduino.h>

// WiFi station link with auto-reconnect and a persistent-failure signal
// (used to fall back into the setup portal when credentials are wrong or
// the network is gone).

namespace wifilink {

void begin(const String& ssid, const String& pass);
bool connected();
bool failed_hard();          // several failures in a short window
String ip();

}  // namespace wifilink

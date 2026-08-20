#pragma once
#include <Arduino.h>

// First-boot provisioning: opens a WPA2 access point named Clawdito-XXXX
// with a one-time password, serves a small config form at 192.168.4.1,
// and reboots into normal mode after saving.

namespace portal {

void start();                 // switch to AP mode and serve the form
void loop();                  // service DNS + HTTP; call from loop()
bool active();
String ap_name();
String ap_password();

}  // namespace portal

#pragma once
#include <Arduino.h>

// Runtime configuration persisted to NVS. Provisioned via the setup portal;
// wiped by a very-long BOOT press.

struct AppConfig {
  String   wifi_ssid;
  String   wifi_pass;
  String   bridge_host;
  uint16_t bridge_port = 8787;
  String   bridge_token;
  uint32_t poll_ms = 5000;

  bool valid() const { return wifi_ssid.length() > 0 && bridge_host.length() > 0; }
};

namespace config {

AppConfig& get();     // in-RAM copy, loaded once at boot
void load();
void save(const AppConfig& c);
void wipe();          // erase NVS namespace

}  // namespace config

#pragma once
#include <Arduino.h>

// Runtime configuration persisted to NVS. Provisioned via the setup portal;
// wiped by a very-long BOOT press.

static const uint8_t MAX_PROFILES = 2;

// One bridge target: a computer on the LAN running clawdito_bridge.py.
// Two accounts (personal + work) = two profiles the device polls in turn.
struct BridgeProfile {
  String   label;
  String   host;
  uint16_t port = 8787;
  String   token;

  bool valid() const { return host.length() > 0; }
};

struct AppConfig {
  String        wifi_ssid;
  String        wifi_pass;
  BridgeProfile prof[MAX_PROFILES];
  uint32_t      poll_ms = 5000;

  // Configured profiles, counted from index 0 (a gap ends the count).
  uint8_t profile_count() const {
    uint8_t n = 0;
    while (n < MAX_PROFILES && prof[n].valid()) n++;
    return n;
  }

  bool valid() const { return wifi_ssid.length() > 0 && prof[0].valid(); }
};

namespace config {

AppConfig& get();     // in-RAM copy, loaded once at boot
void load();
void save(const AppConfig& c);
void wipe();          // erase NVS namespace

}  // namespace config

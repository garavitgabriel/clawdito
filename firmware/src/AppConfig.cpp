#include "AppConfig.h"
#include <Preferences.h>

static const char* NS = "clawdito";
static AppConfig s_cfg;

// Per-profile NVS keys are suffixed with the profile index.
static String key(const char* base, uint8_t i) { return String(base) + String(i); }

namespace config {

AppConfig& get() { return s_cfg; }

void load() {
  Preferences p;
  if (!p.begin(NS, /*readOnly=*/true)) {
    // namespace doesn't exist yet on first boot — leave defaults
    return;
  }
  s_cfg.wifi_ssid = p.getString("ssid", "");
  s_cfg.wifi_pass = p.getString("pass", "");
  s_cfg.poll_ms   = p.getULong("poll", 5000);
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    s_cfg.prof[i].label = p.getString(key("lbl", i).c_str(), "");
    s_cfg.prof[i].host  = p.getString(key("host", i).c_str(), "");
    s_cfg.prof[i].port  = p.getUShort(key("port", i).c_str(), 8787);
    s_cfg.prof[i].token = p.getString(key("tok", i).c_str(), "");
  }
  // v1.0 stored a single flat bridge target; adopt it as profile 0 so an
  // already-provisioned device survives the update without re-provisioning.
  String legacy_host = p.getString("host", "");
  bool migrate = !s_cfg.prof[0].valid() && legacy_host.length() > 0;
  if (migrate) {
    s_cfg.prof[0].label = "default";
    s_cfg.prof[0].host  = legacy_host;
    s_cfg.prof[0].port  = p.getUShort("port", 8787);
    s_cfg.prof[0].token = p.getString("token", "");
  }
  p.end();

  if (migrate) {
    Serial.println("[config] migrating legacy bridge target to profile 0");
    save(s_cfg);
    p.begin(NS, /*readOnly=*/false);
    p.remove("host");
    p.remove("port");
    p.remove("token");
    p.end();
  }
}

void save(const AppConfig& c) {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.putString("ssid", c.wifi_ssid);
  p.putString("pass", c.wifi_pass);
  p.putULong("poll", c.poll_ms);
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    p.putString(key("lbl", i).c_str(), c.prof[i].label);
    p.putString(key("host", i).c_str(), c.prof[i].host);
    p.putUShort(key("port", i).c_str(), c.prof[i].port);
    p.putString(key("tok", i).c_str(), c.prof[i].token);
  }
  p.end();
  s_cfg = c;
}

void wipe() {
  Preferences p;
  p.begin(NS, false);
  p.clear();
  p.end();
  s_cfg = AppConfig{};
}

}  // namespace config

#include "AppConfig.h"
#include <Preferences.h>

static const char* NS = "clawdito";
static AppConfig s_cfg;

namespace config {

AppConfig& get() { return s_cfg; }

void load() {
  Preferences p;
  if (!p.begin(NS, /*readOnly=*/true)) {
    // namespace doesn't exist yet on first boot — leave defaults
    return;
  }
  s_cfg.wifi_ssid    = p.getString("ssid", "");
  s_cfg.wifi_pass    = p.getString("pass", "");
  s_cfg.bridge_host  = p.getString("host", "");
  s_cfg.bridge_port  = p.getUShort("port", 8787);
  s_cfg.bridge_token = p.getString("token", "");
  s_cfg.poll_ms      = p.getULong("poll", 5000);
  p.end();
}

void save(const AppConfig& c) {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.putString("ssid", c.wifi_ssid);
  p.putString("pass", c.wifi_pass);
  p.putString("host", c.bridge_host);
  p.putUShort("port", c.bridge_port);
  p.putString("token", c.bridge_token);
  p.putULong("poll", c.poll_ms);
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

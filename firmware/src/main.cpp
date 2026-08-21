#include <Arduino.h>
#include "DisplayJD9853.h"
#include "TouchAXS5106.h"
#include "LvglGlue.h"
#include "Ui.h"
#include "AppConfig.h"
#include "WifiLink.h"
#include "BridgeClient.h"
#include "SetupPortal.h"
#include "BootButton.h"

// Clawdito — a little desk companion that shows your Claude Code usage.
// Boot flow: splash -> (configured? join WiFi + poll bridge : setup portal).

static const uint32_t SPLASH_MS = 1500;
static const uint32_t UI_REFRESH_MS = 400;

static bool s_running = false;      // main pages visible / bridge polling
static uint32_t s_boot_ms = 0;
static uint32_t s_last_refresh = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Clawdito v1.1 ===");

  display::init();
  touch::init();
  lvgl_glue::init();
  ui::init();                       // splash showing
  button::init();
  config::load();
  s_boot_ms = millis();
}

static void enter_portal() {
  portal::start();
  ui::show_portal(portal::ap_name(), portal::ap_password());
  s_running = false;
}

static void leave_splash_once() {
  static bool done = false;
  if (done || millis() - s_boot_ms < SPLASH_MS) return;
  done = true;

  const AppConfig& c = config::get();
  if (!c.valid()) {
    Serial.println("[boot] no config — entering setup portal");
    enter_portal();
    return;
  }
  wifilink::begin(c.wifi_ssid, c.wifi_pass);
  bridge::begin(c);

  String labels[MAX_PROFILES];
  uint8_t n = c.profile_count();
  for (uint8_t i = 0; i < n; i++) {
    labels[i] = c.prof[i].label.length() ? c.prof[i].label : String("bridge");
  }
  ui::set_profiles(labels, n);
  ui::show_main();
  s_running = true;
}

void loop() {
  lvgl_glue::loop();
  portal::loop();
  leave_splash_once();

  switch (button::poll()) {
    case ButtonEvent::TAP:
      if (s_running) ui::next_page();
      break;
    case ButtonEvent::HOLD_2S:
      if (s_running) ui::switch_profile();
      break;
    case ButtonEvent::HOLD_5S:
      Serial.println("[button] wiping config, back to setup");
      config::wipe();
      ESP.restart();
      break;
    default:
      break;
  }

  if (s_running) {
    // WiFi credentials wrong or network gone: fall back to the portal
    if (wifilink::failed_hard() && !portal::active()) {
      Serial.println("[boot] wifi failed repeatedly — entering setup portal");
      enter_portal();
      return;
    }
    uint32_t now = millis();
    if (now - s_last_refresh >= UI_REFRESH_MS) {
      s_last_refresh = now;
      UsageSnapshot snaps[MAX_PROFILES];
      uint8_t n = bridge::count();
      for (uint8_t i = 0; i < n; i++) bridge::snapshot(i, snaps[i]);
      ui::update(snaps, n);
    }
  }

  delay(2);
}

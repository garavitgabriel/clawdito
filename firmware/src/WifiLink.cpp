#include "WifiLink.h"
#include <WiFi.h>

static volatile bool s_connected = false;
static volatile uint8_t  s_drop_count = 0;
static volatile uint32_t s_first_drop_ms = 0;

static const uint8_t  HARD_FAIL_DROPS = 3;
static const uint32_t HARD_FAIL_WINDOW_MS = 30000;

namespace wifilink {

static void on_event(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      s_connected = true;
      s_drop_count = 0;
      Serial.printf("[wifi] up, ip=%s rssi=%d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      s_connected = false;
      uint32_t now = millis();
      if (s_drop_count == 0 || now - s_first_drop_ms > HARD_FAIL_WINDOW_MS) {
        s_first_drop_ms = now;
        s_drop_count = 1;
      } else {
        s_drop_count++;
      }
      Serial.println("[wifi] down, reconnecting");
      WiFi.reconnect();
      break;
    }
    default:
      break;
  }
}

void begin(const String& ssid, const String& pass) {
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(on_event);
  WiFi.setAutoReconnect(true);
  Serial.printf("[wifi] connecting to \"%s\"\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());
}

bool connected() { return s_connected; }

bool failed_hard() {
  return !s_connected && s_drop_count >= HARD_FAIL_DROPS &&
         millis() - s_first_drop_ms <= HARD_FAIL_WINDOW_MS + 5000;
}

String ip() {
  return s_connected ? WiFi.localIP().toString() : String("0.0.0.0");
}

}  // namespace wifilink

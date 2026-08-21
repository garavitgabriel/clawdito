#include "BridgeClient.h"
#include "WifiLink.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static UsageSnapshot s_snap[MAX_PROFILES];
static SemaphoreHandle_t s_lock = nullptr;

static String   s_url[MAX_PROFILES];
static String   s_auth[MAX_PROFILES];
static uint8_t  s_count = 0;
static uint32_t s_poll_ms = 5000;

// Refuse to parse absurdly large responses (defense against a misbehaving
// endpoint filling device RAM); normal payloads are ~1KB.
static const int MAX_BODY = 16 * 1024;

namespace bridge {

static bool poll_once(uint8_t idx, UsageSnapshot& s) {
  if (!wifilink::connected()) return false;

  HTTPClient http;
  http.setConnectTimeout(2000);
  http.setTimeout(3000);
  if (!http.begin(s_url[idx])) return false;
  if (s_auth[idx].length()) http.addHeader("Authorization", s_auth[idx]);

  int code = http.GET();
  if (code != 200) {
    if (code == 401) Serial.println("[bridge] 401 — token rejected");
    else             Serial.printf("[bridge] http %d\n", code);
    http.end();
    return false;
  }
  if (http.getSize() > MAX_BODY) {
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    Serial.printf("[bridge] json: %s\n", err.c_str());
    return false;
  }

  JsonObject lim = doc["limits"].as<JsonObject>();
  s.limits_ok = lim["ok"] | false;
  if (s.limits_ok) {
    uint32_t p5 = lim["five_hour"]["pct"] | 0u;
    uint32_t p7 = lim["seven_day"]["pct"] | 0u;
    s.pct_5h = p5 > 100 ? 100 : (uint8_t)p5;
    s.pct_7d = p7 > 100 ? 100 : (uint8_t)p7;
    s.reset_5h_s = lim["five_hour"]["resets_in_s"] | 0u;
    s.reset_7d_s = lim["seven_day"]["resets_in_s"] | 0u;
  }

  s.today_usd = doc["today"]["cost_usd"] | 0.0f;
  s.month_usd = doc["month"]["cost_usd"] | 0.0f;

  JsonArray last7 = doc["last7"].as<JsonArray>();
  for (uint8_t i = 0; i < 7; i++) {
    s.last7_usd[i] = 0;
    s.last7_day[i][0] = 0;
    if (i < last7.size()) {
      s.last7_usd[i] = last7[i]["cost_usd"] | 0.0f;
      const char* date = last7[i]["date"] | "";
      size_t n = strlen(date);
      if (n >= 2) {                       // keep the day-of-month digits
        s.last7_day[i][0] = date[n - 2];
        s.last7_day[i][1] = date[n - 1];
        s.last7_day[i][2] = 0;
      }
    }
  }
  return true;
}

// One task, profiles polled back-to-back: two concurrent HTTPClients would
// cost more RAM than a second account is worth.
static void task(void*) {
  for (;;) {
    for (uint8_t i = 0; i < s_count; i++) {
      UsageSnapshot fresh{};
      bool ok = poll_once(i, fresh);
      if (xSemaphoreTake(s_lock, portMAX_DELAY) == pdTRUE) {
        if (ok) {
          fresh.online = true;
          fresh.polls = s_snap[i].polls + 1;
          s_snap[i] = fresh;
        } else {
          s_snap[i].online = false;
        }
        xSemaphoreGive(s_lock);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(s_poll_ms));
  }
}

void begin(const AppConfig& c) {
  s_count = c.profile_count();
  for (uint8_t i = 0; i < s_count; i++) {
    const BridgeProfile& p = c.prof[i];
    s_url[i] = "http://" + p.host + ":" + String(p.port) + "/usage";
    s_auth[i] = p.token.length() ? ("Bearer " + p.token) : "";
  }
  s_poll_ms = c.poll_ms;
  if (!s_count) return;
  if (!s_lock) s_lock = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(task, "bridge_poll", 8192, nullptr, 1, nullptr, 0);
  for (uint8_t i = 0; i < s_count; i++) {
    Serial.printf("[bridge] polling %s every %lums\n", s_url[i].c_str(),
                  (unsigned long)s_poll_ms);
  }
}

void snapshot(uint8_t idx, UsageSnapshot& out) {
  if (idx >= s_count) return;
  if (s_lock && xSemaphoreTake(s_lock, pdMS_TO_TICKS(20)) == pdTRUE) {
    out = s_snap[idx];
    xSemaphoreGive(s_lock);
  }
}

uint8_t count() { return s_count; }

}  // namespace bridge

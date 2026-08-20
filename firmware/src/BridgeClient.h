#pragma once
#include <Arduino.h>

// Polls the Clawdito bridge over HTTP and keeps the latest snapshot in a
// mutex-guarded struct the UI thread can copy at any time.

struct UsageSnapshot {
  bool     online = false;        // last poll succeeded
  uint32_t polls  = 0;

  // Official Claude rate limits
  bool     limits_ok = false;
  uint8_t  pct_5h = 0;            // 0..100
  uint32_t reset_5h_s = 0;        // seconds until the 5h window resets
  uint8_t  pct_7d = 0;
  uint32_t reset_7d_s = 0;

  // API-equivalent spend from local transcripts
  float today_usd = 0;
  float month_usd = 0;
  float last7_usd[7] = {0};       // [0] = 6 days ago ... [6] = today
  char  last7_day[7][3] = {{0}};  // day-of-month labels, e.g. "19"
};

namespace bridge {

void begin(const String& host, uint16_t port, const String& token,
           uint32_t poll_ms);
void snapshot(UsageSnapshot& out);

}  // namespace bridge

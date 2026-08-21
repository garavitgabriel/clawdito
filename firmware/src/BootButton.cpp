#include "BootButton.h"

static const int      PIN = 0;
static const uint32_t DEBOUNCE_MS = 30;
static const uint32_t TAP_MAX_MS = 500;
static const uint32_t HOLD2_MS = 2000;
static const uint32_t HOLD_MS = 5000;

static bool     s_stable = true;      // pull-up: true = released
static bool     s_raw_last = true;
static uint32_t s_raw_since = 0;
static uint32_t s_press_at = 0;
static bool     s_hold_sent = false;

namespace button {

void init() {
  pinMode(PIN, INPUT_PULLUP);
  s_stable = s_raw_last = digitalRead(PIN);
}

ButtonEvent poll() {
  uint32_t now = millis();
  bool raw = digitalRead(PIN);
  if (raw != s_raw_last) {
    s_raw_last = raw;
    s_raw_since = now;
    return ButtonEvent::NONE;
  }
  if (raw == s_stable) {
    // held down long enough? fire HOLD once while still pressed
    if (!s_stable && !s_hold_sent && now - s_press_at >= HOLD_MS) {
      s_hold_sent = true;
      return ButtonEvent::HOLD_5S;
    }
    return ButtonEvent::NONE;
  }
  if (now - s_raw_since < DEBOUNCE_MS) return ButtonEvent::NONE;

  s_stable = raw;
  if (!s_stable) {                    // just pressed
    s_press_at = now;
    s_hold_sent = false;
    return ButtonEvent::NONE;
  }
  // just released. HOLD_2S is classified here, not during the hold: firing it
  // mid-press would trip on the way to the 5s wipe.
  if (s_hold_sent) return ButtonEvent::NONE;
  uint32_t held = now - s_press_at;
  if (held <= TAP_MAX_MS) return ButtonEvent::TAP;
  if (held >= HOLD2_MS)   return ButtonEvent::HOLD_2S;
  return ButtonEvent::NONE;
}

}  // namespace button

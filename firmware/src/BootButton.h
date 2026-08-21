#pragma once
#include <Arduino.h>

// Debounced BOOT button (GPIO 0). Polled, never via ISR — GPIO 0 doubles
// as the ESP32-S3 download-mode strap pin.

enum class ButtonEvent { NONE, TAP, HOLD_2S, HOLD_5S };

namespace button {

void init();
ButtonEvent poll();   // call from loop()

}  // namespace button

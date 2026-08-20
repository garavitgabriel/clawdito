#pragma once
#include <Arduino.h>

// AXS5106L capacitive touch (I2C 0x63) on the Waveshare
// ESP32-S3-Touch-LCD-1.47. Reports landscape coordinates matching the
// display orientation (320x172).

namespace touch {

void init();
bool read(uint16_t* x, uint16_t* y);   // true while a finger is down

}  // namespace touch

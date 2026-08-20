#pragma once
#include <Arduino.h>

// JD9853 172x320 IPS panel driven in landscape (320x172) over SPI.
// Pin map: Waveshare ESP32-S3-Touch-LCD-1.47.

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  172

namespace display {

void init();                     // SPI + panel init + backlight on
void blit(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
          const uint16_t* pixels);           // push a rectangle of RGB565
void set_brightness(uint8_t percent);        // 0..100

}  // namespace display

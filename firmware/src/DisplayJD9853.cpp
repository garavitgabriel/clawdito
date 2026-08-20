#include "DisplayJD9853.h"
#include <SPI.h>

// ----- Wiring (Waveshare ESP32-S3-Touch-LCD-1.47) -----
static const int PIN_MOSI = 39;
static const int PIN_SCLK = 38;
static const int PIN_CS   = 21;
static const int PIN_DC   = 45;
static const int PIN_RST  = 40;
static const int PIN_BL   = 46;   // backlight, PWM

static const uint32_t SPI_HZ     = 40000000;
static const uint32_t BL_PWM_HZ  = 1000;
static const uint8_t  BL_PWM_RES = 10;      // bits

// The panel RAM is 172 columns (offset 34) x 320 rows. In landscape
// (MADCTL MV|MX) the X axis runs along the 320 rows (no offset) and the
// Y axis along the columns (+34).
static const uint16_t OFF_X = 0;
static const uint16_t OFF_Y = 34;

static SPIClass s_spi(FSPI);

namespace display {

static void cmd(uint8_t c) {
  s_spi.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_DC, LOW);
  s_spi.transfer(c);
  digitalWrite(PIN_CS, HIGH);
  s_spi.endTransaction();
}

static void dat(uint8_t d) {
  s_spi.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_DC, HIGH);
  s_spi.transfer(d);
  digitalWrite(PIN_CS, HIGH);
  s_spi.endTransaction();
}

// Send a command followed by n data bytes from a flat table.
static void cmd_n(uint8_t c, const uint8_t* d, size_t n) {
  cmd(c);
  for (size_t i = 0; i < n; i++) dat(d[i]);
}

// JD9853 power-up register sequence (values per the panel vendor's
// reference init for this 172x320 module), ending in landscape mode.
static void panel_init() {
  cmd(0x11);                                  // sleep out
  delay(120);

  static const uint8_t r_df[] = {0x98, 0x53};                    // unlock
  static const uint8_t r_b2[] = {0x23};
  static const uint8_t r_b7a[] = {0x00, 0x47, 0x00, 0x6F};
  static const uint8_t r_bb[] = {0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0};
  static const uint8_t r_c0[] = {0x44, 0xA4};
  static const uint8_t r_c1a[] = {0x16};
  static const uint8_t r_c3[] = {0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77};
  static const uint8_t r_c4a[] = {0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16,
                                  0x79, 0x0B, 0x0A, 0x16, 0x82};
  static const uint8_t r_c8[] = {                                 // gamma
      0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17,
      0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28,
      0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00};
  static const uint8_t r_d0[] = {0x04, 0x06, 0x6B, 0x0F, 0x00};
  static const uint8_t r_d7[] = {0x00, 0x30};
  static const uint8_t r_e6[] = {0x14};
  static const uint8_t r_de1[] = {0x01};
  static const uint8_t r_b7b[] = {0x03, 0x13, 0xEF, 0x35, 0x35};
  static const uint8_t r_c1b[] = {0x14, 0x15, 0xC0};
  static const uint8_t r_c2[] = {0x06, 0x3A};
  static const uint8_t r_c4b[] = {0x72, 0x12};
  static const uint8_t r_be[] = {0x00};
  static const uint8_t r_de2[] = {0x02};
  static const uint8_t r_e5a[] = {0x00, 0x02, 0x00};
  static const uint8_t r_e5b[] = {0x01, 0x02, 0x00};
  static const uint8_t r_de0[] = {0x00};
  static const uint8_t r_35[] = {0x00};                          // TE on
  static const uint8_t r_3a[] = {0x05};                          // RGB565
  static const uint8_t r_2a[] = {0x00, 0x22, 0x00, 0xCD};        // col 34..205
  static const uint8_t r_2b[] = {0x00, 0x00, 0x01, 0x3F};        // row 0..319
  static const uint8_t r_36[] = {0x60};                          // MV|MX landscape

  cmd_n(0xDF, r_df, sizeof(r_df));
  cmd_n(0xB2, r_b2, sizeof(r_b2));
  cmd_n(0xB7, r_b7a, sizeof(r_b7a));
  cmd_n(0xBB, r_bb, sizeof(r_bb));
  cmd_n(0xC0, r_c0, sizeof(r_c0));
  cmd_n(0xC1, r_c1a, sizeof(r_c1a));
  cmd_n(0xC3, r_c3, sizeof(r_c3));
  cmd_n(0xC4, r_c4a, sizeof(r_c4a));
  cmd_n(0xC8, r_c8, sizeof(r_c8));
  cmd_n(0xD0, r_d0, sizeof(r_d0));
  cmd_n(0xD7, r_d7, sizeof(r_d7));
  cmd_n(0xE6, r_e6, sizeof(r_e6));
  cmd_n(0xDE, r_de1, sizeof(r_de1));
  cmd_n(0xB7, r_b7b, sizeof(r_b7b));
  cmd_n(0xC1, r_c1b, sizeof(r_c1b));
  cmd_n(0xC2, r_c2, sizeof(r_c2));
  cmd_n(0xC4, r_c4b, sizeof(r_c4b));
  cmd_n(0xBE, r_be, sizeof(r_be));
  cmd_n(0xDE, r_de2, sizeof(r_de2));
  cmd_n(0xE5, r_e5a, sizeof(r_e5a));
  cmd_n(0xE5, r_e5b, sizeof(r_e5b));
  cmd_n(0xDE, r_de0, sizeof(r_de0));
  cmd_n(0x35, r_35, sizeof(r_35));
  cmd_n(0x3A, r_3a, sizeof(r_3a));
  cmd_n(0x2A, r_2a, sizeof(r_2a));
  cmd_n(0x2B, r_2b, sizeof(r_2b));
  cmd_n(0xDE, r_de2, sizeof(r_de2));
  cmd_n(0xE5, r_e5a, sizeof(r_e5a));
  cmd_n(0xDE, r_de0, sizeof(r_de0));
  cmd_n(0x36, r_36, sizeof(r_36));

  cmd(0x21);                                  // inversion on (IPS)
  delay(10);
  cmd(0x29);                                  // display on
}

void init() {
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_RST, OUTPUT);

  // backlight PWM
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BL, BL_PWM_HZ, BL_PWM_RES);
#else
  ledcSetup(0, BL_PWM_HZ, BL_PWM_RES);
  ledcAttachPin(PIN_BL, 0);
#endif
  set_brightness(90);

  s_spi.begin(PIN_SCLK, -1, PIN_MOSI);

  // hardware reset
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_RST, LOW);
  delay(50);
  digitalWrite(PIN_RST, HIGH);
  delay(50);

  panel_init();
}

static void set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint16_t xs = x1 + OFF_X, xe = x2 + OFF_X;
  uint16_t ys = y1 + OFF_Y, ye = y2 + OFF_Y;
  cmd(0x2A);
  dat(xs >> 8); dat(xs & 0xFF); dat(xe >> 8); dat(xe & 0xFF);
  cmd(0x2B);
  dat(ys >> 8); dat(ys & 0xFF); dat(ye >> 8); dat(ye & 0xFF);
  cmd(0x2C);
}

void blit(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
          const uint16_t* pixels) {
  set_window(x1, y1, x2, y2);
  uint32_t count = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1) * 2;
  s_spi.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_DC, HIGH);
  s_spi.writeBytes((const uint8_t*)pixels, count);
  digitalWrite(PIN_CS, HIGH);
  s_spi.endTransaction();
}

void set_brightness(uint8_t percent) {
  if (percent > 100) percent = 100;
  uint32_t duty = (uint32_t)percent * ((1 << BL_PWM_RES) - 1) / 100;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_BL, duty);
#else
  ledcWrite(0, duty);
#endif
}

}  // namespace display

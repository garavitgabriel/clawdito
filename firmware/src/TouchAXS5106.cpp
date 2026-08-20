#include "TouchAXS5106.h"
#include "DisplayJD9853.h"
#include <Wire.h>

static const int PIN_SDA = 42;
static const int PIN_SCL = 41;
static const int PIN_RST = 47;
static const int PIN_INT = 48;

static const uint8_t I2C_ADDR  = 0x63;
static const uint8_t REG_ID    = 0x08;
static const uint8_t REG_TOUCH = 0x01;

static bool     s_down = false;
static uint16_t s_x = 0, s_y = 0;

namespace touch {

// The AXS5106L does not support I2C repeated-start: issue a STOP between
// the register-pointer write and the read.
static bool reg_read(uint8_t reg, uint8_t* out, uint8_t len) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return false;
  int n = Wire.requestFrom((int)I2C_ADDR, (int)len);
  if (n < len) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (int i = 0; i < len; i++) out[i] = Wire.read();
  return true;
}

void init() {
  // The controller needs a long reset pulse and settle time before it
  // starts reporting.
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(200);
  digitalWrite(PIN_RST, HIGH);
  delay(300);
  pinMode(PIN_INT, INPUT);

  Wire.begin(PIN_SDA, PIN_SCL, 400000);

  uint8_t id[3] = {0};
  if (reg_read(REG_ID, id, 3)) {
    Serial.printf("[touch] id %02X%02X rev %02X\n", id[0], id[1], id[2]);
  } else {
    Serial.println("[touch] controller not responding");
  }
}

bool read(uint16_t* x, uint16_t* y) {
  uint8_t d[14] = {0};
  if (reg_read(REG_TOUCH, d, sizeof(d))) {
    uint8_t fingers = d[1];
    if (fingers > 0) {
      // 12-bit coordinates in the panel's native portrait frame
      // (x 0..171, y 0..319); rotate into landscape.
      uint16_t px = (((uint16_t)(d[2] & 0x0F)) << 8) | d[3];
      uint16_t py = (((uint16_t)(d[4] & 0x0F)) << 8) | d[5];
      uint16_t lx = py;
      uint16_t ly = (SCREEN_HEIGHT - 1) - px;
      if (lx >= SCREEN_WIDTH)  lx = SCREEN_WIDTH - 1;
      if (ly >= SCREEN_HEIGHT) ly = SCREEN_HEIGHT - 1;
      s_x = lx;
      s_y = ly;
      s_down = true;
    } else {
      s_down = false;
    }
  }
  if (s_down) { *x = s_x; *y = s_y; }
  return s_down;
}

}  // namespace touch

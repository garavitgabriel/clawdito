# Clawdito — Hardware Notes

Target board: **Waveshare ESP32-S3-Touch-LCD-1.47**
(ESP32-S3R8, 16MB QIO flash, 8MB OPI PSRAM, 1.47" 172×320 IPS, capacitive touch)

> ⚠️ This is the **Touch** variant. The non-touch `ESP32-S3-LCD-1.47` looks
> identical but has a different display controller (ST7789 vs **JD9853**),
> a completely different display pin map, and an onboard RGB LED where this
> board routes the LCD clock. Firmware for one shows a black screen on the
> other.

## Pin map (Touch variant)

| Function | GPIO |
|---|---|
| LCD MOSI | 39 |
| LCD SCLK | 38 |
| LCD CS | 21 |
| LCD DC | 45 |
| LCD RST | 40 |
| LCD backlight (PWM) | 46 |
| Touch SDA | 42 |
| Touch SCL | 41 |
| Touch RST | 47 |
| Touch INT | 48 |
| BOOT button | 0 |
| SD card | 13–18 |

## Display (JD9853, SPI)

- 172×320 native portrait; Clawdito drives it in **landscape 320×172** via
  MADCTL `0x60` (MV|MX). In landscape the X axis needs no offset; the Y
  axis (the 172-pixel physical columns) needs **+34**.
- Pixel format RGB565, **big-endian on the wire** — LVGL must be built with
  `LV_COLOR_16_SWAP 1` or every color arrives byte-swapped (dark navy turns
  lavender, antialiased text turns to speckle).
- 40 MHz SPI is reliable on these GPIO-matrix-routed pins.
- Inversion ON (`0x21`) — it's an IPS panel.

## Touch (AXS5106L, I²C)

- Address `0x63`, ID register `0x08` returns `51 06 xx`.
- **No repeated-start support**: issue a STOP between the register-pointer
  write and the read, or every read fails.
- Needs a long reset: RST low 200 ms → high, then ~300 ms settle before it
  responds.
- Touch data at register `0x01`: byte 1 = finger count; point 1 as 12-bit
  X/Y pairs in bytes 2–5 (portrait frame — rotate to match the display).
- Note: some vendor materials disagree about which of GPIO 47/48 is RST vs
  INT. On this board **47 = RST, 48 = INT** (verified: the ID read only
  succeeds after resetting via 47).

## Button

GPIO 0 doubles as the ESP32-S3 download-mode strap pin — poll it, never
attach an interrupt (spurious edges during reset/wake).

## Power

Any 5V USB-C source. Flashing needs a data cable; after provisioning the
device runs standalone on a wall charger.

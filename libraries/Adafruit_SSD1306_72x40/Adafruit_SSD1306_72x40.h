#ifndef ADAFRUIT_SSD1306_72X40_H
#define ADAFRUIT_SSD1306_72X40_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display dimensions
#define SSD1306_72X40_WIDTH   72
#define SSD1306_72X40_HEIGHT  40

// The SSD1306 controller has an internal 128px wide buffer.
// The 72px display is centered within it, starting at column 28.
#define SSD1306_72X40_COL_OFFSET  28

// I2C address (standard for most SSD1306 displays)
#define SSD1306_72X40_I2C_ADDR    0x3C

class Adafruit_SSD1306_72x40 : public Adafruit_SSD1306 {
public:
  // Constructor: pass SDA and SCL pin numbers for your board
  Adafruit_SSD1306_72x40(uint8_t sda_pin, uint8_t scl_pin);

  // Call this in setup() instead of the standard begin()
  bool begin();

  // Call this instead of display() to send the buffer with correct column offset
  void display();

private:
  uint8_t _sda;
  uint8_t _scl;

  // Sends the correct init sequence for the 0.42" 72x40 OLED
  // Based on U8g2's u8x8_d_ssd1306_72x40_er_init_seq
  void _fixInit();
};

#endif // ADAFRUIT_SSD1306_72X40_H

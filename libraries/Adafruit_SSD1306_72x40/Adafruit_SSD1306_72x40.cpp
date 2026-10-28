#include "Adafruit_SSD1306_72x40.h"

Adafruit_SSD1306_72x40::Adafruit_SSD1306_72x40(uint8_t sda_pin, uint8_t scl_pin)
  : Adafruit_SSD1306(SSD1306_72X40_WIDTH, SSD1306_72X40_HEIGHT, &Wire, -1),
    _sda(sda_pin), _scl(scl_pin) {
}

bool Adafruit_SSD1306_72x40::begin() {
  Wire.begin(_sda, _scl);

  if (!Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC, SSD1306_72X40_I2C_ADDR)) {
    return false;
  }

  // Override Adafruit's generic init with the correct sequence for this display
  _fixInit();
  return true;
}

void Adafruit_SSD1306_72x40::_fixInit() {
  // Init sequence derived from U8g2's u8x8_d_ssd1306_72x40_er_init_seq.
  // Key differences from the standard 128x64 init:
  //   - Multiplex ratio 0x27 (40 lines instead of 64)
  //   - Internal IREF 0xAD/0x30 (specific to 0.42" OLED)
  //   - Horizontal addressing mode 0x20/0x00
  //   - Segment remap 0xA1 + COM scan reverse 0xC8 (correct orientation)
  //   - Scroll deactivated 0x2E
  ssd1306_command(0xAE);          // Display off
//  ssd1306_command(0xD5);          // Set clock divide ratio / oscillator frequency
//  ssd1306_command(0x80);
  ssd1306_command(0xA8);          // Set multiplex ratio
  ssd1306_command(0x27);          // 40 lines (0x27 = 39, zero-indexed)
//  ssd1306_command(0xD3);          // Set display offset
//  ssd1306_command(0x00);          // No vertical offset
  ssd1306_command(0xAD);          // Internal IREF setting (0.42" OLED specific)
  ssd1306_command(0x30);
//  ssd1306_command(0x8D);          // Charge pump setting
//  ssd1306_command(0x14);          // Enable charge pump
//  ssd1306_command(0x40);          // Set display start line to 0
//  ssd1306_command(0x20);          // Set memory addressing mode
//  ssd1306_command(0x00);          // Horizontal addressing mode
//  ssd1306_command(0xA1);          // Segment remap: col 127 mapped to SEG0
//  ssd1306_command(0xC8);          // COM scan direction: reverse (row 0 = COM[N-1])
  ssd1306_command(0xDA);          // Set COM pins hardware configuration
  ssd1306_command(0x12);          // Alternative COM pin config, no left/right remap
  ssd1306_command(0x81);          // Set contrast
  ssd1306_command(0xAF);
  ssd1306_command(0xD9);          // Set pre-charge period
  ssd1306_command(0x22);
  ssd1306_command(0xDB);          // Set VCOMH deselect level
  ssd1306_command(0x20);
  ssd1306_command(0x2E);          // Deactivate scroll
  ssd1306_command(0xA4);          // Output follows RAM content
  ssd1306_command(0xA6);          // Normal (non-inverted) display
  ssd1306_command(0xAF);          // Display on
}

void Adafruit_SSD1306_72x40::display() {
  // The SSD1306 controller has a 128px wide internal buffer.
  // This display is 72px wide and sits at column offset 28 (128-72)/2 = 28).
  // We must set the column window before each transfer so data lands correctly.

  ssd1306_command(0x21);                              // Set column address
  ssd1306_command(SSD1306_72X40_COL_OFFSET);          // Start: column 28
  ssd1306_command(SSD1306_72X40_COL_OFFSET +
                  SSD1306_72X40_WIDTH - 1);           // End:   column 99

  ssd1306_command(0x22);                              // Set page address
  ssd1306_command(0x00);                              // Start: page 0
  ssd1306_command(0x04);                              // End:   page 4 (5 x 8px = 40px)

  // Send the display buffer over I2C in 32-byte chunks.
  // ESP32 has a limited I2C buffer, so we flush periodically.
  uint8_t  *buf     = getBuffer();
  uint16_t  bufSize = SSD1306_72X40_WIDTH *
                      ((SSD1306_72X40_HEIGHT + 7) / 8);

  Wire.beginTransmission(SSD1306_72X40_I2C_ADDR);
  Wire.write(0x40); // Co=0, D/C=1: data bytes follow

  for (uint16_t i = 0; i < bufSize; i++) {
    Wire.write(buf[i]);
    if ((i & 0x1F) == 0x1F) {  // Flush every 32 bytes
      Wire.endTransmission();
      Wire.beginTransmission(SSD1306_72X40_I2C_ADDR);
      Wire.write(0x40);
    }
  }
  Wire.endTransmission();
}

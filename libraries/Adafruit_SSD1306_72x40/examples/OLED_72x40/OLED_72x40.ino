// Demo sketch for Adafruit_SSD1306_72x40 library
// Tested on ESP32-C3 with a 0.42" 72x40 OLED display

#include "Adafruit_SSD1306_72x40.h"

#define SDA_PIN 5
#define SCL_PIN 6

Adafruit_SSD1306_72x40 display(SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(115200);

  if (!display.begin()) {
    Serial.println(F("SSD1306 failed"));
    for (;;);
  }

  // Show Adafruit splash logo for 1 second, then clear
  display.display(); delay(1000);
  display.clearDisplay();

  // Draw a border around the display
  for (int p = 0; p < 72; p++) {
    display.drawPixel(p, 0, 1);   // Top edge
    display.drawPixel(p, 39, 1);  // Bottom edge
    if (p < 40) {
      display.drawPixel(0, p, 1);  // Left edge
      display.drawPixel(71, p, 1); // Right edge
    }
  }

  // Print 5 lines of text (each line is 8px tall, 5 x 8 = 40px)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Adafruit    "));
  display.println(F("   _SSD1306 "));
  display.println(F("   _72x40   "));
  display.println(F("123456789012"));
  display.println(F("ABCDEFGHIJKL"));

  display.display();
}

void loop() {}

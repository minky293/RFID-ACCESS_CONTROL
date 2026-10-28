# Adafruit_SSD1306_72x40
Fix for 72x40 OLED displays. Requires and extends
[Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
– not affiliated with Adafruit®.

# Problem

SSD1306 is a chip for controlling 128x64 pixel displays.
So it is not ideal for handling a display with 72x40 pixels.
Some manufactors did that anyway and it requires some exceptions to work.

# Why this library exists

The [U8g2](https://github.com/olikraus/u8g2) library has made these exceptions, but as
[Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
is a bit faster, this library exists.
U8g2 was a great inspiration to get this to work.

Adafruit does not sell any displays with SSD1306 and 72x40 pixels 
so they have obviously no interest in making a driver for it.
So I did it.
If Adafruit some day include 72x40 in the Adafruit_GFX library
this extension will become obsolete.

# Example

Take a look the the ESP32-C3 OLED 0.42" 72x40
[OLED_72x40.ino](examples/OLED_72x40/OLED_72x40.ino)
example.

This library was used in [PipeGIF](https://github.com/fun6400/PipeGIF).

## License

MIT — see [LICENSE](LICENSE) for details.

Author: Hans Schou <hans@schou.dk> © 2026

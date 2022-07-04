# AEROMETAR-PAPER

An IoT device based on ESP32 WIFI SoC to display aviation weather observation (METAR) & prediction (TAF) for a given airport

## Hardware requirements

- ESP32 - Wifi/BLE SoC
- [Embedded Artist 2.7" inch E-Paper](https://www.embeddedartists.com/products/2-7-inch-e-paper-display/) based on [Pervasive Displays](https://www.pervasivedisplays.com/) EM027AS012

## Software dependencies

- IDE - [Platformio](https://platformio.org/)
- Core - [arduino-esp32](https://github.com/espressif/arduino-esp32)
- Lib - [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library) to create frame buffer
- Lib - FRAME overloaded GFXcanvas1 class from Adafruit-GFX-Library
- Lib - EPD [updated from Embedded Artist example](https://www.embeddedartists.com/wp-content/uploads/2018/06/epaper_arduino_130412.zip) to control e-Paper display
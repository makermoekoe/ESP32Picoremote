# ESP8285_OLED_REMOTE

<img src="docs/picoremote.JPG" width="450px"></a>

WiFi remote based on ESP32 Pico D4 MCU and a beautiful 160x80 pixels mini TFT display. Additionally there is a I2C temperature & humidity sensor as well as an accelerometer on board.

The power latching circuit ensures that the ESP32 Pico D4 don't need to be in deep sleep mode while in OFF state. This puts it down to around 3µA during 'sleep', due to the battery protection. It can be woke up only by the middle button. The battery protection occupies reverse voltage protection through the mosfet. Overdischarge voltage of the XB5353A is 2.4volts, which is only for safety reasons. The battery voltage should always be checked with the voltage divider, which gives you an estimation of the battery state.

Main components are:
- ESP32 Pico D4
- CP2102N USB to serial bridge
- 160x80px ST7735 13pin TFT display
- HDC1080 temperature and humidity sensors
- LIS3DHTR accelerometer
- WS2812 LEDs
- XB5353A battery protection
- SE7401U polarity
- MCP73831 charging
- RT9193 regulator
- Molex 3D antenna

## Circuit & PCB

`pico32_remote_v1.0` was the initial schematic of this PCB. Unfortunately it occupies a few mistakes:
- Button two is connected to the ESP32 directly behind the button, which gives `VCC` directly to one of the GPIOs of the MCU - meh. The 1k resistor should be between the pulldown and the button.
- Furthermore the footprint of the LIS3DHTR is mirrored (not visible in the schematics).
- Last but not least the `VUSB` of the USB-C connector and the power selection circuit wasn't connected properly because the VUSB on one side was just a label and not the name of connection itself - shame on me.

All these mistakes are fixed in version 1.1. Additionally I have added a USB activation diode, which enables the latching circuit ones a USB cable is plugged into the board. This is nice to have if you want to visualize battery charging. Another advantage is that one of the buttons don't have to be pressed during firmware upload.

## Code

Check out the code in the 'code' section. It is used in PlatformIO, but can be shifted to Arduino IDE easily. The code is exactly the same.

The ST7735 display can be used easily with the [TFT_eSPI library](https://github.com/Bodmer/TFT_eSPI) from Bodmer. This library comes with a `User_Setup.h` to setup the dedicated display you are using. I have attached this file for this specific board in the code section of this repo.

When you have questions then feel free to ask.

Have fun! :)

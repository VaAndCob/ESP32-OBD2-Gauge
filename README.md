# ESP32-OBD2-Gauge
A fully functional code-updated
Last Update: August 29th, 2024

This is ESP32 OBD2 Gauge for vehicles that are compatible with obd2
(Special made for FORD vehicle)
 - Prototype https://youtu.be/PkQaUJbzTNM
 - Current Model Short Demo https://youtu.be/vvBIeim7XTE
 - 
![](/picture/page.jpg)
![](/picture/config.jpg)
![](enclosure/enclosure.jpg)

## Software:
- Arduino IDE 2.3.2 + ESP32 arduino core 2.0.17
- modified TFT_eSPI library 2.5.43

## Hardware
- ESP32 TFT2.8 320x240 with "RESISTIVE" touch board.
buy here  https://s.click.aliexpress.com/e/_Ew95gMl 
- ELM327 Bluetooth adaptor (recommended v1.5 not v2.1)
  Highly recommend ELM327 Bluetooth  adaptor https://s.click.aliexpress.com/e/_oo3THvG
- A small speaker for mobile phone 1 Watt 8 ohm 9x28 mmm.
- A push button 6x6x5 2 legs
- 3d print enclosure
- 12v to 5v dc micro usb power regulator module
- Gauge magnetic stand

## Features:
- Show vehicle data 7 pids
* MAP - manifold air pressure (PSI)
* PCM - pcm voltage (volt)
* ENG LOAD - engine load (%)
* ENG SPD - engine speed (RPM)
* Coolant - coolant temperature (c)
* Oil Temp - engine oil temperature (c)
* TFT - Ford transmission fluid temperature (c)

- 8 layout page selectable display
- DTC read and clear function (Engine warning light only)
- Warning when parameter reaches setting value
- Adjustable warning value for each PIDs
- Configurable CPU overheat protection
- Configurable gauge Auto turn on/off
- Auto screen brightness
- Change off screen to user screen with micro SDcard
- Firmware updatable (micro SDcard and WiFi)
- VIN Read on the About page.

## [☕ Buy me coffee](buymeacoffee.com/vaandcob)

# ESP32-OBD2-Gauge
This is ESP32 OBD2 Gauge for vehicle that compatible with obd2
(Special made for FORD vehicle)
[Youtube]https://youtu.be/PkQaUJbzTNM
![My Image](/pictures/layout1.jpeg)

## Software:
Arduino IDE 2.0.4
TFT_eSPI with modified library in sketch folder 
https://github.com/Bodmer/TFT_eSPI

## Hardware
- ESP32 TFT2.8 320x240 with touch board. https://www.youtube.com/watch?v=d2OXlVcRYrU
- A small speaker for mobile phone 1Watt
- A push button 6x6x5 2 legs
- Aluminium Case Cover
- ELM327 bluetooth adaptor
- 12v to 5v dc micro usb power regulator module
- Gauge magnetic stand

## Features:
- Show vehicle data 7 pids
MAP - manifold air pressure (PSI)
PCM - pcm voltage (volt)
ENG LOAD - engine load (%)
ENG SPD - engine speed (RPM)
Coolant - coolant temperature (c)
Oil Temp - engine oil temperature (c)
TFT - ford transmission fluid temperature (c)

- 5 layout page selectable display
- Warning when parameter reach setting value
- Adjustable warning value for each PIDs
- CPU overheat protection
- Auto turn on/off
- Auto screen brightness
- Change off screen to user screen with micro SDcard
- Firmware updatable with micro SDcard
-

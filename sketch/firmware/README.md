there are 3 compiled firmware version
1. VaandcobOBD2_T6_2023.bin  
- this is an original firmware from that work with ford T6 up with transmission 6R80, 10R80 
- work very well before some libraries updated.

2. VaandcobOBD2_T5.bin
- this is a new firmware for old ford T5 vehicle with old automatic transmission

3. for firmware suffix with  _nogenuine.bin are the version that skip genuine board check.


----------------------------------------
How to flash ESP32 with web flasher

1. goto https://adafruit.github.io/Adafruit_WebSerial_ESPTool/
2. enter offset / upload file as following

0x1000	bootloader.biin
0x8000  partition.bin
0xE000  boot_app0.bin
0x10000  vaandcobobd2gauge.bin (version you want to use)

3. Click Program and wait until flashing done.

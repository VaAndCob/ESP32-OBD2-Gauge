/* This is the first sketch to upload for OBDII gauge
- add serial no. by set in #define serial_no
- micro sdcard firmware upload


*/

#include <Update.h>  //
#include <FS.h>      // File System
#include <SD.h>      // SD Card ESP32
#include <Preferences.h>//save permanent data
#include <TFT_eSPI.h> //Hardware-specific library

const String serial_to_write = "V&C-001";//010


//Pin configuration
#define SCA_PIN 21//not used same pin with TFT_BL
#define SCL_PIN 22//not used (avaialable) CAN_TX
#define ADC_PIN 35//not used (available)  CAN_RX
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17 
#define LDR_PIN 34    //LDR sensor
#define BUZZER_PIN 26//speaker
#define SELECTOR_PIN 27//push button
// setting PWM properties
#define backlightChannel 0
#define buzzerChannel 2

String line[12] = {"","","","","","","","","","","",""};//buffer for each line in terminal function
Preferences pref;//create preference

TFT_eSPI tft = TFT_eSPI();

//TERMINAL terminal for debugging display text
void Terminal(String texts,uint16_t x,uint16_t y,uint16_t w,uint16_t h) {
   uint8_t max_line = round((h-y)/16.0); 
   tft.fillRect(x, y,x+ w,y+ h, TFT_BLACK);
   tft.setTextColor(TFT_WHITE);
   for (uint8_t i=0;i<max_line;i++) {
     if (i<=max_line - 2) {
      line[i] = line[i+1];
     } else {
       line[i] = texts;
     } 
      tft.drawString(line[i],x,i*20+y,2);
   }//for

} 

// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize) {
   if (Update.begin(updateSize)) {      

     //update progress
      Serial.print(F("Progress: "));
     Update //progress callback
     .onProgress([](size_t written, size_t total) {
      uint8_t progress = written * 100 / total;
      if (progress%10 == 0) {
        Serial.print(F("*"));
        Terminal("*",0,48,320,191);
      }

    }); 

      size_t written = Update.writeStream(updateSource, updateSize);
      if (written == updateSize) {
         Serial.println("Written : " + String(written) + " successfully");
      }
      else {
         Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
      }
      if (Update.end()) {
         Serial.println("OTA done!");
         if (Update.isFinished()) {
            Serial.println("Update successfully completed.");
            Terminal("Update successfully completed.",0,48,320,191);
            ESP.restart();
         }
         else {
            Serial.println("Update not finished? Something went wrong!");
         }
      }
      else {
         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

   }
   else
   {
      Serial.println("Not enough space to begin OTA");
   }
}

// check given FS for valid update.bin and perform update if available
void updateFromFS(fs::FS &fs) {
   File updateBin = fs.open("/VaAndCobOBD2Gauge.bin");
   if (updateBin) {
      if(updateBin.isDirectory()){
         Serial.println("Error, update.bin is not a file");
         updateBin.close();
         return;
      }

      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
         Serial.println("Try to start update");
         performUpdate(updateBin, updateSize);
      }
      else {
         Serial.println("Error, file is empty");
      }

      updateBin.close();
    
      // whe finished remove the binary from sd card to indicate end of the process
     
   }
   else {
      Serial.println("Could not load update.bin from sd root");
   }
}
//---------------------------
void setup() {
  pinMode(SELECTOR_PIN,INPUT_PULLUP);//button
    //pwm setup
  ledcSetup(buzzerChannel, 1500, 10);//buzzer 10 bit
  ledcSetup(backlightChannel, 12000, 8);//backlight 8 bit
  ledcAttachPin(BUZZER_PIN, buzzerChannel);//attach buzzer

  Serial.begin(115200);
  delay(100);
  //init TFT display
  tft.init();
  tft.setRotation(1);//landcape 
  ledcAttachPin(TFT_BL, backlightChannel);//attach backlight
  ledcWrite(backlightChannel, 255);//full bright
  tft.fillScreen(TFT_BLACK);//show start page
  tft.drawString("Va&Cob OBDII Gauge Factory Initialize",0,0,2);
  Serial.println("Va&Cob OBDII Gauge Factory Initialize");
  Serial.println("-------------------------------------");
  Serial.print("Read Serial No: ");
  Terminal("Read Serial No: ",0,48,320,191);
//read current serial
  pref.begin("security", false);//write /read 
  String serial_no = pref.getString("serialno","");
  if (serial_no == "") {//serial no not found
    Serial.println("XXX No Serial XXX");
    Terminal("XXX No Serial XXX",0,48,320,191);
  } else {
    Serial.println(serial_no);
    Terminal(serial_no,0,48,320,191);
  }
//write a new serial no
  Serial.print("Write Serial No: ");
  Serial.println(serial_to_write);
  Terminal("Write Serial No: ",0,48,320,191);
  Terminal(serial_to_write,0,48,320,191);
  pref.putString("serialno",serial_to_write);
  pref.end();//close pref
 
  Serial.println("-------------------------------------");
  Serial.println("Insert micro sdcard with firmware \"VaAndCobOBD2Gauge.ini\"");
  Terminal("-------------------------------------",0,48,320,191);
  Terminal("Insert micro sdcard with firmware",0,48,320,191);
  Terminal("\"VaAndCobOBD2Gauge.ini\"",0,48,320,191);

  Serial.println("-------------------------------------");
  Serial.print("Press BUTTON to flash firmware");
  Terminal("-------------------------------------",0,48,320,191);
  Terminal("Press BUTTON to flash firmware",0,48,320,191);

  while (digitalRead(SELECTOR_PIN) == HIGH) {
  delay(10);
  }

   //first init and check SD card
   if (!SD.begin()) {
      Serial.println("Card Mount Failed");
   }

  uint8_t cardType = SD.cardType();

   if (cardType == CARD_NONE) {
      Serial.println("No SD_MMC card attached");
      ESP.restart();
   }else{
      updateFromFS(SD);
  }
}

//---------------------------
void loop() {
  yield();
}
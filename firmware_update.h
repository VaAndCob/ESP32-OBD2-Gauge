//Firmware updater library
#include <esp_ota_ops.h>//ota partition boot option
#include <Update.h>  // main updater
#include <FS.h>      // File System
#include <SD.h>      // SD Card ESP32
#include <SPIFFS.h>  //safe file

//Wifi updater
#include <WiFi.h>   //wifi
#include <WebServer.h>

const char* ssid = "Va&Cob OBD2 Gauge";
const char* password = "12345678";//default password
IPAddress local_IP(192, 168, 4,1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

#define firmware_filename "/VaAndCobOBD2Gauge.bin"//firmware file 


/*------------ SOUND EFFECT ----------*/
 void beepbeep() { //2 shot beep
  ledcWriteTone(buzzerChannel,2000);
  delay(50);
  ledcWriteTone(buzzerChannel,0);
  delay(50);
  ledcWriteTone(buzzerChannel,2000);
  delay(50);
  ledcWriteTone(buzzerChannel,0);
 }              
 void beep() { 
  ledcWriteTone(buzzerChannel,2000);
  delay(500);
  ledcWriteTone(buzzerChannel,0);
 }  
 void clickSound() {
  ledcWriteTone(buzzerChannel,5000);
  delay(5);
  ledcWriteTone(buzzerChannel,0);
 } 
 /*-----------------------------*/
//html update file
static const char uploadIndex[] PROGMEM =
R"(
<!DOCTYPE html>
<head>
<meta name='viewport' content='width=device-width'/>
</head>
<body>
<img src="data:image/jpeg;base64,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"width="100%">
<h3 style='color:blue;'>Please choose file 'vaandcobobd2gauge.bin'</h3>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update' accept=".bin" id='file1' onchange='uploadFile()'>
</form>
<p style='align:center'>
<progress id='progressBar' value='0' max='100' style='width:100%'></progress>
<div id='status' style='text-align:center';>0%</div>
</p>
<script>
function _(el) {
  return document.getElementById(el);
}
function uploadFile() {
  let file = _('file1').files[0];
  // alert(file.name+' | '+file.size+' | '+file.type);
  let req = new XMLHttpRequest();
  let formdata = new FormData();
  formdata.append('file1', file);
   req.upload.addEventListener('progress', progressHandler, false);
  req.addEventListener('load', completeHandler, false);
  req.addEventListener('error', errorHandler, false);
  req.addEventListener('abort', abortHandler, false);
  req.open('POST', '/update'); 
  req.send(formdata);
}
function progressHandler(event) {
  var percent = (event.loaded / event.total) * 100;
  _('progressBar').value = Math.round(percent);
  _('status').innerHTML = Math.round(percent) + '%';
}
function completeHandler(event) {
  _('status').innerHTML = 'Restarting...';
}
function errorHandler(event) {
  _('status').innerHTML = 'Upload Failed';
}
function abortHandler(event) {
  _('status').innerHTML = 'Upload Aborted';
}
</script>
</body>
</html>
)";
/*----- OTA Web Server ------*/
void  StartOTAserver() {
  //open upload page  http://192.168.4.1
  server.on("/", HTTP_GET, []() {
    Serial.println(F("Request upload firmware..."));
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", uploadIndex);
  });

  //handling uploading firmware file http://192.168.4.1/update
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    if (Update.hasError()) {//error 
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Error: bad firmware file!",159,170,4); 
      beep();
    } 
    else {
      esp_ota_mark_app_valid_cancel_rollback();//prevent from boot old firmware
      delay(3000);
      ESP.restart();//successful upload restart esp
    }  
  }, []() {
    
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      beepbeep();
      Serial.printf("Firmware: %s\n", upload.filename.c_str());
      tft.setTextColor(TFT_BLACK,TFT_GREEN);
      tft.drawCentreString("Updating... please wait",159,170,4); 
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);        
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      // flashing firmware to ESP
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Successful... Restart");
        tft.setTextColor(TFT_BLACK,TFT_GREEN);
        tft.drawCentreString("Update Successful... Restart",159,170,4); 
        beepbeep();

      } else {
        Update.printError(Serial);  
      }
    }
  });
  server.begin();
}//startOTAserver
/*-------------- WiFi Updater----------------*/
void updateFirmwareWIFI() {  
  WiFi.mode(WIFI_AP);//set wifi mode soft access point
  String pwd = String(random(99999999));//random new password at least 8 digit long to get it work
  for (uint8_t i=pwd.length();i<8;i++) {
    pwd = "0"+pwd;//add 0 prefix to set 8 char length password
  }
  password = pwd.c_str();
  WiFi.softAP(ssid, password);//start ACCESS Point
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {//error start soft AP
    Serial.println(F("STA Failed to configure"));
    tft.setTextColor(TFT_WHITE,TFT_RED);
    tft.drawCentreString("Failed to start WiFi!",159,120,4); 
    beep();
    delay(3000); 
  } else {//softAP config OK
    StartOTAserver();//start server
  //show WiFi softAP information
    tft.fillRect(0,30,320,239,TFT_BLACK);
    tft.setTextColor(TFT_CYAN,TFT_BLACK);
    tft.drawCentreString("> Connect to gauge Wi-Fi <",159,40,4); 
    tft.setTextColor(TFT_WHITE,TFT_BLACK); 
    String txt = "SSID: "+String(ssid);
    Serial.println(txt);
    tft.drawString(txt,0,80,4);
    txt = "Password: "+String(password);
    Serial.println(txt);
    tft.drawString(txt,0,110,4);
    txt = "IP Address: "+ WiFi.softAPIP().toString();
    Serial.println(txt);
    tft.drawString(txt,0,140,4);  
    tft.setTextColor(TFT_BLACK,TFT_WHITE);
    tft.drawCentreString("[- Press button to exit -]",159,215,4);  
    while (digitalRead(SELECTOR_PIN) == HIGH) {//wait for button press to exit       
      autoDim();
      checkCPUTemp(); 
      server.handleClient();//update server handle      
      delay(300);//delay button press
    } //exit wifi update      
    clickSound();   
    server.stop();
    WiFi.mode(WIFI_MODE_STA);
    WiFi.softAPdisconnect();//turn off WiFi
  }//else WiFi.softapconfig
} //updateFirmwareWiFI
/*----------- SD Card Updater --------------*/
// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize) {
  if (Update.begin(updateSize, U_FLASH, LED_RED_PIN, LOW)) { 
      //add progressbar
      tft.fillRoundRect(50,61, 218, 28, 14, TFT_LIGHTGREY);
      tft.fillRect(59,67,200,16,TFT_BLACK);//0% progress bar
      //update progress
      Serial.print(F("Progress: "));
      //progress callback
     Update.onProgress([&](size_t written, size_t total) {
      uint8_t progress = written * 100 / total;
      tft.fillRectHGradient(59,67,progress*2,16,TFT_RED,TFT_GREEN);//progressbar 
      if (progress%10 == 0) Serial.print(F("*"));
      ledcWriteTone(buzzerChannel,random(4000)+8000);//acceleraet sound effect
    }); 

    //write firmware from SDCard here
      size_t written = Update.writeStream(updateSource);
      Serial.printf("%d bytes written",written);

     if (Update.end()) {//updaet is done
         if (Update.isFinished()) {
            Serial.println(F("Update Successful... Restart"));
            tft.setTextColor(TFT_BLACK,TFT_GREEN);
            tft.drawCentreString("Update Successful... Restart",159,120,4);  
            beepbeep();
            esp_ota_mark_app_valid_cancel_rollback();//prevent from boot old firmware
            delay(1000);
            ESP.restart();//restart
         }
         else {
            Serial.println(F("Error! Update not finished"));
            tft.setTextColor(TFT_WHITE,TFT_RED);
            tft.drawCentreString("Error! Update not finished",159,120,4); 
         }
      } else {//update stop during updating
         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

   }//if update.begin
   else
   {
      Serial.println(F("Not enough space to load firmware"));
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Not enough space to upload!",159,120,4); 
   }
}
/*------------------*/
// check given FS for valid update.bin and perform update if available
void updateFromFS(fs::FS &fs) {
   Serial.println(F("Loading firmware..."));
   tft.setTextColor(TFT_WHITE,TFT_BLUE);
   tft.drawCentreString("Loading firmware...",159,120,4);
   File updateBin = fs.open(firmware_filename);
   if (updateBin) {
 
      if(updateBin.isDirectory()){
         Serial.println(F("\"VaandCobOBD2Gauge.bin\" is not a file"));
         tft.setTextColor(TFT_WHITE,TFT_RED);
         tft.drawCentreString("\"VaandCobOBD2Gauge.bin\" is not a file",159,120,4);
         updateBin.close();
         return;
      }
    
      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
         Serial.println(F("Updating... please wait"));
         tft.setTextColor(TFT_WHITE,TFT_BLUE);
         tft.drawCentreString("Updating... please wait",159,120,4);
         performUpdate(updateBin, updateSize);
      }
      else {
         Serial.println(F("Error! file is empty"));
         tft.setTextColor(TFT_WHITE,TFT_RED);
         tft.drawCentreString("Error! file is empty",159,120,4);               
      }
      updateBin.close();
      // whe finished remove the binary from sd card to indicate end of the process
      //fs.remove(firmware_filename);  
   
   } else {
      Serial.println(F("Firmware not found!"));
      tft.pushImage(130,44,60,60,nofile);//show fileicon
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Firmware not found!",159,120,4);
   }

}
/*------------------*/
void updateFirmwareSD() {
  tft.fillRect(0,30,320,239,TFT_BLACK);
  tft.setTextColor(TFT_BLACK,TFT_WHITE);
  tft.drawCentreString("[- Press button to exit -]",159,215,4);  
   if(!SD.begin()) {//sdcard not attach
    Serial.println(F("Micro SD Card not mounted!"));
    tft.pushImage(129,44,60,60,sdcard);//show sdcard icon
    tft.setTextColor(TFT_WHITE,TFT_RED);
    tft.drawCentreString("Please insert Micro SD Card",159,120,4);
  } else {//sd card attached
    Serial.println(F("Micro SD Card..OK")); 
    uint8_t cardType = SD.cardType(); 
    if (cardType == CARD_NONE) {//bad card
      Serial.println(F("Micro SD Card Error!"));
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Micro SD Card Error!",159,120,4);
     } else {//good sd card attached
      updateFromFS(SD);//update firmware from sd card
       
     }//else  
  }//else
   SD.end();
   beep();
}//updatefirmware
/*----------------*/

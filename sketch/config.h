/* "config.h"
this header file is for configuration menu
*/
#include <Update.h>  //
#include <FS.h>      // File System
#include <SD.h>      // SD Card ESP32

//configuration menu 
const String menuList[6] = {
  "Show pid/s & CPU Temp",//toggle show system
  "Load my pic 320x240 px",//load user own pic
  "Update firmware",//update new firmware
  "Warning setting",//setting warning parameter (warning data, offset temp)
  "About",//about
  "Exit"
  }; 
//menu list for warning setting
const String parameterList[8][2] = {
    {"CPU Overheat Temperature "+String(tempOverheat)+"`c , offset "," `c  "},
    {"- Engine Load"," %  "},
    {"- Coolant Temperature"," `c  "},
    {"- Manifold Air Pressure"," psi "},
    {"- Engine Speed"," rpm "},
    {"- PCM Voltage"," volt"},
    {"- Engine Oil Temperature"," `c  "},
    {"- Transmission Fluid Temperature"," `c  "}
  };  
//increasement and decreasement of each PID  
const int pidWarnStep[8] = {1,5,1,5,500,1,1,1};
    
#define firmware_filename "/VaAndCobOBD2Gauge.bin"//firmware file 
#define mypic_filename "/mypic.jpg"//mypic file
#define FORMAT_SPIFFS_IF_FAILED true

/*------------------*/ 
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
/*--------------------*/
//highlight and change warning each menu
void setWarning(int8_t index, int8_t change) {
  for (int i = 0;i<maxpidIndex+1;i++) {//draw all pid list
     tft.setTextColor(TFT_WHITE,TFT_BLACK);
     tft.drawString(parameterList[i][0],0,57+(i*16),2);//paramter list
     tft.drawRightString(parameterList[i][1],319,57+(i*16),2);//unit
     String result = "";
     if (i == 0)  result = String(tempOffset);
     else result = warningValue[i-1];
      switch (result.length()) {//remove indent
      case 1: result = "   " + result; break;  
      case 2: result = "  " + result; break;
      case 3: result = " " + result; break;
    }//switch
    tft.drawRightString(result,285,57+(i*16),2);
  }

    tft.setTextColor(TFT_BLACK,TFT_YELLOW);//highlight yellow
    //update number on screen
    if (index == 0)  {//cpu overheat
      int min = -34;
      int max = 0;
      if (change == -1) {//decrease
        tempOffset = tempOffset - pidWarnStep[index];
        if (tempOffset < min) tempOffset = min;          
      }
      if (change == 1) {//incrase
        tempOffset = tempOffset + pidWarnStep[index];
        if (tempOffset > max) tempOffset = max;
      }
      String result = String(tempOffset);
      switch (result.length()) {
      case 1: result = "   " + result; break;  
      case 2: result = "  " + result; break;
      case 3: result = " " + result; break;
      }//switch
      tft.drawRightString(result,285,57+(index*16),2);//cpu offset
    } else {//other pid

      int min = pidConfig[index-1][4].toInt();//get min
      int max = pidConfig[index-1][5].toInt();//get max
      if (change == -1) {//decrease  
        warningValue[index-1] = String(warningValue[index-1].toInt() - pidWarnStep[index]);
        if (warningValue[index-1].toInt() < min) warningValue[index-1] = String(min);          
      }
      if (change == 1) {//increase
       warningValue[index-1] = String(warningValue[index-1].toInt() + pidWarnStep[index]);
        if (warningValue[index-1].toInt() > max) warningValue[index-1] = String(max);
      }
      String result = warningValue[index-1];
      switch (result.length()) {
      case 1: result = "   " + result; break;   
      case 2: result = "  " + result; break;
      case 3: result = " " + result; break;
      }//switch
      tft.drawRightString(result,285,57+(index*16),2);//pid warning value
     }  //if i = 0   
  delay(300);//touch screen delay
}
 /*--------------------*/
void loadMyPic() {
  if(!SD.begin()) {//sdcard not attach
    tft.pushImage(129,44,60,60,sdcard);//show sdcard icon
    Serial.println("Micro SD Card not mounted!");
    tft.setTextColor(TFT_WHITE,TFT_RED);
    tft.drawCentreString("Please insert Micro SD Card",159,120,4);
  } else {//sd card attached
    Serial.println("Micro SD Card..OK"); 
    uint8_t cardType = SD.cardType(); 
    if (cardType == CARD_NONE) {//bad card
      Serial.println("Micro SD Card Error!");
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Micro SD Card Error!",159,120,4);
     
    } else {//good sd card attached
      if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("Error: SPIFFS failed!");
        tft.setTextColor(TFT_WHITE,TFT_RED);
        tft.drawCentreString("Error: SPIFFS failed!",159,120,4); 

      } else {  
       Serial.println("Deleting...\"mypic.jpg\""); 
       SPIFFS.remove(mypic_filename);//remove old file first    
      File sourceFile = SD.open(mypic_filename);
      if (sourceFile) {//found file
         
         File destFile = SPIFFS.open(mypic_filename, FILE_WRITE);
         Serial.println("Copying...\"mypic.jpg\""); 
         tft.setTextColor(TFT_BLACK,TFT_GREEN);
         tft.drawCentreString("Copying...\"mypic.jpg\"",159,120,4); 
         static uint8_t buf[512];
         while(sourceFile.read( buf, 512) ) {  
          destFile.write( buf, 512 );//copy file from sdcard to spiffs
         }
         sourceFile.close();
         destFile.close();
         //load mypic.jpg from spiffs to show on screen
         show_spiffs_jpeg_image(mypic_filename, 0, 0);
    
        } else {
        Serial.println("\"mypic.jpg\" not found!");
        tft.pushImage(130,44,60,60,nofile);//show fileicon
        tft.setTextColor(TFT_WHITE,TFT_RED);
        tft.drawCentreString("\"mypic.jpg\" not found!",159,120,4);
      }//if sourcefile 
    }//if SPIFFS.begin         
   }//else sccard errror
  }//else sdcard attach
   SD.end();
   SPIFFS.end();
   beep();
}//loadmypic
/*----------------*/
  
// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize) {
  if (Update.begin(updateSize, U_FLASH, LED_RED_PIN, LOW)) { 
      //add progressbar
      tft.fillRoundRect(50,61, 218, 28, 14, TFT_LIGHTGREY);
      tft.fillRect(59,67,200,16,TFT_BLACK);//0% progress bar
      //tft.setTextColor(TFT_WHITE,TFT_BLACK);

     Update //progress callback
     .onProgress([&](size_t written, size_t total) {
      uint16_t progress = written * 100 / total;
      Serial.printf("Progress: %d \n",progress);
      tft.fillRectHGradient(59,67,progress*2,16,TFT_RED,TFT_GREEN);//progressbar 
     // String txt = String(progress)+"%";
     // tft.drawString(txt,65+progress*2+5,60,4);//draw text%
      ledcWriteTone(buzzerChannel,20*progress);//acceleraet sound effect
    }); 

    //write firmware from SDCard here
      size_t written = Update.writeStream(updateSource);
      Serial.printf("%d bytes written",written);

     if (Update.end()) {//updaet is done
         if (Update.isFinished()) {
            Serial.println("Update Successful... restart");
            tft.setTextColor(TFT_BLACK,TFT_GREEN);
            tft.drawCentreString("Update Successful... restart",159,120,4);  
            beep();
            delay(1000);
            ESP.restart();//restart
         }
         else {
            Serial.println("Error! Update not finished");
            tft.setTextColor(TFT_WHITE,TFT_RED);
            tft.drawCentreString("Error! Update not finished",159,120,4); 
         }
      } else {//update stop during updating
         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

   }//if update.begin
   else
   {
      Serial.println("Not enough space to load firmware");
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Not enough space to load firmware",159,120,4); 
   }
}
/*------------------*/
// check given FS for valid update.bin and perform update if available
void updateFromFS(fs::FS &fs) {
   Serial.println("Loading firmware...");
   tft.setTextColor(TFT_WHITE,TFT_BLUE);
   tft.drawCentreString("Loading firmware...",159,120,4);
   File updateBin = fs.open(firmware_filename);
   if (updateBin) {
 
      if(updateBin.isDirectory()){
         Serial.println("\"VaandCobOBD2Gauge.bin\" is not a file");
         tft.setTextColor(TFT_WHITE,TFT_RED);
         tft.drawCentreString("\"VaandCobOBD2Gauge.bin\" is not a file",159,120,4);
         updateBin.close();
         return;
      }
    
      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
         Serial.println("Updating... please wait");
         tft.setTextColor(TFT_WHITE,TFT_BLUE);
         tft.drawCentreString("Updating... please wait",159,120,4);
         performUpdate(updateBin, updateSize);
      }
      else {
         Serial.println("Error! file is empty");
         tft.setTextColor(TFT_WHITE,TFT_RED);
         tft.drawCentreString("Error! file is empty",159,120,4);               
      }
      updateBin.close();
      // whe finished remove the binary from sd card to indicate end of the process
      fs.remove(firmware_filename);  
   
   } else {
      Serial.println("Firmware not found!");
      tft.pushImage(130,44,60,60,nofile);//show fileicon
      tft.setTextColor(TFT_WHITE,TFT_RED);
      tft.drawCentreString("Firmware not found!",159,120,4);
   }

}
/*------------------*/
void updateFirmware() {
   if(!SD.begin()) {//sdcard not attach
    Serial.println("Micro SD Card not mounted!");
    tft.pushImage(129,44,60,60,sdcard);//show sdcard icon
    tft.setTextColor(TFT_WHITE,TFT_RED);
    tft.drawCentreString("Please insert Micro SD Card",159,120,4);
  } else {//sd card attached
    Serial.println("Micro SD Card..OK"); 
    uint8_t cardType = SD.cardType(); 
    if (cardType == CARD_NONE) {//bad card
      Serial.println("Micro SD Card Error!");
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
void listMenu(uint8_t choice) {
  //draw icon
  if (showsystem) tft.pushImage(0,42,25,25,switchon);//switchon image
  else tft.pushImage(0,42,25,25,switchoff);//switchon image
  tft.pushImage(0,74,25,25,image);//image
  tft.pushImage(0,106,25,25,cpu);//firmware
  tft.pushImage(0,138,25,25,warning);//parameter
  tft.pushImage(0,170,25,25,about);//about
  tft.pushImage(159,170,25,25,quit);//exit
  for (uint8_t i=0;i<6;i++) {
    if (choice == i)  tft.setTextColor(TFT_WHITE,TFT_BLUE);//highlight 
    else  tft.setTextColor(TFT_WHITE,TFT_BLACK); 
    if (i<5) tft.drawString(menuList[i],32,42 + 32 * i,4);//write menu list
    else tft.drawString(menuList[i],191,42 + 32 * (i-1),4);//write menu list
    
  }//for i       
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.drawString("Next Menu -> Press & Release Button",0,207,2);
  tft.drawString("Select    -> Press & Hold Button",0,223,2);
}

/*----------------*/
void configMenu() {//control configuration menu
  uint8_t select = 0;
  bool pressed = false;
  long holdtimer = 0;
  bool currentFlag = showsystem;//keep current flag status  
    beep();
    tft.fillScreen(TFT_BLACK);//clear screen
    tft.fillRectVGradient(0, 0, 320, 26, TFT_ORANGE, 0xfc00);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("[---   Configuration Menu   ---]",159,0,4);

    listMenu(select);
    delay(1000);

    while (true) {//loop check button press

      if (digitalRead(SELECTOR_PIN) == LOW) {//button pressed   
        if (!pressed) {
          pressed = true;//set press flag
          holdtimer = millis();  
        } else if (holdtimer - millis() > 3000) {//hold 3 sec enter config menu
          //TOGGLE SHOW SYSTEM STATUS
          if (select == 0) {
            beep();
            showsystem = !showsystem;//toggle      
            tft.setTextColor(TFT_WHITE,TFT_BLUE);
            if (showsystem) tft.pushImage(0,42,25,25,switchon);//switchon image
            else tft.pushImage(0,42,25,25,switchoff);//switchon image
         
          } 
          //LOAD MY PIC
          if (select == 1) {
           clickSound(); 
           tft.fillRect(0,30,320,180,TFT_BLACK);
           loadMyPic();//load pic from sdcard to spiffs
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to exit -]",159,180,2);
           while (digitalRead(SELECTOR_PIN) == HIGH) {
             //wait for button press to exit
             checkCPUTemp();
             autoDim();
           }//while 
           tft.fillScreen(TFT_BLACK);//clear screen
           tft.setTextColor(TFT_BLACK,TFT_ORANGE);
           tft.drawCentreString("[---   Configuration Menu   ---]",159,0,4);

           listMenu(select);
          }

          //UPDATE FIRMWARE
          if (select ==2) {
           clickSound(); 
           tft.fillRect(0,30,320,180,TFT_BLACK);
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to SAVE and Exit -]",159,180,2);
           updateFirmware();
           while (digitalRead(SELECTOR_PIN) == HIGH) {
             //wait for button press to exit
             checkCPUTemp();
             autoDim();
           }//while 
           tft.fillRect(0,30,320,210,TFT_BLACK);
           listMenu(select);
          }
          
          //WARNING SETTING
          if (select ==3) {
           int8_t warningMenuIndex = 0;//index for warning setting
           int8_t inc_dec = 0;//-1=decrease,0=no change, 1=increase
           clickSound();
           tft.fillRect(0,30,320,210,TFT_BLACK);
           tft.setTextColor(TFT_WHITE,TFT_BLACK);
           tft.drawCentreString("[ Warning Parameter Setting]",159,30,2);
           tft.drawFastHLine(0,50,320,TFT_RED);            
           tft.setTextColor(TFT_WHITE,TFT_RED);
           tft.drawCentreString("[- Press button to Save & Exit -]",159,190,2);
           for (int i = 0;i<5;i++) //draw buttons
               tft.fillRoundRect(i*64,211,60,30,5,TFT_NAVY);
           tft.setTextColor(TFT_WHITE);    
           tft.drawCentreString("DEFAULT",31,217,2);//reset
           tft.drawCentreString("^ PREV",95,217,2);//prev
           tft.drawCentreString("NEXT v",159,217,2);//next
           tft.drawCentreString("-",223,206,6);//-
           tft.fillRect(286,219,3,16,TFT_WHITE);//+
           tft.fillRect(280,225,16,3,TFT_WHITE);//+
           setWarning(warningMenuIndex,inc_dec);//list menu first
           delay(500);
           while (digitalRead(SELECTOR_PIN) == HIGH) {//wait for button press to exit
             checkCPUTemp();
             autoDim();
             uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
             bool touched = tft.getTouch(&t_x, &t_y);
             if (touched && (t_y > 211)) {
               clickSound();
               #ifdef SERIAL_DEBUG
               Serial.printf("%d - %d\n",t_x,t_y);
               #endif
               if (t_x < 64) {//DEFAULT
                 tempOffset = factoryTempOffset;//load default
                 //read default warning value
                 for (int i=0;i < maxpidIndex; i++) warningValue[i] = pidConfig[i][8];
               }
               if ((t_x >= 64) && (t_x <128)) warningMenuIndex--;//prev
               if ((t_x >= 128) && (t_x <192)) warningMenuIndex++;//next
               if ((t_x >= 192) && (t_x <256)) inc_dec = -1;//decrease
               if ((t_x >= 256) && (t_x <320)) inc_dec = 1;//increase
               if (warningMenuIndex == maxpidIndex+1) warningMenuIndex = 0;
               if (warningMenuIndex < 0) warningMenuIndex = maxpidIndex;

               setWarning(warningMenuIndex,inc_dec);//update list
               inc_dec = 0;//reset 
        
             }//if touched
           }//while 
           //exit and save warning data to preference
           pref.putInt("tempOffset",tempOffset);//save tempoffset
          for (int i=0;i < maxpidIndex; i++) {//save all pid
            pref.putString(pidConfig[i][0].c_str(),warningValue[i]);          
          }          
          tft.setTextColor(TFT_BLACK,TFT_GREEN);
          tft.drawCentreString("Parameters Saved.",159,120,4); 
          beep();
          delay(1000);
          tft.fillRect(0,30,320,210,TFT_BLACK);
          listMenu(select);
          }

          //ABOUT
          if (select == 4) {
           clickSound();
           tft.fillRect(0,30,320,180,TFT_BLACK);
           tft.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
           tft.drawString("Firmware build date",0,37,2);
           tft.drawRightString(compile_date,319,37,2);
           tft.drawFastHLine(0,61,320,TFT_RED);
           tft.drawString("MAP/ENG LOAD/ECT/EOT/TFT/ENG SPD/PCM Volt",0,69,2);
           String txt = "Warning, Automatic Dim/OnOff/O็verheat Shutdown";
           tft.drawString(txt,0,85,2);
           tft.drawString("* FW-\"VaandCobOBD2Gauge.bin\" / Image-\"mypic.jpg\"",0,101,2);

           tft.drawString("* Youtube : www.youtube.com/@vacob6494",0,133,2);
           tft.drawString("* Facebook : www.facebook.com/vaandcob",0,149,2);
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to exit -]",159,180,2);
           delay(500);
           while (digitalRead(SELECTOR_PIN) == HIGH) { //wait for button press to exit
              starwars();//play starwars theme song
           }//while  
           tft.fillRect(0,30,320,210,TFT_BLACK);
           listMenu(select);
          }
          
          //EXIT
          if (select == 5) {
            clickSound();
            if (showsystem != currentFlag) {//save to eeprom only when flag changed
             pref.putBool("showsystem", showsystem);//save to NVR
            }            
            break;//exist menu loop while
          }//select = 4
                      
           pressed = false;   
          } //if holdtimer > 3000
         delay(250);//delay avoid bounce
       //**********************
        } else {//button release
          if (pressed) {//button release
            select++;//next menu
            if (select == 6) select = 0;
            listMenu(select);//next menu
            clickSound();
            pressed = false;  
            delay(200);
          }//pressed

      }//if press&& digitalread
      checkCPUTemp();
      autoDim();
    
    } //while     
    tft.fillScreen(TFT_BLACK);//clear screen
}//configMenu

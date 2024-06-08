/* "config.h"
this header file is for configuration menu
*/
//freeRTOS multi tasking
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "music.h"   //songs data
#include "nes_audio.h" //nes audio player
#include "firmware_update.h" //firmware updater function
#include "elm327.h"// read VIN function

#define mypic_filename "/mypic.jpg"//mypic file
#define FORMAT_SPIFFS_IF_FAILED true//format if no spiffs


Cartridge player(BUZZER_PIN);//not specified 4 pin , use 1 pin at buzzer pin

//configuration menu 
const String menuList[8] = {
  "Show pid/s & CPU Temp",//toggle show system
  "Load my pic 320x240 px",//load user own pic
  "Update firmware",//update new firmware
  "Warning setting",//setting warning parameter (warning data, offset temp)
  "DTCs",//dtc engine 
  "Auto-off",//auto turn off
  "About",//about
  "Exit"
  }; 
const uint8_t maxMenu = array_length(menuList);//length of menu list

//menu list for warning setting
const String parameterList[8][2] = {
    {"CPU Overheat Temperature "+String(tempOverheat)+"`c ,offset "," `c  "},
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

//Sprites
TFT_eSprite car = TFT_eSprite(&tft);
TFT_eSprite bk = TFT_eSprite(&tft);
TFT_eSprite pole = TFT_eSprite(&tft);
TFT_eSprite fence = TFT_eSprite(&tft);

//------ NES Audio play on Task0
TaskHandle_t TaskHandle0 = NULL;//create taskhandle0

void TaskPlayMusic(void *pvParameters) {
  esp_task_wdt_init(62, false);//62 sec wd timer / disable wd
  while(1) {
 // player.frame_counter_cb(danceLight);//cal back fucntion (not use)
  player.play_nes(song , true, 0.25); //play nes music, loop, volume 0.25, <lower vol lower distortion>
 
  }
}
//----------------------------
void animation() {//start field effect
//sprite variable
#define BK_HEIGHT 80
#define BK_WIDTH 235
  uint8_t cur_x = 68;
  uint8_t cur_y = 10;
  uint8_t dash_x = 20;
  int16_t tag_x = 235;
  int16_t fence_x = 0;
  uint8_t ani_speed = 2;

//create spirtes
  bk.createSprite(BK_WIDTH,BK_HEIGHT);

  car.createSprite(CAR_WIDTH,CAR_HEIGHT);//image size 114x50
  car.setSwapBytes(true);
  car.pushImage(0,0,CAR_WIDTH,CAR_HEIGHT,futurecar);//image size 114x50

  pole.createSprite(POLE_WIDTH,POLE_HEIGHT);
  pole.setSwapBytes(true);
  pole.pushImage(0,0,POLE_WIDTH,POLE_HEIGHT,pole1);

  fence.createSprite(FENCE_WIDTH,FENCE_HEIGHT);
  fence.setSwapBytes(true);
  fence.pushImage(0,0,FENCE_WIDTH,FENCE_HEIGHT,fence1);
//-------------------------------
  tft.fillScreen(TFT_BLACK);//clear screen  
//draw QRCode
  tft.drawBitmap(0, 159, qrcode, QRCODE_WIDTH, QRCODE_HEIGHT, TFT_WHITE);
//draw text
  tft.setTextColor(TFT_WHITE);
  tft.drawString(">MAP/ENG LOAD/ECT/EOT/TFT/ENG SPD/PCM Volt",0,40,2);
  tft.drawString(">Warning, Automatic Dim/OnOff/O็verheat Shutdown",0,60,2);
  tft.drawString(">Read DTC Code & Clear MIL Status",0,80,2);
  tft.drawString("* FW-\"VaandCobOBD2Gauge.bin\" * Image-\"mypic.jpg\"",0,100,2);
  tft.drawString("* Facebook : www.facebook.com/vaandcob",0,120,2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("  [ Manual ] ----------------[ Press button to exit ]",0,140,2);
  tft.setTextColor(TFT_CYAN); 
  String txt = "[ "+serial_no+" ] BUILD : "+compile_date;
  tft.drawString(txt,0,0,2);
  //get VIN
  getPID("ATE0");//force echo off
  txt = "VIN :  " + getVIN(getPID("0902"));
  tft.setTextColor(TFT_GREENYELLOW); 
  tft.drawString(txt,0,20,2);

  bk.setTextColor(TFT_BLACK,TFT_ORANGE);
//loop draw Animation
  while (digitalRead(SELECTOR_PIN) == HIGH) {//exit if press button

  //draw road
    bk.fillSprite(0x4228);//draw road
    bk.fillRect(0,0,235,16,TFT_LIGHTGREY);
    bk.fillRect(0,16,235,3,0x8430);

  //draw dash line
    for(int16_t i = 0; i < 5; i++) {
                     //speed     //space //posY //length
   // bk.drawFastHLine(dash_x         +(40 * i), 20,10,TFT_WHITE);//far end dash line
      bk.drawFastHLine((dash_x-20)*1.5+(60 * i),45,15,TFT_WHITE);//middle dash line
   // bk.drawFastHLine((dash_x-40)*2.0+(80 * i),79,20,TFT_WHITE);//nearest dash line
    }
    dash_x = dash_x - ani_speed;
    if (dash_x <= 0 ) dash_x = 40;

  //draw traffic 
    tag_x = tag_x - ani_speed;
    if (tag_x <= -80) tag_x = 235;
    //bk.setTextColor(random(0xffff));
    bk.drawString("< Thank You >",tag_x,0,2);//draw billboard

  //draw car
    int8_t move = random(-1,2);
    cur_x = cur_x + move;
    move = random(-1,2);
    cur_y = cur_y + move;
    if (cur_x > 235-CAR_WIDTH) cur_x = 235-CAR_WIDTH;
    if (cur_x < 1) cur_x = 1;
    if (cur_y < 1) cur_y = 1;
    if (cur_y > 79-CAR_HEIGHT) cur_y = 79-CAR_HEIGHT;
    car.pushToSprite(&bk,cur_x,cur_y,TFT_BLACK);//put car into bk, white color as transparent

  //draw fence
  for(int i=0;i<8;i++)//7 sprites
     fence.pushToSprite(&bk,fence_x*2+(FENCE_WIDTH*i),80-FENCE_HEIGHT,TFT_BLACK);
  fence_x = fence_x - ani_speed;
  if (fence_x <= -FENCE_WIDTH + ani_speed) fence_x = 0;//loop back

  //draw pole 
  pole.pushToSprite(&bk,tag_x*2,80-POLE_HEIGHT,TFT_BLACK);//draw bush

  //show background sprite
  bk.pushSprite(85,159);
  //led blinking effect
  digitalWrite(LED_RED_PIN, random(0,2));
  digitalWrite(LED_GREEN_PIN, random(0,2));
  digitalWrite(LED_BLUE_PIN, random(0,2));
  checkCPUTemp();
  autoDim();
  }
 //remove sprites
  car.deleteSprite();
  bk.deleteSprite();
  pole.deleteSprite();
  fence.deleteSprite();
}//task star field

/*------------------*/ 
//highlight and change warning each menu
void setWarning(int8_t index, int8_t change) {
  for (uint8_t i = 0;i<maxpidIndex+1;i++) {//draw all pid list
     tft.setTextColor(TFT_WHITE,TFT_BLACK);
     tft.drawString(parameterList[i][0],0,58+(i*16),2);//paramter list
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
      int min = -99;//min temp offset
      int max = 0;//max temp offset
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
    Serial.println(F("Micro SD Card not mounted!"));
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
      if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println(F("Error: SPIFFS failed!"));
        tft.setTextColor(TFT_WHITE,TFT_RED);
        tft.drawCentreString("Error: SPIFFS failed!",159,120,4); 

      } else {
       tft.setTextColor(TFT_BLACK,TFT_GREEN);
       tft.drawCentreString("Copying...\"mypic.jpg\"",159,120,4);      
       Serial.println(F("Deleting...\"mypic.jpg\"")); 
       SPIFFS.remove(mypic_filename);//remove old file first    
      File sourceFile = SD.open(mypic_filename);
      if (sourceFile) {//found file
         
         File destFile = SPIFFS.open(mypic_filename, FILE_WRITE);
         Serial.println(F("Copying...\"mypic.jpg\"")); 
         tft.setTextColor(TFT_BLACK,TFT_GREEN);
         static uint8_t buf[512];
         while(sourceFile.read( buf, 512) ) {  
          destFile.write( buf, 512 );//copy file from sdcard to spiffs
         }
         sourceFile.close();
         destFile.close();
         //load mypic.jpg from spiffs to show on screen
         show_spiffs_jpeg_image(mypic_filename, 0, 0);
    
        } else {
        Serial.println(F("\"mypic.jpg\" not found!"));
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

void listMenu(uint8_t choice) {
  //draw icon
  if (showsystem) tft.pushImage(0,35,25,25,switchon);//switchon image
  else tft.pushImage(0,35,25,25,switchoff);//switchon image
  tft.pushImage(0,65,25,25,image);//image
  tft.pushImage(0,95,25,25,cpu);//firmware
  tft.pushImage(0,125,25,25,warning);//parameter
  tft.pushImage(0,155,25,25,engine);//engine
  tft.pushImage(159,155,25,25,autooff);//off
  tft.pushImage(0,185,25,25,about);//about
  tft.pushImage(159,185,25,25,quit);//exit
  for (uint8_t i=0;i<maxMenu;i++) {
    if (choice == i)  tft.setTextColor(TFT_WHITE,TFT_BLUE);//highlight 
    else  tft.setTextColor(TFT_WHITE,TFT_BLACK); 
    if (i<4) tft.drawString(menuList[i],32,35 + 30 * i,4);//write menu list
    else if (i <6 ) tft.drawString(menuList[i],32+(i-4)*159,35 + 30 *4,4);//write menu list
    else tft.drawString(menuList[i],32+(i-6)*159,35 + 30 *5,4);//write menu list
    
  }//for i       
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.drawString("Next Menu -> Press & Release Button",0,211,2);
  tft.drawString("Select    -> Press & Hold Button",0,227,2);
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
//----------  TOGGLE SHOW SYSTEM STATUS  -----------
          if (select == 0) {
            beepbeep();
            showsystem = !showsystem;//toggle      
            tft.setTextColor(TFT_WHITE,TFT_BLUE);
            if (showsystem) tft.pushImage(0,35,25,25,switchon);//switchon image
            else tft.pushImage(0,35,25,25,switchoff);//switchon image
         
          } 
//---------  LOAD MY PIC  -----------
          if (select == 1) {
           clickSound(); 
           tft.fillRect(0,30,320,239,TFT_BLACK);
           loadMyPic();//load pic from sdcard to spiffs
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to exit -]",159,215,4); 
           while (digitalRead(SELECTOR_PIN) == HIGH) {
             //wait for button press to exit
             checkCPUTemp();
             autoDim();
           }//while 
           clickSound();
           tft.fillScreen(TFT_BLACK);//clear screen
           tft.setTextColor(TFT_BLACK,TFT_ORANGE);
           tft.drawCentreString("[---   Configuration Menu   ---]",159,0,4);
           listMenu(select);
          }

//---------  UPDATE FIRMWARE  -----------
          if (select ==2) {
           clickSound(); 
           tft.fillRect(0,30,320,239,TFT_BLACK);
           tft.pushImage(56,44,60,60,sdcard);//show sdcard icon
           tft.pushImage(203,44,60,60,wifi);//show wifi icon
           tft.setTextColor(TFT_CYAN,TFT_BLACK);
           tft.drawCentreString("Touch an icon to choose",159,125,4);
           tft.drawCentreString("firmware update method.",159,155,4);     
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to exit -]",159,215,4);    
           delay(1000);    
           while (digitalRead(SELECTOR_PIN) == HIGH) {//wait for button press to exit       
              autoDim();
              checkCPUTemp(); 
              //touching
              uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
              bool touched = getTouch(&t_x, &t_y);
              if (touched && (t_y > 44) && (t_y < 104)) {
                if ((t_x >= 56) && (t_x <116)) {
                  clickSound();
                  updateFirmwareSD();//update by sd card
                }
                if ((t_x >= 203) && (t_x <263)) {
                  clickSound();
                  updateFirmwareWIFI();//update by wifi
                  break;
                }
              }//if touched
              delay(300);//touch screen delay
            }//while 
            
            tft.fillRect(0,30,320,210,TFT_BLACK);
            listMenu(select);
          }
          
//------------ WARNING SETTING -----------
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
            for (uint8_t i = 0;i<5;i++) //draw buttons
              tft.fillRoundRect(i*64,211,60,30,5,TFT_NAVY);
            tft.setTextColor(TFT_WHITE);    
            tft.drawCentreString("DEFAULT",31,217,2);//reset
            tft.drawCentreString("^ PREV",95,217,2);//prev
            tft.drawCentreString("NEXT v",159,217,2);//next
            tft.drawCentreString("-",223,206,6);//-
            tft.fillRect(286,219,3,16,TFT_WHITE);//+
            tft.fillRect(280,225,16,3,TFT_WHITE);//+
            setWarning(warningMenuIndex,inc_dec);//list menu first
            delay(1000);
            while (digitalRead(SELECTOR_PIN) == HIGH) {//wait for button press to exit       
              autoDim();
              checkCPUTemp(); 
              String status = "";
              if (foundOBD2) {//temperature read not working if not connect to bluetooh or 
                status = "    " +String(tempRead,1)+"`c"; 
              } else {
                status = "--.-`c";
              }  
              tft.setTextColor(TFT_YELLOW,TFT_BLACK);
              tft.drawRightString(status.c_str(),319,30,2);  //show cpu temp on top-right corner
              //touching
              uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
              bool touched = getTouch(&t_x, &t_y);
              if (touched && (t_y > 211)) {
                clickSound();
                #ifdef SERIAL_DEBUG
                Serial.printf("%d - %d\n",t_x,t_y);
                #endif
                if (t_x < 64) {//DEFAULT
                  tempOffset = factoryTempOffset;//load default
                 //read default warning value
                  for (uint8_t i=0;i < maxpidIndex; i++) warningValue[i] = pidConfig[i][8];
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
          for (uint8_t i=0;i < maxpidIndex; i++) {//save all pid
            pref.putString(pidConfig[i][0].c_str(),warningValue[i]);          
          }          
          tft.setTextColor(TFT_BLACK,TFT_GREEN);
          tft.drawCentreString("Parameters Saved.",159,120,4); 
          beep();
          delay(1000);
          tft.fillRect(0,30,320,210,TFT_BLACK);
          listMenu(select);
          }

//--------------- DTC -----------
        if (select == 4) {//https://www.elmelectronics.com/wp-content/uploads/2016/07/ELM327DS.pdf 
  
          clickSound();
          tft.fillRect(0,30,320,239,TFT_BLACK);
          tft.pushImage(0,28,25,25,engine);//about
          tft.setTextColor(TFT_YELLOW,TFT_BLACK);
          tft.drawString("Diagnostic Trouble Codes",30,28,4);
          delay(500);
          getPID("ATE0");//force echo off
          //#1 get number of DTC & MIL status          
          //getAB2("41 01 86 0E 88 88 41 01 81 0C 00 00 ","41","01");//(A = 80+1,80h=MIL ON,no of dtc = A-80h)
          uint8_t error_cnt = 0;
          uint8_t no_of_dtc = 0;   
          A = 0xFF;//supoose error read A
          while ((A == 0xFF) && (error_cnt <3) ) {//if A == 0xFF repeat reading again (A must be 0x80+) try 3 times
            Serial.print(F("MIL Status 0101->"));
            getAB2(getPID("0101"),"41","01");//try read MIL status
            //Terminal(String(A).c_str(),0,88,320,125); 
            error_cnt++;
            delay(100);
           } 
#ifdef TEST_DTC
A = 0x86;   
#endif

          if (A == 0xFF) {//Error reading MIL status
           Serial.println(F("Error Reading MIL Status!"));
           tft.setTextColor(TFT_WHITE,TFT_RED);
           tft.drawCentreString("* Error Reading MIL Status! *",159,120,4);
           tft.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
           tft.drawCentreString("[ Please try again ]",159,148,4);
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to exit -]",159,215,4); 
           beep();
          } else {
            if (A == 0x00) {//41 01 00 = Mil is OFF No DTC
             tft.setTextColor(TFT_GREEN,TFT_BLACK);
             tft.drawCentreString("MIL is OFF - No DTC",159,120,4);
             tft.setTextColor(TFT_BLACK,TFT_WHITE);
             tft.drawCentreString("[- Press button to exit -]",159,215,4); 
            } else if (A >= 0x80) {//41 01 8x if A =>80h then MIL status ON else skip to NO MIL below
              Serial.print(F("A = "));Serial.println(A,HEX);                   
      //  #2 get Raw Diagnostic Troulbe Code from ELM327
            //(for 1 dtc)43 06 00 7D 
            //(more than 1 dtc) 00E 0: 43 06 00 7D C6 93 1: 01 08 C6 0F 02 E9 02 e: E0 CC CC CC CC CC CC 43 01 C4 01 
              uint8_t error_cnt = 0;
              String dtcRead =  "";
              
              //check first header 43 or 00
              while ((dtcRead.substring(0,2) != "43") && (dtcRead.substring(0,2) != "00") && (error_cnt < 5))  {
                Serial.print(F("Read DTC 03->"));
                dtcRead = getPID("03");//try read DTC code
#ifdef TEST_DTC            
dtcRead = "00E 0: 43 06 00 7D C6 93 1: 01 08 C6 0F 02 E9 02 2: E0 CC CC CC CC CC CC 43 01 C4 01";                
#endif
              //Terminal(dtcRead,0,88,320,125); 
                error_cnt++;
                delay(100);
              }  
              if (error_cnt >= 10) {//exist by error counter
                Serial.println(F("Error Reading DTCs!"));
                tft.setTextColor(TFT_WHITE,TFT_RED);
                tft.drawCentreString("* Error Reading DTCs! *",159,120,4);
                tft.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
                tft.drawCentreString("[ Please try again ]",159,148,4);
                tft.setTextColor(TFT_BLACK,TFT_WHITE);
                tft.drawCentreString("[- Press button to exit -]",159,215,4); 
                beep();
              } else {
       //#3 Interpret get the real DTC code 5 digits
            //(for 1 dtc)43 06 00 7D 
            // 00E 0: <43 06> /00 7D /C6 93 /1: /01 08 /C6 0F /02 E9 /02 2: E0 /CC CC /CC CC /CC CC /<43 01>/C4 01 
              beepbeep();        
              int byteCount = 0;//keep number of data byte
              String hexcode[128];//4x16
              uint8_t StringCount = 0; 
              dtcRead.trim();              
              while (dtcRead.length() > 0) {//split to hex byte
                int index = dtcRead.indexOf(' ');
                String getByte =  dtcRead.substring(0, index);//get first byte
                if (getByte.indexOf(':') == -1 ) {//no :
                  if (getByte.length() >= 3) {
                  byteCount = strtol(getByte.c_str(), NULL, 16);//get byte count = 14 byte
                  Serial.print(byteCount);
                  Serial.print(" = ");
                  } else {//skip ':' byte
                  hexcode[StringCount++] = getByte;//copy strings from 0 to space to hexcode
                  Serial.print(getByte + ",");             
                  if (StringCount == byteCount) break;//stop when byte count reach, ignore the rest
                  //4306007DC6930108C60F02E902E0 = 14 byte
                  }                  
                }   
                dtcRead = dtcRead.substring(index + 1);//copy the rest behind space to elm_rs             
              }//while

              Serial.println(); //debug split hex byte
            
            //interpret DTC 007D=P007D C693=U0693 0108=P0108 C60F=U060F 02E9=P02E9 02E0=P02E0
              String dtcList[16] = {};//to keep all DTC in list                
              no_of_dtc = 0;
              for (uint8_t index = 0;index < StringCount; index++) {   
                if (hexcode[index] == "43") {//check if first nibble is 43
                  index = index + 2;//skip 2 byte to next pid
                }
                String dtc_1 = hexcode[index];//C6
                uint8_t code = strtol(dtc_1.substring(0,1).c_str(),0,16);//0x0C compare with dtcMap table
                dtc_1 = dtcMap[code]+dtc_1.substring(1);//U0+6 = U06 change prefix,remove 1st char get the rest
                index++;
                String dtc_2 = hexcode[index];//0F              
                dtcList[no_of_dtc] = dtc_1+dtc_2;//U060F 
                Serial.print(dtcList[no_of_dtc] + ",");//got real DTC code here
                no_of_dtc++;//keep real no of dtc in no_of_dtc  
              }//for
              tft.setTextColor(TFT_RED,TFT_BLACK);
              String txt = "MIL is ON - No of DTCs = "+String(no_of_dtc);
              tft.drawCentreString(txt,159,58,4);
              tft.drawFastHLine(0,85,320,TFT_WHITE);
              Serial.printf("\nNo of DTCs = %d\n",no_of_dtc);
                    
        //#4  List all DTC on screen
              tft.setTextColor(TFT_WHITE,TFT_BLACK);           
              for (uint8_t i=0;i<no_of_dtc;i=i+4) {
                tft.setTextColor(TFT_PINK,TFT_BLACK);   
                tft.drawString(dtcList[i].c_str(),0,92+30*round(i/4),4);
                tft.setTextColor(TFT_YELLOW,TFT_BLACK);   
                tft.drawString(dtcList[i+1].c_str(),80,92+30*round(i/4),4);
                tft.setTextColor(TFT_GREEN,TFT_BLACK);   
                tft.drawString(dtcList[i+2].c_str(),160,92+30*round(i/4),4);
                tft.setTextColor(TFT_BLUE,TFT_BLACK);   
                tft.drawString(dtcList[i+3].c_str(),240,92+30*round(i/4),4);
              }
        //#5 show clear MIL button
               tft.setTextColor(TFT_BLACK,TFT_WHITE);
               tft.drawRightString("[Press button to exit]",319,220,2); 
               tft.fillRoundRect(0,215, 127, 24, 12, TFT_RED);
               tft.setTextColor(TFT_WHITE);
               tft.drawCentreString("Clear MIL",63,217,4); 
            }//else !error_read//
           }//else if A >0x80
          }//else if error_ent>5 

           while (digitalRead(SELECTOR_PIN) == HIGH) { //wait for button press to exit
             checkCPUTemp();
             autoDim();
             //CLEAR MIL
             if (no_of_dtc !=0) {//if there is dtc, touch checking for clear MIL
             uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
             bool touched = getTouch(&t_x, &t_y);
             #ifdef SERIAL_DEBUG
              Serial.printf("%d - %d\n",t_x,t_y);
             #endif
             if (touched && (t_y > 225) && (t_x < 127)) {//touch clear MIL button
               clickSound();
               BTSerial.print("04\r");//clear MIL 04->7F 04 78  7F 04 78
               tft.setTextColor(TFT_BLACK,TFT_GREEN);
               tft.drawCentreString("* Successful Clear MIL *",159,120,4);
               Serial.println(F("* Successful Clear MIL *"));
               beepbeep();
               delay(3000);
               break;
              }//if touch   
            }//if (no_of_dtc)
            yield();
           }//while button exit
           tft.fillRect(0,28,320,239,TFT_BLACK);
           listMenu(select);
      }//select = 4
//---------- Auto OFF Voltage -----------
          if (select == 5) {
           clickSound();
           tft.fillRect(0,30,320,210,TFT_BLACK);
           tft.pushImage(0,30,25,25,autooff);//about
           tft.setTextColor(TFT_WHITE,TFT_BLACK);
           tft.drawString("Gauge Auto-off Setting",40,30,4);
           tft.setTextColor(TFT_WHITE,TFT_BLACK);           
           tft.drawString("Set detection PCM voltage to turn off gauge.",10,65,2);
           tft.drawString("If gauge turn off while the engnine is running",10,85,2);
           tft.drawString("Please lower down the voltage",10,105,2);
           tft.drawString("Volt",220,135,4);
           tft.setTextColor(TFT_BLACK,TFT_WHITE);
           tft.drawCentreString("[- Press button to exit -]",159,180,4); 
           for (uint8_t i = 0;i<5;i++) //draw buttons
              tft.fillRoundRect(i*64,211,60,30,5,TFT_NAVY);
           tft.setTextColor(TFT_WHITE);    
           tft.drawCentreString("DEFAULT",31,217,2);//reset
           tft.drawCentreString("-",223,206,6);//-
           tft.fillRect(286,219,3,16,TFT_WHITE);//+
           tft.fillRect(280,225,16,3,TFT_WHITE);//+   
           tft.setTextColor(TFT_YELLOW,TFT_BLACK);
           String result = String(ecu_off_volt, 1);
           tft.drawCentreString(result.c_str(),159,135,4);
           delay(1000);
           while (digitalRead(SELECTOR_PIN) == HIGH) {//wait for button press to exit
            checkCPUTemp();
            autoDim();
            uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
            bool touched = getTouch(&t_x, &t_y);
            if (touched && (t_y > 211)) {
              clickSound();
              #ifdef SERIAL_DEBUG
              Serial.printf("%d - %d\n",t_x,t_y);
              #endif
              if (t_x < 64) {//DEFAULT
                ecu_off_volt = factoryECUOff;
              }

              if ((t_x >= 192) && (t_x <256)) {//decrease
                ecu_off_volt = ecu_off_volt - 0.1;
                if (ecu_off_volt <= 11.0) ecu_off_volt = 11.0; 
              }
              if ((t_x >= 256) && (t_x <320)) {//increase
                ecu_off_volt = ecu_off_volt + 0.1; 
                if (ecu_off_volt >= 15.0) ecu_off_volt = 15.0; 
              }
              String result = String(ecu_off_volt, 1);
              tft.drawCentreString(result.c_str(),159,135,4);
              delay(300);//touch screen delay
            }//if touched
           }//while 
           //exit and save warning data to preference
           pref.putFloat("ecu_off_volt",ecu_off_volt);//save ecu_off_volt
       
          tft.setTextColor(TFT_BLACK,TFT_GREEN);
          tft.drawCentreString("Parameters Saved.",159,120,4); 
          beep();
          delay(1000);
          tft.fillRect(0,30,320,210,TFT_BLACK);
          listMenu(select);
      }//select = 5      
          
//---------------------ABOUT-----------
          if (select == 6) {
            clickSound();
            delay(500);
            xTaskCreatePinnedToCore(
             &TaskPlayMusic
            , "play music in core 0"   // A name just for humans
            , 4096// This stack size can be checked & adjusted by reading the Stack Highwater
            , NULL// no variable passting to task
            , 3 //Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
            , &TaskHandle0
            , 0);//core0      
            animation();//play star field animation
            vTaskDelete(TaskHandle0);//stop animation and text animation
            pinMode(BUZZER_PIN,OUTPUT);//set pin mode again unless speaker stop working
            ledcAttachPin(BUZZER_PIN, buzzerChannel);//reattach buzzer pin
            clickSound();
            tft.fillScreen(TFT_BLACK);//clear screen
            tft.fillRectVGradient(0, 0, 320, 26, TFT_ORANGE, 0xfc00);
            tft.setTextColor(TFT_BLACK);
            tft.drawCentreString("[---   Configuration Menu   ---]",159,0,4);
            listMenu(select);
          }//if (select ==6)
//---------------------EXIT-----------
          if (select == 7) {
            clickSound();
            if (showsystem != currentFlag) {//save to eeprom only when flag changed
             pref.putBool("showsystem", showsystem);//save to NVR
            }            
            break;//exist main config menu loop while
          }//select = 7              
           pressed = false;   
           prompt = true;//to trig reading elm again 
          } //if holdtimer > 3000
        delay(250);//delay avoid bounce
       //**********************
        } else {//button release
          if (pressed) {//button release
            select++;//next menu
            if (select == maxMenu) select = 0;
            listMenu(select);//next menu
            clickSound();
            pressed = false;  
            delay(200);
          }//pressed

       }//if press&& digitalread
     checkCPUTemp();
     autoDim();
    
    } //while (true)    
    tft.fillScreen(TFT_BLACK);//clear screen
}//configMenu

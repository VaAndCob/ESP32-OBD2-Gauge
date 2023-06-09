/*=========  Va&Cob OBD2 Gauge ==========
- Hardware 2.8" TFT ESP32 LVGL https://www.youtube.com/watch?v=d2OXlVcRYrU
- Small speaker
- Small push button
- Aluminium case
- Micro SD Card
Partition scheme 1.9MB APP/ 190kB SPIFFS /OTA


*******  TO DO ***********

*/
//--- FLAG SETTING ----- for debugging
//#define TERMINAL //temrinal mode (no gauge)
//#define SERIAL_DEBUG //to show data in serial port
//#define SKIP_CONNECTION //skip elm327 BT connection to view meter

//Intermal temperature sensor function declaration
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();  //declare intermal temperature sensor functio

#define array_length(x) (sizeof(x) / sizeof(x[0])) //macro to calculate array length

//library load
#include <Preferences.h>//save permanent data
Preferences pref;//create preference
#include <BluetoothSerial.h>
#include <TFT_eSPI.h> //Hardware-specific library
#include <SPI.h>
#include <SPIFFS.h>   //safe file
#include <JPEGDecoder.h>

//#define TERMINAL// for show terminal window
//Pin configuration
#define SCA_PIN 21
#define SCL_PIN 22
#define ADC_PIN 35
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17
#define LDR_PIN 34
#define BUZZER_PIN 26//buzzer  pin
#define SELECTOR_PIN 27//select display layout
// setting PWM properties
#define backlightChannel 0
#define buzzerChannel 2

//DISPLAY
TFT_eSPI tft = TFT_eSPI();
#define TFT_GREY 0x5AEB
#define LOOP_PERIOD 100 // Display updates every 35 ms
#define CALIBRATION_FILE "/TouchCalData1"//save calibrate file

//ELM327 init
const String elm327Init[8] = {"ATWS","ATBRD23","ATAT2","ATL0","ATH0","ATSP0","ATE0"};
const uint8_t elm327InitCount = array_length(elm327Init);//get number of elm327init command
uint8_t initIndex = 0;//index for current pid
/*------ PIDS total 7 now -------------------
Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit, warn }
label = Pid label show on meter
unit = unit for pid
pid = string pid 0104
formula = formula no to calculate 
min = lowest value
max = highest value
skip = delay reading 0 - 3; 3 = max delay read
digit = want to show digit or not
warn = default warning value
*/
const String pidConfig[7][9] = {
  //[pid][data]
  { "ENG LOAD", "%", "0104", "2", "0", "100", "0", "0", "80" },     //0 = 0104
  { "Coolant", "`C", "0105", "1", "0", "120", "3", "0", "99" },     //1 = 0105
  { "MAP", "psi", "010B", "0", "0", "40", "0", "1", "35" },         //2 = 010B
  { "ENG SPD", "rpm", "010C", "3", "0", "5000", "0", "0", "4000" }, //3 = 010C
  { "PCM Volt", "volt", "0142", "4", "0", "16", "1", "1", "15" },   //4 = 0142
  { "Oil Temp", "`C", "015C", "1", "0", "120", "3", "0", "99" },    //5 = 015C
  { "TFT", "`C", "221E1C", "5", "0", "120", "3", "1", "99" }        //6 = 221E1C
};
//hold warning value
String warningValue[7] = {"80","99","35","4000","15","99","99"};
const int8_t factoryTempOffset = -34;//default offset adjustment temp up to each tested esp32
/*  User configuration here to change display 
      layout 0      layout 1       layout 2     layout 3      layout 4
    █ 1 █ █ 7 █   █ 1 █  █  █    █  █ 4 █  █   █  █  █ 7 █   █  █  █  █
    █ 2 █ █ 8 █   █ 2 █  3  4    1  █ 5 █  4   1  2  █ 8 █   1  2  3  4
    █ 3 █ █ 9 █   █ 3 █  █  █    █  █ 6 █  █   █  █  █ 9 █   █  █  █  █

set up meter here which pid to use on each cell
0 - engine load
1 - coolant
2 - manifold Pressure
3 - engine Speed
4 - pcm volt
5 - oil Temp
6 - trans Temp
*/
const byte pidInCell[5][7] = { //[layout][cellNo] 
//the last cell must be 3 (engine speed) to check engine off
  {0,2,3,1,5,6,3},//layout 0 -> {load,map,pcm,coolant,oil,tft,engspd
  {1,5,6,3,2,4,3},//layout 1 -> {cooland,oil,tft,map,pcmvolt}
  {2,1,5,6,4,0,3},//layout 2 -> {MAP,coolant,oiltemp,tft,volt,load,rpm}
  {2,0,1,5,6,4,3},//layout 3 -> {MAP,engload,coolant,oiltemp,tft,engload,rpm}
  {1,5,6,4,0,2,3},//layout 4 -> {coolant,oiltemp,tft,pcmvolt,map,engspd}
};
// User configuration here to change display  >

/*---------------------------*/
//if NO DATA value will set to 0 to skip reading
uint8_t layout = 0;//pidInCelltype 0-4 EEPROM 0x00
const uint8_t max_layout = array_length(pidInCell);//total 5 type of display page
uint8_t pidIndex = 0;//point to PID List
const uint8_t maxpidIndex = array_length(pidConfig);//total 7 pids in picConfig
bool prompt = false;//bt elm ready flag
bool skip = false;//pid skip flag
uint8_t A = 0;//get A
uint8_t B = 0;//get B

//general variable
const uint32_t SLEEP_DURATION = 180 * 1000000; //180 sec* us = 3 min
uint32_t updateTime = 3;// time for next update
String line[12] = {"","","","","","","","","","","",""};//buffer for each line in terminal function
bool    dim = false;//back light dim flag
uint8_t LIGHT_LEVEL = 40;//backlight control by light level Higher = darker light
long runtime = 0;//to check FPS
long holdtime = 0;//hold time for button pressed check
const char compile_date[] = __DATE__ " - " __TIME__;//get built date and time
float tempRead = 0.0;//current reading cpu temp
int8_t tempOffset = 0;//offset adjustment temp up to each tested esp32
const uint8_t tempOverheat = 55;//max operating cpu temp 55c
bool press = false;// button press flag
bool showsystem = false; //show fps and temp EEPROM 0x01
uint8_t pidRead = 0;//counter that hold number of pids been read to check pid/sec

//BLUETOOTH
String bt_message = "";//bluetooth message buffer
String se_message = "";//serial port message buffer
String deviceName[8] = {"","","","","","","",""};//discoveryBT name
String deviceAddr[8] = {"","","","","","","",""};//discover BT addr
int btDeviceCount = 0;//discovered bluetooth devices counter
#define BT_DISCOVER_TIME  5000//bluetooth discoery time
esp_bd_addr_t client_addr = {0x00,0x00,0x00,0x00,0x00,0x00};//obdII mac addr
String  client_name = "OBDII";//adaptor name to search
esp_spp_sec_t sec_mask=ESP_SPP_SEC_NONE; // or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE to request pincode confirmation
esp_spp_role_t role=ESP_SPP_ROLE_SLAVE; // or ESP_SPP_ROLE_MASTER
bool foundOBD2 = false;

BluetoothSerial BTSerial;//bluetooth serial
//---- Include Header File ---
#include "image.h"

//---------------------------
/*----------- global function ---------*/
void touch_calibrate() {//calibrate touch screen
  uint16_t calData[5];
  uint8_t calDataOK = 0;
  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }
  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }//if f
  }
//if can read calibrate data from spiffs file AND button not push
  if (calDataOK && (digitalRead(SELECTOR_PIN) == HIGH)) {
    tft.setTouch(calData); // calibration data valid
  } else {
    // data not valid  or press button during start
    Serial.println("Touch screen calibration...");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_BLACK, TFT_CYAN);
    tft.drawCentreString("Touch screen calibration",tft.width()/2,tft.height()/2-10,2);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Touch corners as indicated");
    tft.setTextFont(1);
    tft.println();
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    SPIFFS.remove(CALIBRATION_FILE);//Delete if we want to re-calibrate
    File f = SPIFFS.open(CALIBRATION_FILE, "w");   // store data
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }//if f
   }//else calDataOK
}//touch calibrate
//------------------------------------------------------------------------------------------
void checkCPUTemp() { //temperature can read only when BT or Wifi Connected
  if (temprature_sens_read()!= 128) {//skip 128 value read
    tempRead = (temprature_sens_read() - 32) / 1.8 + tempOffset;
    #ifdef SERIAL_DEBUG
    Serial.print(tempRead,1);   //print Celsius temperature and tab
    Serial.println("°c");
    #endif 
    if (tempRead >= tempOverheat) {
      Serial.printf("CPU Overheat Shutdown at %d°C\n",tempOverheat);
      tft.fillRectVGradient(0, 0, 320, 240, TFT_RED, TFT_BLACK);
      tft.pushImage(127,5,64,64,overheat,TFT_RED);//show sdcard icon
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("CPU Overheat Shutdown!",159,75,4);
      tft.setTextColor(TFT_WHITE,TFT_BLUE);
      tft.drawCentreString("Auto-restart within 3 minutes",159,140,4);
       tft.drawCentreString("OR",159,170,4);
      tft.drawCentreString("Press button to restart",159,200,4);
      ledcWriteTone(buzzerChannel, 1500);
      delay(5000);
      ledcWriteTone(buzzerChannel, 0);
      tft.fillScreen(TFT_BLACK);
      tft.writecommand(0x10); //TFT sleep
      //enter deep sleep
      esp_sleep_enable_timer_wakeup(SLEEP_DURATION);//sleep timer 3 min
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_27,LOW); //wake when button pressed
      esp_deep_sleep_start();  //sleep shutdown backlight auto off with esp32
    }//if tempread >= tempoverheat
  }//if temp read = 128  
}
/*------------------*/
//TERMINAL terminal for debugging display text
void Terminal(String texts,int x,int y,int w,int h) {
   int max_line = round((h-y)/16.0); 
   tft.fillRect(x, y,x+ w,y+ h, TFT_BLACK);
   tft.setTextColor(TFT_WHITE);
   for (int i=0;i<max_line;i++) {
     if (i<=max_line - 2) {
      line[i] = line[i+1];
     } else {
       line[i] = texts;
     } 
      tft.drawString(line[i],x,i*20+y,2);
   }//for
   bt_message = "";//reset bt message   
} 
//---- Include Header File ---
//#include "image.h"
#include "bluetooth.h"
#include "meter.h"
#include "starwars.h"
#include "config.h"
//---------------------------
//#### SETUP ########
void setup(void) {
  //pin configuration
  analogSetAttenuation(ADC_0db); // 0dB(1.0 ครั้ง) 0~800mV   for LDR
  pinMode(LDR_PIN,ANALOG);//ldr analog input read brightness
  pinMode(BUZZER_PIN,OUTPUT);//speaker
  pinMode(SELECTOR_PIN,INPUT_PULLUP);//button
  pinMode(LED_RED_PIN,OUTPUT);//red
  pinMode(LED_GREEN_PIN,OUTPUT);//green
  pinMode(LED_BLUE_PIN,OUTPUT);//blue
  digitalWrite(LED_RED_PIN, LOW);//on
  digitalWrite(LED_GREEN_PIN, HIGH);//off
  digitalWrite(LED_BLUE_PIN, HIGH);//off
  //pwm setup
  ledcSetup(buzzerChannel, 1500, 10);//buzzer 10 bit
  ledcSetup(backlightChannel, 12000, 8);//backlight 8 bit
  ledcAttachPin(BUZZER_PIN, buzzerChannel);//attach buzzer
 //init communication
  Serial.begin(115200);
  //memory check
  psramInit();
  Serial.println("\n---------------------------------------\n");
  Serial.printf("Free Internal Heap %d/%d\n",ESP.getFreeHeap(),ESP.getHeapSize());
  Serial.printf("Free SPIRam Heap %d/%d\n",ESP.getFreePsram(),ESP.getPsramSize());
  Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.println("\n---------------------------------------\n");
  Serial.println("<<  Va&Cob OBDII Gauge  >>");
  Serial.print("by Ratthanin W. build -> ");
  Serial.println(compile_date);

  //init TFT display
  tft.init();
  tft.setRotation(1);//landcape 
  touch_calibrate();//hold button at start to calibrate touch
  tft.setSwapBytes(true);//to display correct image color
  tft.pushImage(0,0,320,240,vaandcob);//show logo
  delay(3000);
  //backlight ledcAttachPin must set after tft.init()
  ledcAttachPin(TFT_BL, backlightChannel);//attach backlight
  for (int i=255;i>0;i--) {//fading effect
    ledcWrite(backlightChannel, i);//full bright
    delay(10);
  }
  digitalWrite(LED_RED_PIN, HIGH);//red off
  tft.fillScreen(TFT_BLACK);//show start page
  tft.fillRectVGradient(0, 0, 320, 26, TFT_YELLOW, 0x8400);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("<<  Va&Cob OBDII Gauge  >>",159,0,4);
  tft.setTextColor(TFT_WHITE,TFT_BLUE);
  tft.drawString("by Ratthanin W.  build -> ",0,26,2);
  tft.setTextColor(TFT_WHITE,TFT_RED);
  tft.drawRightString(compile_date,319,26,2);
  ledcWrite(backlightChannel,255);//full bright

//init variable
 pref.begin("setting", false);
 /* Create a namespace called "setting" with read/write mode
{ pref.get...(key,default)
layout : UShort 
showsystem: bool
tempOffset : int
warning value: int
}*/ 
 layout = pref.getUShort("layout",0);//load layout setting from NVR
 if (layout > max_layout) layout = 0;
 showsystem = pref.getBool("showsystem",false);//load showsytem 
 tempOffset = pref.getInt("tempOffset",0);//load defaultTempoffset
 for (int i=0;i < maxpidIndex; i++) {//read warning value from pid name if not found load from default
   warningValue[i] = pref.getString(pidConfig[i][0].c_str(),pidConfig[i][8]);
 }
 
//start bluetooth
  if(!BTSerial.begin("Va&Cob OBDII Gauge", true) ) {
    Serial.println("Bluetooth..error!");
    Terminal("Bluetooth..error!",0,48,320,191);
    abort();
  }else{
    Serial.println("Bluetooth..OK");
    Terminal("Bluetooth..OK",0,48,320,191);
  }
  runtime = millis();
  #ifdef SKIP_CONNECTION
  foundOBD2 = true;
  #endif
}//setup

/*###################################*/
void loop() {
//button press HOLD to config menu
if (digitalRead(SELECTOR_PIN) == LOW) {//button pressed
    if (!press) {
      press = true;//set flag
      holdtime = millis();//set timer
    } else if (holdtime - millis() > 3000) {//press once and longer than 3 sec
      configMenu();//open config menu
      press = false;//reset flag
      if (foundOBD2) initScreen();//open new layout screen
    } //else if holdtimer > 30000
    delay(200);//delay avoid bounce
} else {//button release
  if ((press) && (foundOBD2)) {//change layout next page
      layout++;//change to next layout page
      if (layout == max_layout) layout = 0;
      ledcWriteTone(buzzerChannel,5000);//play click sound
      delay(5);
      ledcWriteTone(buzzerChannel,0);
      initScreen();//open meter screen
       //reset variable
      engine_off_count = 0;
      BTSerial.flush();
      bt_message = "";
      skip = false;
      pidIndex = maxpidIndex-1;//will be +1 to be 0
      for (int i=0;i<maxpidIndex-1;i++) old_data[i] = 0.0;
      press = false;//reset flag
      delay(200);//delay avoid bounce
      prompt = true;//to trig reading elm again 
      
  }//if press
 
}//else digitalRead

//reading Bluetooth
  while (BTSerial.available()>0) {
  char incomingChar = BTSerial.read();
  //check CR/NL
    bt_message = bt_message + incomingChar;//keep elm327 respond
    if (incomingChar == '>') {
      #ifdef SERIAL_DEBUG
      Serial.println(bt_message);
      #endif 
      #ifdef TERMINAL //for degbugging
      getAB(bt_message);
      Terminal(bt_message+"->"+String(A)+","+String(B),0,48,320,191);
      #endif

      BTSerial.flush();
      prompt = true;//pid response ready to calculate & display
    }//else inocmingchar
  }

//reading Serial port
  if (Serial.available()>0) {
  char incomingChar = Serial.read();
  //check CR/NL
      if ((incomingChar != '\r') && (incomingChar !='\n')) {
       se_message = se_message + incomingChar;
       //Serial.print(incomingChar,HEX);
      } else {

       if (se_message != "") {
         #ifdef TERMINAL
         Terminal(se_message,0,48,320,191);//display terminal
         #endif                 
         BTSerial.print(se_message+"\r");
         se_message = "";
         Serial.flush();     
        }//if se_message

    }//else if incomingChar
  }//if Serial.available

 //init ELM327 
 if (!foundOBD2) {
   scanBTdevice();
   autoDim();//auto backlight handle
   //checkCPUTemp(); check temp never work if BT or WIFI not connected
  } 

//communicate with ELM327
if (prompt) {//> request pid
  prompt = false;//no prompt from ELM327
 
  if (initIndex < elm327InitCount) {//send init ELM327 AT command
    BTSerial.print(elm327Init[initIndex]+"\r");//initialize ELM327     
    initIndex++;
    if (initIndex == elm327InitCount) {//all init command sent to ELM327
      initScreen();
      //2 shot beep
      ledcWriteTone(buzzerChannel,2000);
      delay(50);
      ledcWriteTone(buzzerChannel,0);
      delay(50);
      ledcWriteTone(buzzerChannel,2000);
      delay(50);
      ledcWriteTone(buzzerChannel,0);
      //ready!
    }//if initIndex

  } else {//read each pid
    #ifdef TERMINAL
    #else
    if (!skip) {
      updateMeter(pidIndex,bt_message);//update meter screen
      pidRead++;//for calculate pid/s
    }
    #endif
    pidIndex++;//point to next pid
    if (pidIndex >= maxpidIndex) {
      pidIndex = 0;//back to pid 0 
      autoDim();//auto backlight handle
      checkCPUTemp();//check temperature.     
      if (showsystem) {
        String status = String(pidRead*1000.0/(millis()-runtime),0)+" p/s "+String(tempRead,1)+"`c";
        tft.setTextColor(TFT_YELLOW,TFT_BLACK);
        tft.drawCentreString(status.c_str(),159,0,2);
        runtime = millis();  
      }//if showsystem
      pidRead = 0;
      
    }  
    if (pidCurrentSkip[pidIndex] <= 0) {//end of skip loop , go next index 
      pidCurrentSkip[pidIndex] = pidReadSkip[pidIndex];//read max delay loop to current      
      skip = false;
      BTSerial.print(pidList[pidIndex]+"\r");//send PID request
    } else {//skip sending request
      pidCurrentSkip[pidIndex] = pidCurrentSkip[pidIndex] - 1;//decrese loop count 
      prompt = true;
      skip = true;
    } //else  
   
  }//if pid Loop

}//else prompt
 
/*------------------*/
}// loop

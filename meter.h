/* Meter display function */
/*mix color  int color = (red << (5 + 6)) | (green << 5) | blue;
uint16_t red =    tft.color565(255, 0, 0);
*/
const uint16_t screenWidth = tft.height();//landscape
const uint16_t screenHeight = tft.width();
/*---- METER ------_*
 Type 0:numericMeter
 Type 1:arcMeters
 Type 2:vBarMeter
 
coordinate for each cell (numieric meter & arc meter)
1 , 4 , 7
2 , 5 , 8
3 , 6 , 9 */
const uint16_t cell1_x[9] = { 0, 0, 0, 80, 80, 80, 160, 160, 160 };  //cell type 1 x origin cooridate
const uint16_t cell1_y[9] = { 0, 80, 160, 0, 80, 160, 0, 80, 160 };  //cell type 1 y origin coordiate
/*coordinate for each cell type 2 (Vertical bar meter)
1 , 2 , 3 , 4 */
const uint16_t cell2_x[4] = { 0, 80, 160, 240 };
bool blinkCell[9] = {false,false,false,false,false,false,false,false,false};//flag for blinking

String pidList[maxpidIndex] = {};          //{"0104","0105","010C","010B","010C","0142","015C"}
uint8_t pidReadSkip[maxpidIndex] = {};     //skip reading 0 - 3; 3 = max delay read
uint8_t pidCurrentSkip[maxpidIndex] = {};  //current skip counter
uint8_t engine_off_count = 0;//counter for sleep
float old_data[maxpidIndex] = {};//keep old data for each gauge to improve speed for pidIndex 0-6

//backlight variable
uint8_t low_count = 0;
uint8_t high_count = 0; 

// ####################################
/* draw digital numeric meter  160x80
// ####################################
cell 
1 , 2 , 3
4 , 5 , 6
7 , 8 , 9 */
void numericMeter(int cell, byte pid) {
  int w = screenWidth / 2;
  int h = screenHeight / 3;
  int x = cell1_x[cell - 1];
  int y = cell1_y[cell - 1];
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pid][0];  //get PID to label
  tft.drawRect(x, y, w, screenHeight, TFT_GREY);
  tft.fillRect(x + 2, y + 2, w - 5, h - 5, TFT_BLACK);
  tft.fillRectVGradient(x + 2, y + 2, w - 5, h * 0.33,TFT_BLUE ,0x0011);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(label, x + w / 2, y + 2, 4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawRightString("---", x + w - 5, y + (h * 0.25) + 11, 6);

}  //numericMeter
/*---------------------------*/
//plot number on digital Meter
void plotNumeric(int cell, float data, int warn, bool digit) {
  int  compare = warn/data;//compare data and warn
  if (compare == 0) {//warning
    int w = screenWidth / 2 - 1;
    int h = screenHeight / 3 - 1;
    int x = cell1_x[cell - 1];
    int y = cell1_y[cell - 1];
    if (blinkCell[cell]) {//show
      String result = String(data, digit);
      switch (result.length()) {
        case 2: result = "    " + result; break;
        case 3: result = "   " + result; break;
        case 4: result = "  " + result; break;
      }//switch
      old_data[pidIndex] = data;//update old data
      tft.setTextColor(TFT_RED, TFT_BLACK);//red color font
      tft.drawRightString(result.c_str(), x + w - 5, y + (h * 0.25) + 11, 6);
  
    }  else {//hiding make blinking effect
      tft.fillRect(x + 2, y + 28, w - 5, 50, TFT_BLACK);//delete old number
    } 
    blinkCell[cell] = !blinkCell[cell];
  
  } else {//no warning

    if (data != old_data[pidIndex]) {//if value change then redraw new data
     int w = screenWidth / 2 - 1;
     int h = screenHeight / 3 - 1;
     int x = cell1_x[cell - 1];
     int y = cell1_y[cell - 1];

    String result = String(data, digit);
    switch (result.length()) {
     case 2: result = "    " + result; break;
     case 3: result = "   " + result; break;
     case 4: result = "  " + result; break;
    }//if data
    tft.setTextColor(TFT_GREEN, TFT_BLACK);//green color font
    tft.drawRightString(result.c_str(), x + w - 5, y + (h * 0.25) + 11, 6);
    old_data[pidIndex] = data;//update old data
    } //if nothing changed then skip
  }//if warn/data == 0
}

// ##########################
/* draw analog meter 160x80
// ##########################
cell 
1 , 2 , 3
4 , 5 , 6
7 , 8 , 9 */
void arcMeter(int cell, byte pid) {
  int w = screenWidth / 2;
  int h = screenHeight / 3;
  int x = cell1_x[cell - 1];
  int y = cell1_y[cell - 1];
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pid][0];     //get PID to label
  String unit = pidConfig[pid][1];      //get PID to unit
  int min = pidConfig[pid][4].toInt();  //get PID to min
  int max = pidConfig[pid][5].toInt();  //get PID to max
  // Meter outline
  tft.fillRect(x, y, w, h, TFT_GREY);
  tft.fillRect(x + 2, y + 2, 156, 76, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);  // Text colour
  tft.drawCentreString(String(min).c_str(), x + 16, y + 10, 2);
  tft.drawCentreString(String(max).c_str(), x + w - 16, y + 10, 2);
  tft.drawRightString(unit.c_str(), x + 150, y + 60, 2);   // Units at bottom right
  tft.drawCentreString(label.c_str(), x + 80, y + 58, 2);  // Comment out to avoid font 4
  tft.setTextColor(TFT_GREEN);
  tft.drawString("---", x + 10, y + 60, 2);  // data
  //draw arc animation
  for (int angle = 135; angle < 225; angle++) {
    tft.drawSmoothArc(x + 80, y + 90, 80, 60, 134, angle, TFT_GREEN, TFT_DARKGREY, false);
   }
  for (int angle = 224; angle > 135; angle--) {
    tft.drawSmoothArc(x + 80, y + 90, 80, 60, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
  }
}
// #########################################################################
void plotArc(int cell, String label, float data, int warn, int min, int max, bool digit) {
if (data != old_data[pidIndex]) {
  //int w = screenWidth / 2 - 1;
  //int h = screenHeight / 3 - 1;
  int x = cell1_x[cell - 1];
  int y = cell1_y[cell - 1];

  String result = String(data, digit);
  switch (result.length()) {
     case 2: result = result+"   "; break;
     case 3: result = result+"  "; break;
     case 4: result = result+" "; break;
    }
  
  if (data < min) data = min;
  if (data > max) data = max;

  int arcColor = TFT_GREEN;
  int  compare = warn/data;//compare data and warn
  if (compare == 0) //warning
     arcColor = TFT_RED;//red font red arc
  else // no warn
     arcColor = TFT_GREEN;//gren font green arc
  tft.setTextColor(arcColor, TFT_BLACK);
  tft.drawString(result.c_str(), x + 10, y + 60, 2);  // data
  int angle = map(data, min, max, 135, 225);
  if (data >= old_data[pidIndex])//new data > old data -> draw only green arc
     tft.drawSmoothArc(x + 80, y + 90, 80, 60, 134, angle, arcColor, TFT_DARKGREY, false);
  else //new data < old data -> draw only white arc
     tft.drawSmoothArc(x + 80, y + 90, 80, 60, angle + 1, 226, TFT_WHITE, TFT_DARKGREY, false);
  old_data[pidIndex] = data;//update old data
  }//if 

}
/* ######################################
//draw linear vertical meter 80x240
// ######################################
cell  1 , 2 , 3 , 4 */
//vertical meter cell 1 to 4 full screen height
void vBarMeter(int cell, byte pid) {
  int x = cell2_x[cell - 1];  //start origin
  int w = screenWidth / 4;
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pid][0];     //get PID to label
  String unit = pidConfig[pid][1];      //get PID to unit
  //int min = pidConfig[pid][4].toInt();  //get PID to min
  int max = pidConfig[pid][5].toInt();  //get PID to max
  tft.drawRect(x, 0, w, screenHeight, TFT_DARKGREY);
  tft.fillRectVGradient(x + 2, 2, w - 3, screenHeight * 0.12 - 10, TFT_BLUE, 0x0011);
  tft.fillRect(x + 2, screenHeight * 0.12, w - 3, screenHeight - screenHeight * 0.24, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(label, x + w / 2, 2, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(unit,x + 3, screenHeight * 0.12-8, 1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("---", x + w / 2, screenHeight - (screenHeight * 0.12) + 1, 4);
  float smalltick = screenHeight * 0.75 / 50;
  //float bigtick = screenHeight*0.75/10;
  //draw number
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // tft.drawRigscreenHeighttString("120",x+w-2,y+screenHeight*0.12+2,1);
  for (int i = 0; i < 51; i++) {  //draw tick
    tft.drawFastHLine(x + w / 3, screenHeight * 0.12 + smalltick * i+3, 10, TFT_WHITE);
    if (i % 5 == 0) {
      tft.drawFastHLine(x + w / 3, screenHeight * 0.12 + smalltick * i+3, 20, TFT_WHITE);
      if (i % 10 == 0) {
        int mark = max - (max * i * 2 / 100);
        tft.drawRightString(String(mark), x + w - 3, screenHeight * 0.12 + smalltick * i - 7, 2);
      }                                         //i%10
    }                                           //i%5
  }                                             //for i
                                                //animation
  for (int barH = 0; barH < 183; barH++) {  //draw bar
   tft.fillRect(x + 5, 212 - barH, 20, barH, TFT_RED);//lower
  }
  for (int barH = 182; barH >= 0; barH--) {  //draw bar  
    tft.fillRect(x + 5, 29, 20, 183 - barH, TFT_BLACK);  //upper
  }
}  //drawLinerVmeter
/*-------------------*/
//vertical meter cell 1 to 4 full screen height
void plotVBar(int cell, float data, int warn, int min, int max, bool digit) {  //cell 1-4
if (data != old_data[pidIndex]) {//if value change then redraw new data
  int x = cell2_x[cell - 1];
  int w = screenWidth / 4;
  if (data < min) data = min;
  if (data > max) data = max;
  int barColor = TFT_GREEN;
  int  compare = warn/data;//compare data and warn
  if (compare == 0) //warning
     barColor = TFT_RED;//red bar red font
  else // no warn
     barColor = TFT_GREEN;//green bar green font
  //draw new bar
  int barH = map(data, min, max, 0, 182);
  if (data > old_data[pidIndex]) //new data > old data -> draw red bar
    tft.fillRectVGradient(x + 5, 212 - barH, 20, barH, barColor, 0x8000);//lower
  else //new data < old data -> draw black bar
    tft.fillRect(x + 5, 29, 20, 183 - barH, TFT_BLACK);  //upper

  tft.setTextColor(barColor, TFT_BLACK);//draw text
  String result = String(data, digit);
  switch (result.length()) {
    case 2: result = "   " + result + "   "; break;
    case 3: result = "  " + result + "  "; break;
    case 4: result = " " + result + " "; break;
  }
  tft.drawCentreString(result.c_str(), x + w / 2, screenHeight - (screenHeight * 0.12) + 1, 4);
  old_data[pidIndex] = data;//update old data
 }//if
}
/*-------------------*/
//initialize dislay by layout choose
void initScreen() {
  //String  pidList[6] = {"0105","0142","015C","010B","221E1C","010C"};
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  #ifdef SERIAL_DEBUG    
  Serial.printf("Display layout -> %d\n",layout);
  #endif
  for (int i = 0; i < 7; i++) {
    pidList[i] = pidConfig[pidInCell[layout][i]][2];                 //get PID to list
    pidReadSkip[i] = pidConfig[pidInCell[layout][i]][6].toInt();     //get read skip
    pidCurrentSkip[i] = pidReadSkip[i];  //skip for skip reading pid
    old_data[i] = 0;//clear old data
  }
  tft.fillScreen(TFT_BLACK);  //clear screen
  switch (layout) {
    case 0:
      {                                    //6 meter
                                           /*page 0
    █ 1 █ █ 7 █ 
    █ 2 █ █ 8 █ 
    █ 3 █ █ 9 █ */
        arcMeter(1, pidInCell[0][0]);  //cell,pid05
        arcMeter(2, pidInCell[0][1]);
        arcMeter(3, pidInCell[0][2]);
        numericMeter(7, pidInCell[0][3]);
        numericMeter(8, pidInCell[0][4]);
        numericMeter(9, pidInCell[0][5]);
      }
      break;
    case 1:
      {  //5 meter
         /*page 1
    █ 1 █  █  █ 
    █ 2 █  3  4 
    █ 3 █  █  █ */
        numericMeter(1, pidInCell[1][0]);
        numericMeter(2, pidInCell[1][1]);
        numericMeter(3, pidInCell[1][2]);
        vBarMeter(3, pidInCell[1][3]);
        vBarMeter(4, pidInCell[1][4]);
      }
      break;
    case 2:
      {  //5 meter
         /*page 2
  █  █ 4 █  █ 
  1  █ 5 █  4
  █  █ 6 █  █ */
        vBarMeter(1, pidInCell[2][0]);
        numericMeter(4, pidInCell[2][1]);
        numericMeter(5, pidInCell[2][2]);
        numericMeter(6, pidInCell[2][3]);
        vBarMeter(4, pidInCell[2][4]);
      }
      break;
    case 3:
      {  //5 meter
         /*page 3
    █  █  █ 7 █
    1  2  █ 8 █
    █  █  █ 9 █ */

        vBarMeter(1, pidInCell[3][0]);
        vBarMeter(2, pidInCell[3][1]);
        numericMeter(7, pidInCell[3][2]);
        numericMeter(8, pidInCell[3][3]);
        numericMeter(9, pidInCell[3][4]);
      }
      break;
    case 4:
      {  //4 meter
         /*  page 4
    █  █  █  █
    1  2  3  4
    █  █  █  █  */
        vBarMeter(1, pidInCell[4][0]);
        vBarMeter(2, pidInCell[4][1]);
        vBarMeter(3, pidInCell[4][2]);
        vBarMeter(4, pidInCell[4][3]);
      }
      break;
  }  //switch page

}  //initscreen
/*-------------------------------*/

//get A B from response return in global A B variable
void getAB(String str) {  //41 05 2A 3C 01>
  if (str == "NO DATA\r\r>") {//pid no data
    A = 0;
    B = 0;

  } else {
    //check if pid is 2 or 3 command length.
    uint8_t charcnt = pidList[pidIndex].length() / 2;  //0105->2,221E1C->3
    String strs[8];
    uint8_t StringCount = 0;
    while (str.length() > 0) {
      int index = str.indexOf(' ');
      if (index == -1)  // No space found
      {
        strs[StringCount++] = str;
        A = strtol(strs[charcnt].c_str(), NULL, 16);      //save to A
        B = strtol(strs[charcnt + 1].c_str(), NULL, 16);  //save to B
        break;
      } else {
        strs[StringCount++] = str.substring(0, index);
        str = str.substring(index + 1);
      }
    }  //while
  }    //else
}//getAB
/*-------------------------------*/
//check engine status if off then turn off gauge
void engine_onoff(int data, byte pid) {  //use engine load
//check if pid is 010C eng speed and value = 0
//String t = pidList[pid] + " - " + (String)value;
//Terminal(t,0,0,80,80);
  if (pidList[pid] == "010C") { //pis = 010C
    if (data == 0) 
       engine_off_count++;
    else
       engine_off_count = 0;  //reset
    if (engine_off_count > 20) {  //engine is stop
      if(pref.getUShort("layout",false) != layout) {//if current layout not NVR load layout
         pref.putUShort("layout", layout);//save current layout to NVR
      }      
      show_spiffs_jpeg_image("/mypic.jpg", 0, 0);//show mypic
      ledcWriteTone(buzzerChannel, 1500);//beep
      delay(500);
      ledcWriteTone(buzzerChannel, 0);
      for (int i=255;i>0;i--) {//fading effect
       ledcWrite(backlightChannel, i);//full bright
       delay(10);
      }//turn off backlight
      tft.fillScreen(TFT_BLACK);
      tft.writecommand(0x10); //TFT sleep
      esp_deep_sleep_start();//sleep shutdown backlight auto off with esp32
    }//if engine_off_count>20
  }//if PID = "010C"
}
/*-------------------------------*/
//coolant volt oiltemp vaporpressure TFT Load
// {"01051","01421","015C1","01321","221E1C1","01041"};
void updateMeter(uint8_t pidNo, String response) {  //update parameter on screen
  // Array data -> { Label , unit, pid, fomula, min, max, ,skip, digit }
  String label = pidConfig[pidInCell[layout][pidNo]][0];
  //String unit = pidConfig[pidInCell[layout][pidNo]][1];
  byte formula = pidConfig[pidInCell[layout][pidNo]][3].toInt();
  int min = pidConfig[pidInCell[layout][pidNo]][4].toInt();
  int max = pidConfig[pidInCell[layout][pidNo]][5].toInt();
  bool digit = pidConfig[pidInCell[layout][pidNo]][7].toInt();
  int warn = pidConfig[pidInCell[layout][pidNo]][8].toInt();

  getAB(response);  //get AB
  float data = 0.0;
  switch (formula) {                  //choose fomula
    case 0: data = A * 0.145; break;  //psi
    case 1: data = A - 40; break;     //temp
    case 2: data = A * 100.0 / 255; break;
    case 3: data = (256 * A + B) / 4.0; break;
    case 4: data = (256 * A + B) / 1000.0; break;
    case 5: data = (256 * A + B) / 16.0; break;
      //more formula
  }  //switch fomula

  switch (layout) {
    case 0:
      {  //6 meter
         /*page 0
    █ 1 █ █ 7 █ 
    █ 2 █ █ 8 █ 
    █ 3 █ █ 9 █ */
        switch (pidNo) {
          case 0: plotArc(1, label, data, warn, min, max, digit); break;
          case 1: plotArc(2, label, data, warn, min, max, digit); break;
          case 2: plotArc(3, label, data, warn, min, max, digit); break;
          case 3: plotNumeric(7, data, warn, digit); break;
          case 4: plotNumeric(8, data, warn, digit); break;
          case 5: plotNumeric(9, data, warn, digit); break;
          case 6: engine_onoff(data, pidNo); break;
        }
      }  //case 0
      break;
    case 1:
      {  //5 meter
         /*page 1
    █ 1 █  █  █ 
    █ 2 █  3  4 
    █ 3 █  █  █ */
        switch (pidNo) {
          case 0: plotNumeric(1, data, warn, digit); break;
          case 1: plotNumeric(2, data, warn, digit); break;
          case 2: plotNumeric(3, data, warn, digit); break;
          case 3: plotVBar(3, data, warn, min, max, digit); break;
          case 4: plotVBar(4, data, warn,  min, max, digit); break;
          case 5: engine_onoff(data, pidNo); break;
          case 6: engine_onoff(data, pidNo); break;
        }
      }  //case 1
      break;
    case 2:
      {  //5 meter
         /*page 2
    █  █ 4 █  █ 
    1  █ 5 █  4
    █  █ 6 █  █ */
        switch (pidNo) {
          case 0: plotVBar(1, data, warn,  min, max, digit); break;
          case 1: plotNumeric(4, data, warn, digit); break;
          case 2: plotNumeric(5, data, warn, digit); break;
          case 3: plotNumeric(6, data, warn, digit); break;
          case 4: plotVBar(4, data, warn,  min, max, digit); break;
          case 5: engine_onoff(data, pidNo); break;
          case 6: engine_onoff(data, pidNo); break;
        }
      }  //case 2
      break;
    case 3:
      {  //5 meter
         /*page 3
    █  █  █ 7 █
    1  2  █ 8 █
    █  █  █ 9 █ */
        switch (pidNo) {
          case 0: plotVBar(1, data, warn,  min, max, digit); break;
          case 1: plotVBar(2, data, warn,  min, max, digit); break;
          case 2: plotNumeric(7, data, warn, digit); break;
          case 3: plotNumeric(8, data, warn, digit); break;
          case 4: plotNumeric(9, data, warn, digit); break;
          case 5: engine_onoff(data, pidNo); break;
          case 6: engine_onoff(data, pidNo); break;
        }
      }  //case 3
      break;
    case 4:
      {  //4 meter
         /*  page 4
    █  █  █  █
    1  2  3  4
    █  █  █  █  */
        switch (pidNo) {
          case 0: plotVBar(1, data, warn,  min, max, digit); break;
          case 1: plotVBar(2, data, warn,  min, max, digit); break;
          case 2: plotVBar(3, data, warn,  min, max, digit); break;
          case 3: plotVBar(4, data, warn,  min, max, digit); break;
          case 4: engine_onoff(data, pidNo); break;
          case 5: engine_onoff(data, pidNo); break;
          case 6: engine_onoff(data, pidNo); break;
        }
      }  //case 4
      break;
  }                 //switch page
  bt_message = "";  //reset
}  //updatemeter
/*-------------------------------*/

//auto dim backlight function
void autoDim() {

  int light = analogRead(LDR_PIN);//read light

  if (light > LIGHT_LEVEL) {//low light
    low_count++;
    if (low_count > 10) {
      low_count = 10;//low amb light 10 times count
      if (!dim) {
        ledcWrite(backlightChannel, 50);//dim light
        dim = true;
        #ifdef SERIAL_DEBUG
        Serial.println("Backlight -> dim");
        #endif
      }//if !dim
    }//if low_count
    high_count = 0;
  
  } else {
    high_count++;
    if (high_count >10) {
      high_count = 10;//high amb light 10 times count
      if (dim) {
        ledcWrite(backlightChannel, 255);//brightest
        dim = false;
        #ifdef SERIAL_DEBUG
        Serial.println("Backlight -> bright");
        #endif
      }//if dim
    }//if high_count  
    low_count = 0;
  }

}//autodim
//---------------------------
/*############################################*/

#include <XPT2046_Touchscreen.h>

#define TOUCH_CS 33  
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK 25
//#define TOUCH_IRQ  36

XPT2046_Touchscreen ts(TOUCH_CS);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(TOUCH_CS, 255);  // Param 2 - 255 - No interrupts
//XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);  // Param 2 - Touch IRQ Pin - interrupt enabled polling

SPIClass *vspi = NULL;// Declare a pointer to SPIClass

#define _height 240
#define _width 320
#define CALIBRATION_FILE "/TouchCalData1"//save calibrate file
// Initialise with example calibration values so processor does not crash if setTouch() not called in setup()
uint16_t touchCalibration_x0 = 300, touchCalibration_x1 = 3600, touchCalibration_y0 = 300, touchCalibration_y1 = 3600;
uint8_t  touchCalibration_rotate = 1, touchCalibration_invert_x = 2, touchCalibration_invert_y = 0;
long _pressTime;

//------------------------------------------------------------------------------------------
//load calibration data
void setTouch(uint16_t *parameters){
  touchCalibration_x0 = parameters[0];
  touchCalibration_x1 = parameters[1];
  touchCalibration_y0 = parameters[2];
  touchCalibration_y1 = parameters[3];

  if(touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if(touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if(touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if(touchCalibration_y1 == 0) touchCalibration_y1 = 1;

  touchCalibration_rotate = parameters[4] & 0x01;
  touchCalibration_invert_x = parameters[4] & 0x02;
  touchCalibration_invert_y = parameters[4] & 0x04;
}
//-------------------------------------
//calibrate touch screen within windows 0,0,320,240
void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size){
  int16_t values[] = {0,0,0,0,0,0,0,0};

  for(uint8_t i = 0; i<4; i++){
    tft.fillRect(0, 0, size+1, size+1, color_bg);
    tft.fillRect(0, _height-size-1, size+1, size+1, color_bg);
    tft.fillRect(_width-size-1, 0, size+1, size+1, color_bg);
    tft.fillRect(_width-size-1, _height-size-1, size+1, size+1, color_bg);

    if (i == 5) break; // used to clear the arrows
    
    switch (i) {
      case 0: // up left
        tft.drawLine(0, 0, 0, size, color_fg);
        tft.drawLine(0, 0, size, 0, color_fg);
        tft.drawLine(0, 0, size , size, color_fg);
        break;
      case 1: // bot left
        tft.drawLine(0, _height-size-1, 0, _height-1, color_fg);
        tft.drawLine(0, _height-1, size, _height-1, color_fg);
        tft.drawLine(size, _height-size-1, 0, _height-1 , color_fg);
        break;
      case 2: // up right
        tft.drawLine(_width-size-1, 0, _width-1, 0, color_fg);
        tft.drawLine(_width-size-1, size, _width-1, 0, color_fg);
        tft.drawLine(_width-1, size, _width-1, 0, color_fg);
        break;
      case 3: // bot right
        tft.drawLine(_width-size-1, _height-size-1, _width-1, _height-1, color_fg);
        tft.drawLine(_width-1, _height-1-size, _width-1, _height-1, color_fg);
        tft.drawLine(_width-1-size, _height-1, _width-1, _height-1, color_fg);
        break;
      }

    // user has to get the chance to release
    if(i>0) delay(1000);

    for(uint8_t j= 0; j<8; j++){
      // Use a lower detect threshold as corners tend to be less sensitive
      while(!ts.touched());
      TS_Point p = ts.getPoint();
      values[i*2  ] += p.x;
      values[i*2+1] += p.y;
      }
    values[i*2  ] /= 8;
    values[i*2+1] /= 8;
  }

  // from case 0 to case 1, the y value changed. 
  // If the measured delta of the touch x axis is bigger than the delta of the y axis, the touch and TFT axes are switched.
  touchCalibration_rotate = false;
  if(abs(values[0]-values[2]) > abs(values[1]-values[3])){
    touchCalibration_rotate = true;
    touchCalibration_x0 = (values[1] + values[3])/2; // calc min x
    touchCalibration_x1 = (values[5] + values[7])/2; // calc max x
    touchCalibration_y0 = (values[0] + values[4])/2; // calc min y
    touchCalibration_y1 = (values[2] + values[6])/2; // calc max y
  } else {
    touchCalibration_x0 = (values[0] + values[2])/2; // calc min x
    touchCalibration_x1 = (values[4] + values[6])/2; // calc max x
    touchCalibration_y0 = (values[1] + values[5])/2; // calc min y
    touchCalibration_y1 = (values[3] + values[7])/2; // calc max y
  }

  // in addition, the touch screen axis could be in the opposite direction of the TFT axis
  touchCalibration_invert_x = false;
  if(touchCalibration_x0 > touchCalibration_x1){
    values[0]=touchCalibration_x0;
    touchCalibration_x0 = touchCalibration_x1;
    touchCalibration_x1 = values[0];
    touchCalibration_invert_x = true;
  }
  touchCalibration_invert_y = false;
  if(touchCalibration_y0 > touchCalibration_y1){
    values[0]=touchCalibration_y0;
    touchCalibration_y0 = touchCalibration_y1;
    touchCalibration_y1 = values[0];
    touchCalibration_invert_y = true;
  }

  // pre calculate
  touchCalibration_x1 -= touchCalibration_x0;
  touchCalibration_y1 -= touchCalibration_y0;

  if(touchCalibration_x0 == 0) touchCalibration_x0 = 1;
  if(touchCalibration_x1 == 0) touchCalibration_x1 = 1;
  if(touchCalibration_y0 == 0) touchCalibration_y0 = 1;
  if(touchCalibration_y1 == 0) touchCalibration_y1 = 1;

  // export parameters, if pointer valid
  if(parameters != NULL){
    parameters[0] = touchCalibration_x0;
    parameters[1] = touchCalibration_x1;
    parameters[2] = touchCalibration_y0;
    parameters[3] = touchCalibration_y1;
    parameters[4] = touchCalibration_rotate | (touchCalibration_invert_x <<1) | (touchCalibration_invert_y <<2);
    /*
    Serial.println("x0 = " + String(touchCalibration_x0));
    Serial.println("x1 = " + String(touchCalibration_x1));
    Serial.println("y0 = " + String(touchCalibration_y0));
    Serial.println("y1 = " + String(touchCalibration_y1));
    */
  }
}
//-------------------------------------
void touch_calibrate() {//calibrate touch screen
  uint16_t calData[5];
  uint8_t calDataOK = 0;
  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println(F("Formating file system"));
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
    setTouch(calData); // calibration data valid
  } else {
    // data not valid  or press button during start
    Serial.println(F("Touch screen calibration..."));
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_BLACK, TFT_CYAN);
    tft.drawCentreString("Touch screen calibration",tft.width()/2,tft.height()/2-10,2);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println(F("Touch corners as indicated"));
    tft.setTextFont(1);
    tft.println();
    calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println(F("Calibration complete!"));
    SPIFFS.remove(CALIBRATION_FILE);//Delete if we want to re-calibrate
    File f = SPIFFS.open(CALIBRATION_FILE, "w");   // store data
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }//if f
   }//else calDataOK
}//touch calibrate


//--------------------------
//get touch point 
bool getTouch(uint16_t *x, uint16_t *y) {
  int threshold;

  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    uint16_t x_tmp = p.x, y_tmp = p.y, xx, yy;

    if (_pressTime > millis()) threshold=20;
    uint8_t n = 5;
    uint8_t valid = 0;
    while (n--)
    {
     if (ts.touched()) valid++;;
    }
    if (valid<1) { _pressTime = 0; return false; }
    _pressTime = millis() + 50;

  //compensate
  if(!touchCalibration_rotate){
    xx=(x_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(y_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  } else {
    xx=(y_tmp-touchCalibration_x0)*_width/touchCalibration_x1;
    yy=(x_tmp-touchCalibration_y0)*_height/touchCalibration_y1;
    if(touchCalibration_invert_x)
      xx = _width - xx;
    if(touchCalibration_invert_y)
      yy = _height - yy;
  }
    
    if (xx >= _width || yy >= _height) return false;//out of window  
    *x = xx;
    *y = yy;
    return true;
  } else {
    return false;
  }
}
//--------------------------
void testTouch() {//for test touch screen
uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  tft.fillScreen(TFT_BLACK);
  while (true) {
    if(getTouch(&t_x, &t_y)) {
    Serial.printf("%d , %d\n",t_x,t_y);
    tft.drawPixel(t_x,t_y,TFT_WHITE);
    delay(300);  
    }
  }
}  
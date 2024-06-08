/* Bluetooth function */
//----------------------------------
//convert bt address to text
//{0x00,0x1d,0xa5,0x00,0x12,0x92} -> 00:1d:a5:00:12:92
String ByteArraytoString(esp_bd_addr_t bt_address) {
  String txt = "";
  String nib = "";
  for (uint8_t i=0;i<ESP_BD_ADDR_LEN-1;i++) {//0-4
    nib = String(bt_address[i],HEX);
    if (nib.length() < 2) nib = "0"+nib;
    txt = txt + nib+":";
  }//for
    
  nib = String(bt_address[ESP_BD_ADDR_LEN-1],HEX);//5
  if (nib.length() < 2) nib = "0"+nib;
  txt = txt + nib;
  return txt;
}
/*=======================*/

void scanBTdevice() {//scan bluetooth device
    digitalWrite(LED_BLUE_PIN,LOW);//blue led on
    Serial.println("Scanning for OBDII Adaptor...");
    Terminal("Scanning for OBDII Adaptor...",0,48,320,191);
    btDeviceCount = 0;
 // BTScanResults* btDeviceList = BTSerial.getScanResults();  // maybe accessing from different threads!
  if (BTSerial.discoverAsync([](BTAdvertisedDevice* pDevice) {
      // BTAdvertisedDeviceSet*set = reinterpret_cast<BTAdvertisedDeviceSet*>(pDevice);
      // btDeviceList[pDevice->getAddress()] = * set;
    String txt = pDevice->toString().c_str();
    Serial.printf("Found a new device: %s\n", pDevice->toString().c_str());
    deviceName[btDeviceCount] = pDevice->getName().c_str();
    deviceAddr[btDeviceCount] = pDevice->getAddress().toString().c_str();
    btDeviceCount++; 
    } )
    ) {
    delay(BT_DISCOVER_TIME);
    BTSerial.discoverAsyncStop();
    #ifdef SERIAL_DEBUG
    Serial.println("Discovering stopped");
    #endif
    digitalWrite(LED_BLUE_PIN,HIGH);//blue led off
    delay(1000);//redsicovery delay
  
  } else {
    #ifdef SERIAL_DEBUG
    Serial.println("Error on discovering bluetooth clients.");
    #endif
  }

  String txt = "Found "+String(btDeviceCount)+" device(s)";
  Terminal(txt,0,48,320,191);

  //matching scan obd2 and config obd2
  for (uint8_t i=0;i<btDeviceCount;i++) {
    txt = String(i+1)+". "+deviceName[i]+" - "+deviceAddr[i];
    Terminal(txt,0,48,320,191);//list devices
    delay(100)  ;

    if (deviceName[i] == client_name) {//match name.
    //found device name "OBDII"
      foundOBD2 = true;
      //00:1d:a5:00:12:92 -> {0x00,0x1d,0xa5,0x00,0x12,0x92};
      //copy match bt mac address to client_name to connect
      String str = deviceAddr[i];
      uint8_t StringCount = 0;
      while (str.length() > 0) {
        int8_t index = str.indexOf(':');
        if (index == -1)  {// No : found
          client_addr[StringCount] = strtol(str.c_str(), 0, 16);//convert hex string to byte
          str = "";//set length to 0;
          //exit loop while
        } else {
          client_addr[StringCount] = strtol(str.substring(0, index).c_str(),0,16);//convert hex string to byte
          StringCount++;
          str = str.substring(index+1);
        }
      }//while mac address copied to client_addr for connect below code
      if (ByteArraytoString(client_addr) != ByteArraytoString(recent_client_addr)) {
        pref.putBytes("recent_client",client_addr,sizeof(client_addr));//save new bt addr to pref.
        Serial.println(F("Save a new BT client address"));
      }
    } else //not match
       foundOBD2 = false;
 
  }//for loop list device


  //connect to obd2
  if (foundOBD2) {
    txt = "Connecting to " + client_name +" - " + ByteArraytoString(client_addr);
    Terminal(txt,0,48,320,191);
    Serial.println(txt);
    BTSerial.connect(client_addr, 0, sec_mask, role);//connect to OBDII adaptor
    bool blink = false;
    while(!BTSerial.connected(1000)) {
      Serial.print(F("."));
      blink =! blink;
      digitalWrite(LED_BLUE_PIN,blink);//blue led offset
      if (blink) tft.setTextColor(TFT_BLUE,TFT_BLACK);
      else tft.setTextColor(TFT_BLACK,TFT_BLACK);
      tft.drawRightString("$",319,210,4);
    }

    Terminal("Connected Successfully!",0,48,320,191);  
    Serial.println(F("Connected Successfully!"));
    prompt = true;
    digitalWrite(LED_GREEN_PIN, LOW);//green led 

  } else {
    Terminal("OBDII Adaptor not found!",0,48,320,191);
    Serial.println(F("OBDII Adaptor not found!"));
  } //foundOBD2
}//scanBTDevice

//---------------------------
//connect to recent obdII for fast connection, skip scanning
void connectLastOBDII() {
  digitalWrite(LED_BLUE_PIN,LOW);//blue led on  
  String txt = "Connecting to " + client_name +" - " + ByteArraytoString(recent_client_addr);
  Terminal(txt,0,48,320,191);
  Serial.println(txt);
  BTSerial.connect(recent_client_addr, 0, sec_mask, role);//connect to OBDII adaptor
  uint8_t try_count = 0;
  bool blink = false;
  while(!BTSerial.connected(1000) && (try_count <3)) {
    Serial.print(F("."));
    blink =! blink;
    digitalWrite(LED_BLUE_PIN,blink);//blue led offset
    try_count++;
  }
  if (try_count == 3) {//cannot connect
    Terminal("OBDII Adaptor not found!",0,48,320,191);
    Serial.println(F("OBDII Adaptor not found!"));
    BTSerial.disconnect();
    foundOBD2 = false;
  } else {  
    Terminal("Connected Successfully!",0,48,320,191);  
    Serial.println(F("Connected Successfully!"));
    prompt = true;
    digitalWrite(LED_GREEN_PIN, LOW);//green led 
    foundOBD2 = true;
  }
 
}//connectLasbtOBDII
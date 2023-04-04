/* Bluetooth function */
/*=======================*/
void scanBTdevice() {//scan bluetooth device
   digitalWrite(LED_BLUE_PIN,LOW);//blue led on
   #ifdef SERIAL_DEBUG
   Serial.println("Discovering bluetooth client...");
   #endif
   Terminal("Scanning for OBDII Adaptor...",0,42,320,230);

  btDeviceCount = 0;
 // BTScanResults* btDeviceList = BTSerial.getScanResults();  // maybe accessing from different threads!
  if (BTSerial.discoverAsync([](BTAdvertisedDevice* pDevice) {
      // BTAdvertisedDeviceSet*set = reinterpret_cast<BTAdvertisedDeviceSet*>(pDevice);
      // btDeviceList[pDevice->getAddress()] = * set;
    String txt = pDevice->toString().c_str();
    #ifdef SERIAL_DEBUG
    Serial.printf("Found a new device: %s\n", pDevice->toString().c_str());
    #endif
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
  Terminal(txt,0,42,320,230);

  //matching scan obd2 and config obd2
  for (int i=0;i<btDeviceCount;i++) {
    txt = String(i+1)+". "+deviceName[i]+" - "+deviceAddr[i];
    Terminal(txt,0,42,320,230);//list devices
    delay(100)  ;

    if (deviceName[i] == client_name) {//match name.
    //found device name "OBDII"
       foundOBD2 = true;
       //00:1d:a5:00:12:92 -> {0x00,0x1d,0xa5,0x00,0x12,0x92};
       //copy match bt mac address to client_name to connect
       String str = deviceAddr[i];
       int StringCount = 0;
       while (str.length() > 0)
       {
         int index = str.indexOf(':');
         if (index == -1)  {// No : found
          client_addr[StringCount] = strtol(str.c_str(), 0, 16);//convert hex string to byte
          str = "";//to go out of while loop
        } else {
          client_addr[StringCount] = strtol(str.substring(0, index).c_str(),0,16);//convert hex string to byte
          StringCount++;
          str = str.substring(index+1);
        }
       }//while mac address copied to client_addr for connect below code
    } else //not match
       foundOBD2 = false;
 
  }//for loop list device

  //connect to obd2
  if (foundOBD2) {
   txt = "Connecting to " + client_name +" - " + ByteArraytoString(client_addr);
   Terminal(txt,0,42,320,230);
   #ifdef SERIAL_DEBUG
   Serial.println(txt);
   #endif
   BTSerial.connect(client_addr, 0, sec_mask, role);
   prompt = true;
   delay(1000);
   digitalWrite(LED_GREEN_PIN, LOW);//green led 
  } else {
   Terminal("OBDII Adaptor not found!",0,42,320,230);
   #ifdef SERIAL_DEBUG
   Serial.println("OBD2 not found");
   #endif
  } //foundOBD2
}//scanBTDevice


//---------------------------
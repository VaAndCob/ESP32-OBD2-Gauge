//elm327 interpreter

//DTC prefex code mapping
const String dtcMap[16] = {"P0","P1","P2","P3","C0","C1","C2","C3","B0","B1","B2","B3","U0","U1","U2","U3"};

//get A B from response return in global A B variable
void getAB2(String elm_rsp,String mode,String para) {  //41 05 2A 3C 01>
    elm_rsp.trim();
    String hexcode[16];
    uint8_t StringCount = 0;
    while (elm_rsp.length() > 0) {//keep reading each char
      int index = elm_rsp.indexOf(' ');//check space
      if (index == -1)  // No space found
      { //only data without space is now in hexcode {41,05,aa,bb}
        hexcode[StringCount++] = elm_rsp; //last byte
        //for (index=0;index<StringCount;index++) Serial.println(hexcode[index]);
        if ((hexcode[0] == mode) && (hexcode[1] == para)) {//check correct response
        A = strtol(hexcode[2].c_str(), NULL, 16);  //byte 3 save to A
        B = strtol(hexcode[3].c_str(), NULL, 16);  //byte 4 save to B
        } else A = 0xFF;//error
        break;
      } else {//found space
        hexcode[StringCount++] = elm_rsp.substring(0, index);//copy strings from 0 to space to hexcode
        elm_rsp = elm_rsp.substring(index + 1);//copy the rest behind space to elm_rsp
      }
    }  //while

}//getAB
/*------------------*/ 
String getPID(String pid) {//fucntion getpid response from elm327
    while (BTSerial.available()>0) {//clear rx buffer
      BTSerial.read();
    }  
   String bt_response = "";
   BTSerial.print(pid+"\r");//get no of dtc
   delay(500);//wait for response
   while (BTSerial.available()>0) {//reading Bluetooth
    char incomingChar = BTSerial.read();
    //check CR/NL
    if (incomingChar == '>') {
      Serial.println(bt_response);
      BTSerial.flush();
    } else {
      bt_response.concat(incomingChar);//keep elm327 respond
    }
   } //while BTSerial
   return bt_response;
}
/*------------------*/ 
/* VIN reading
String resp = "014 
0: 49 02 01 4D 50 42 
1: 41 4D 46 30 36 30 4E 
2: 58 34 33 37 30 39 33 "; */
//convert ascii code to Charactor
const char asciiTable[0x60]= " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}`";
char HextoChar(uint8_t asciiCode) {
  if ((asciiCode > 0x20) && (asciiCode < 0x80)) {
      return(asciiTable[asciiCode-32]);
  }
  else return '\0';//null-terminator
}
//----------------------------
//function get VIN   Serial.println(getVIN(getPID(0902)));
String getVIN(String elm_rsp) {
  String VIN = "";
  uint8_t byteCount = 0;
  elm_rsp.trim();
  while (elm_rsp.length() > 0) {//keep reading each char
    int index = elm_rsp.indexOf(' ');//check space
    String getByte =  elm_rsp.substring(0, index);//get first byte
    if (index == -1)  {//no space found
        byte ascii = strtol(getByte.c_str(), NULL, 16);//read ascii
        VIN.concat(HextoChar(ascii));//last char
        //check correct VIN
        if (VIN.length() == byteCount) {
          VIN = VIN.substring(3);//remove first 3 string
          Serial.println("VIN: " + VIN);
          return VIN;//return VIN number
        } else {
          return "Cannot read VIN";
        }

    } else {//found space
      if (getByte.indexOf(':') == -1 ) {
        if (getByte.length() >= 3) {
          byteCount = strtol(getByte.c_str(), NULL, 16);//get byte count
        } else {//skip ':' byte
          byte ascii = strtol(getByte.c_str(), NULL, 16);//read ascii
          VIN.concat(HextoChar(ascii));//convert to hex charactor
        }
      } 
      elm_rsp = elm_rsp.substring(index + 1);//copy the rest behind space to elm_rsp.
    }//else
  }  //while
  return "Cannot read VIN";
}//getVIN
//----------------------------


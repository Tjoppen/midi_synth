//NOTE: 6N136 has just an NPN output compared to 6N138 which has an
//      NPN darlington output. It needs to be complemented with a
//      suitable NPN transistor such as BC546B.

void setup() {
  Serial.begin(31250);
}

byte ctrl = 0;
byte isvel = 0;
byte shouldoutput = 0;

void loop() {
  if (Serial.available() > 0) {
    byte in = Serial.read();
    if (in & 0x80) {
      Serial.write(in);
      ctrl = in & 0xF0;
      isvel = 0;
    } else {
      if (ctrl == 0x90) {
        //note on
        if (isvel) {
          if (shouldoutput) {
            Serial.write(in);
          }
        } else {
          //only output C3 and up
          shouldoutput = in >= 60;
          if (shouldoutput) {
            Serial.write(in);
          }
        }
        isvel = !isvel;
      } else {
        //pass anything that isn't "note on"
        Serial.write(in);
      }
    }
  }
}


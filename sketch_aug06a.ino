//NOTE: 6N136 has just an NPN output compared to 6N138 which has an
//      NPN darlington output. It needs to be complemented with a
//      suitable NPN transistor such as BC546B.

unsigned short freq[128];
byte vol[128];

#define SRATE 20000         //sample rate
#define PINOFS 2
#define MAX_KEYS 5
unsigned short osc[MAX_KEYS];
unsigned char keys[MAX_KEYS];
unsigned char nkeys = 0;

void computeKeys() {
  for (unsigned char x = 0; x < MAX_KEYS; x++) {
    keys[x] = 0;
  }
  nkeys = 0;
  for (unsigned char x = 0; x < 128; x++) {
    if (vol[x]) {
      keys[nkeys++] = x;
      if (nkeys == MAX_KEYS)
        break;
    }
  }
}

void setup() {
  Serial.begin(31250);

  // initialize timer1
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 16000000 / SRATE; // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS10);    // 1 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt

  for (char x = PINOFS; x < PINOFS+MAX_KEYS; x++) {
    pinMode(x, OUTPUT);
  }

  for (char x = 0;; x++) {
    //A4 = 69 (440 Hz)
    float f = 440.0f * powf(2, (x - 69) / 12.0);
    freq[x] = 65536 * f / SRATE;
    //osc[x] = 0;
    vol[x] = 0;
    if (x == 127)
      break;
  }
  computeKeys();
  interrupts();             // enable all interrupts
}

ISR(TIMER1_COMPA_vect) {
  for (unsigned char x = 0; x < nkeys; x++) {
    digitalWrite(x + PINOFS, (osc[x] += freq[keys[x]]) >> 15);
  }
}

byte key = 0;
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
          vol[key] = in;
          if (shouldoutput) {
            Serial.write(in);
          }
          //key up or key down - recompute keys
          computeKeys();
        } else {
          //only output C3 and up
          key = in;
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


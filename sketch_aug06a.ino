//NOTE: 6N136 has just an NPN output compared to 6N138 which has an
//      NPN darlington output. It needs to be complemented with a
//      suitable NPN transistor such as BC546B.

unsigned short freq[128+1];
byte vol[128];

#define SRATE 20000         //sample rate
#define PINOFS 3
#define MAX_KEYS 5
unsigned short osc[MAX_KEYS];
unsigned char keys[MAX_KEYS];
unsigned char nkeys = 0;

void computeKeys() {
  for (unsigned char x = 0; x < MAX_KEYS; x++) {
    keys[x] = 128;
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

  DDRB = 1;
  DDRD = 0xFF;

  for (char x = 0;; x++) {
    //A4 = 69 (440 Hz)
    float f = 440.0f * powf(2, (x - 69) / 12.0);
    freq[x] = 65536 * f / SRATE;
    //osc[x] = 0;
    vol[x] = 0;
    if (x == 127)
      break;
  }
  freq[128] = 0;
  computeKeys();
  interrupts();             // enable all interrupts
}

void oldout() {
	for (unsigned char x = 0; x < nkeys; x++) {
	    digitalWrite(x + PINOFS, (osc[x] += freq[keys[x]]) >> 15);
	}
}

void newout() {
	unsigned char out = 0;
	for (unsigned char x = 0; x < nkeys; x++) {
		osc[x] += freq[keys[x]];
		out = (out >> 1) | (osc[x] >> 8) & 0x80;
	}
	PORTD = out;
}

void fixout() {
	unsigned char out = 0;
	for (unsigned char x = 0; x < MAX_KEYS; x++) {
		osc[x] += freq[keys[x]];
		out = (out >> 1) | (osc[x] >> 8) & 0x80;
	}
	PORTD = out;
}

ISR(TIMER1_COMPA_vect) {
  PORTB = 1;
  //oldout();	//5 notes = 34.40 µs
  //newout();	//5 notes = 10.56 µs
  fixout();		//5 notes =  8.08 µs
  PORTB = 0;
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


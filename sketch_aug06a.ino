//NOTE: 6N136 has just an NPN output compared to 6N138 which has an
//      NPN darlington output. It needs to be complemented with a
//      suitable NPN transistor such as BC546B.

unsigned short freq[128+1];
byte vol[128];

#define SRATE 44100         //sample rate
#define PINOFS 3
#define MAX_KEYS 8
#define DAC_BITS 6

short osc[MAX_KEYS];
unsigned char keys[MAX_KEYS];
unsigned char vols[MAX_KEYS];
unsigned char keys2[MAX_KEYS];
unsigned char vols2[MAX_KEYS];
unsigned char newkeys;      //set when a new keys/vols buffer can be copied
unsigned char lut[1 << DAC_BITS];	//DAC correction LUT

#define DAC_IN A0			//pin to read DAC from

void computeKeys() {
  while (newkeys);

  for (unsigned char x = 0; x < MAX_KEYS; x++) {
    keys2[x] = 128;
    vols2[x] = 0;
  }
  unsigned char nkeys = 0;
  for (unsigned char x = 0; x <= MAX_KEYS; x++) {
    if (vol[x]) {
      vols2[nkeys]   = vol[x];
      keys2[nkeys++] = x;
    }
  }

  newkeys = 1;
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
  newkeys = 0;

  for (char x = 0; x <= 127; x++) {
    //A4 = 69 (440 Hz)
    float f = 440.0f * powf(2, (x - 69) / 12.0);
    freq[x] = 65536 * f / SRATE;
    //osc[x] = 0;
    vol[x] = 0;
  }
  freq[128] = 0;
  computeKeys();

  //calibrate DAC
  //first measure each output voltage
  unsigned char dac[1 << DAC_BITS];
  for (unsigned char x = 0; x != (1 << DAC_BITS); x++) {
	  PORTD = x << (8 - DAC_BITS);

	  //let the output settle a bit - total time is still just 202 ms
	  //TODO: put a buffer/amp between the DAC and speaker so output impedance doesn't disturb this
	  //multiple calls because one delay(5) doesn't work properly for some reason
	  delay(1);
	  delay(1);
	  delay(1);
	  delay(1);
	  delay(1);

	  //ADC gives us 10 bits, but we only care about the DAC_BITS topmost ones
	  PORTB = 1;
	  dac[x] = (analogRead(DAC_IN)) >> (10 - DAC_BITS);
	  PORTB = 0;
  }

  //next figure out which value is closest to what we want borka borka
  for (unsigned char x = 0; x != (1 << DAC_BITS); x++) {
	  unsigned char besty = x;
	  unsigned char best = 0xFF;
	  for (unsigned char y = 0; y != (1 << DAC_BITS); y++) {
		  unsigned char diff;
		  if (dac[y] > x) {
			  diff = dac[y] - x;
		  } else {
			  diff = x - dac[y];
		  }

		  if (diff < best || (diff == best && x == y)) {
			  best = diff;
			  besty = y;
		  }
	  }
	  lut[x] = besty << (8 - DAC_BITS);
  }
  PORTD=0;

  interrupts();             // enable all interrupts
}


void volout() {
	unsigned char out = 0;
	for (unsigned char x = 0; x < MAX_KEYS; x++) {
		osc[x] += freq[keys[x]];
		if (osc[x] >= 0) {
			//detect overflow
			unsigned char temp = out + vols[x];
			out = !(temp & 0x80) && (out & 0x80) ? 0xFF : temp;
		}
	}
	PORTD = lut[out >> (8 - DAC_BITS)];
}

unsigned char x = 0;
ISR(TIMER1_COMPA_vect) {
	//PORTD = lut[(x++) >> (8 - DAC_BITS)];
	//return;

  PORTB = 1;
  volout();
  PORTB = 0;

  if (newkeys) {
    PORTB = 1;
    for (unsigned char x = 0; x < MAX_KEYS; x++) {
      keys[x] = keys2[x];
      vols[x] = vols2[x];
    }
    newkeys = 0;
    PORTB = 0;
  }
}

byte key = 0;
byte ctrl = 0;
byte isvel = 0;
byte shouldoutput = 0;

void decay() {
  for (unsigned char x = 0; x < 128; x++) {
    if (vol[x]) {
      vol[x]--;
    }
  }
  for (unsigned char x = 0; x < MAX_KEYS; x++) {
    if (vols[x]) {
      vols[x]--;
    }
  }
  //computeKeys();
}

byte m = 0, d = 0;
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
  } else {
    byte m2 = millis();
    if (m != m2) {
      m = m2;
      if (d++ >= 10) {
        decay();
        d = 0;
      }
    }
  }
}


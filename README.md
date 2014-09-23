# midi_synth

A simple Arduino Nano based MIDI synthesizer/tone generator with 8-note polyphony and 44.1 kHz sample rate.

## How it works

MIDI commands are received on the UART at 31250 baud via an optocoupler.
Commands are decoded and up to eight software oscillators are configured accordingly.
Synthesized samples are output on a 6-bit DAC (PORTD7..2).
The DAC is calibrated during startup via ADC channel 0.
DAC output is fed to an amplifier, then either to a speaker or line-out.

## TODO

* ADSR, configurable via potentiometers hooked to ADC pins?

* decent low-power amplifier or line-out

## Construction

The entire thing can be built on a breadboard if you use an Arduino Nano (or an ATMega328P with a 16 MHz crystal).

### DAC

The DAC is most easily built as an R2R ladder - search the internet if you don't know how to do this.
Since the DAC is calibrated during startup the resistor values don't have to be terribly accurate - 10 and 22 Ohm, 5% tolerance worked fine for me.
Connect the DAC to a suitable amplifier, even if you're just going for line-out. The reason is that putting a load on the DAC messes with the calibration and the cable from the line-out might be arbitrarily long.
Also connect the DAC to ADC channel 0 (PORTC0).

### MIDI interface

Use an optocoupler, easiest is a Darlington kind (6N138). I only had a 6N136 on hand which meant its output had to be amplified with an NPN transistor (BC546B, collector -> pin 6, base -> pin 5, emitter -> GND).

Connect pin 4 from the MIDI jack (01:30-o'-clock, second-rightmost pin looking at the back of the connector) via a 220 Ohm resistor to the anode (+, pin 2) of the optocoupler LED. Connect pin 5 (10:30-o'-clock, second-*leftmost* pin) to the cathode (-, pin 3). Finally connect a small diode (1N4148) in reverse across the optocoupler LED to protect it from back emf. Connect the optocoupler's output (pin 6) to the RX pin on the Arduino. Don't forget to power the optocoupler with VCC -> pin 8. Double-check with the internet if you're unsure.

TODO: diagram?


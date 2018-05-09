
// github.com/FortySevenEffects/arduino_midi_library is used in conjunction with
// github.com/ddiakopoulos/hiduino to provide MIDI functionality via USB
#include "MIDI.h"

byte lastNoteOn;
int notePlaying;

MIDI_CREATE_DEFAULT_INSTANCE();

// Implements SPI for use with the MCP4921 DAC, which
// sends CV to the Gakken
#define DATAOUT 11 //MOSI
#define DATAIN 12 //MISO - not used, but part of builtin SPI
#define SPICLOCK  13 //sck
#define SLAVESELECT 10 //ss

void gakken_write_value(uint16_t sample) {
  uint8_t dacSPI0 = 0;
  uint8_t dacSPI1 = 0;
  dacSPI0 = (sample >> 8) & 0x00FF;
  dacSPI0 |= 0x10;
  dacSPI1 = sample & 0x00FF;
  digitalWrite(SLAVESELECT,LOW);
  
  SPDR = dacSPI0; // Start the transmission
  
  // Wait for the end of the transmission
  while (!(SPSR & (1<<SPIF))){};
  
  SPDR = dacSPI1;
  
  // Wait for the end of the transmission
  while (!(SPSR & (1<<SPIF))){};
  
  digitalWrite(SLAVESELECT,HIGH);
  delay(5);
}

uint16_t note_to_cv(byte pitch) {
  // DACval note MIDIval
  // 1270 C4 60
  // 1125 C3 48 (according to tuner this is actually G2)
  // 980  C2 36
  // 835  C1 24
  // 690  C0 12
  
  /*
    -- Neu Tabelen --
    
    G6  2000
    G2  1125
    B2  1200
    A2  btwn 1152 -> 1154 (!!)
    C3  1210
  */
  
  int diff = 48 - pitch;
  diff *= 12;
  return 1210 - diff;
}

void gakken_noteOn(byte channel, byte pitch, byte velocity) {
  lastNoteOn = pitch;
  notePlaying = 1;
  uint16_t cv_value = note_to_cv(pitch);
  gakken_write_value(cv_value);
}

void gakken_noteOff(byte channel, byte pitch, byte velocity) {
  if(pitch == lastNoteOn && notePlaying == 1){
      notePlaying = 0;
      gakken_write_value(0);
  }
}

void gakken_pitchBend(byte channel, int bend) {
  if(notePlaying == 1){
    int current_cv_value = note_to_cv(lastNoteOn);
    float bendAmount = (1 + ((float) bend) / 8190);
    int bentVal = current_cv_value * bendAmount;
    
    if (bentVal > 0)
      gakken_write_value(bentVal);
  }
}

void setup()
{
  // SPI configuration
  byte clr;
  pinMode(DATAOUT, OUTPUT);
  pinMode(DATAIN, INPUT);
  pinMode(SPICLOCK,OUTPUT);
  pinMode(SLAVESELECT,OUTPUT);
  digitalWrite(SLAVESELECT,HIGH); //disable device
  
  SPCR = (1<<SPE)|(1<<MSTR);
  clr=SPSR;
  clr=SPDR;
  
  delay(10);

  // MIDI library configuration
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(gakken_noteOn);
  MIDI.setHandleNoteOff(gakken_noteOff);
  MIDI.setHandlePitchBend(gakken_pitchBend);
}

void loop() {
    MIDI.read();
}


// github.com/FortySevenEffects/arduino_midi_library is used in conjunction with
// github.com/ddiakopoulos/hiduino to provide MIDI functionality via USB
#include "MIDI.h"

/*
  State keeps track of the synthesizer's current state in a
  globally accessable struct.
  
  Values are updated on an as-needed basis by handler functions according to messages
  recieved via MIDI. Values are read in cases where the functionality of some handlers is
  affected by the synthesizer's current state, or when such state is needed to enforce sanity
  checks.
  
  For example:
  
  noteOn:
    On any noteOn message, the lastNotePlayed and lastNoteCV values are updated to match the
    current note. To reflect thefact that a note is being held down, the noteIsPlaying value will be set to 1.
    Finally, the noteOn handler will write the latest value to the DAC, taking into account any pitch bending
    being applied.
  
  noteOff:
    noteOff should will apply only to the most recent note that was played. Some implementations I've seen
    do not perform this check, so the Gakken will stop playing completely when any noteOff message comes in
    (which is not what we want).
    
    To enforce this, the noteOff message handler will check against the current state: A noteOff for C3 would cause C3 to stop
    playing if and only if the value of lastNotePlayed is C3, and a note is currently being played (i.e noteIsPlaying = 1).
    In practice, this works like so: if you were to play C, then tap E (while still holding C), then release C (while still
    holding E), E would remain playing.
  
  Pitch Bend:
    The pitchbend handler will always react to incoming pitchbend messages, but the result will only be applied in certain cases.
    This handler performs a sanity check on the calculated pitchBendMultiplier against the current CV value, to ensure that invalid
    pitchbend multipliers are not generated for the current note.
    
    Assuming the check passes, the State's pitchBendMultiplier will be updated with the new value. Finally, the handler will write
    a new, pitch-bent CV value to the DAC only if a note is playing. This is what allows you to apply pitch bend in realtime to
    a note you're currently holding.
  
  Modulation/Midi CC:
    The Midi Control Change (CC) messages handler acts as a dispatch to handlers for various CC message types.
    
*/
struct State {
  byte lastNotePlayed;
  uint16_t lastNoteCV;
  int noteIsPlaying;
  float pitchBendMultiplier;
  float noteGlideRate;
};

struct State state;

// Implements SPI for use with the MCP4921 DAC, which
// sends CV to the Gakken
#define DATAOUT 11 //MOSI
#define DATAIN 12 //MISO - not used, but part of builtin SPI
#define SPICLOCK  13 //sck
#define SLAVESELECT 10 //ss

MIDI_CREATE_DEFAULT_INSTANCE();

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

void glide_to_note(uint16_t cv_value){
  // state.noteIsGliding (?)
  return;
}

void handle_noteOn(byte channel, byte pitch, byte velocity) {
  state.lastNotePlayed = pitch;
  state.lastNoteCV = note_to_cv(pitch);
  state.noteIsPlaying = 1;
  
  // Hook noteOn to apply a glide between notes, if applicable.
  // state.noteGlideRate will equal 0 if the Mod wheel (CC #1) is zeroed
  if(state.noteGlideRate > 0)
    glide_to_note(state.lastNoteCV);
  
  gakken_write_value(state.lastNoteCV * state.pitchBendMultiplier);
}


void handle_noteOff(byte channel, byte pitch, byte velocity) {
  
  // We should only handle not OFF messages if the message is for the note that's
  // currently being held, and additionally only if a note is currently playing.
  if(pitch != state.lastNotePlayed || state.noteIsPlaying == 0)
    return;

  state.noteIsPlaying = 0;
  gakken_write_value(0);
}

void handle_pitchBend(byte channel, int bend) {
  float bendAmount = 1 + (((float) bend) / 8190);
  int bentVal = state.lastNoteCV * bendAmount;

  /*
    Sanity check - After bending, the next CV value we're going to
    write should never be less than 0.
    
    This function is called on any pitchbend message, so it's
    OK to short circuit here. Only valid results will be applied.
  */
  if (bentVal < 0)
    return;
    
  state.pitchBendMultiplier = bendAmount;
    
  if(state.noteIsPlaying == 1)
    gakken_write_value(bentVal);
}

void handle_glideParamUpdate(uint16_t cv_value){
  return;
}

// Recieves all incoming Control Change messages (CC), and dispatches
// the payloads to their appropriate handlers
void handle_CC(byte channel, byte number, byte value){
  
  switch(number){
    case 1: // MIDI CC #1 is the Mod Wheel
      handle_glideParamUpdate(channel, number, value);
      break;
      
    default:
      return;
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
  
  state.lastNotePlayed = 0;
  state.noteIsPlaying = 0;
  state.pitchBendMultiplier = 1;
  state.noteGlideRate = 0;

  // MIDI library configuration
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handle_noteOn);
  MIDI.setHandleNoteOff(handle_noteOff);
  MIDI.setHandlePitchBend(handle_pitchBend);
  MIDI.setHandleControlChange(handle_CC);
}

void loop() {
    MIDI.read();
    
    // NOTE: might be necessary to perform note gliding here to prevent blocking
    // the main thread in a handler?
}

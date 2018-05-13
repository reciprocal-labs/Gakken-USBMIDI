/*
  Gakken SX-150 MIDI Implementation
  (c) 2018 Charlton Trezevant
  https://github.com/reciprocal-band/Gakken-USBMIDI
  
  The stated goal of this project is to be the best MIDI CV generator available for the Gakken SX-150, and
  potentially for other stylophone-type synthesizers, as well. I'm hopeful that this will enable more
  people to find creative inspiration and enjoyment from these inexpensive, yet powerful little machines.
  
  This project leverages work by the following geniuses:
  
    The SPI implementation used in this project (seen in configure_SPI and gakken_write_value) was adapted from
    mrbook.org/blog/2008/11/22/controlling-a-gakken-sx-150-synth-with-arduino/
    
    Levi helped a lot with getting things wired together properly.
    
    Mr.Ginex was immensely helpful with getting tuning dialed in.
    
    The Arduino MIDI library is used in conjunction with HIDuino to provide
    MIDI functionality via USB.

    github.com/FortySevenEffects/arduino_midi_library
    github.com/ddiakopoulos/hiduino
 
  All other components are my own.
    
  ---

  Notes and Documentation

  State keeps track of the synthesizer's current state in a
  globally accessable struct, which is used as a form of mutex.
  
  Message Handling
  
  Values are updated on an as-needed basis by handler functions according to messages
  recieved via MIDI. Values are read in cases where the functionality of some handlers is
  affected by the synthesizer's current state, or when such state is needed to enforce sanity
  checks.

  noteOn:
    On any noteOn message, the lastNotePlayed and lastNoteCV values are updated to match the
    current note. To reflect the fact that a note is being held down, the noteIsPlaying value will be set to 1.
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
    The Midi Control Change (CC) message handler acts as a dispatch to sub-handlers for various CC message types. As an example
    (but mostly just for fun), I've used this to trap modwheel CCs for note gliding functionality.
*/

#include "MIDI.h"

// The maximum possible time to glide to a new note in glide mode
#define MAX_GLIDE_TIME 2000

struct State {
  int noteIsPlaying; // Is a note currently playing?
  int noteIsGliding; // Are we currently gliding to a note?
  float noteGlideTime; // How fast are we supposed to glide? (set by mod wheel)
  byte lastNotePlayed; // What was the most recent note that was played?
  uint16_t lastNoteCV; // What's the corresponding CV value for that note? (in terms of the DAC value)
  float pitchBendMultiplier; // Multiplier used for pitch bending (set by the pitch bend wheel)
};

struct State state;

// Implements SPI for use with the MCP4921 DAC, which
// generates CV for the Gakken
#define DATAOUT 11 //MOSI
#define DATAIN 12 //MISO - not used, but part of builtin SPI
#define SPICLOCK  13 //sck
#define SLAVESELECT 10 //ss

// Instantiates the MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

void configure_SPI(){
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
}

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
    -- Neu Tabelen (?) --
    
    G6  2000
    G2  1125
    B2  1200
    A2  btwn 1152 -> 1154 (!!)
    C3  1210
  */
  
  int diff = 48 - pitch;
  diff *= 12;
  return 1125 - diff;
}

void glide_to_note(uint16_t target_cv){
  uint16_t tmp_cv = state.lastNoteCV;
  int pitch_range;
  
  // Encountered odd problems with abs(), so this is my workaround
  if(target_cv < state.lastNoteCV)
    pitch_range = tmp_cv - target_cv;
  else
    pitch_range = target_cv - tmp_cv;
  
  int time_between_pitches = state.noteGlideTime / pitch_range;

  while(tmp_cv != target_cv && state.noteIsGliding == 1){
    
    if(target_cv < state.lastNoteCV)
      tmp_cv--;
    else
      tmp_cv++;

    gakken_write_value(tmp_cv);
    delay(time_between_pitches); // THIS could be the problem is time_between_pitches isn't calculated right.
  }
  
  state.noteIsGliding = 0;
  
  return;
}

void handle_noteOn(byte channel, byte pitch, byte velocity) {
  // Hook noteOn to apply a glide between notes, if applicable.
  // state.noteGlideTime will equal 0 if the Mod wheel (CC #1) is zeroed
  if(state.noteGlideTime > 0){
    state.noteIsGliding = 1;
    glide_to_note(note_to_cv(pitch));
  }
  
  state.lastNotePlayed = pitch;
  state.lastNoteCV = note_to_cv(pitch);
  state.noteIsPlaying = 1;
  
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
  float bend_amount = 1 + (((float) bend) / 8190);
  int bent_val = state.lastNoteCV * bend_amount;

  /*
    Sanity check - After bending, the next CV value we're going to
    write should never be less than 0.
    
    This function is called on any pitchbend message, so it's
    OK to short circuit here. Only valid results will be applied.
  */
  if (bent_val < 0)
    return;
    
  state.pitchBendMultiplier = bend_amount;
    
  if(state.noteIsPlaying == 1)
    gakken_write_value(bent_val);
}

void handle_glideParamUpdate(byte channel, byte number, byte value){
  
  if(value == 0){
    state.noteGlideTime = 0;
    state.noteIsGliding = 0;
  }
  
  state.noteGlideTime = (((float) value) / 127) * MAX_GLIDE_TIME;
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

  configure_SPI();
  
  state.lastNotePlayed = 0;
  state.noteIsPlaying = 0;
  state.pitchBendMultiplier = 1;
  state.noteGlideTime = 0;
  state.noteIsGliding = 0;
    
  // MIDI library configuration
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handle_noteOn);
  MIDI.setHandleNoteOff(handle_noteOff);
  MIDI.setHandlePitchBend(handle_pitchBend);
  MIDI.setHandleControlChange(handle_CC);
}

void loop() {
    MIDI.read();
    
    // NOTE: might be necessary to perform note gliding here, to prevent blocking
    // the main thread in a handler (?)
}

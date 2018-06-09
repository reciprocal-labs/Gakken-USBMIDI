// Implements SPI for use with the MCP4921 DAC, which
// generates CV for the Gakken
#define DATAOUT 16 //MOSI
#define DATAIN 14 //MISO - not used, but part of builtin SPI
#define SPICLOCK  15 //sck
#define SLAVESELECT 4 //ss

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

void setup()
{
  configure_SPI();
  gakken_write_value(0);
}

void loop() {
  for(int i = 1000; i < 2000; i++)
    gakken_write_value(i);

  for(int i = 2000; i >= 1000; i--)
    gakken_write_value(i);
}

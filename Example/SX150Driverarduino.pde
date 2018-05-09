#define DATAOUT 11//MOSI
#define DATAIN 12//MISO - not used, but part of builtin SPI
#define SPICLOCK  13//sck
#define SLAVESELECT 10//ss

void setup()
{
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

  Serial.begin(9600);
}

void write_value(uint16_t sample)
{
  uint8_t dacSPI0 = 0;
  uint8_t dacSPI1 = 0;
  dacSPI0 = (sample >> 8) & 0x00FF;
  dacSPI0 |= 0x10;
  dacSPI1 = sample & 0x00FF;
  digitalWrite(SLAVESELECT,LOW);
  SPDR = dacSPI0;                    // Start the transmission
  while (!(SPSR & (1<<SPIF)))     // Wait the end of the transmission
  {
  };
  
  SPDR = dacSPI1;
  while (!(SPSR & (1<<SPIF)))     // Wait the end of the transmission
  {
  };  
  digitalWrite(SLAVESELECT,HIGH);
  delay(5);
}

void playNote(int incomingByte)
{
  int diff = 48 - incomingByte;
  diff *= 12;
  write_value (1125 - diff);
  // 1270 C4 60
  // 1125 C3 48
  // 980  C2 36    
  // 835  C1 24 
  // 690  C0 12      
}
void noteOff()
{
  write_value(0);
}
int incomingByte = 0;

void loop()
{
   if(Serial.available() > 0){
       incomingByte = Serial.read();
       if(incomingByte == 0)
         noteOff();
       else {
         playNote(incomingByte);
       }
   }
}


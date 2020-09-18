/*
 * 9129 VDP Tester
 * 
 * The CPU communicates with the VDP through an eight-bit bidirectional data bus, three control lines, and an interrupt line.
 * The three control signals are ~ (chip select read), CSW (chip
 * select write), and MODE. CSR and CSW determine whether the VDP gets information off the
 * data bus or puts information onto it. If ~ is active, the VDP will output information for the
 * CPU onto the data bus. If CSW is active, the VDP will get information sent by the CPU off the
 * data bus.
 * 
 * To speed up IO, this sketch makes use of PORT_MANIPULATION (https://www.arduino.cc/en/Reference/PortManipulation) to quicky change
 * the mode (input/output) & write/read values of a series of PINs:
 * Arduino has a set of ports, each having a range of the I/O PINs:
 * See http://www.datasheetcafe.com/wp-content/uploads/2015/10/ATmega328P-Pinout-Datasheet.gif => Atmega 328P has PORTB (PB0-PB7), PORTC & PORTD
 * 
 * This sketch uses PORTD & PORTC:
 *  PORTD: Arduino Digital pins 0-7
 *  PORTC: Arduino Analog pins A0-A7 (6&7 not being accessible - except on Adruino Mini)
 * 
 * Each of these ports have 3 registers to manipulate them quicky / read & write from/to it:
 *    - DDRx: Data Direction Register for port x (e.g. DDRD => for PORT D = Arduino digital pins 0-7)
 *        => Writing a byte to it allows to set the I/O mode of every PIN at once (faster than doing that by 8 pinMode statements).

 *        e.g. DDRD = |       B11111110; => 1 set the corresponding PIN as output, 0 as input
 *                    |-> PIN  76543210
 *        
 *                                                                 PIN    76543210
 *    - PORTx: the register for the state of the outputs => e.g. PORTD = B10101000; // sets digital pins 7,5,3 HIGH
 *    
 *    - PINx: Input register variable It will read all of the digital input pins at the same time.
 * 
 *           Removed code that did *not* make use of PORT_MANIPULATION as it did not get it to the final bytecode anyway, 
 *           This due to the #define PORT_MANIPULATION & IFDEF PORT_MANIPULATION statements
 * 
 * Arduino - VDP connections:
 * 
 * VDP   ARDUINO
 * CD0 - 7 - PORTB |
 * CD1 - 6 - PORTB |
 * CD2 - 5 - PORTB |-> Databus lowest nibble >
 * CD3 - 4 - PORTB |
 *                 
 * CD4 - A3 - PORTC |
 * CD5 - A2 - PORTC |
 * CD6 - A1 - PORTC |-> Databus highest nibble >
 * CD7 - A0 - PORTC |
 * 
 * !CSR   8 (chip select read)   |
 * !CSW   3 (chip select write)  |-> these define whether to write or read date to/from databus
 * MODE   2 
 * !RESET  9
 * 
 * Interrupt PIN from 9129 VDP is not connected > Would suggest to connect it to a free Interrupt capable PIN & attach an Interrupt Service Routine to it.
 * 
 * Some investigation
 * - Backdrop: Display built of a range of planes stacked on each other. Backdrop is a plane that is larger then the other plane types => so you see a border.
 *      Backdrop colour is defined by 4 bits in VDP register 7 (0-F)
 *      
 * - Interrupt line:
 *   Needs to be enabled: configured in VDP register 1, bit 2 (p 5-2 VDP programmer's guide):
 *   Bit 2 = IE (Interrupt Enable)
      0 -Disables VDP interrupt
      1 -Enables VDP interrupt
        If the VDP interrupt is connected in hardware and enabled by this bit, it will occur at the end of
        the active screen display area, just before vertical retrace starts. Exceptionally smooth, clean
        pattern drawing and sprite movement can be achieved by writing to the VDP during the period
        this interrupt is active.
        
 * - To reset the interrupt, the status register needs to be read
 */

#define MODE 2
#define CSW 3
#define CSR 8
#define RESET 9

//int bytePins[8] =   { 14, 15, 16, 17, 4, 5, 6, 7 }; => No longer used

void setDBReadMode() {
  //PIN          76543210
  DDRD = DDRD & B00001111; // Set Pin 4..7 as inputs (leave 0-3 untouched). High nibble of databus (Data Direction Register Port D) !!
  DDRC = DDRC & B11110000; // Set Analog pin 0..3 as inputs (Data Direction Register Port C) !! Doing the & with 0, forces A4-A7 0-3 as inputs - no problem as unused :-)
}

void setDBWriteMode() {
  //PIN          76543210
  DDRD = DDRD | B11110000; // Set Pin 4..7 as outputs (leave 0-3 untouched). High nibble of databus.
  DDRC = DDRC | B00001111; // Set Analog pin 0..3 as outputs, leave others untouched
}

void reset() {
  Serial.println("Resetting");
  digitalWrite(RESET, HIGH);
  delayMicroseconds(100);
  digitalWrite(RESET, LOW);
  delayMicroseconds(5);  
  digitalWrite(RESET, HIGH);
}

inline void setPort(unsigned char value) { //TODO: check for correctness
    PORTD = (PIND & 0x0F) | (value & 0xF0); //Sets value of the PINS on PORTD (0-7)
    PORTC = (PINC & 0xF0) | (value & 0x0F);    
}

inline unsigned char readPort() { //TODO: check for correctness
    unsigned char memByte = 0;    
    memByte  = (PIND & 0xF0) | (PINC & 0x0F);
    return memByte;
}

//Writes a byte to databus for register access
void writeByte( unsigned char value) {  
    setDBWriteMode();
    setPort(value); 
    digitalWrite(MODE, HIGH);        
    digitalWrite(CSW, LOW);            
    delayMicroseconds(10);
    digitalWrite(CSW, HIGH);
    setDBReadMode();
}

//Reads a byte from databus for register access
unsigned char  readByte( ) {
    unsigned char memByte = 0;
    digitalWrite(MODE, HIGH);            
    digitalWrite(CSR, LOW);        
    delayMicroseconds(10);
    memByte = readPort();
    digitalWrite(CSR, HIGH);
    return memByte;
}

//Writes a byte to databus for vram access
void writeByteToVRAM( unsigned char value) {     
    digitalWrite(MODE, LOW);        
    digitalWrite(CSW, LOW);            
    setDBWriteMode();  
    setPort(value);    
    delayMicroseconds(10);
    digitalWrite(CSW, HIGH);
    setDBReadMode();
    delayMicroseconds(10);       
}

//Reads a byte from databus for vram access
unsigned char  readByteFromVRAM( ) {
    unsigned char memByte = 0;
    digitalWrite(MODE, LOW);            
    digitalWrite(CSR, LOW);        
    delayMicroseconds(1);
    memByte = readPort();
    digitalWrite(CSR, HIGH);
    delayMicroseconds(10);    
    return memByte;
}


void setRegister(unsigned char registerIndex, unsigned char value) {
  writeByte(value);
  writeByte(0x80 | registerIndex);  
}

void setWriteAddress( unsigned int address) {
  writeByte((address & 0xFFC0)>>6);
  writeByte(0x40 | (address & 0x3F));  
}

void setReadAddress( unsigned int address) {
  writeByte((address & 0xFFC0)>>6);
  writeByte((address & 0x3F));  
}

void setup() {
  setDBReadMode();
  Serial.begin(115200);
  pinMode(MODE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(CSW, OUTPUT); // 
  pinMode(CSR, OUTPUT);  
  
  digitalWrite(RESET, HIGH);
  digitalWrite(MODE, HIGH);  
  digitalWrite(CSW, HIGH);    
  digitalWrite(CSR, HIGH);  

  reset();
  delay(2000);

/*
 *   Needs to be enabled: configured in VDP register 1, bit 2 (p 5-2 VDP programmer's guide):
 *   Bit 2 = IE (Interrupt Enable)
      0 -Disables VDP interrupt
      1 -Enables VDP interrupt
 **/

  setRegister(0, 0x00);   //Graphics I Mode
  setRegister(1, 0xE0);   //Graphics I Mode + INTERRUPT  
 /*  This sets register 1 to 0xE0 (B11100000) => So bit 0, 1 & 2 are set, meaning:
  *    - Selecting 16Kb of VRAM (bit 0)
  *    - Enable the active display (bit 1)
  *    - Enable interrupt (bit 2)
  */
 
  setRegister(2, 0x00);   //Name table
  setRegister(3, 0x00);    
  setRegister(4, 0x00);   //Pattern generator   
  setRegister(5, 0x00);       
  setRegister(6, 0x00);         
  setRegister(7, 0x00);  
  
  Serial.println("Clearing RAM");
  setWriteAddress(0);
  for (int i = 0;i<16384;i++) {
    writeByteToVRAM(0);
  }
  
   setWriteAddress(0x0);  
   for (int i = 0x800;i<16384;i=i+8) {
   writeByteToVRAM(0x20);
   writeByteToVRAM(0x50);
   writeByteToVRAM(0x88);
   writeByteToVRAM(0x88);
   writeByteToVRAM(0xF8);
   writeByteToVRAM(0x88);
   writeByteToVRAM(0x88);
   writeByteToVRAM(0x00);   
   } 
}

int i = 0;
void loop() { 
  
  //Testing the Backdrop Colors, defined by four bits in Register 7
  //The lower four bits contain the color for the Backdrop in all modes
  setRegister(7, 0x50 | i);
  delay(1000);
  i = i + 1;
  if (i>15) i = 0;

//Writing into PATERN table?

   setWriteAddress(0x0);  
   for (int j = 0x800;j<2057;j=j+8) {
   writeByteToVRAM(0x20);
      delay(300);
   writeByteToVRAM(0x50);
      delay(300);
   writeByteToVRAM(0x88);
      delay(300);
   writeByteToVRAM(0x88);
      delay(300);
   writeByteToVRAM(0xF8);
      delay(300);
   writeByteToVRAM(0x88);
      delay(300);
   writeByteToVRAM(0x88);
      delay(300);
   writeByteToVRAM(0x00);   
      delay(300);   
  /*
   * 
   * READING THE VDP STATUS REGISTER to reset the interrupt that could have been fired. Note, better to connect the VDP's interrupt pin to Arduino & call the 3 lines below in the ISR.
   * The Status Register contents can be read with a single byte transfer, just by doing a read from
   * address Hex C002 (see Table 4-2, p 4-1 VDP programmer's guide).
   */
  setReadAddress(0xC002);
  setDBReadMode();
  readByte(); //Read from the status register (quick & dirty, discarding the result)
   }  
}

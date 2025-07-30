// Episode 9
// PCF8474 Port Expander

// PCF8574 IC connected to pins 21 (SDA) & 22 (SCL) - interrupt on pin 19
// switches that wire to ground on the PCF8574 pins P4, P5, P6 & P7
// LEDs connected from supply voltage, via a 1k resistor each to PCF8574 pin P0, P1, P2 & P3

#include <PCF8574.h>

PCF8574 pcf(0x20);                // create our library object to talk to the PCF8574

volatile bool flag = false;       // flag used for the INT input, active low
#define INTPIN  19                // interrupt is on pin 19

// interrupt service routine, note this should have minimal code!
void pcf_irq()
{
  flag = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial){}

  Serial.println("");   // blank line between the ESP32 boot up code and the programme text
  Serial.println("A Leopard's Tail - Episode 9 - PCF8574 8 Bit I/O port expander");   // feel free to change this!

  Wire.begin();   // initialise the I2C library
  pcf.begin();    // initialise the PCF8574

  pinMode(INTPIN, INPUT_PULLUP);    // set up the interrupt pin
  attachInterrupt(digitalPinToInterrupt(INTPIN), pcf_irq, FALLING);   // connect the interrupt pin to the service routine, triggers when pin 19 goes from HIGH to LOW

}

void loop() {
  // put your main code here, to run repeatedly:

  if (flag)
  {
    uint8_t Val = pcf.read8();    // read all pins
    //printBin(Val);              // optional write to the serial port, if you want to use this change "FALLING" on line 33 to "BOTH"

    Val = Val >> 4;         // bitwise shift 4 places to the left, maps the switch inputs to the LED outputs
    Val |= 0b11110000;      // logical OR to set the four high bits to be "1" while leaving the low four bits unchanged, keeps pins P4-P7 as inputs

    pcf.write8(Val);        // write all the pins
    flag = false;           // clear the flag so as not to do this again until we need to
  }
}

// helper utility, takes an 8 bit value and displays it as two four bit "nibbles" with a space in between for clarity
// and ensure the leading zeros are retained
void printBin(uint8_t V)
{
  char Buffer[20] = {0};
  uint8_t ptr = 0;

  for (int8_t i=7; i>=0; i--)
  {
    uint8_t j = V >> i;
    j &= 0b00000001;


    if (j == 0)
    {
      Buffer[ptr] = '0';
    }
    else
    {
      Buffer[ptr] = '1';
    }
    ptr++;
    if (i == 4) { Buffer[ptr] = ' '; ptr++;}
  }
  Serial.println(Buffer);
}
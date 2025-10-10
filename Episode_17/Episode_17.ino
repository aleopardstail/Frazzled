// AtTiny85 Function Decoder
// A Leopard's Tail, Episode 17
// base on Examples supplied with the NmrdDcc Library

#include <NmraDcc.h>

#define DEFAULT_DECODER_ADDRESS 3     // DCC address we wish to use as a default
#define DCC_PIN     2                 // arduino pin to use as the DCC input, AtTiny physical pin 7

#define LED_PIN_FWD 0                 // output pin for the forward headline, AtTiny physical pin 5
#define LED_PIN_REV 1                 // output pin for the reverse headlint, AtTiny physical pin 6
#define F1_PIN      3                 // output pin for function 1, AtTiny physical pin 2
#define F2_PIN      4                 // output pin for function 2, AtTiny physical pin 3

// state variables, used to determine when things change
uint8_t newLedState = 0;
uint8_t lastLedState = 0;

uint8_t newDirection = 0;
uint8_t lastDirection = 0;

uint8_t newF1State = 0;
uint8_t lastF1State = 0;

uint8_t newF2State = 0;
uint8_t lastF2State = 0;

// custom structure to hold CV number & value pairs
struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

// Default CV Values Table
CVPair FactoryDefaultCVs [] =
{
  // the default (short) address
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, DEFAULT_DECODER_ADDRESS},

  // the default (long) address, uses helpfer functions to calculate the actual values
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, CALC_MULTIFUNCTION_EXTENDED_ADDRESS_MSB(DEFAULT_DECODER_ADDRESS)},
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, CALC_MULTIFUNCTION_EXTENDED_ADDRESS_LSB(DEFAULT_DECODER_ADDRESS)},

  // configures CV29 to use extended addressing and 28/128 speed steps
  {CV_29_CONFIG, CV29_EXT_ADDRESSING | CV29_F0_LOCATION}
};

// create the main library object
NmraDcc  Dcc;                    

// set up a counter for use when resetting to factory default CVs
uint8_t FactoryDefaultCVIndex = 0;

// called when a CV value changes
void notifyCVChange( uint16_t CV, uint8_t Value)
{
  // not used in this episode
}

// notification when the speed or direction changes
void notifyDccSpeed( uint16_t Addr, DCC_ADDR_TYPE AddrType, uint8_t Speed, DCC_DIRECTION Dir, DCC_SPEED_STEPS SpeedSteps )
{
  // store the notified direction, note this will be sent as the speed changes as well but this information is discarded
  newDirection = Dir;
}

// notification that a function state changed
void notifyDccFunc(uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState)
{
  // this if statement is true if F0, F1, F2, F3 or F4 have changed
  if(FuncGrp == FN_0_4)
  {
    newLedState = (FuncState & FN_BIT_00) ? 1 : 0;    // set the state of the headlights, F0
    newF1State = (FuncState & FN_BIT_01) ? 1 : 0;     // set the state of F1
    newF2State = (FuncState & FN_BIT_02) ? 1 : 0;     // set the state of F2
  }
}

// respond to reading a cv, this needs to swap in and then out at least 60mA
void notifyCVAck(void)
{
  digitalWrite(LED_PIN_FWD, HIGH);
  digitalWrite(LED_PIN_REV, HIGH);
  digitalWrite(F1_PIN, HIGH);
  digitalWrite(F2_PIN, HIGH);

  delay(8);

  digitalWrite(LED_PIN_FWD, LOW);
  digitalWrite(LED_PIN_REV, LOW);
  digitalWrite(F1_PIN, LOW);
  digitalWrite(F2_PIN, LOW);
}

void notifyCVResetFactoryDefault()
{
  // Set FactoryDefaultCVIndex non-zero to signal loop() to reset CVs
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
}

void setup() 
{
  // initialise LEDs as outputs
  pinMode(LED_PIN_FWD, OUTPUT);
  pinMode(LED_PIN_REV, OUTPUT);
  pinMode(F1_PIN, OUTPUT);
  pinMode(F2_PIN, OUTPUT);

  // setup DCC object, with the pin
  Dcc.pin(0, DCC_PIN, 1);

  // initialise DCC object, flagged to only get messages addressed to this address and to run the reset code if CV7 & CV8 are both set to '255'
  Dcc.init( MAN_ID_DIY, 10, FLAGS_MY_ADDRESS_ONLY | FLAGS_AUTO_FACTORY_DEFAULT, 0 );

}

void loop() 
{
  // this needs to be called frequently, here once per loop is enough
  Dcc.process();

  // manage the factory reset code, if the 'FactoryDefaultCVIndex is non zero and the EEPROM system is ready to write a value, write the next CV in sequence
  // note this will operate over multiple cycles
  if( FactoryDefaultCVIndex && Dcc.isSetCVReady())
  {
    FactoryDefaultCVIndex--;
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }

  // manage the headlights if either F0 or the direction setting has changed
  if( (newLedState != lastLedState) || (newDirection != lastDirection) )
  {
    if(newLedState)   // if the headlights are on
    {
    if(newDirection)  // if in forwards
      {
        digitalWrite(LED_PIN_FWD, HIGH);
        digitalWrite(LED_PIN_REV, LOW);
      }
      else  // else in reverse
      {
        digitalWrite(LED_PIN_FWD, LOW);
        digitalWrite(LED_PIN_REV, HIGH);
      }
    }
    else  // otherwise turn them off
    {
      digitalWrite(LED_PIN_FWD, LOW);
      digitalWrite(LED_PIN_REV, LOW);
    }    
  }

  // manage the output of F1
  if(newF1State != lastF1State)
  {
    lastF1State = newF1State;
    digitalWrite(F1_PIN, newF1State);
  }

  // manage the output of F2
  if(newF2State != lastF2State)
  {
    lastF2State = newF2State;
    digitalWrite(F2_PIN, newF2State);
  }

}

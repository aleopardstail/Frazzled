// Episode 11
// Rotary Encoder

// What we want
// 11 --> 10 --> 00 --> 01
// 11 --> 01 --> 00 --> 10

// encoder pin connections, swap the pins to reverse the direction of rotation id desired
#define ENCA  17    // Encoder pin A
#define ENCB  16    // Encoder pin B

uint32_t TNow = 0;              // the time now
uint32_t TPrevious = 0;         // the time at the last iteration
uint32_t TElapsed = 0;          // the difference between the above

uint32_t TLastUpdate = 0;       // the time of the last update
uint32_t TLastAction = 0;       // the time of the last action
uint32_t TLastO = 0;            // the time of the last change in output, used to manage acceleration

uint8_t EncVal = 0;             // the encoder value, bits 0 & 1 are the current reading 2 & 3 the previous one
int16_t O = 0;                  // output value, starts at zero
int16_t LastO = 0;              // the previous output value, makes it easy to see when it changes
int8_t LastV = 0;               // last intermediate value, used as part of the debouncing code

//int8_t table[]= {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};      // array to show all possible transitions
int8_t table[]= {0,0,0,0,0,0,0,1,0,0,0,-1,0,0,0,0};         // array showing only the vlaid "click" transitions

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial)
  {
    delay(1);
  }

  pinMode(ENCA, INPUT_PULLUP);      // pin for encoder port A, internal pull up, no need for external resistor
  pinMode(ENCB, INPUT_PULLUP);      // pin for encoder port B, internal pull up, no need for external resistor

  Serial.println("Episode 11 - Rotary Encoder Test");

}

void loop() {
  // put your main code here, to run repeatedly:
  TNow = millis();
  TElapsed = TNow - TPrevious;
  TPrevious = TNow;

  if (TNow - TLastAction > 1)                               // loop puts 1ms minimum between readings for sanity, a loop doing other things may not need this
  {
    EncVal <<= 2;                                           // 0b00000011 --> 0b00001100
    if(digitalRead(ENCA) == HIGH) { EncVal |= 0x01;}        // 0b0000000A
    if(digitalRead(ENCB) == HIGH) { EncVal |= 0x02;}        // 0b000000BA
    EncVal &= 0x0f;                                         // 0b00001111   masks the top for bits, value is 0-15

    int8_t V = table[EncVal];                               // intermediate value from the table
    if (V != 0)                                             // if the intermediate value is not zero, i.e. we have a transition
    {
      if (LastV == V)                                       // if the transition is the same as the previous one (helps remove bouncing)
      {
        int8_t inc = table[EncVal];                         // get the increment from the table

        uint32_t T = TNow - TLastO;                         // calculate how long since the last time we did this

        // acceleration
        if (T < 50)                                         // if the time is less than 50ms
        {
          inc *= 4;                                         // go at x4 speed
        }
        else if(T < 200)                                    // otherwise if less than 200ms
        {
          inc *= 2;                                         // go at twice speed
        }
        // default is whatever is in the table

        O += inc;

        // cap collar
        if (O > 126){O = 126;}
        if (O < -126) {O = -126;}

        TLastO = TNow;                                      // store the time we made the change
      }
      LastV = V;                                            // store the intermediate value for next time
    }
    TLastAction = TNow;                                     // time for the action loop
  }



  if (O != LastO)                                           // if the value of O has changed, output it
  {
    Serial.print(EncVal, HEX);
    Serial.print("  ");
    Serial.println(O);
    LastO = O;
  }

}

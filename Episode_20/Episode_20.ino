// Episode 20
// Basic Stepper Motor

#include <Stepper.h>

// initialize the stepper library on pins 8 through 11:
// note IN1, IN3, IN2, IN4
Stepper myStepper(2048, 8, 10, 9, 11);

uint32_t TNow = 0;          // the time now, in milliseconds
uint32_t TLast = 0;         // the time on the previous iteration
uint32_t TEllapsed = 0;     // the difference between the above

uint32_t TInterval = 5;    // how many ms between each increment of movement
uint32_t TLastMove = 0;     // when did we last move?

int16_t Pos = 0;            // where are we?
int8_t  Delta = 1;          // current increment, use +1 or -1 to get single steps

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("Non Blocking Stepper Motor Test");
  myStepper.setSpeed(1);

  TNow = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
  TNow = millis();
  TEllapsed = TNow - TLast;
  TLast = TNow;

  if (TNow - TLastMove >= TInterval)
  {
    myStepper.step(Delta);
    Pos += Delta;

    if (Pos > 2048)
    {
      Serial.println("reverse");
      Delta = -1;
    }
    else if(Pos < 0)
    {
      Serial.println("forwards");
      Delta = 1;
    }

    TLastMove = TNow;
  }

}

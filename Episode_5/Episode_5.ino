#include <ESP32Servo.h> 

// Harware Definitions

#define LED_RED       2
#define LED_GREEN     4
#define LED_YELLOW    5

#define BUTTON_BLUE   18
#define BUTTON_RED    19
#define BUTTON_GREEN  21

#define SERVO_PIN     13

bool      Changed = true;
uint8_t   CurrentState = 0;           // 0 = red, 1 = green
bool      Interlock = false;

uint8_t ButtonFlags = 0b00000000;     // bit 0 = green, bit 1 = red, bit 3 = blue
uint8_t ButtonState = 0b00000000;     // bit 0 = green, bit 1 = red, bit 3 = blue

Servo myservo;
uint8_t CurrentPosition = 90;         // mid point, this is the position the servo is currently at
uint8_t TargetPosition = 90;          // this is where we are moving to
uint32_t TDelay = 25;                 // time delay in ms between each 1 degree step
uint32_t TLastMove = 0;

// timing functions
uint32_t  TNow = 0;
uint32_t  TEllapsed = 0;
uint32_t  TOld = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_RED, OUTPUT);     digitalWrite(LED_RED, HIGH);
  pinMode(LED_GREEN, OUTPUT);   digitalWrite(LED_GREEN, LOW);
  pinMode(LED_YELLOW, OUTPUT);  digitalWrite(LED_YELLOW, LOW);

  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);

  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);// Standard 50hz servo
  myservo.attach(SERVO_PIN, 500, 2400);     // defines the servo with minumum and maximum parameters

  Serial.begin(115200);
  while (!Serial){}
}

void loop() {
  // put your main code here, to run repeatedly:
  TNow = millis();            // get the current time
  TEllapsed = TNow - TOld;    // calculate the time ellapsed since the last loop
  TOld = TNow;                // store the current time for the next loop

  if (Serial.available() > 0)
  {
    char C;
    C = Serial.read();
    
    if (C == 'g')
    {
      if (Interlock == false)
      {
        CurrentState = 1;
        Changed = true;  
      }  
      else
      {
        Serial.println("+++++ Interlock Error +++++");
      }    
    }
    else if (C == 'r')
    {
      CurrentState = 0;
      Changed = true; 
    }
  }

  // read buttons
  uint8_t ButtonNow = 0;
  uint8_t junk;

  junk = digitalRead(BUTTON_GREEN); if (junk) {ButtonNow = ButtonNow & 0b11111110;} else {ButtonNow = ButtonNow | 0b00000001;}
  junk = digitalRead(BUTTON_RED);   if (junk) {ButtonNow = ButtonNow & 0b11111101;} else {ButtonNow = ButtonNow | 0b00000010;} 
  junk = digitalRead(BUTTON_BLUE);  if (junk) {ButtonNow = ButtonNow & 0b11111011;} else {ButtonNow = ButtonNow | 0b00000100;}

  if (ButtonNow != ButtonState)
  {
    if (ButtonNow & 0b00000001) // green button pressed
    {
      if (Interlock == false)
      {
        CurrentState = 1;
        Changed = true; 
      }    
      else
      {
        Serial.println("+++++ Interlock Error +++++");
      }    
    }
    if (ButtonNow & 0b00000010) // red button pressed
    {
      CurrentState = 0;
      Changed = true; 
    }
    if (ButtonNow & 0b00000100) // blue button pressed
    {
      if (Interlock) {Interlock = false;} else {Interlock = true; CurrentState = 0;}
      Changed = true;
    }

    ButtonState = ButtonNow;
  }


  if (Changed)
  {
    if (CurrentState == 0)        // red
    {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      Serial.println("Stop");

      TargetPosition = 25;
    }
    else if (CurrentState == 1)   // green
    {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      Serial.println("Proceed");

      TargetPosition = 155;
    }

    if (Interlock)
    {
      digitalWrite(LED_YELLOW, HIGH);
      Serial.println("Interlock On");
    }
    else
    {
      digitalWrite(LED_YELLOW, LOW);
      Serial.println("Interlock Off");
    }

    Changed = false;
  }

  // servo loop
  if (CurrentPosition != TargetPosition)
  {
    if (TNow - TDelay > TLastMove)
    {
      if (CurrentPosition < TargetPosition)
      {
        CurrentPosition++; 
      }
      else
      {
        CurrentPosition--;
      }

      myservo.write(CurrentPosition);
      TLastMove = TNow;
    }
  }

}

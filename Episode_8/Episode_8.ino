// Episode 8
// Screens and Displays
//
// Adding a 2004 I2C Liquid Crystal Display to an Arduino Nano
// over the I2C interface

#include <LCD_I2C.h>

LCD_I2C lcd(0x27, 20, 4);

uint8_t I[6] = {4,5,6,7,8,9};     // sensor pins
uint8_t V[6] = {0};               // sensor values
uint8_t O[3] = {0};               // output values


void setup() {
  // put your setup code here, to run once:
  lcd.begin();

  for (uint8_t cntr = 0; cntr <6; cntr++)
  {
    pinMode(I[cntr], INPUT_PULLUP);
  }

  lcd.setCursor(0,0);
  lcd.print("1 2 3 4 5 6");

  lcd.noBacklight();
}

void loop() {
  // put your main code here, to run repeatedly:
  for (uint8_t cntr = 0; cntr <6; cntr++)
  {
    V[cntr] = digitalRead(I[cntr]);
  }

  // A  B   Out
  // 0  0   1
  // 0  1   1
  // 1  0   1
  // 1  1   0

  O[0] = !(V[0] & V[1]);

  lcd.setCursor(0, 1);
  for (uint8_t cntr = 0; cntr <6; cntr++)
  {
    lcd.print(V[cntr]);
    lcd.print(' ');
  }

  lcd.setCursor(1, 3);
  lcd.print(O[0]);

  delay(1000);


}

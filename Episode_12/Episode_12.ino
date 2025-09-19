
#include <Adafruit_NeoPixel.h>


#define LED_PIN     4               // which digital pin is the string connected to?
#define LED_COUNT   4               // how many LEDs in the string?
#define BRIGHTNESS  50              // Set BRIGHTNESS to about 1/5 (max = 255)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

void setup() 
{
  // LED setup
  strip.begin();                    // initialisation
  strip.show();                     // update the string, turns all off by default
  strip.setBrightness(BRIGHTNESS);  // set the global brightness

}


void loop() 
{
  for (uint8_t i = 0; i < 4; i++)
  {    
    strip.setPixelColor(i, strip.Color(255,0,0,0)); strip.show(); delay(250);   // red
    strip.setPixelColor(i, strip.Color(0,255,0,0)); strip.show(); delay(250);   // green
    strip.setPixelColor(i, strip.Color(0,0,255,0)); strip.show(); delay(250);   // blue
    strip.setPixelColor(i, strip.Color(0,0,0,255)); strip.show(); delay(250);   // white
  }

  delay(1000);

  for (uint8_t V = 255; V > 0; V--)
  {
    strip.setPixelColor(0,strip.Color(0,0,0,V));
    strip.setPixelColor(1,strip.Color(0,0,0,V));
    strip.setPixelColor(2,strip.Color(0,0,0,V));
    strip.setPixelColor(3,strip.Color(0,0,0,V));

    strip.show();
    delay(10);
  }

  for (uint8_t V = 0; V < 255; V++)
  {
    strip.setPixelColor(0,strip.Color(0,0,0,V));
    strip.setPixelColor(1,strip.Color(0,0,0,V));
    strip.setPixelColor(2,strip.Color(0,0,0,V));
    strip.setPixelColor(3,strip.Color(0,0,0,V));

    strip.show();
    delay(10);
  }

  colourWipe(strip.Color(255,0,0,0), 100);    // red
  colourWipe(strip.Color(255,255,0,0), 100);
  colourWipe(strip.Color(0,255,0,0), 100);
  colourWipe(strip.Color(0,255,255,0), 100);
  colourWipe(strip.Color(0,0,255,0), 100);
  colourWipe(strip.Color(255,0,255,0), 100);
  colourWipe(strip.Color(0,0,255,255), 100);
  colourWipe(strip.Color(255,0,0,255), 100);
   
}


void colourWipe(uint32_t colour, int wait)
{
  for (uint8_t i = 0; i < LED_COUNT; i++)
  {
    strip.setPixelColor(i, colour);
    strip.show();
    delay(wait);
  }
}



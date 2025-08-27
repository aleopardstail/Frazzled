// IMPORTANT NOTE: This code is *very* layout specific, it is provided for reference only

/*  Leopard street Board Controller (V2)

    Display usage (8 rows x 20 characters)
    [0] - Title text & Wifi strength (updated on heartbeat)
    [1] - Wifi Status (IP or connection count etc)
    [2] - MQTT Status
    [3] - Turnout Status, 16 characters, 'C', 'T', 'M', '?'  or '-'
    [4] - Relay Status, 8 characters, 'C', 'O' or '-'
    [5] - Lights status, 12 characters, '1', '0', 'i', or '-' lights 0-11
    [6] - Lights status, 12 characters, '1', '0', 'i', or '-' lights 12-23
    [7] - Block status, 8 characters, '1', '0' or '-'

*/

// -------------------------------------------
/*
    Status:
    - WiFi working, with strength indicator, still connection issues upstairs, good downstairs though
    - Heartbeat signal working (blue LED flashing)
    - MQTT connection working
    - LED functions working
    - OTA update working
    - PCF8575 inputs working, reporting via MQTT, not tested with DTC8 yet
    - Servo code uploaded, not yet validated - OE pin clashing with PCF IRQ pin
    - Relay output code uploaded, not yet verified
*/
// -------------------------------------------

#define LED_BUILTIN 2               // on board Blue LED
const char BoardID = 'D';           // Board identifier - used by MQTT etc
char DisplayBuffer[8][21] = {0};    // display screen text buffer, 8 rows, 20 characters + trailing Null

// OTA update capability settings
#include <ArduinoOTA.h>

// OLED Settings
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Settings
#include <WiFi.h>
#include "WiFiDetails.h"
WiFiClient espClient;

// MQTT Settings
#include <PubSubClient.h>
const char *mqtt_server = "192.168.0.39";		// replace with your MQTT broker IP address
//const char *topic = "emqx/esp32";
const char *mqtt_username = "your_mqtt_username";
const char *mqtt_password = "your_mqtt_passwword";
const int mqtt_port = 1883;
PubSubClient client(espClient);
bool MQTTState = false;

// TLC5947 LED controller Settings
#include "Adafruit_TLC5947.h"
#define NUM_TLC5947 1           // we only have one board connected, they can daisy chain
#define LEDdata     23          // data pin  
#define LEDclock    18          // clock pin
#define LEDlatch    5           // latch/chip select pin
#define LEDoe       4           // output enable, note this is manually controlled and not by the library

Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, LEDclock, LEDdata, LEDlatch);    // LED controller object

// PCF8575 16 bit I/O controller - used for frog relays and block detection inputs
// PCF8575 Settings             // Interrupt on D33
#include "PCF8575.h"
PCF8575 PCF(0x20);
volatile bool PCFIRQflag = false;
const int IRQPIN = 33;
uint16_t PCFLastRead = 0;

// PCA9685 Servo Settings
#include "PCA9685_servo_controller.h"	// this is a custom library, don't worry about it for now
#include "PCA9685_servo.h"
const uint8_t NumServos = 2;
PCA9685_servo_controller ServoController(0x40);
const uint8_t ServoEnablePin = 32;      // Note - hardware needs updating!
PCA9685_servo Servos[2] = { PCA9685_servo(&ServoController, 0),
                            PCA9685_servo(&ServoController, 1)};
void StartMoveHandler(PCA9685_servo *theServo);	  // Servo callback
void StopMoveHandler(PCA9685_servo *theServo);		// Servo callback

// timing related Settings
uint32_t TNow;
uint32_t TLast;
uint32_t TEllapsed;
uint32_t TLastHeartbeat = 0;
uint32_t THeartbeatInterval = 1000;
uint8_t  HeartbeatState = LOW;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);   // set up the onboard Blue LED to use as a heartbeat indicator
  Serial.begin(115200);           // serial port I/O, only used when connected via USB

  digitalWrite(LEDoe, HIGH);      // by default outputs are to be disabled (there is a pull up as well)
  pinMode(LEDoe, OUTPUT);         // set up the output enable pin

  digitalWrite(ServoEnablePin, HIGH); // by default servos are disabled
  pinMode(ServoEnablePin, OUTPUT);

  // OLED set up
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE,SSD1306_BLACK); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  snprintf(DisplayBuffer[0],21, "Startup...");
  UpdateScreen();

  // WiFi Setup
  snprintf(DisplayBuffer[1],21, "WiFi: ");
  UpdateScreen();
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  CheckWiFi();	// this code handles actually connecting if not connected

  // OTA Setup
  ArduinoOTA.setHostname("Leopard Street Board D V2");
  ArduinoOTA.setPassword("leopard");
  ArduinoOTA.onStart([]() 
  {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    }
    );
    ArduinoOTA.begin();

  // LED set up & light test
  tlc.begin();
  //digitalWrite(LEDoe, LOW);   // enable LED outputs (enabled by default on this board)
  // initial LED test
  for (uint8_t i = 0; i<24; i++)
  {
    tlc.setPWM(i, 2047);    // half intensity    
  }
  tlc.write();
  delay(1000);
  for (uint8_t i = 0; i<24; i++)
  {
    tlc.setPWM(i, 0);    // off   
  }
  tlc.write();
  delay(1000);

  // PCF Setup
  PCF.begin();
  PCF.write16(0x0FFF);            // high bits 8-11 as outputs (relays, set high to disable the relay), 
                                  // high bits 12-15 not used, configured as outputs
                                  // low bits 0-7 as inputs (set high to be pulled low)
  pinMode(IRQPIN, INPUT_PULLUP);  // setup the IRQ pin
  attachInterrupt(digitalPinToInterrupt(IRQPIN), pcf_irq, FALLING);   // Configure the interrupt

  // Servo controller set up, note: using same end points for all here

  ServoController.begin();
  for (uint8_t i = 0; i< NumServos; i++)
  {
    Servos[i].onStartMove = StartMoveHandler;
    Servos[i].onStopMove = StopMoveHandler;
    Servos[i].setMode(MODE_TCONSTANT);
    Servos[i].setMinAngle(-25);
    Servos[i].setMidAngle(0);
    Servos[i].setMaxAngle(30);      
    Servos[i].setPosition(0);    // default to the mid point
    Servos[i].setAddress(i+1);    // address is the index the servo reports under, 1-16 not 0-15
  } 

  digitalWrite(ServoEnablePin, LOW);  // enable servos
                                      // MQTT as it sets up will set the correct positions

  // MQTT Setup - note, no screen update, this is handled in the main loop
  // note Servos and LEDs are configured prior to this to be set up when the MQTT data arrives
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  MQTTState = CheckMQTT();
  if (MQTTState)
  {
    snprintf(DisplayBuffer[2],21, "MQTT: Ok");
    UpdateScreen();
  }
  else
  {
    snprintf(DisplayBuffer[2],21, "MQTT: Fail");
    UpdateScreen();
  }

  // set up the turnouts
  snprintf(DisplayBuffer[3], 21, "TN: ----------------");   // default is all unknown

  // set up the relays
  snprintf(DisplayBuffer[4], 21, "RL: --------        ");   // default is all unknown

  // set up the lights
  snprintf(DisplayBuffer[5], 21, "L(l): ------------  ");
  snprintf(DisplayBuffer[6], 21, "L(h): ------------  ");

  // set up the block detection
  snprintf(DisplayBuffer[7], 21, "Block: --------     ");

  // end of configuration
  snprintf(DisplayBuffer[0],21, "Board %c", BoardID);
  UpdateScreen();
}

void loop() 
{
  // put your main code here, to run repeatedly:
  // update timing code, means we know how long the loop lasts - used by the servo code library loop
  TNow = millis();
  TEllapsed = TNow - TLast;
  TLast = TNow;


  CheckWiFi();    // checks we are connected, and will not return until we are

  // Check MQTT
  CheckMQTT();  // checks if we are connected, and if the state has changed

  // Check PCF8575
  CheckPCF();

  // Servo loop
  for (uint8_t i = 0; i< NumServos; i++)
  {
    Servos[i].loop(TEllapsed);
  }

  ArduinoOTA.handle();  // handle OTA stuff
  client.loop();  // performs the MQTT checks


  // provide a very basic "heart beat"
  if (TNow - TLastHeartbeat > THeartbeatInterval)
  {
    if(HeartbeatState == LOW)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      HeartbeatState = HIGH;
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
      HeartbeatState = LOW;
    }
    TLastHeartbeat = TNow;

    // display wifi strength
    int8_t S = WiFi.RSSI();
    snprintf(DisplayBuffer[0],21, "Board %c   WiFi: %d", BoardID, S);
    UpdateScreen();
  }


}

// MQTT Callback, this is where "control inputs" from JMRI are handled
void callback(char* topic, byte* payload, unsigned int length)
{
  uint8_t lTopic = strlen(topic);   // grab this so we can validate against it
  // we are subscribed to track/turnout/+ & track/light/+

  // turnouts
  // these are in the form track/turnout/[Board][number]   we want board = BoardID
  // payload will be "CLOSED" or "THROWN" or "MID" as a command
  if (lTopic >= 16)
  {
    if (topic[6] == 't' && topic[14] == BoardID)
    {
      // turnout message, and its for us, extract the turnout number
      uint8_t TurnoutNumber = 0;
      char *ptr = &topic[15];
      sscanf(ptr, "%d", &TurnoutNumber);

      // number must be 1 - 16 - note we only have defined two servos so we clip to that to avoid out of bounds error
      if (TurnoutNumber >= 1 && TurnoutNumber <= 2)
      {
        if (payload[0] == 'C')
        {
          // set CLOSED
          Servos[TurnoutNumber-1].throwServo(0);

          // now set up the relays
          switch (TurnoutNumber)
          {
            case 1:
            {
              PCF.write(8, 0);  // frog
              DisplayBuffer[4][4] = 'O';
              break;
            }
            case 2:
            {
              PCF.write(9, 0);  // frog
              DisplayBuffer[4][5] = 'O';
              break;
            }
            default:
            {
              break;
            }
          }
          UpdateScreen();
        }
        else if (payload[0] == 'T')
        {
          // set THROWN
          Servos[TurnoutNumber-1].throwServo(1);

          // now set up the relays
          switch (TurnoutNumber)
          {
            case 1:
            {
              PCF.write(8, 1);  // frog
              DisplayBuffer[4][4] = 'C';
              break;
            }
            case 2:
            {
              PCF.write(9, 1);  // frog
              DisplayBuffer[4][5] = 'C';
              break;
            }
            default:
            {
              break;
            }
          }
          UpdateScreen();
        }
        else if (payload[0] == 'M')
        {
          // set MID (note, non-standard message)
          Servos[TurnoutNumber-1].setPosition(1);
        }
      }

    }
  }

  // lights
  // these are in the form track/light/[Board][number]     we want board = BoardID
  // payload will be "ON", "OFF" or "INTENSITY 0.00"
  if (lTopic >= 14)
  {
    if (topic[6] == 'l' && topic[12] == BoardID)
    {
      // light message, for us, first we extract the light number
      uint8_t LightNumber = 0;
      char *ptr = &topic[13];   // point to the start of the number, will be 0 or higher
      sscanf(ptr, "%d", &LightNumber);

      // for now we only care about numbers 0-23
      if (LightNumber < 24)
      {
        if(payload[0] == 'O' && payload[1] == 'N')   // "ON"
        {
          tlc.setPWM(LightNumber, 4095);    // full intensity
          tlc.write();
          if (LightNumber < 12)
          {
            // low bank of 12
            DisplayBuffer[5][LightNumber+6] = '1';
          }
          else
          {
            // high bank of 12
            DisplayBuffer[6][LightNumber-6] = '1';
          }
        }
        else if(payload[1] == 'F')  // "OFF"
        {
          tlc.setPWM(LightNumber, 0);    // off
          tlc.write();
          if (LightNumber < 12)
          {
            // low bank of 12
            DisplayBuffer[5][LightNumber+6] = '0';
          }
          else
          {
            // high bank of 12
            DisplayBuffer[6][LightNumber-6] = '0';
          }
        }
        else if(payload[0] == 'I')  // "Intensity 0.00"
        {
          // determine brightness
          float i = 0.0;
          char myBuffer[30] = {0};
          for (uint8_t j = 0; j<length; j++)
          {
            myBuffer[j] = payload[j];
          }
          sscanf(myBuffer, "INTENSITY %f   ", &i);
          int Brightness = i * 100.0;
          //Serial.println(Brightness);   // 0-100
          Brightness = map(Brightness, 0, 100, 0, 4095);
          tlc.setPWM(LightNumber, Brightness);
          tlc.write();

          if (LightNumber < 12)
          {
            // low bank of 12
            DisplayBuffer[5][LightNumber+6] = 'i';
          }
          else
          {
            // high bank of 12
            DisplayBuffer[6][LightNumber-6] = 'i';
          }
        }
      }
    }
  }
  UpdateScreen();
}

// check MQTT is connected, manages reconnection and topic subscriptions
bool CheckMQTT(void)
{
  if (!client.connected())
  {
    // try to connected
    bool B = client.connect("Leopard Street Board E", mqtt_username, mqtt_password);
    if (B)
    {
      // we connected!

      // if we were not connected, and now we are, update the screen
      if (!MQTTState)
      {
        snprintf(DisplayBuffer[2],21, "MQTT: Ok  ");
        UpdateScreen();
      }

      // subscribe to desired topics
      client.subscribe("track/turnout/+");  // subscribe to all turnouts
      client.subscribe("track/light/+"); 
    }
    else
    {
      
      // if we were connected and now we are not we need to update the screen
      if (MQTTState)
      {
        snprintf(DisplayBuffer[2],21, "MQTT: Fail");
        UpdateScreen();
        MQTTState = false;
      }

      return false;   // we tried, we failed
    }
  }

  return true;
}

// check WiFi Status, if not connected, try to reconnect and return when done
void CheckWiFi(void)
{
  uint8_t Flag = 0;
  uint8_t i = 0;
  // this loop traps until reconnected
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);    // adds a sane time between reconnection attempts, could reduce but needs the screen
                    // update code adapting to use a non-blocking function
    snprintf(DisplayBuffer[1],21, "                    ", i);   // clear the line buffer before we start to blank any IP that is shown
    snprintf(DisplayBuffer[1],21, "WiFi: %d  ", i);
    UpdateScreen();
    i++;
    Flag = 1;

    //WiFi.reconnect();   // try to reconnect
  }

  if (Flag == 1)
  {
    // we were disconnected and have now reconnected
    IPAddress myIP = WiFi.localIP();
    snprintf(DisplayBuffer[1],21, "WiFi: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
    UpdateScreen();
  }
  
}

// PCF8575 IRQ handler, sets a flag and returns
void pcf_irq()
{
  PCFIRQflag = true;
}

// check the status of the PCF8575 chip
void CheckPCF(void)
{
  if (PCFIRQflag) // if the IRQ flag is set, read the chip, otherwise do nothing
  {
    PCFIRQflag = false;
    uint16_t x = PCF.read16();  // reads the chip, note we only actually care about the lower 8 bits, the rest are outputs

    
    
    if (x != PCFLastRead)
    {
      // something changed!
      // we need to work out which actual bits have changed, we will then publish appropriate messages, note the
      // DTC8 is Active Low

      // scan the bits (0-7 are inputs, 8-15 are outputs)
      for (uint8_t i = 0; i<8; i++)
      {
        if (is_bit_set(x,i) != is_bit_set(PCFLastRead,i))
        {
          if (is_bit_set(x,i))
          {
            char myBuffer[30] = {0};
            sprintf(myBuffer, "track/sensor/%cBlock%d", BoardID, i); // we are reporting Block0-Block7 so no offsets needed
            char myMessage[] = "INACTIVE";
            client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

            // manage the screen
            DisplayBuffer[7][i+7] = '0';
          }
          else
          {
            char myBuffer[30] = {0};
            sprintf(myBuffer, "track/sensor/%cBlock%d", BoardID, i);
            char myMessage[] = "ACTIVE";
            client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

            // manage the screen
            DisplayBuffer[7][i+7] = '1';
          }
        }
      }
      UpdateScreen();
    }

    PCFLastRead = x;
  }
}
void StartMoveHandler(PCA9685_servo *theServo)	// Servo callback
{
  // servo is in motion, update the feedback and sensor channels

  // need to set the appropriate sensor values to "INACTIVE"
  char myBuffer[30] = {0};
  char myMessage[30] = {0};
  sprintf(myMessage, "INACTIVE");
  sprintf(myBuffer, "track/sensor/ET%dThrown", theServo->getAddress());
  client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

  sprintf(myBuffer, "track/sensor/ET%dClosed", theServo->getAddress());
  client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

  // set the feedback channel
  sprintf(myMessage, "INCONSISTENT");
  sprintf(myBuffer, "track/turnout/E%d/feedback", theServo->getAddress());
  client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

  // update the screen to the inconsistant position
    DisplayBuffer[3][theServo->getAddress()+3] = '?';
    UpdateScreen();

}
void StopMoveHandler(PCA9685_servo *theServo)		// Servo callback
{
  //servo has ceased movement, update the feedback and sensor channels
  // needs to be able to cope with the mid point setting as "else" to show INCONSISTENT in JMRI

  if(theServo->isThrown())
  {
    // thrown
    char myBuffer[30] = {0};
    char myMessage[30] = {0};
    sprintf(myBuffer, "track/sensor/%cT%dThrown", BoardID, theServo->getAddress());
    sprintf(myMessage, "ACTIVE");
    client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 
    sprintf(myMessage, "INACTIVE");
    sprintf(myBuffer, "track/sensor/%cT%dClosed", BoardID, theServo->getAddress());
    client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

    sprintf(myMessage, "THROWN");
    sprintf(myBuffer, "track/turnout/%c%d/feedback", BoardID, theServo->getAddress());
    client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

    // update the screen to the final position
    DisplayBuffer[3][theServo->getAddress()+3] = 'T';
    UpdateScreen();
  }
  else
  {
    // clear
    char myBuffer[30] = {0};
    char myMessage[30] = {0};
    sprintf(myBuffer, "track/sensor/%cT%dThrown", BoardID, theServo->getAddress());
    sprintf(myMessage, "INACTIVE");
    client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 
    sprintf(myMessage, "ACTIVE");
    sprintf(myBuffer, "track/sensor/%cT%dClosed", BoardID, theServo->getAddress());
    client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 

    sprintf(myMessage, "CLOSED");
    sprintf(myBuffer, "track/turnout/%c%d/feedback", BoardID, theServo->getAddress());
    client.publish(myBuffer, (uint8_t *) myMessage, strlen(myMessage), true); 
    // update the screen to the final position
    DisplayBuffer[3][theServo->getAddress()+3] = 'C';
    UpdateScreen();
  }
}


// this maps the display character buffer to the OLED screen as a single function call
void UpdateScreen(void)
{
  display.setCursor(0, 0);
  for (uint8_t i = 0; i < 8; i++)
  {
    display.println(DisplayBuffer[i]);
  }
  display.display();
}

bool is_bit_set(unsigned value, unsigned bitindex)
{
    return (value & (1 << bitindex)) != 0;
}

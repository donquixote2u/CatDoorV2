/* Arduino cat door control; (for Arduino Uno / 5v Pro Mini)
*   if metal sensor detects collar tag (D5/T1) , fires door control solenoid and alert (D9)
*   serial port used for status output and receiving control commands
*  D2 for Hall effect door state sensor, and D3 for servo control
*/ 
#define UNO   // comment out for other version, different pins
#ifdef UNO
#define ALERT_PIN 9
#define SENSEPIN 2
#define SERVOLINE 3
#else
#define ALERT_PIN 1 // digital out pin for threshold alert (D1, chip pin 6) 
#endif
#define SerialRate 9600
#include <ServoTimer2.h>
ServoTimer2 myservo;  // create servo object to control a servo
int pos;        // desired position of servo

int DoorState, lastDoorState;  // state of Hall effect sensor on door
int lastTriggerValue=0;       // stored trigger value to detect change
// Number of cycles from external counter needed to generate a signal event
#define CYCLES_PER_SIGNAL 5000

// Frequency delta threshold for ALERT to trigger
#define ALERT_THRESHOLD 49

// Common Pin definitions
#define SENSITIVITY_POT_APIN A0
#define RESET_BTN_PIN 12

unsigned long lastSignalTime = 0;
unsigned long signalTimeDelta = 0;

boolean firstSignal = true;
unsigned long storedTimeDelta = 0;

// This signal is called whenever OCR1A reaches 0
// (Note: OCR1A is decremented on every external clock cycle)
SIGNAL(TIMER1_COMPA_vect)
{
  unsigned long currentTime = micros();
  signalTimeDelta =  currentTime - lastSignalTime;
  lastSignalTime = currentTime;

  if (firstSignal)
  {
    firstSignal = false;
  }
  else if (storedTimeDelta == 0)
  {
    storedTimeDelta = signalTimeDelta;
  }

  // Reset OCR1A
  OCR1A += CYCLES_PER_SIGNAL;
}

void setup()
{
   pinMode(SENSEPIN, INPUT); 
   DoorState = HIGH;
   lastDoorState = DoorState;    // init door state change = same = NO
   Serial.begin(SerialRate);	// Setup serial interface for test data outputs
   pos=10;
   doorlatch(pos);              // home servo
 
 // INITIALISE METAL DETECTOR TIMER 
  // Set WGM(Waveform Generation Mode) to 0 (Normal)
  TCCR1A = 0b00000000;
    // Set CSS(Clock Speed Selection) to 0b111 (External clock source on T0 pin
  // (ie, pin 5 on UNO). Clock on rising edge.)
  TCCR1B = 0b00000111;
  // Enable timer compare interrupt A (ie, SIGNAL(TIMER1_COMPA_VECT))
  TIMSK1 |= (1 << OCIE1A);
  // Set OCR1A (timer A counter) to 1 to trigger interrupt on next cycle
  OCR1A = 1;

  pinMode(ALERT_PIN, OUTPUT);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);

  // DEBUG ONLY - show adj pin value
  Serial.print("adj=");
  Serial.println(analogRead(SENSITIVITY_POT_APIN));
}

void loop()
{

    float sensitivity = mapFloat(analogRead(SENSITIVITY_POT_APIN), 0, 1023, 0.5, 10.0);
    int storedTimeDeltaDifference = (storedTimeDelta - signalTimeDelta) * sensitivity;
    if(storedTimeDeltaDifference!=lastTriggerValue) 
      { Serial.print("val=");
        Serial.println(storedTimeDeltaDifference);
      }  
    if (storedTimeDeltaDifference > ALERT_THRESHOLD)
    {
      digitalWrite(ALERT_PIN, LOW);
      Serial.print("alert triggered; val=");
      Serial.println(storedTimeDeltaDifference);
    }
    else
    {
      digitalWrite(ALERT_PIN, HIGH);
    }

  if (digitalRead(RESET_BTN_PIN) == LOW)
  {
    storedTimeDelta = 0;
  }
  delay(500);
   DoorState = digitalRead(SENSEPIN);
   if (DoorState != lastDoorState) {
     Serial.print("\nDoor state change:"); 
   // if the state has changed and change is to LOW door is closed
   if (DoorState == LOW) {
        Serial.print("Door closed"); 
        pos=2100; doorlatch(pos);             //if door clowsed, ensure latch also closed
        }
     else { 
        Serial.print("Door open"); 
        pos=1100; doorlatch(pos);             //if door open, ensure latch also open
      	}
     lastDoorState=DoorState;        // store latest door state   
    }        
 
}

float mapFloat(int input, int inMin, int inMax, float outMin, float outMax)
{
  float scale = (float)(input - inMin) / (inMax - inMin);
  return ((outMax - outMin) * scale) + outMin;
}

void doorlatch(int pos) {
  myservo.attach(SERVOLINE);  // attaches the servo to pwm line
  int val=pos;
  myservo.write(val);              // tell servo to go to position in variable 'pos'
  delay(100);
  myservo.detach();                 // detach pwm line
  }

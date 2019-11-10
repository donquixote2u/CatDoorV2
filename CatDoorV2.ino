/* Arduino cat door control; (for Arduino Uno / 5v Pro Mini)
*   if metal sensor detects collar tag (D5/T1) , fires door control solenoid and alert (D9)
*   serial port used for status output and receiving control commands
*  D2 for Hall effect door state sensor, and D3 for servo control
*/ 
#define UNO   // comment out for other version, different pins
#ifdef UNO
#define SENSEPIN 2
#define SERVOLINE 3
#else		//          for e.g. Attiny85
#define ALERT_PIN 1 // digital out pin for threshold alert (D1, chip pin 6) 
#endif
#define SerialRate 115200
#include <AltSoftSerial.h>
Altsoftserial Ser2;         // set up second serial line for HZ1050
#include <ServoTimer2.h>
ServoTimer2 myservo;  // create servo object to control a servo
int pos;        // desired position of servo
int DoorState, lastDoorState;	// state of Hall effect sensor on door
int lastTriggerValue=0;	   	// stored trigger value to detect change
int Mode="B";				        // stores door status mode (I,O,B,L)

void setup()
{
   pinMode(SENSEPIN, INPUT); 
   DoorState = HIGH;
   lastDoorState = DoorState;    // init door state change = same = NO
   Serial.begin(SerialRate);	// Setup serial interface for test data outputs
   pos=10;
   doorlatch(pos);              // home servo
}

void loop()
{
   DoorState = digitalRead(SENSEPIN);
   if (DoorState != lastDoorState) {
     Serial.print("\nDoor state change:"); 
   // if the state has changed and change is to LOW door is closed
   if (DoorState == LOW) {
        Serial.print("Door closed"); 
	if(Mode!="B")
           { pos=2100; doorlatch(pos); }     //if door closed, ensure latch also closed
        }
     else { 
        Serial.print("Door open"); 
        pos=1100; doorlatch(pos);             //if door open, ensure latch also open
      	}
     lastDoorState=DoorState;        // store latest door state   
    } 
    checkControls();      
 }

void checkControls() {
int data;
bool cmdmode=false;
 if(Serial.available() > 0)
   {
   while(Serial.available() > 0) {
    data=Serial.read();
    if(cmdmode) {
      switch (data) {
        case 66:
          Mode="B";
          pos=1100; doorlatch(pos); 
          break;
        case 73:
          Mode="I";
          break;
        case 76:
          Mode="L";
          break;
        case 79:
	  Mode="O";
          pos=2150; doorlatch(pos);
          break;
        }         // end switch
	Serial.print("status "+Mode);
      }           // end cmdmode
    else { if(data==35)			// "#" precedes a single-char mode command
             { 
              Serial.println("\ncommand mode on");
              cmdmode=true; 
             }
           else
              { cmdmode=false; }
         } 
   }    // end while
 }    // end if  
}

void doorlatch(int pos) {
  myservo.attach(SERVOLINE);  // attaches the servo to pwm line
  int val=pos;
  myservo.write(val);              // tell servo to go to position in variable 'pos'
  delay(100);
  myservo.detach();                 // detach pwm line
  }

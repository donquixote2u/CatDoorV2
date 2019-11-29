/* Arduino cat door control; (for Arduino Uno / 5v Pro Mini)
*   if metal sensor detects collar tag (D5/T1) , fires door control solenoid and alert (D9)
*   serial port used for status output and receiving control commands
*  D2 for Hall effect door state sensor, and D3 for servo control
*/ 
#define UNO   // comment out for other version, different pins
#ifdef UNO
#define SENSEPIN 2
#define SERVOLINE 3
#define RX1 8
#define TX1 9
#endif
#define SerialRate 115200
#define Ser2Rate 19200
#define INDEX_SIZE 48 // buffer size set to 48 char 
#include <SoftwareSerial.h> // reserves pins 8(Rx)9(Tx)
SoftwareSerial RFin(RX1,TX1);         // set up second serial line for HZ1050
#include <ServoTimer2.h>
ServoTimer2 myservo;  // create servo object to control a servo
int pos;        // desired position of servo
char rxbuffer[INDEX_SIZE] = {}; // RFin receive buffer
int buffptr = 0; // RFin position in circular buffer above
long ID=0;        // tag ID read
int DoorState, lastDoorState;	// state of Hall effect sensor on door
int lastTriggerValue=0;	   	// stored trigger value to detect change
int Mode="B";				        // stores door status mode (I,O,B,L)
bool Debug=true;            // debug (serial output) mode switch

void setup()
{
   RFin.begin(Ser2Rate);       // set up RFI serial comms
   pinMode(SENSEPIN, INPUT);  // door state sensor
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
   // if the state has changed and change is to LOW door is closed
   if (DoorState == LOW) {
        if(Debug) { Serial.print("DC\n"); } 
	 if(Mode!="B")
           { pos=2100; doorlatch(pos); }     //if door closed, ensure latch also closed
        }
     else { 
        if(Debug) { Serial.print("DO\n"); } 
        pos=1100; doorlatch(pos);             //if door open, ensure latch also open
      	}
     lastDoorState=DoorState;        // store latest door state   
    } 
    checkControls();                  // any control commands received?     
    buffptr=0;
    if(RFin.available()>0)
     {
      buffptr=0; ID=0;
      while(RFin.available()>0)
       {
        rxbuffer[buffptr]=RFin.read();
        buffptr++;
       }              // end while serial
      for(int i=0;i<buffptr;i++)
        {     
          // DEBUG Serial.write(rxbuffer[i]);
          ID <<= 8;      // shift left 8 bits, add new byte to ID number
          ID |= rxbuffer[i];
        }
      if(Debug) {  
        Serial.print("ID");
        Serial.print(ID,DEC);
        Serial.print("\n");
        }
      // if(ID==tag1 || ID==tag2)  // if authoried tag detected, 
      // {
      // pos=1100; doorlatch(pos); delay(15000); pos=2150; doorlatch(pos);
      // } 
      // else { Serial.print("\nunidentified tag"); }
      }
    delay(500); 
}

void checkControls() {
int data;
char serbuffer[INDEX_SIZE] = {}; // Serial control line receive buffer
int serptr = 0;                  // position in serbuffer above
bool cmdmode=false;
 if(Serial.available() > 0)
   {
   serptr=0; 
   while(Serial.available() > 0) {
    serbuffer[serptr]=Serial.read();
    serptr++;
    }
    Serial.flush();
    for(int i=0;i<serptr;i++) {
    if(cmdmode) {
      // Serial.print(serbuffer[i],HEX);
      switch (serbuffer[i]) {
        case 66:
          Mode="B";
          pos=1100; doorlatch(pos); 
          break;
        case 68:
          Debug=true;
          Serial.print("Debug mode ON\n"); 
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
      Serial.print("*M");
	    Serial.write(Mode);
      Serial.print("\n");
      cmdmode=false;
      }           // end cmdmode
    else { if(serbuffer[i]==35)			// "#" precedes a single-char mode command
             { 
              if(Debug) {Serial.print("command mode on\n");}
              cmdmode=true; 
             }
         } // end if #
     }    // end for
   }    // end if available  
}

void doorlatch(int pos) {
  myservo.attach(SERVOLINE);  // attaches the servo to pwm line
  int val=pos;
  myservo.write(val);              // tell servo to go to position in variable 'pos'
  delay(100);
  myservo.detach();                 // detach pwm line
  }

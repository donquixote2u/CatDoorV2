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
String Mode="B";				        // stores door status mode (I,O,B,L)
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
        if(Debug) { Serial.print("*DC\n"); } 
   //if door closed and mode not (B)oth ways, ensure latch also closed.
   // 2020-03-13: suppressed for now     
	 // if(Mode!="B")
   //     { pos=2100; doorlatch(pos); }     
        }
     else { 
        if(Debug) { Serial.print("*DO\n"); } 
        //if door open, ensure latch also open
        // 2020-03-13: suppressed for now 
        // pos=1050; doorlatch(pos);             
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
        Serial.print("*ID");
        Serial.print(ID,DEC);
        Serial.print("\n");
        }
      // if(ID==tag1 || ID==tag2)  // if authorised tag detected, 
      // {
      // pos=1100; doorlatch(pos); delay(15000); pos=2150; doorlatch(pos);
      // } 
      // else { Serial.print("\nunidentified tag"); }
      }
    delay(200); 
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
          Mode="B";    // BOTH WAYS
          pos=1050; doorlatch(pos); 
          break;
        case 67:
          Mode="C";    // CLEAR (RESET) CDC
          break;
        case 68:
          Debug=true;
          Serial.print("Debug ON\n"); 
          break;  
        case 73:
          Mode="I";
          break;
        case 76:
          Mode="L";
          break;
        case 79:
	        Mode="O";
          pos=2100; doorlatch(pos);
          break;
        case 81:
                        // Mode="Q"; query;  just return current Mode setting
          break;
        case 84:
          Mode="T";    // Train; save next tag read as a valid one
          break;
        }         // end switch
      String ack="*";
      ack=ack+Mode+"\n";  
      Serial.print(ack);
      cmdmode=false;
      }           // end cmdmode
    else { if(serbuffer[i]==35)			// "#" precedes a single-char mode command
             { 
              // if(Debug) {Serial.print("command mode on\n");}
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

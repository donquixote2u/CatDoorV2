/*
 * Copyright (c) 2016 Evan Kale  circuit proven 18/7/19 BW
 * Media: @EvanKale91
 * Email: EvanKale91@gmail.com
 * Website: www.ISeeDeadPixel.com
 *          www.evankale.blogspot.ca
 *          www.youtube.com/EvanKale91
 *
 * This file is part of ArduinoMetalDetector.
 *
 * ArduinoMetalDetector is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define UNO   // comment out for Attiny version, different pins
#include <SoftwareSerial.h>  // convert to SS for Attiny port
#ifdef UNO
#define RX 0      // *** D0, Pin 0 / RX  
#define TX 1      // *** D1  Pin 1 / TX
#define ALERT_PIN 9
#else
#define RX 3      // *** D3, Pin 2 
#define TX 4      // *** D4, Pin 3
#define ALERT_PIN 1 // digital out pin for threshold alert (D1, chip pin 6) 
#endif
// Number of cycles from external counter needed to generate a signal event
#define CYCLES_PER_SIGNAL 5000

// Base tone frequency (speaker)
#define BASE_TONE_FREQUENCY 280

// Frequency delta threshold for ALERT to trigger
#define ALERT_THRESHOLD 49

// Common Pin definitions
#define SENSITIVITY_POT_APIN A0
#define SPEAKER_PIN 2

#define TRIGGER_BTN_PIN 11
#define RESET_BTN_PIN 12

SoftwareSerial Sercom(RX, TX);  // use SS for portability

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
  Sercom.begin(115200); // Setup serial interface for test data outputs
 
  // Set WGM(Waveform Generation Mode) to 0 (Normal)
  TCCR1A = 0b00000000;
  
  // Set CSS(Clock Speed Selection) to 0b111 (External clock source on T0 pin
  // (ie, pin 5 on UNO). Clock on rising edge.)
  TCCR1B = 0b00000111;

  // Enable timer compare interrupt A (ie, SIGNAL(TIMER1_COMPA_VECT))
  TIMSK1 |= (1 << OCIE1A);

  // Set OCR1A (timer A counter) to 1 to trigger interrupt on next cycle
  OCR1A = 1;

  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(ALERT_PIN, OUTPUT);
  pinMode(TRIGGER_BTN_PIN, INPUT_PULLUP);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);

  // DEBUG ONLY - show adj pin value
  Sercom.print("adj=");
  Sercom.println(analogRead(SENSITIVITY_POT_APIN));
}

void loop()
{
  // trigger button not implemented yet, constant operation
  // if (digitalRead(TRIGGER_BTN_PIN) == LOW)
  {
    float sensitivity = mapFloat(analogRead(SENSITIVITY_POT_APIN), 0, 1023, 0.5, 10.0);
    int storedTimeDeltaDifference = (storedTimeDelta - signalTimeDelta) * sensitivity;
    tone(SPEAKER_PIN, BASE_TONE_FREQUENCY + storedTimeDeltaDifference);

    Sercom.print("val=");
    Sercom.println(storedTimeDeltaDifference);
    if (storedTimeDeltaDifference > ALERT_THRESHOLD)
    {
      digitalWrite(ALERT_PIN, LOW);
    }
    else
    {
      digitalWrite(ALERT_PIN, HIGH);
    }
  }
  // trigger not implemented
  // else
  // {
  //   noTone(SPEAKER_PIN);
  //   digitalWrite(ALERT_PIN, HIGH);
  // }

  if (digitalRead(RESET_BTN_PIN) == LOW)
  {
    storedTimeDelta = 0;
  }
  delay(500);
}

float mapFloat(int input, int inMin, int inMax, float outMin, float outMax)
{
  float scale = (float)(input - inMin) / (inMax - inMin);
  return ((outMax - outMin) * scale) + outMin;
}

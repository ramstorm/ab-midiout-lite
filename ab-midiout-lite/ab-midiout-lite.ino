/**************************************************************************
 * Author:       trash80                                                  *
 * Modified by:  ledfyr                                                   *
 **************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define BYTE_DELAY 80
#define BIT_DELAY 2
#define BIT_READ_DELAY 0
#define PIN_GB_CLOCK 0
#define PIN_GB_SERIALOUT 1
#define PIN_MIDI_INPUT_POWER 4


int countGbClockTicks = 0;
byte midiData[] = {0, 0, 0};
byte midiChannels[] = {0, 1, 2, 3};
byte midiCcNumbers[] = {1, 2, 3, 7, 10, 11, 12};
int midiOutLastNote[4] = {-1, -1, -1, -1};
unsigned long midioutNoteTimer[4] = {0, 0, 0, 0};
int midioutNoteTimerThreshold = 10;
boolean midiValueMode = false;
int countClockPause = 0;
byte incomingMidiByte;

void setup() {
  DDRC  = B00000011; // Set analog in pins as inputs
  PORTC = B00000001;

  pinMode(PIN_MIDI_INPUT_POWER, OUTPUT);
  digitalWrite(PIN_MIDI_INPUT_POWER, HIGH); // Turn on the optoisolator
  Serial.begin(31250); // Not using USB mode
  digitalWrite(PIN_GB_CLOCK, HIGH); // Gameboy wants a HIGH line
  digitalWrite(PIN_GB_SERIALOUT, LOW); // No data to send
}

void loop() {
  if(getIncommingSlaveByte()) {
    if(incomingMidiByte > 0x6f) {
      switch(incomingMidiByte) {
        case 0x7E: //seq stop
          Serial.write(0xFC);
          stopAllNotes();
          break;
        case 0x7D: //seq start
          Serial.write(0xFA);
          break;
        default:
          midiData[0] = incomingMidiByte - 0x70;
          midiValueMode = true;
          break;
      }
    }
    else if (midiValueMode == true) {
      midiValueMode = false;
      midioutDoAction(midiData[0], incomingMidiByte);
    }
  }
}

void midioutDoAction(byte m, byte v) {
  if(m < 4) {
    // Note message
    if(v > 0) {
      playNote(m,v);
    }
    else {
      stopNote(m);
    }
  }
  else if (m < 8) {
    m-=4;
    // CC message
    playCC(m,v);
  }
}

void stopNote(byte m) {
  midiData[0] = 0x80 + midiChannels[m];
  midiData[1] = midiOutLastNote[m];
  midiData[2] = 0x00;
  Serial.write(midiData,3);
  midiOutLastNote[m] = -1;
}

void playNote(byte m, byte n) {
  midiData[0] = 0x90 + midiChannels[m];
  midiData[1] = n;
  midiData[2] = 0x7F;
  Serial.write(midiData,3);
  midioutNoteTimer[m] = millis();
  midiOutLastNote[m] = n;
}

void playCC(byte m, byte n) {
  byte v = n & 0x0F;
  v = v*8 + (v>>1); // CC value will span the whole range 0-127
  n = (n>>4) & 0x07;
  midiData[0] = 0xB0 + midiChannels[m];
  midiData[1] = midiCcNumbers[n];
  midiData[2] = v;
  Serial.write(midiData,3);
}

void stopAllNotes() {
  for(int m=0;m<4;m++) {
    if(midiOutLastNote[m]>=0) {
      stopNote(m);
    }
    midiData[0] = 0xB0 + midiChannels[m];
    midiData[1] = 123;
    midiData[2] = 0x7F;
    Serial.write(midiData,3);
  }
}

boolean getIncommingSlaveByte() {
  delayMicroseconds(BYTE_DELAY);
  PORTC = B00000000; // Rightmost bit is clock line, 2nd bit is data to gb, 3rd is DATA FROM GB
  delayMicroseconds(BYTE_DELAY);
  PORTC = B00000001;
  delayMicroseconds(BIT_DELAY);
  if((PINC & B00000100)) {
    incomingMidiByte = 0;
    for(countClockPause=0;countClockPause!=7;countClockPause++) {
      PORTC = B00000000;
      delayMicroseconds(BIT_DELAY);
      PORTC = B00000001;
      delayMicroseconds(BIT_READ_DELAY);
      incomingMidiByte = (incomingMidiByte<<1) + ((PINC & B00000100)>>2);
    }
    return true;
  }
  return false;
}


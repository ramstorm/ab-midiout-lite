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
#include <MIDI.h>
#include <TimerOne.h>

#define BYTE_DELAY 80
#define BIT_DELAY 2
#define BIT_READ_DELAY 0
#define PIN_GB_CLOCK 0
#define PIN_GB_SERIALOUT 1
#define PIN_MIDI_INPUT_POWER 4
#define CLOCKS_PER_BEAT 24
#define MIN_BPM 800

MIDI_CREATE_DEFAULT_INSTANCE();

byte midiChannels[4] = {1, 2, 3, 4};
byte midiCcNumbers[7] = {1, 2, 3, 7, 10, 11, 12};
int midiOutLastNote[4] = {-1, -1, -1, -1};
int velocity[4] = {100, 100, 100, 100};

int bpm = 1280; // BPM in tenths of a BPM

int chord[4] = {-1, -1, -1, -1};
int chordIx = 1;
byte chords[15][6] = {
  {3,0,3,7},
  {3,0,4,7},
  {3,7,12,15},
  {3,7,12,16},
  {2,0,3},
  {2,0,4},
  {4,0,3,7,10},
  {4,0,4,7,11},
  {5,0,3,7,10,14},
  {5,0,4,7,11,14},
  {3,0,5,7},
  {2,0,7},
  {3,0,7,12},
  {3,2,0,12},
  {3,0,12,24}
};

int countGbClockTicks = 0;
byte midiData = 0;
boolean midiValueMode = false;
int countClockPause = 0;
byte incomingMidiByte;

void setup() {
  DDRC  = B00000011; // Set analog in pins as inputs
  PORTC = B00000001;

  pinMode(PIN_MIDI_INPUT_POWER, OUTPUT);
  digitalWrite(PIN_MIDI_INPUT_POWER, HIGH); // Turn on the optoisolator
  digitalWrite(PIN_GB_CLOCK, HIGH); // Gameboy wants a HIGH line
  digitalWrite(PIN_GB_SERIALOUT, LOW); // No data to send

  MIDI.begin();

  // MIDI clock timer interrupt
  Timer1.initialize(calculateIntervalMicroSecs(bpm));
  Timer1.attachInterrupt(sendClockPulse);
}

void loop() {
  if (getIncomingSlaveByte()) {
    if (incomingMidiByte > 0x6f) {
      switch (incomingMidiByte) {
        case 0x7E: //seq stop
          MIDI.sendRealTime(midi::Stop);
          stopAllNotes();
          break;
        case 0x7D: //seq start
          MIDI.sendRealTime(midi::Start);
          break;
        default:
          midiData = incomingMidiByte - 0x70;
          midiValueMode = true;
          break;
      }
    }
    else if (midiValueMode == true) {
      midiValueMode = false;
      midioutDoAction(midiData, incomingMidiByte);
    }
  }
}

void midioutDoAction(byte m, byte v) {
  if (m < 4) {
    // Note message
    if (v > 0) {
      if (midiOutLastNote[m] >= 0) {
        stopNote(m);
      }
      playNote(m,v);
    }
    else {
      stopNote(m);
    }
  }
  else if (m < 8) {
    m -= 4;
    // CC message
    playCC(m,v);
  }
}

void stopNote(byte m) {
  if (chord[m] >= 0) {
    for (chordIx = 1; chordIx <= chords[chord[m]][0]; chordIx++) {
      MIDI.sendNoteOff(midiOutLastNote[m] + chords[chord[m]][chordIx], 100, midiChannels[m]);
    }
  }
  else {
    MIDI.sendNoteOff(midiOutLastNote[m], 100, midiChannels[m]);
  }
  midiOutLastNote[m] = -1;
}

void playNote(byte m, byte n) {
  if (chord[m] >= 0) {
    for (chordIx = 1; chordIx <= chords[chord[m]][0]; chordIx++) {
      MIDI.sendNoteOn(n + chords[chord[m]][chordIx], velocity[m], midiChannels[m]);
    }
  }
  else {
    MIDI.sendNoteOn(n, velocity[m], midiChannels[m]);
  }
  midiOutLastNote[m] = n;
}

void playCC(byte m, byte n) {
  byte v = n & 0x0F; // GB CC value 0-15
  n = (n >> 4) & 0x07; // GB CC number 0-6
  switch (n) {
    case 3: // Set velocity 1-127
      if (v == 0) {
        velocity[m] = 1;
      }
      else {
        velocity[m] = v * 8 + (v >> 1);
      }
      break;
    case 4: // Set clock interval
      Timer1.setPeriod(calculateIntervalMicroSecs(MIN_BPM + v * 80));
      Timer1.restart();
      break;
    case 5: // Set chord with 1-15, 0 => chord off
      chord[m] = v - 1;
      break;
    case 6: // Set the current channel to MIDI ch 1-16
      midiChannels[m] = v + 1;
      break;
    default: // Send CC
      MIDI.sendControlChange(midiCcNumbers[n], v * 8 + (v >> 1), midiChannels[m]);
      break;
  }
}

void stopAllNotes() {
  for(int m=0; m < 4; m++) {
    if(midiOutLastNote[m] >= 0) {
      stopNote(m);
    }
    MIDI.sendControlChange(123, 0x7F, midiChannels[m]);
  }
}

long calculateIntervalMicroSecs(int bpm) {
  return 60L * 1000 * 1000 * 10 / bpm / CLOCKS_PER_BEAT;
}

void sendClockPulse() {
  MIDI.sendRealTime(midi::Clock);
}

boolean getIncomingSlaveByte() {
  delayMicroseconds(BYTE_DELAY);
  PORTC = B00000000; // Rightmost bit is clock line, 2nd bit is data to gb, 3rd is data from gb
  delayMicroseconds(BYTE_DELAY);
  PORTC = B00000001;
  delayMicroseconds(BIT_DELAY);
  if ((PINC & B00000100)) {
    incomingMidiByte = 0;
    for (countClockPause = 0; countClockPause != 7; countClockPause++) {
      PORTC = B00000000;
      delayMicroseconds(BIT_DELAY);
      PORTC = B00000001;
      delayMicroseconds(BIT_READ_DELAY);
      incomingMidiByte = (incomingMidiByte << 1) + ((PINC & B00000100) >> 2);
    }
    return true;
  }
  return false;
}


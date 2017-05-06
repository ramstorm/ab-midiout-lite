/***************************************************************************
 * Author:       trash80                                                   *
 * Modified by:  ledfyr                                                    *
 * Teensy port:  catskull                                                  *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <TimerOne.h>
#include <MIDI.h>

/***************************************************************************
* Teensy 3.2, Teensy 3.0, Teensy LC
*
* Notes on Teensy: Pins are not the same as in the schematic, the mapping is below.
* Feel free to change, all related config in is this block.
* Be sure to compile
***************************************************************************/
#if defined (__MK20DX256__) || defined (__MK20DX128__) || defined (__MKL26Z64__)
#define USE_TEENSY 1
#define PIN_GB_CLOCK 16
#define PIN_GB_SERIALOUT 17
#define PIN_MIDI_INPUT_POWER 4

#if defined (__MKL26Z64__)
#define GB_SET(bit_cl,bit_out,bit_in) GPIOB_PDOR = ((bit_in<<3) | (bit_out<<1) | bit_cl)
#else
#define GB_SET(bit_cl,bit_out,bit_in) GPIOB_PDOR = (GPIOB_PDIR & 0xfffffff4) | ((bit_in<<3) | (bit_out<<1) | bit_cl)
#endif

/***************************************************************************
* Arudino Atmega 328 (assumed)
***************************************************************************/
#else
#define GB_SET(bit_cl,bit_out,bit_in) PORTC = (PINC & B11111000) | ((bit_in<<2) | ((bit_out)<<1) | bit_cl)
// ^ The reason for not using digitalWrite is to allign clock and data pins for the GB shift reg.

#define PIN_GB_CLOCK 0
#define PIN_GB_SERIALOUT 1
#define PIN_MIDI_INPUT_POWER 4

MIDI_CREATE_DEFAULT_INSTANCE();

#endif

#define BYTE_DELAY 80
#define BIT_DELAY 2
#define BIT_READ_DELAY 0
#define CLOCKS_PER_BEAT 24
#define MIN_BPM 80
#define MAX_BPM 200

// CC numbers for setting velocity, tempo, chord and channel.
// Only 0-6 can be used. Set to 7 or greater to turn off a function and free up a CC.
#define VELOCITY_CC 3
#define TEMPO_CC 4
#define CHORD_CC 5
#define CHANNEL_CC 6

#define STOP 0xFC
#define START 0xFA
#define CLOCK 0xF8

byte midiChannels[4] = {1, 2, 3, 4};
byte midiCcNumbers[7] = {1, 2, 3, 7, 10, 11, 12};
int midiOutLastNote[4] = {-1, -1, -1, -1};
int velocity[4] = {100, 100, 100, 100};
boolean clockOn = false;
int bpm = 128;
long minTapInterval = 60L * 1000 * 1000 / MAX_BPM;
long maxTapInterval = 60L * 1000 * 1000 / MIN_BPM;
long firstTapTime = 0;
long lastTapTime = 0;
long timesTapped = 1;
long now = 0;

int chord[4] = {-1, -1, -1, -1};
int chordIx = 1;

// List of chords. First number = chord size, the rest = key 1, 2, 3 etc.
byte chords[15][6] = {
  {3,0,3,7},       // Minor 3 keys
  {3,0,4,7},       // Major 3 keys
  {3,7,12,15},     // Minor 3 keys, highest key is an octave below
  {3,7,12,16},     // Major 3 keys, highest key is an octave below
  {2,0,3},         // Minor 2 keys
  {2,0,4},         // Major 2 keys
  {4,0,3,7,10},    // M7
  {4,0,4,7,11},    // Maj7
  {5,0,3,7,10,14}, // M9
  {5,0,4,7,11,14}, // Maj9
  {3,0,5,7},       // Sus4
  {2,0,7},         // Power chord 2 keys
  {3,0,7,12},      // Power chord 3 keys
  {2,0,12},        // Octave 2 keys
  {3,0,12,24}      // Octave 3 keys
};

int countGbClockTicks = 0;
byte midiData = 0;
boolean midiValueMode = false;
int countClockPause = 0;
byte incomingMidiByte;

volatile byte sendClock = 0;

void setup() {
  DDRC  = B00000011; // Set analog in pins as inputs
  PORTC = B00000001;

  pinMode(PIN_MIDI_INPUT_POWER, OUTPUT);
  digitalWrite(PIN_MIDI_INPUT_POWER, HIGH); // Turn on the optoisolator
  digitalWrite(PIN_GB_CLOCK, HIGH); // Gameboy wants a HIGH line
  digitalWrite(PIN_GB_SERIALOUT, LOW); // No data to send

#ifndef USE_TEENSY
  MIDI.begin();
#endif

  // MIDI clock timer interrupt
  Timer1.initialize(clockIntervalMicros(bpm));
  Timer1.attachInterrupt(sendClockPulse);
  Timer1.stop();
}

void loop() {
  if (getIncomingSlaveByte()) {
    if (incomingMidiByte > 0x6f) {
      switch (incomingMidiByte) {
        case 0x7E: //seq stop
#ifdef USE_TEENSY
          usbMIDI.sendRealTime(STOP);
#else
          MIDI.sendRealTime(midi::Stop);
#endif
          stopAllNotes();
          break;
        case 0x7D: //seq start
          if (clockOn) {
            Timer1.restart();
          }
#ifdef USE_TEENSY
          usbMIDI.sendRealTime(START);
#else
          MIDI.sendRealTime(midi::Start);
#endif
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
  if (sendClock > 0) {
#ifdef USE_TEENSY
    usbMIDI.sendRealTime(CLOCK);
#else
    MIDI.sendRealTime(midi::Clock);
#endif
    sendClock = 0;
  }
}

void midioutDoAction(byte m, byte v) {
  if (m < 4) {
    // Note message
    if (v > 0) {
      if (midiOutLastNote[m] >= 0) {
        stopNote(m);
      }
      playNote(m, v);
    }
    else {
      stopNote(m);
    }
  }
  else if (m < 8) {
    m -= 4;
    playCC(m, v);
  }
  else if(m < 0x0C) {
    m -= 8;
    playPC(m, v);
  }
}

void stopNote(byte m) {
  if (chord[m] >= 0) {
    for (chordIx = 1; chordIx <= chords[chord[m]][0]; chordIx++) {
#ifdef USE_TEENSY
      usbMIDI.sendNoteOff(midiOutLastNote[m] + chords[chord[m]][chordIx], 100, midiChannels[m]);
#else
      MIDI.sendNoteOff(midiOutLastNote[m] + chords[chord[m]][chordIx], 100, midiChannels[m]);
#endif
    }
  }
  else {
#ifdef USE_TEENSY
    usbMIDI.sendNoteOff(midiOutLastNote[m], 100, midiChannels[m]);
#else
    MIDI.sendNoteOff(midiOutLastNote[m], 100, midiChannels[m]);
#endif
  }
  midiOutLastNote[m] = -1;
}

void playNote(byte m, byte n) {
  if (chord[m] >= 0) {
    for (chordIx = 1; chordIx <= chords[chord[m]][0]; chordIx++) {
#ifdef USE_TEENSY
      usbMIDI.sendNoteOn(n + chords[chord[m]][chordIx], velocity[m], midiChannels[m]);
#else
      MIDI.sendNoteOn(n + chords[chord[m]][chordIx], velocity[m], midiChannels[m]);
#endif
    }
  }
  else {
#ifdef USE_TEENSY
    usbMIDI.sendNoteOn(n, velocity[m], midiChannels[m]);
#else
    MIDI.sendNoteOn(n, velocity[m], midiChannels[m]);
#endif
  }
  midiOutLastNote[m] = n;
}

void playCC(byte m, byte n) {
  byte v = n & 0x0F; // GB CC value 0-15
  n = (n>>4) & 0x07; // GB CC number 0-6
  switch (n) {
    case VELOCITY_CC: // Set velocity 1-127
      if (v == 0) {
        velocity[m] = 1;
      }
      else {
        velocity[m] = v*8 + (v>>1);
      }
      break;
    case TEMPO_CC: // Set clock interval using CC value or tap tempo
      if (v == 0) {
        Timer1.stop();
        clockOn = false;
        return;
      }

      now = micros();
      if (now - lastTapTime < minTapInterval) {
        return;
      }

      if (now - lastTapTime > maxTapInterval) {
        Timer1.setPeriod(clockIntervalMicros(MIN_BPM + v*8));
        firstTapTime = now;
        timesTapped = 1;
      }
      else {
        Timer1.setPeriod((now - firstTapTime) / timesTapped / CLOCKS_PER_BEAT);
        timesTapped++;
      }
      lastTapTime = now;

      if (!clockOn) {
        Timer1.start();
        clockOn = true;
      }
      break;
    case CHORD_CC: // Set chord with 1-15, 0 => chord off
      chord[m] = v - 1;
      break;
    case CHANNEL_CC: // Set the current channel to MIDI ch 1-16
      midiChannels[m] = v + 1;
      break;
    default: // Send CC
#ifdef USE_TEENSY
      usbMIDI.sendControlChange(midiCcNumbers[n], v*8 + (v>>1), midiChannels[m]);
#else
      MIDI.sendControlChange(midiCcNumbers[n], v*8 + (v>>1), midiChannels[m]);
#endif
      break;
  }
}

void playPC(byte m, byte n) {
#ifdef USE_TEENSY
  usbMIDI.sendProgramChange(n, midiChannels[m]);
#else
  MIDI.sendProgramChange(n, midiChannels[m]);
#endif
}

void stopAllNotes() {
  for(int m=0; m<4; m++) {
    if(midiOutLastNote[m] >= 0) {
      stopNote(m);
    }
#ifdef USE_TEENSY
    usbMIDI.sendControlChange(123, 0x7F, midiChannels[m]);
#else
    MIDI.sendControlChange(123, 0x7F, midiChannels[m]);
#endif
  }
}

long clockIntervalMicros(int bpm) {
  return 60L * 1000 * 1000 / bpm / CLOCKS_PER_BEAT;
}

void sendClockPulse() {
  sendClock = 1;
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

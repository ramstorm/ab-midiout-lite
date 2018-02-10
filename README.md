# Arduinoboy lite - MIDI out mode
This is a custom MIDI out version of trash80's Arduinoboy software. Both Arduino and Teensy boards are supported.

## New functions
* MIDI clock! Tempo is set with X4y (y=0: OFF, y=1: 88 BPM, y=2: 96 BPM, ... y=8: 144 BPM, ... and so on). Clock is OFF when starting the Arduinoboy.
* Tap-tempo to control MIDI clock. Send X4y on every beat for a number of beats (4-8 is enough). The first X4y sets tempo as above, the following ones use the time taken between beats to set the clock tempo.
* Velocity. Set velocity of the current channel with X3y (y=0: velocity=1, ... increases in steps of 8).
* Chords. X50 = chord off, X51 = first chord (minor), X52 second chord (major). Look in the code for the complete list of chords.
* Channel switching. X61 = current LSDJ channel sends on MIDI channel 1, ... X6A current LSDJ channel sends on MIDI channel 10, ... and so on.
* Dropped notes don't occur anymore.
* MIDI CC values sent from LSDJ are scaled to let you use the whole range 0-127 instead of 0-120.

## Limitations
* Only LSDJ MIDI out mode available, no other modes.
* No LED blinking.
* No configuration via Max patch.

## Installation
* Use the Arduino IDE.
* Install libraries:
  * Open the IDE, open the "Sketch" menu and then Include Library -> Manage Libraries.
  * Search for and install the libraries "TimerOne" and "MIDI Library".
* For Teensy, download and install the Teensyduino add-on for the Arduino IDE. Select Card: "Teensy LC" (or other card if you use one) and USB Type: "Serial + MIDI" in the Tools menu.
* Upload the software to your Arduinoboy/Teensyboy the usual way (disconnect MIDI and GB cables first).
* Tested with:
  * DMG, GBC and GBA. (Busy MIDI sequencing works best on GBC and GBA).
  * LSDJ 4.7.3, 4.8.8 and 5.3.4 (Arduinoboy versions).
  * Arduino Uno Rev. 3 (with Arduino IDE 1.8.5).
  * 2 different Arduinoboy shields (by Lowtoy and Catskull). Any shield/design should work if it works with the original software.
  * Teensyboy by Catskull (Teensy LC, Arduino IDE 1.8.5 and Teensyduino 1.41).

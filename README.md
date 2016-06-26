# Arduinoboy lite - MIDI out mode
Custom version of the Arduinoboy code. This is an attempt to solve the problem with occasional missing MIDI notes in LSDJ MIDI out mode.
* Only LSDJ MIDI out mode available, no other modes.
* No note-off is sent when notes overlap.
  * Relieving the Arduino from keeping track of the last note and sending note-off greatly reduces note "misses".
  * You have to use another device/software to detect overlapping notes and send note-off.
    * E.g. use a Raspberry Pi with MIDIcloro: https://github.com/ledfyr/midicloro (see *Mono mode*).
* No LED blinking.
* No program changes.
* No configuration via Max patch.
* Upload to your Arduino the usual way.
* Tested with:
  * Gameboy Color CPU-04 and CPU-06.
  * Arduino Uno Rev. 3.
  * LSDJ 4.7.3 Arduinoboy version.
  * 2 different Arduinoboy shields (by Lowtoy and Catskull). Any shield/design should work if it follows the original schematic by trash80.
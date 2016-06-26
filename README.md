# Arduinoboy lite - MIDI out mode
This is a custom version of trash80's Arduinoboy software which I made as an attempt to solve problems with occasional silent or missing MIDI notes in LSDJ MIDI out mode.

And it actually works! I hardly notice any lost/missing notes anymore. This should be useful for anyone wanting to use a Gameboy + LSDJ as their main sequencer.

Some limitations:
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
  * 2 different Arduinoboy shields (by lowtoy and catskull). Any shield/design should work if it follows the original schematic by trash80.

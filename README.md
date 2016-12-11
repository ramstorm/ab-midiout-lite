# Arduinoboy lite - MIDI out mode
This is a custom version of trash80's Arduinoboy software which I made as an attempt to solve problems with dropped out MIDI notes in LSDJ MIDI out mode.

Some notes:
* Only LSDJ MIDI out mode available, no other modes.
* No LED blinking.
* No program changes.
* No configuration via Max patch.
* Extra feature: MIDI CC values sent from LSDJ are scaled to let you use the whole range 0-127 instead of 0-120.
* Upload to your Arduino the usual way.
* Tested with:
  * GBC and GBA.
  * Arduino Uno Rev. 3.
  * LSDJ 4.7.3 Arduinoboy version.
  * 2 different Arduinoboy shields (by lowtoy and catskull). Any shield/design should work if it follows the original schematic by trash80.

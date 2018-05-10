# Gakken SX-150 Mk II - USB MIDI Mod

Welcome!

This project provides a standalone MIDI interface for the Gakken SX-150 synthesizer, based on the Arduino UNO r3. The interface is simple to use: Simply plug the USB connector into your workstation, and connect the remaining two cables to your Gakken to provide CV.

As the interface itself is a fully class compliant USB MIDI device, the unit is fully standalone. No drivers are needed, and there is no client-side software to install or run- just plug and play! This interface has been successfully tested on macOS and iOS.

This project draws heavily from the work of:
  
  - Initial inspiration and SPI code for the MCP4921: [Controlling a Gakken SX-150 synth with Arduino (Mrbook.org)](http://mrbook.org/blog/2008/11/22/controlling-a-gakken-sx-150-synth-with-arduino/)
  - [HIDuino](https://github.com/ddiakopoulos/hiduino), which provides hardware support for USB MIDI on Arduino UNO and Mega 2560
  - The wonderful [Arduino Midi Library](https://github.com/FortySevenEffects/arduino_midi_library)

Also of note is the wonderful [SendMIDI utility](https://github.com/gbevin/SendMIDI), which is very useful for debugging.

A great deal of background information on the SX-150, including schematics, can be found in `Documentation`.
More information, including wiring schematics for the interface, setup/build instructions, and more are forthcoming. The project is functional and very near complete, but remains a work in progress at the moment.

See it in action:
  * [Gakken SX-150 MIDI Mod: Sound Demo](https://www.youtube.com/watch?v=0jBtd83PXs0)
  * [Gakken SX-150 MIDI Mod: Sequencing and Control Demo](https://www.youtube.com/watch?v=DJfP2l-co0g)
  * [Gakken SX-150 MIDI Mod: Pitch Bend Controls! (and effects)](https://www.youtube.com/watch?v=q7sJMQLbhoQ)


TODO/Caveats:
- Tuning is way out of whack! (I'll need to bust out the multimeter and fiddle around)

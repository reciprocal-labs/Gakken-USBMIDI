#!/usr/bin/env bash

if [ "$1" == "factory" ]; then
  echo "Flashing Factory firmware to ATMega16u2..."
  dfu-programmer atmega16u2 erase
  dfu-programmer atmega16u2 flash ./Firmwares/atmega16u2_UNOr3_default_firmware.hex
  echo "Done!"
fi

if [ "$1" == "midi" ]; then
  echo "Flashing MIDI firmware to ATMega16u2..."
  dfu-programmer atmega16u2 erase
  dfu-programmer atmega16u2 flash ./Firmwares/arduino_midi.hex
  echo "Done!"
fi

# midi-to-gpio
MIDI listener to trigger relays from GPIO pins.

## Requirements
- Raspbian
- timidity
- ASLA
- wiringPi

## Compilation
`gcc -lwiringPi -lasound -g -Wall midi-to-gpio.c -o midi-to-gpio.out`

## Pins
Run `gpio readall`

Connect pins as appropriate to your setup.

## Playback
- Run `./midi-to-gpio.out` to start listening
- On a separate terminal, run `aplaymidi -p 14:0 filename.mid`
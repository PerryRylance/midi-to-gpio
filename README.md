# air-horn-piano
MIDI listener to trigger relays from GPIO pins.

## Requirements
- Raspbian
- timidity
- ASLA
- wiringPi

## Compilation
`gcc -lwiringPi -lasound -g -Wall air-horn-piano.c`

## Pins
Run `gpio readall`

Connect pins as appropriate to your setup.

## Playback
- Run `./a.out` to start listening
- On a separate terminal, run `aplaymidi -p 14:0 filename.mid`
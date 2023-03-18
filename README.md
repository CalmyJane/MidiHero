# MidiHero
A GuitarHero-controller modified to output 5-pin midi signal to any synthesizer.


## General
This repository contains the arduino-code and documentation on how to turn a GuitarHero controller (6 button, PS3 generation) into a midi controller. The guitar will send midi notes through a 5-pin midi-out to any standart synthesizer.

## Guitar Components
* note-buttons (6 buttons at the neck)
* up/down trigger
* shift-button
* d-pad (can be pressed down completely to adjust midi channel)
* scale LEDs

## How to Play?
### Play a note
To play a note, hold down one or more of the note-buttons and hit the up/down-trigger. The note will appear for as long as you hold down the note-button, like holding the string of a guitar. You can hit the trigger in both directions to shred faster melodies. 
### Using the Tremolo
You can push on the tremolo, it will by defaul pitchbend down by 1 semitone if you fully push it in. You can change the range of the pitchbend from -12 to 12 semitones to bend all notes up or down up to an octave. To change the range, press shift and move the d-pad left or right. Do not press any other buttons while doing this.

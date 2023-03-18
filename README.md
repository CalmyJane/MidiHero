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

### Change the preset
After start the guitar contains 4 presets:
1. C major scale (C, D, E, F, G, A)
2. E minor pentatonic (E, G, A, Bb, D)
3. C major pentatonic (C, D, E, G, A, C)
4. C minor chord on top three notes, G minor chord on bottom three

The currently selected preset is indicated by one of the 4 LEDs lit up. You can change the preset by pressing the d-pad up/down.

### Using the Tremolo
You can push on the tremolo, it will by defaul pitchbend down by 1 semitone if you fully push it in. You can change the range of the pitchbend from -12 to 12 semitones to bend all notes up or down up to an octave. To change the range, press shift and move the d-pad left or right. You can navigate through the list of modes and a short flash of all LEDs indicates the mode was switched. When no flash appears, you're at the end of the list, you can navigate the other direction.
Tremolo modes are:
*Pitch - Pitches down/up semitones according to tremolo range (can be edited by shift+left/right)
*Aftertouch - Sends aftertouch (velocity) messages to modify the currently played notes. With a synthesizer like the Arturia MicroFreak this parameter can be used to control anything through the modulation matrix (

### Tune the guitar
The notes of every preset can be modified. Simply select a preset you want to modify, hold down notes you want to change pitch, and press Shift + d-pad up/down to pitch the notes by a semitone. So to change any preset to a different scale, simply hold down all notes and press shift + d-pad up/down. Be aware that the presets cannot be stored, whenever you restart the guitar it will be reset to defaults. But this is at least useful to reset if you messed up your presets.

### change midi channel
By default the guitar sends notes on midi channel 1. Easiest is to turn your synthesizer to accept all midi notes, this can be done somewhere in the settings. But if you require a specific midi channel you can change it by pressing down the d-pad completely, and then use the up/down trigger to change the channel. The channel is displayed with flashing LEDs in binary from Channel 1 (1000), Channel  2 (0100), up to Channel 15 (1111), Channel 16 (0000).



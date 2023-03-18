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


🎸 MidiHero

Transform your GuitarHero controller into a rockin' 5-pin MIDI controller for synths, man!
🎶 Overview

This groovy repo provides the Arduino code and docs you need to turn a 6-button, PS3-gen GuitarHero controller into a rad MIDI controller. The modded axe sends MIDI notes through a 5-pin MIDI output, making it jam with any standard synth.
🎸 Guitar Components

    Note buttons (6 buttons on the neck, dude!)
    Up/down trigger
    Shift button
    Directional pad (D-pad)
    Scale indicator LEDs (lights up your fretboard!)

🤘 Usage Instructions
Playing Notes

To shred a note, press and hold one or more note buttons and hit that up/down trigger. The note will sustain as long as you hold the button, just like a real guitar, bro! Strum the trigger in both directions to play some wicked fast licks.
Preset Selection

Your guitar comes preloaded with four sick presets:

    C major scale (C, D, E, F, G, A) - the happy one!
    E minor pentatonic (E, G, A, Bb, D) - unleash your inner rock god!
    C major pentatonic (C, D, E, G, A, C) - jam on this groovy scale!
    C minor chord on top three notes, G minor chord on bottom three - get ready to riff!

The active preset shines bright with one of the four LEDs. Change presets by pressing the D-pad up/down.
Tremolo Functionality

Rock that tremolo arm! By default, it'll bend your pitch down by one semitone at full extension. You can adjust the bend range from -12 to 12 semitones, bending notes up or down by up to an octave! To change the range, press the Shift button while moving the D-pad left/right. Navigate the modes and watch for a flash of all LEDs to know you've made the switch. If no flash, you've hit the end of the list - just go the other way, man!

Tremolo modes include:

    Pitch - Pitch bends down or up by semitones based on tremolo range, adjustable with Shift + left/right.
    Aftertouch - Sends aftertouch (velocity) messages to modify current notes. With a synth like the Arturia MicroFreak, you can control anything through the modulation matrix - let your creativity flow!

Guitar Tuning

Wanna tweak your presets? No problem, dude! Select a preset, then press and hold the notes you want to change. Adjust the pitch with Shift + D-pad up/down for a semitone up or down. Transpose an entire preset by holding all notes and pressing Shift + D-pad up/down. Just remember, custom presets won't be saved when you restart the guitar - they'll go back to default. But hey, it's a great way to reset if things get funky!
MIDI Channel Selection

By default, your guitar sends notes on MIDI channel 1. We recommend setting your synth to accept all MIDI notes - check the settings! But if you need a specific channel, press the D-pad down and use the up/down trigger to change the channel. The active channel is shown in binary with flashing LEDs, from Channel 1 (1000) to Channel 15 (1111), and Channel 16 (0000). Rock on!

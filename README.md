# 🎸 MidiHero
Transform your GuitarHero controller into a rockin' 5-pin MIDI controller for synths, man!
Btw, this readme was written by chatGPT in a "cool and authentic musician-to-musician-style" - enjoy!

## 🎶 Overview
This groovy repo provides the Arduino code and docs you need to turn a 6-button, PS3-gen GuitarHero controller into a rad MIDI controller. The modded axe sends MIDI notes through a 5-pin MIDI output, making it jam with any standard synth.

## 🎸 Guitar Components
![Overview of the Midi Hero Guitar and it's Controls.](pictures/MidiHeroComponents.png)


## 🤘 Usage Instructions
### Playing Notes
To shred a note, press and hold one or more `note` buttons and hit that up/down `trigger`. The note will sustain as long as you hold the `note` button, just like a real guitar, bro! Strum the `trigger` in both directions to play some wicked fast licks.

### Preset Selection
Your guitar comes preloaded with four sick presets:
1. C major scale (C, D, E, F, G, A) - the happy one!
2. E minor pentatonic (E, G, A, Bb, D, E) - unleash your inner rock god!
3. C major pentatonic (C, D, E, G, A, C) - jam on this groovy scale!
4. C minor chord on top three notes, G minor chord on bottom three - get ready to riff!

The active preset shines bright with one of the four `LEDs`. Change presets by pressing the `HeroPower button`.

### Tremolo Functionality
Rock that `tremolo`! By default, it'll bend your pitch down by one semitone at full extension. You can adjust the bend range from 1 to 12 semitones! To change the range, press the `D-pad up/down`. Navigate the modes and watch for a flash of all `LEDs` to know you've made the switch. If no flash, you've hit the end of the range (1 or 12) - just navigate the other way, man!

Tremolo modes include:
* **Pitch Down** - Pitch bends down all notes by 1..12 semitones
* **Pitch Up** - Pitch bends up all notes by 1..12 semitones
* **Aftertouch** - Sends aftertouch (velocity) messages to modify current notes. With a synth like the Arturia MicroFreak, you can control anything through the modulation matrix - let your creativity flow!

### Guitar Tuning
Wanna tweak your presets? No problem, dude! Select a preset, then press and hold the `notes` you want to change. Adjust the pitch with `Shift + trigger up/down` for a semitone up or down. Transpose an entire preset by holding all notes and pressing `Shift + trigger up/down`. Just remember, custom presets won't be saved when you restart the guitar - they'll go back to default. But hey, it's a great way to reset if things get to funky!

### MIDI Channel Selection
By default, your guitar sends notes on MIDI channel 1. We recommend setting your synth to accept all MIDI notes - check the settings! But if you need a specific channel, press the `D-pad` down and use the `up/down trigger` to change the channel. The active channel is shown in binary with flashing LEDs, from Channel 1 (1000) to Channel 15 (1111), and Channel 16 (0000). Rock on!

## Schematics

![Schematic of the arduino and its IOs ](pictures/Schematic.png)
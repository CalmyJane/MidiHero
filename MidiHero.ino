


/*
  MIDI note player

  This sketch shows how to use the serial transmit pin (pin 1) to send MIDI note data.
  If this circuit is connected to a MIDI synth, it will play the notes
  F#-0 (0x1E) to F#-5 (0x5A) in sequence.

  The circuit:
  - digital in 1 connected to MIDI jack pin 5
  - MIDI jack pin 2 connected to ground
  - MIDI jack pin 4 connected to +5V through 220 ohm resistor
  - Attach a MIDI cable to the jack, then to a MIDI synth, and play music.

  created 13 Jun 2006
  modified 13 Aug 2012
  by Tom Igoe

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Midi
*/
const int pinNotes[6] = {9,10,11,6,7,8};
const int pinLeds[4] = {5,4,3,2};

const int pinUp = 12;
const int pinDown = 13;
// const int pinStart = 13;
const int pinTremolo = A7;
const int pinSelect = A0;
const int pinDpadUp = A3;
const int pinDpadDown = A4;
const int pinDpadLeft = A5;
const int pinDpadRight = A6;
const int pinShift = A2;
const int pinDpadAll = A1;

const int presetCount = 4;
//4 presets of 6 notes each. May be edited later by user
int notes[presetCount][6] = {{0x3C, 0x3E, 0x40, 0x41, 0x43, 0x45}, // C4, D4, E4, F4, G4, A4 - c-major
                             {0x40, 0x43, 0x45, 0x46, 0x4A, 0x4C}, // E4, G4, A4, Bb4, D5, E5 - e-minor pentatonik
                             {0x3C, 0x3E, 0x40, 0x43, 0x45, 0x48}, // C4, D4, E4, G4, A4, C5 - c-major pentatonik
                             {0x3C, 0x3F, 0x43, 0x37, 0x3A, 0x3E}};// C3, D#4, G4, G3, Bb3, D4 - c minor and g minor chords
int currPreset = 0;
int lastPreset = 1;

int tremoloRange = 1; //the range of the pitchbend in semitones (-12 to 12, 0 is skipped)
const int tremoloModeCount = 3; //number of different bend modes
int tremoloMode = 0; //counter for current bend mode (0..bendModes)

int ledCounter = 0; //used to blink the leds
int lastMillis = 0; //used to calculate delta t

int midiChannel = 0x00; //stores the midichannel 0-15, 0 = all channels

bool stateNotes[6] = {0, 0, 0, 0, 0, 0};
bool stateUp = 0;
bool stateDown = 0;
// bool stateStart = 0;
int stateTremolo = 0; //amount of tremolo pressed, 1023 when released, 0 when pressed down
bool stateSelect = 0;
bool stateDpadUp = 0;
bool stateDpadDown = 0;
bool stateDpadLeft = 0;
bool stateDpadRight = 0;
bool stateShift = 0;
bool stateDpadAll = 0;

bool lastStateNotes[6] = {0,0,0,0,0,0};
bool lastStateUp = 0;
bool lastStateDown = 0;
// bool lastStateStart = 0;
int lastStateTremolo = 0;
bool lastStateSelect = 0;
bool lastStateDpadUp = 0;
bool lastStateDpadDown = 0;
bool lastStateDpadLeft = 0;
bool lastStateDpadRight = 0;
bool lastStateShift = 0;
bool lastStateDpadAll = 0;


void setup() {
  // Set MIDI baud rate:
  Serial.begin(31250);

  for(int i = 0; i <= 5; i++){
    pinMode(pinNotes[i], INPUT);
  }

  pinMode(pinUp, INPUT);
  pinMode(pinDown, INPUT);
  pinMode(pinTremolo, INPUT);
  pinMode(pinSelect, INPUT);
  pinMode(pinDpadUp, INPUT);
  pinMode(pinDpadDown, INPUT);
  pinMode(pinDpadLeft, INPUT);
  pinMode(pinDpadRight, INPUT);
  pinMode(pinShift, INPUT);
  pinMode(pinDpadAll, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i <= 3; i++){
    pinMode(pinLeds[i], OUTPUT);
  }
}

void loop() {
  // play notes from F#-0 (0x1E) to F#-5 (0x5A):
  // for (int note = 0x1E; note < 0x5A; note++) {
  //   //Note on channel 1 (0x90), some note value (note), middle velocity (0x45):
  //   noteOn(0x90, note, 0x45);
  //   delay(100);
  //   //Note on channel 1 (0x90), some note value (note), silent velocity (0x00):
  //   noteOn(0x90, note, 0x40);
  //   delay(100);
  // }

  for(int i = 0; i <= 5; i++){
    lastStateNotes[i] = stateNotes[i];
    stateNotes[i] = digitalRead(pinNotes[i]);
  }

  lastStateUp = stateUp;
  stateUp = digitalRead(pinUp) > 0;
  lastStateDown = stateDown;
  stateDown = digitalRead(pinDown);
  // lastStateStart = stateStart;
  // stateStart = digitalRead(pinStart) > 0;
  lastStateTremolo = stateTremolo;
  stateTremolo = analogRead(pinTremolo);
  lastStateSelect = stateSelect;
  stateSelect = analogRead(pinSelect) > 100;
  lastStateDpadUp = stateDpadUp;
  stateDpadUp = analogRead(pinDpadUp) > 100;
  lastStateDpadDown = stateDpadDown;
  stateDpadDown = analogRead(pinDpadDown) > 100;
  lastStateDpadLeft = stateDpadLeft;
  stateDpadLeft = analogRead(pinDpadLeft) > 100;
  lastStateDpadRight = stateDpadRight;
  stateDpadRight = analogRead(pinDpadRight) > 100;
  lastStateShift = stateShift;
  stateShift = analogRead(pinShift) > 100;
  lastStateDpadAll = stateDpadAll;
  stateDpadAll = analogRead(pinDpadAll) > 100;

  if(stateSelect && !lastStateSelect){
    ChangePreset(true);
  }

  if(!lastStateUp && stateUp || (!lastStateDown && stateDown)){
    if(stateDpadAll && stateDpadLeft && stateDpadRight && stateDpadUp && stateDpadDown){
      //currently editing Midi Channel
      ChangeMidiChannel(stateUp);
    } else {
      if(stateShift){
        //Tune Notes up/down
        TuneNotes(stateUp);
      } else {
        //Normal strum, play notes
        PlayNotes();            
      }
    }
  }

  //reset notes when note button is released
  for(int i = 0; i <= 5; i++){
    if(!stateNotes[i] && lastStateNotes[i]){
      //Mute every released note
      PlayNote(notes[currPreset][i], false);
      UpdateTremolo();
    }
  }

  //read tremolo change
  if(abs(lastStateTremolo - stateTremolo) > 3){
      UpdateTremolo();
  }

  //read D-Pad LEFT
  if(stateDpadLeft && !lastStateDpadLeft){
    ChangeTremoloMode(true);    
  }

  //read D-Pad RIGHT
  if(stateDpadRight && !lastStateDpadRight){
    ChangeTremoloMode(false);
  }

  //read dpad UP
  if(!lastStateDpadUp && stateDpadUp){
    //dpad up pressed - change preset
    ChangeTremoloRange(true);
  }

  //read D-Pad DOWN
  if(!lastStateDpadDown && stateDpadDown){
    //cross down pressed
    ChangeTremoloRange(false);
  }

  //Read D-pad PRESSED to change Midi Channel
  if(stateDpadAll && stateDpadLeft && stateDpadRight && stateDpadUp && stateDpadDown){
    //cross pressed down completely
    bool bits[8];
    for (int i = 0; i < 4; i++) {
      bits[i] = ((midiChannel + 0x01) >> (i)) & 0x01;
    }
    UpdateStrobe(50, bits);
  }else{
    UpdateLed();    
  }
}

int strobeCounter = 0;
bool strobeState = false;

void PlayNotes(){
  //Plays the notes of the currently pressed note buttons
  for(int i = 0; i <= 5; i++){
    if(stateNotes[i] != 0){
      PlayNote(notes[currPreset][i], true);
      UpdateTremolo();
    }
  }
}

void TuneNotes(bool up){
  //tunes the curently pressed note buttones one semitone up/down
  bool range = false;
  for(int i = 0; i<=5; i++){
    //make sure, that no note exceeds 0x00..0x77
    range = (((notes[currPreset][i] + 1) >= 0x77) && up) || (((notes[currPreset][i] - 1) < 0x00) && !up) || range;
  }
  for(int i = 0; i<=5; i++){
    if(stateNotes[i]){
      //Mute all notes
      PlayNote(notes[currPreset][i], false);
      if(!range){
        notes[currPreset][i] += up?1:-1; //increment or decrement
      }
      //Play tuned notes
      PlayNote(notes[currPreset][i], true);
    }
  }
}

void ChangeMidiChannel(bool up){
  //Increments or Decrements the midiChannel with overroll on 0..16
  midiChannel += up?0x01:-0x01;
  if(midiChannel > 0x0F){
    midiChannel -= 0x10;
  } else if(midiChannel <0x00){
    midiChannel += 0x10;
  }
}

void ChangeTremoloMode(bool up){
  //change tremoloMode
  ResetTremolo();
  tremoloMode += up?1:-1;
  if(tremoloMode < 0){
    tremoloMode += tremoloModeCount;
  } else if(tremoloMode >= tremoloModeCount){
    tremoloMode -= tremoloModeCount;
  }
  BlinkLeds(80);
}

void ChangeTremoloRange(bool up){
  //change tremoloRange
  tremoloRange += up?1:-1;
  if(tremoloRange > 12){
    tremoloRange = 12;
  } else if(tremoloRange <= 0){
    tremoloRange = 1;
  } else {
    BlinkLeds(80);
  }
}

void ChangePreset(bool up){
  //changes the Preset one number up/down with overroll at 0..4
  //reset midi notes
  for(int i = 0; i<=5; i++){
    //mute all notes
    PlayNote(notes[currPreset][i], false);
  }
  //decrement currPreset
  lastPreset = currPreset;
  currPreset += up?1:-1;
  if(currPreset < 0){
    currPreset += 4;
  } else if(currPreset >=4){
    currPreset -= 4;
  }
  //use leds to display currently selected preset
  UpdateLed();
}

void UpdateStrobe(int rate, bool states[4]){
  //call this function on every update to flicker the LEDs
  //states indicates which LEDs are turned off, and which are flickering
  strobeCounter -= 1;
  if(strobeCounter <= 0){
    strobeState = !strobeState;
    strobeCounter = rate;
    for(int i = 0; i < 4; i++){
      if(states[i]){
        digitalWrite(pinLeds[i], strobeState?HIGH:LOW); //flicker active LEDs  
      } else{
        digitalWrite(pinLeds[i], HIGH); //turn off unactive LEDs
      }
    }
  }
}

void BlinkLeds(int time){
  ledCounter = time;
  lastMillis = millis();
  for(int i = 0; i <= 3; i++){
    digitalWrite(pinLeds[i], LOW);
  }
}

void UpdateLed(){
  if(ledCounter > 0){
    //perform blink
    int mils = millis();
    ledCounter -= mils - lastMillis;
    lastMillis = mils;
    if(ledCounter <= 0){
      //counter done
      ledCounter = 0;
      for(int i = 0; i <= 3; i++){
        digitalWrite(pinLeds[i], HIGH);
      }
    }
  } else{
    //display preset number
    if(currPreset != lastPreset || lastStateDpadAll){
      for(int i = 0; i<=3; i++){
        if(i == currPreset){
          digitalWrite(pinLeds[i], LOW);
        } else{
          digitalWrite(pinLeds[i], HIGH);
        }
      }    
    }    
  }
}

void ResetTremolo(){
  stateTremolo = 1023;
  UpdateTremolo();
}

void UpdateTremolo(){
    int tremoloValue = 1023 - stateTremolo; //tremoloValue from 0 to 1023
    if(tremoloValue <= 5){
      tremoloValue = 0;
    }
    switch(tremoloMode){
    case 0:  // Pitch Down
      PitchWheelChange(tremoloRange * (-(tremoloValue * 8)/12));
      break;
    case 1:  //Pitch Up
      PitchWheelChange(tremoloRange * ((tremoloValue * 8)/12));
      break;
    case 2:  // Pressure
      FilterChange(((tremoloRange * tremoloValue/8)/12));
      break;
    }
}

// Value is +/- 8192
void PitchWheelChange(int value) {
    unsigned int change = 0x2000 + value;  //  0x2000 == No Change
    unsigned char low = change & 0x7F;  // Low 7 bits
    unsigned char high = (change >> 7) & 0x7F;  // High 7 bits
    SendMidi(0xE0, low, high);
}

void FilterChange(int value){
  //value in 0..127
  for(int i=0; i<6; i++){
    if(stateNotes[i] != 0){
      //0xA0 = pressure message for aftertouch from 0..127
      SendMidi(0xA0, notes[currPreset][i], value);
    }
  }
}

void PlayNote(int pitch, bool on){
  //send 0x90 command - play note - with 0x00 (note off) or 0x45 (not on) velocity
  SendMidi(0x90, pitch, on?0x45:0x00);
}

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void SendMidi(int cmd, int pitch, int velocity) {
  //Serial.print("Note: " + (String) notes[i] + "\n");
  int firstByte = cmd & 0xF0;
  firstByte |= (midiChannel & 0x0F);
  Serial.write(firstByte);
  Serial.write(pitch);
  Serial.write(velocity);

  //Debug Code to print out 
  // String str = "\nPresets: \n";
  // for(int i = 0; i < presetCount; i++){
  //   for(int j = 0; j < 6; j++){
  //     str += (String)notes[i][j] + " - ";
  //   }
  //   str += "\n";
  // }
  // Serial.print(str);
  
  //Debug Code to print all Input states
  // Serial.print("N1: " + (String)stateNotes[0] + 
  //             " N2: " + (String)stateNotes[1] + 
  //             " N3: " + (String)stateNotes[2] + 
  //             " N4: " + (String)stateNotes[3] + 
  //             " N5: " + (String)stateNotes[4] + 
  //             " N6: " + (String)stateNotes[5] + 
  //             " UP: " + (String)stateUp + 
  //             " DOWN: " + (String)stateDown + 
  //             // " START: " + (String)stateStart + 
  //             " TREMOLO: " + (String)stateTremolo + 
  //             " SELECT: " + (String)stateSelect + 
  //             " CUP: " + (String)stateDpadUp + 
  //             " CDOWN: " + (String)stateDpadDown + 
  //             " CLEFT: " + (String)stateDpadLeft + 
  //             " CRIGHT: " + (String)stateDpadRight + 
  //             " CMID: " + (String)stateDpadAll + 
  //             " CMIDS: " + (String)stateShift +
  //             " currPreset: " + (String)currPreset + "\n"
  //             );
}

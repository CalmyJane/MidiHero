


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
const int pinNotes[6] = {8,9,10,5,6,7};
const int pinLeds[4] = {12,4,3,2};

const int pinUp = 11;
const int pinDown = A7;
// const int pinStart = 13;
const int pinTremolo = A0;
const int pinSelect = A1;
const int pinCrossUp = A2;
const int pinCrossDown = A3;
const int pinCrossLeft = A4;
const int pinCrossRight = A5;
const int pinCrossMiddle = A6;
const int pinCrossMiddleSmall = 13;

const int presetCount = 4;
//4 presets of 6 notes each. May be edited later by user
int notes[presetCount][6] = {{0x3C, 0x3E, 0x40, 0x41, 0x43, 0x45}, // C4, D4, E4, F4, G4, A4 - c-major
                   {0x40, 0x43, 0x45, 0x46, 0x4A, 0x4C}, // E4, G4, A4, Bb4, D5, E5 - e-minor pentatonik
                   {0x3C, 0x3E, 0x40, 0x43, 0x45, 0x48}, // C4, D4, E4, G4, A4, C5 - c-major pentatonik
                   {0x3C, 0x3F, 0x43, 0x37, 0x3A, 0x3E}};// C3, D#4, G4, G3, Bb3, D4 - c minor and g minor chords
int currPreset = 0;
int lastPreset = 1;

int currBendRange = 1; //the range of the pitchbend in semitones (-12 to 12, 0 is skipped)
const int bendModes = 2; //number of different bend modes
int currBendMode = 0; //counter for current bend mode (0..bendModes)

int ledCounter = 0; //used to blink the leds
int lastMillis = 0; //used to calculate delta t

bool starPower = false; //is true when pinSelect is active
bool lastStarPower = false;

int midiChannel = 0x00; //stores the midichannel 0-15, 0 = all channels

bool stateNotes[6] = {0, 0, 0, 0, 0, 0};
bool stateUp = 0;
bool stateDown = 0;
// bool stateStart = 0;
int stateTremolo = 0; //amount of tremolo pressed, 1023 when released, 0 when pressed down
bool stateSelect = 0;
bool stateCrossUp = 0;
bool stateCrossDown = 0;
bool stateCrossLeft = 0;
bool stateCrossRight = 0;
bool stateCrossMiddle = 0;
bool stateCrossMiddleSmall = 0;

bool lastStateNotes[6] = {0,0,0,0,0,0};
bool lastStateUp = 0;
bool lastStateDown = 0;
// bool lastStateStart = 0;
int lastStateTremolo = 0;
bool lastStateSelect = 0;
bool lastStateCrossUp = 0;
bool lastStateCrossDown = 0;
bool lastStateCrossLeft = 0;
bool lastStateCrossRight = 0;
bool lastStateCrossMiddle = 0;
bool lastStateCrossMiddleSmall = 0;


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
  pinMode(pinCrossUp, INPUT);
  pinMode(pinCrossDown, INPUT);
  pinMode(pinCrossLeft, INPUT);
  pinMode(pinCrossRight, INPUT);
  pinMode(pinCrossMiddle, INPUT);
  pinMode(pinCrossMiddleSmall, INPUT);

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
  stateDown = analogRead(pinDown) > 100;
  // lastStateStart = stateStart;
  // stateStart = digitalRead(pinStart) > 0;
  lastStateTremolo = stateTremolo;
  stateTremolo = analogRead(pinTremolo);
  lastStateSelect = stateSelect;
  lastStarPower = starPower;
  stateSelect = analogRead(pinSelect) > 100;
  starPower = stateSelect;
  lastStateCrossUp = stateCrossUp;
  stateCrossUp = analogRead(pinCrossUp) > 100;
  lastStateCrossDown = stateCrossDown;
  stateCrossDown = analogRead(pinCrossDown) > 100;
  lastStateCrossLeft = stateCrossLeft;
  stateCrossLeft = analogRead(pinCrossLeft) > 100;
  lastStateCrossRight = stateCrossRight;
  stateCrossRight = analogRead(pinCrossRight) > 100;
  lastStateCrossMiddle = stateCrossMiddle;
  stateCrossMiddle = analogRead(pinCrossMiddle) > 100;
  lastStateCrossMiddleSmall = stateCrossMiddleSmall;
  stateCrossMiddleSmall = analogRead(pinCrossMiddleSmall) > 100;

  if(starPower && !lastStarPower || (!starPower && lastStarPower)){
    for(int j = 0; j <=5; j++){
      sendMidi(0x90, notes[currPreset][j] + 12, 0x00);
      sendMidi(0x90, notes[currPreset][j], 0x00);
    }    
  }

  if(!lastStateUp && stateUp || (!lastStateDown && stateDown)){
    if(stateCrossMiddleSmall){
      //currently editing Midi Channel
      if(stateUp){
        midiChannel += 0x01;
        if(midiChannel >= 0x0F){
          midiChannel = 0x0F;
        }
      }
      if(stateDown){
        midiChannel -= 0x01;
        if(midiChannel <= 0x00){
          midiChannel = 0x00;
        }
      }
    } else {
      //Rising Edge on Up or Down Button, play the currently pressed notes, or edit midi
      for(int i = 0; i <= 5; i++){
        if(stateNotes[i] != 0){
          //0x90 = Note on, 0x45 = middle velocity
          int noteOffset = 0;
          if(starPower){
            noteOffset = 12;
          }     
          sendMidi(0x90, notes[currPreset][i] + noteOffset, 0x45);
          UpdateTremolo();
        }
      }      
    }

  }

  //reset notes
  for(int i = 0; i <= 5; i++){
    if(!stateNotes[i] && lastStateNotes[i]){
      //0x90 = Note on, 0x00 = silent velocity = note stops playing
      if(starPower){
        sendMidi(0x90, notes[currPreset][i] + 0x0C, 0x00);
      } else{
        sendMidi(0x90, notes[currPreset][i], 0x00);
      }
      UpdateTremolo();
    }

  }

  //read tremolo change
  if(abs(lastStateTremolo - stateTremolo) > 3){
      UpdateTremolo();
  }

  //read navigation bar LEFT
  if(stateCrossLeft && !lastStateCrossLeft){
    currBendMode += 1;
    if(currBendMode >= bendModes){
      currBendMode = bendModes - 1;
    } else {
      BlinkLeds(80);
    }
  }

  //read navigation bar RIGHT
  if(stateCrossRight && !lastStateCrossRight){
    currBendMode -= 1;
    if(currBendMode < 0){
      currBendMode = 0;
    } else {
      BlinkLeds(80);
    }
  }

  //read navigaion bar UP
  if(!lastStateCrossUp && stateCrossUp){
    //cross up pressed - increment pressed notes by one semitone (tuning)
    if(stateCrossMiddle){
      int tuneNote = false; //about to tune note or bend-range?
      for(int i = 0; i<=5; i++){
        if(stateNotes[i]){
          notes[currPreset][i] += 1;
          tuneNote = true;
          sendMidi(0x90, notes[currPreset][i], 0x45);
        }
      }
      if(!tuneNote){
        //not note tuning, check for pitchbend tuning
        currBendRange += 1;
        if(currBendRange > 12){
          currBendRange = 12;
        } else if(currBendRange == 0){
          currBendRange = 1;
          BlinkLeds(50);
        } else {
          BlinkLeds(50);
        }
      }  else {
        BlinkLeds(50); //blink 100ms   
      }
    } else {
      //reset midi notes
      for(int i = 0; i<=5; i++){
        //0x90 = NoteON, 0x00 = silent velocity (note off)
        sendMidi(0x90, notes[currPreset][i], 0x00);
      }
      //increment preset
      lastPreset = currPreset;
      currPreset += 1; //4 presets
      if(currPreset >= presetCount){
        currPreset = presetCount - 1;
      }
    }

  }
  //read Navigation bar DOWN
  if(!lastStateCrossDown && stateCrossDown){
    //cross down pressed - decrement pressed notes by one semitone (tuning)
    if(stateCrossMiddle){
      int tuneNote = false; //about to tune note or bend-range?
      for(int i = 0; i<=5; i++){
        if(stateNotes[i]){
          sendMidi(0x90, notes[currPreset][i], 0x00);
          notes[currPreset][i] -= 1;
          tuneNote = true;
          sendMidi(0x90, notes[currPreset][i], 0x45);
        }
      }
      if(!tuneNote){
        //not note tuning, check for pitchbend tuning
        currBendRange -= 1;
        if(currBendRange == 0){
          //skip the zero
          currBendRange = -1;
          BlinkLeds(50);
        } else if(currBendRange <- 12){
          currBendRange = -12;
        } else{
          BlinkLeds(50);
        }
      } else {
        BlinkLeds(50); //blink 100ms   
      }
    } else{
      //reset midi notes
      for(int i = 0; i<=5; i++){
        //0x90 = NoteON, 0x00 = silent velocity (note off)
        sendMidi(0x90, notes[currPreset][i], 0x00);
      }
      //decrement currPreset
      lastPreset = currPreset;
      currPreset -= 1;
      if(currPreset < 0){
        currPreset = 0;
      }
    }
  }

  //Read Cross PRESSED to change Midi Channel
  if(stateCrossMiddleSmall){
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
    if(currPreset != lastPreset || lastStateCrossMiddleSmall){
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

void UpdateTremolo(){
    int tremoloValue = 1023 - stateTremolo; //tremoloValue from 0 to 1023
    if(tremoloValue <= 5){
      tremoloValue = 0;
    }
    switch(currBendMode){
    case 0:  // Pitch Bend
      PitchWheelChange(currBendRange * (-(tremoloValue * 8)/12));
      break;
    case 1:  // Pressure
      FilterChange((((currBendRange + 12) * tremoloValue/8)/24));
      break;
    }
}

// Value is +/- 8192
void PitchWheelChange(int value) {
    unsigned int change = 0x2000 + value;  //  0x2000 == No Change
    unsigned char low = change & 0x7F;  // Low 7 bits
    unsigned char high = (change >> 7) & 0x7F;  // High 7 bits
    sendMidi(0xE0, low, high);
}

void FilterChange(int value){
  //value in 0..127
  int offset = 0;
  if(starPower){
    offset = 12;
  }
  for(int i=0; i<6; i++){
    if(stateNotes[i] != 0){
      sendMidi(0xA0, notes[currPreset][i] + offset, value);
    }
  }
}

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void sendMidi(int cmd, int pitch, int velocity) {
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
  //             " CUP: " + (String)stateCrossUp + 
  //             " CDOWN: " + (String)stateCrossDown + 
  //             " CLEFT: " + (String)stateCrossLeft + 
  //             " CRIGHT: " + (String)stateCrossRight + 
  //             " CMID: " + (String)stateCrossMiddle + 
  //             " CMIDS: " + (String)stateCrossMiddleSmall +
  //             " currPreset: " + (String)currPreset +
  //             " currBendRange: " + (String)currBendRange + "\n"
  //             );
}

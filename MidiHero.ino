// ---- MIDI HERO FIRMWARE ----

// Project for ardution that reads several buttons, a potentiometer and a PWM-Signal (dutycycle) and converts them into MIDI-Messages
// uses Arduino nano to output MIDI messages on the Tx pin to a standart 5-pin-plug
// pwm dutycycle is read to get slider-value
// poti is read to get tremolo-value
// all other buttons on the guitar hero controller are read as digital inputs

// This code is written by Calmy Jane and based on various examples
//check out the readme for more information


class MidiWriter{
  public:
    MidiWriter(){
      Serial.begin(31250);
    }

    void write(int cmd, int channel, int data1, int data2){
      //writes 1-3 bytes
      int firstByte = cmd & 0xF0;
      if(firstByte != 0xF0){
      //if not system message, include midi channel information
        firstByte |= (channel & 0x0F);        
      }
      // Serial.print("write: ");
      // Serial.print(firstByte, HEX);
      // Serial.print(" - ");
      // Serial.print(data1, HEX);
      // Serial.print(" - ");
      // Serial.print(data2, HEX);
      // Serial.print(" \n");
      // Serial.write(firstByte);
      // Serial.write(data1);
      // Serial.write(data2);
    }

    void noteOnOff(int note, int channel, bool on){
      //0x90 = play note, 0x45 = medium velocity
      write(0x90, channel, note, on?0x45:0x00);
    }

    void pitchBend(int value, int channel) {   
      // Value is +/- 8192
        unsigned int change = 0x2000 + value;  //  0x2000 == No Change
        unsigned char low = change & 0x7F;  // Low 7 bits
        unsigned char high = (change >> 7) & 0x7F;  // High 7 bits
        write(0xE0, channel, low, high);
    }
    
    void monoPressure(int value, int channel){
      //sends pressure message for monophonic aftertouch. there's also a message for polyphonic aftertoucht
      //value in 0..127
      //0xA0 = pressure message for aftertouch from 0..127
      write(0xA0, 0, value, 0);
    }
};

enum NoteMode {
  singleTone = 0,     // Note key plays a single tone
  power = 1,    // Note key plays a power chord, tone, tone + 7 semitones, tone 1 octave
  major = 2,    // Note key plays a major chord, tone, tone + 4 semitones, tone + 7 semitones
  minor = 3     // Note key plays a minor chord, tone, tone + 3 semitones, tone + 7 semitones
};

class Note{
  private:
    int root = 0x3C;
    NoteMode mode = singleTone;
    bool playing = false;
    MidiWriter midi;
    int midiChannel = 0;
  public:
    Note(){
      //default constructor
    }

    Note(MidiWriter writer, int value, int noteMode, int channel){
      set(writer, value, noteMode, channel);
    }

    void set(MidiWriter writer, int value, int noteMode, int channel){
      //set current note value 0xC3 = C4
      play(false);
      midiChannel = channel;
      midi = writer;
      root = value;
      mode = noteMode;
    }

    void setChannel(int channel){
      play(false);
      midiChannel = channel;
    }

    void setMode(NoteMode value){
      mode = value;
    }

    void play(bool on){
      //send 0x90 command - play note - with 0x00 (note off) or 0x45 (not on) velocity
      if(playing || (!playing && on)){
        switch(mode){
          case singleTone:
            //single note
            midi.noteOnOff(root, midiChannel, on);
            break;
          case power:
            //power chord
            midi.noteOnOff(root + 7, midiChannel, on);
            midi.noteOnOff(root + 12, midiChannel, on);
            midi.noteOnOff(root, midiChannel, on);
            break;
          case major:
            //major chord
            midi.noteOnOff(root + 4, midiChannel, on);
            midi.noteOnOff(root + 7, midiChannel, on);
            midi.noteOnOff(root, midiChannel, on);
            break;
          case minor:
            //minor chord
            midi.noteOnOff(root + 3, midiChannel, on);
            midi.noteOnOff(root + 7, midiChannel, on);
            midi.noteOnOff(root, midiChannel, on);
            break;
        }
        //do not send off when note is not playing, avoid "silent" messages
      }
      playing = on; //store current state
    }

    void tune(bool up){
      // tune 1 semitone up/down
      root += up?1:-1;
    }

    int getRoot(){
      return root;
    }

    int changeMode(bool up){
      play(false);
      mode = (mode + (up?1:-1));
      if(mode >= 4){
        mode = 0;
      } else if (mode < 0){
        mode = 3;
      }
    }
};

class RollingAverage {
  public:
    RollingAverage(uint32_t n) {
      bufferSize = n;
      buffer = new uint32_t[bufferSize];
      bufferIndex = 0;
      bufferSum = 0;
      bufferCount = 0;
      // Initialize the buffer with zeros
      for (uint32_t i = 0; i < bufferSize; i++) {
        buffer[i] = 0;
      }
    }

    ~RollingAverage() {
      delete[] buffer;
    }

    void add(uint32_t amount) {
      bufferSum += amount;
      bufferSum -= buffer[bufferIndex];
      buffer[bufferIndex] = amount;
  
      bufferIndex = ((bufferIndex + 1) % bufferSize + bufferSize) % bufferSize; //positive modulo

      if (bufferCount < bufferSize) {
        bufferCount++;
      }
    }

    uint32_t get() {
      if (bufferCount == 0) {
        return 0;
      }
      return bufferSum / bufferCount;
    }

  private:
    uint32_t bufferSize;
    uint32_t *buffer;
    uint32_t bufferIndex;
    uint32_t bufferSum;
    uint32_t bufferCount;
};

const int pinNotes[5] = {6,7,8,9,10};
const int pinLeds[4] = {2,5,4,3};

const int pinUp = 12;
const int pinDown = 11;
const int pinTremolo = A7;
const int pinSelect = A0;
const int pinDpadUp = A1;
const int pinDpadDown = A2;
const int pinDpadLeft = A3;
const int pinDpadRight = A4;
const int pinShift = 13;
const int pinDpadAll = A5;
const int pinSlide = A6;

const int presetCount = 4;
//4 presets of 6 notes each. May be edited later by user
String presets[presetCount][5] = {{"C4", "F4", "G4", "A4", "C5"}, // C F G A for chords
                             {"C4", "D#4", "F4", "G4", "A#4"}, //c-minor pentatonik
                             {"C4", "D4", "E4", "G4", "A4"}, //c-major pentatonik
                             {"A3", "B3", "D4", "E4", "A5"}};// 4 Chords for Blitskrieg bob (A3-A3-D4-E4, Chorus D4-D4-A3-D4-A3 D4-D4-B3-D4-A3)
int presetModes[presetCount][5] = {{0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {1,1,1,1,0}, //chord-preset containing some chords
};

int getHexNote(String strNote){
  //turns a string note ("C4", "G#4") to a hex note (0x3C, 44)
  // 0x00 = C1
  bool sharp = strNote.length() == 3;
  String letter = "";
  int number = 0;
  if(sharp){
    letter = strNote.substring(0,2);
    number = 12 * strNote.substring(2,3).toInt();
  } else{
    letter = strNote.substring(0,1);
    number = 12 * strNote.substring(1,2).toInt();
  }
  int noteNumber = 0;
  if(letter == "C#"){noteNumber = 1;}
  else if(letter == "D"){noteNumber = 2;}
  else if(letter == "D#"){noteNumber = 3;}
  else if(letter == "E"){noteNumber = 4;}
  else if(letter == "F"){noteNumber = 5;}
  else if(letter == "F#"){noteNumber = 6;}
  else if(letter == "G"){noteNumber = 7;}
  else if(letter == "G#"){noteNumber = 8;}
  else if(letter == "A"){noteNumber = 9;}
  else if(letter == "A#"){noteNumber = 10;}
  else if(letter == "B"){noteNumber = 11;}
  noteNumber += number;
  return noteNumber;
}

int currPreset = 0;
int lastPreset = 1;

int tremoloRange = 1; //the range of the pitchbend in semitones (-12 to 12, 0 is skipped)
const int tremoloModeCount = 3; //number of different bend modes
int tremoloMode = 0; //counter for current bend mode (0..bendModes)

int ledCounter = 0; //used to blink the leds
int lastMillis = 0; //used to calculate delta t

int midiChannel = 0x00; //stores the midichannel 0-15, 0 = all channels

RollingAverage slideVal(50);

bool stateNotes[5] = {0, 0, 0, 0, 0};
bool stateUp = 0;
bool stateDown = 0;
int stateTremolo = 0; //amount of tremolo pressed, 1023 when released, 0 when pressed down
bool stateSelect = 0;
bool stateDpadUp = 0;
bool stateDpadDown = 0;
bool stateDpadLeft = 0;
bool stateDpadRight = 0;
bool stateShift = 0;
bool stateDpadAll = 0;
uint32_t stateSlide = 0;

bool lastStateNotes[5] = {0,0,0,0,0};
bool lastStateUp = 0;
bool lastStateDown = 0;
int lastStateTremolo = 0;
bool lastStateSelect = 0;
bool lastStateDpadUp = 0;
bool lastStateDpadDown = 0;
bool lastStateDpadLeft = 0;
bool lastStateDpadRight = 0;
bool lastStateShift = 0;
bool lastStateDpadAll = 0;
int lastStateSlide = 0;

MidiWriter midi;
Note notes[5];

void setup() {
  // Set MIDI baud rate:
  midi = MidiWriter();

  for(int i = 0; i < 5; i++){
    pinMode(pinNotes[i], INPUT);
    notes[i] = Note(midi, getHexNote(presets[0][i]), presetModes[0][i], midiChannel);
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
  pinMode(pinSlide, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i <= 3; i++){
    pinMode(pinLeds[i], OUTPUT);
  }
}

void loop() {
  for(int i = 0; i <= 4; i++){
    lastStateNotes[i] = stateNotes[i];
    stateNotes[i] = digitalRead(pinNotes[i]);
  }

  lastStateUp = stateUp;
  stateUp = digitalRead(pinUp);
  lastStateDown = stateDown;
  stateDown = digitalRead(pinDown);
  lastStateTremolo = stateTremolo;
  stateTremolo = analogRead(pinTremolo);
  lastStateSelect = stateSelect;
  stateSelect = digitalRead(pinSelect);
  lastStateDpadUp = stateDpadUp;
  stateDpadUp = digitalRead(pinDpadUp);
  lastStateDpadDown = stateDpadDown;
  stateDpadDown = digitalRead(pinDpadDown);
  lastStateDpadLeft = stateDpadLeft;
  stateDpadLeft = digitalRead(pinDpadLeft);
  lastStateDpadRight = stateDpadRight;
  stateDpadRight = digitalRead(pinDpadRight);
  lastStateShift = stateShift;
  stateShift = digitalRead(pinShift);
  lastStateDpadAll = stateDpadAll;
  stateDpadAll = digitalRead(pinDpadAll);
  lastStateSlide = stateSlide;
  slideVal.add(analogRead(pinSlide));
  stateSlide = slideVal.get();
  // TODO properly read slide bar
  int tolerance = 50;
  int slideNote = 0;
  if(abs(stateSlide - 140) <= tolerance){
    slideNote = 1;
  } else if(abs(stateSlide - 350) <= tolerance){
    slideNote = 2;
  } else if(abs(stateSlide - 600) <= tolerance){
    slideNote = 3;
  } else if(abs(stateSlide - 770) <= tolerance){
    slideNote = 4;
  } else if(abs(stateSlide - 1000) <= tolerance){
    slideNote = 5;
  } else if(abs(stateSlide - 500) <= tolerance){
    slideNote = 6;
  }  
  Serial.print("SlideVal: ");
  Serial.print(slideNote);
  Serial.print("\n");

  if(stateSelect && !lastStateSelect){
    // hero power button pressed
    if(stateShift){
      // tuning/ shift pressed - change mode (single note, major chord, minor chord ..) for all active notes
      for (int i=0; i<5; i++){
        if (stateNotes[i]){
          notes[i].changeMode(true);
          notes[i].play(true);
        }
      }
    } else {
      // switch preset
      ChangePreset(true);
    }
  }

  if(!lastStateUp && stateUp || (!lastStateDown && stateDown)){
    if(stateDpadAll){
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
  for(int i = 0; i <= 4; i++){
    if(!stateNotes[i] && lastStateNotes[i]){
      //Mute every released note
      notes[i].play(false);
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

  //Read D-pad middle to change Midi Channel
  if(stateDpadAll){
    //cross pressed down completely
    bool bits[8];
    for (int i = 0; i < 4; i++) {
      bits[i] = ((midiChannel + 0x01) >> (i)) & 0x01;
    }
    UpdateStrobe(50, bits);
  }else{
    UpdateLed();    
  }
    
  // // Debug Code to print all Input states
  // Serial.print("N1: " + (String)stateNotes[0] + 
  //             " N2: " + (String)stateNotes[1] + 
  //             " N3: " + (String)stateNotes[2] + 
  //             " N4: " + (String)stateNotes[3] + 
  //             " N5: " + (String)stateNotes[4] + 
  //             " UP: " + (String)stateUp + 
  //             " DOWN: " + (String)stateDown + 
  //             // " START: " + (String)stateStart + 
  //             " SELECT: " + (String)stateSelect + 
  //             " CUP: " + (String)stateDpadUp + 
  //             " CDOWN: " + (String)stateDpadDown + 
  //             " CLEFT: " + (String)stateDpadLeft + 
  //             " CRIGHT: " + (String)stateDpadRight + 
  //             " CMID: " + (String)stateDpadAll + 
  //             " Shift: " + (String)stateShift +
  //             " TREMOLO: " + (String)stateTremolo + 
  //             " Slide: " + (String)stateSlide + "\n"
  //             );
}

int strobeCounter = 0;
bool strobeState = false;

void PlayNotes(){
  //Plays the notes of the currently pressed note buttons
  for(int i = 0; i < 5; i++){
    if(stateNotes[i] != 0){
      notes[i].play(true);
      UpdateTremolo();
    }
  }
}

void TuneNotes(bool up){
  //tunes the curently pressed note buttones one semitone up/down
  bool range = false;
  for(int i = 0; i<5; i++){
    //make sure, that no note exceeds 0x00..0x77
    range = (((notes[i].getRoot() + 1) >= 0x77) && up) || (((notes[i].getRoot() - 1) < 0x00) && !up) || range;
  }
  for(int i = 0; i<5; i++){
    if(stateNotes[i]){
      //Mute all notes
      notes[i].play(false);
      if(!range){
        notes[i].tune(up); //increment or decrement
      }
      //Play tuned notes
      notes[i].play(true);
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
  for(int i = 0; i < 5; i++){
    notes[i].setChannel(midiChannel);
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
  UpdateTremolo();
}

void ChangePreset(bool up){
  //changes the Preset one number up/down with overroll at 0..4
  //reset midi notes
  for(int i = 0; i < 5; i++){
    //mute all notes
    notes[i].play(false);
  }
  //decrement currPreset
  lastPreset = currPreset;
  currPreset += up?1:-1;
  if(currPreset < 0){
    currPreset += 4;
  } else if(currPreset >=4){
    currPreset -= 4;
  }
  for(int i = 0; i < 5; i++){
    notes[i].set(midi, getHexNote(presets[currPreset][i]), presetModes[currPreset][i], midiChannel);
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
      midi.pitchBend(tremoloRange * (-(tremoloValue * 8)/12), midiChannel);
      break;
    case 1:  //Pitch Up
      midi.pitchBend(tremoloRange * ((tremoloValue * 8)/12), midiChannel);
      break;
    case 2:  // Pressure
      midi.monoPressure(((tremoloRange * tremoloValue/8)/12), midiChannel);
      break;
    }
}
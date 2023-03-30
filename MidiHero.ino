// ---- MIDI HERO FIRMWARE ----

// Project for ardution that reads several buttons, a potentiometer and a PWM-Signal (dutycycle) and converts them into MIDI-Messages
// uses Arduino nano to output MIDI messages on the Tx pin to a standart 5-pin-plug
// pwm dutycycle is read to get slider-value
// poti is read to get tremolo-value
// all other buttons on the guitar hero controller are read as digital inputs

// This code is written by Calmy Jane and based on various examples
//check out the readme for more information

#include <EEPROM.h>

class MidiWriter{
  public:
    int channel = 0;
    MidiWriter(){
      Serial.begin(31250);
    }

    void write(int cmd, int data1, int data2){
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
      Serial.write(firstByte);
      Serial.write(data1);
      Serial.write(data2);
    }

    void noteOnOff(int note, bool on){
      //0x90 = play note, 0x45 = medium velocity
      write(0x90, note, on?0x45:0x00);
    }

    void pitchBend(int value) {   
      // Value is +/- 8192
        unsigned int change = 0x2000 + value;  //  0x2000 == No Change
        unsigned char low = change & 0x7F;  // Low 7 bits
        unsigned char high = (change >> 7) & 0x7F;  // High 7 bits
        write(0xE0, low, high);
    }
    
    void monoPressure(int noteValue, int value){
      //sends pressure message for monophonic aftertouch. there's also a message for polyphonic aftertoucht
      //value in 0..127
      //0xA0 = pressure message for aftertouch from 0..127
      // Serial.print("\n " + (String)value + " \n");
      write(0xA0, noteValue, value);
    }
    void changeMidiChannel(bool up){
      //Increments or Decrements the midiChannel with overroll on 0..16
      channel += up?0x01:-0x01;
      if(channel > 0x0F){
        channel -= 0x10;
      } else if(channel <0x00){
        channel += 0x10;
      }
    }
};

class Note{
  private:
    int root = 0x3C;
    int mode = 0;
    // modes:
    // singleTone = 0,     // Note key plays a single tone
    // power = 1,    // Note key plays a power chord, tone, tone + 7 semitones, tone 1 octave
    // major = 2,    // Note key plays a major chord, tone, tone + 4 semitones, tone + 7 semitones
    // minor = 3     // Note key plays a minor chord, tone, tone + 3 semitones, tone + 7 semitones

  public:
    bool playing = false;
    Note(){
      //default constructor
    }

    Note(MidiWriter midi, int value, int noteMode){
      set(midi, value, noteMode);
    }

    void set(MidiWriter midi, int value, int noteMode){
      //set current note value 0xC3 = C4
      play(midi, false);
      root = value;
      mode = noteMode;
    }

    void setMode(int value){
      mode = value;
    }

    void play(MidiWriter midi, bool on){
      //send 0x90 command - play note - with 0x00 (note off) or 0x45 (not on) velocity
      if(playing || (!playing && on)){
        switch(mode){
          case 0:
            //single note
            midi.noteOnOff(root, on);
            break;
          case 1:
            //power chord
            midi.noteOnOff(root + 7, on);
            midi.noteOnOff(root + 12, on);
            midi.noteOnOff(root, on);
            break;
          case 2:
            //major chord
            midi.noteOnOff(root + 4, on);
            midi.noteOnOff(root + 7, on);
            midi.noteOnOff(root, on);
            break;
          case 3:
            //minor chord
            midi.noteOnOff(root + 3, on);
            midi.noteOnOff(root + 7, on);
            midi.noteOnOff(root, on);
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

    int changeMode(MidiWriter midi, bool up){
      play(midi, false);
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

class Button{
  public:
    bool state = false;
    bool lastState = false;
    bool pressed = false;
    bool released = false;
    int pin = 0;

    Button(){
      
    }

    Button(int pinNumber){
      //Initialize button input
      pin = pinNumber;
      pinMode(pinNumber, INPUT);
    }

    void update(){
      //call periodically
      lastState = state;
      state = digitalRead(pin);
      pressed = !lastState && state; //was pressed in this iteration
      released = lastState && !state; //was released in this iteration
    }
};

class LedBar{
  //handles the 4 leds, blinks them, sets brightness through pwm
  private:
    int* pins;
    int blinkCounter = 0;
    int numericValue = 0; 
    int lastMillis = 0;
    int brightness = 0;
    int brightCounter = 0;
    int darkCounter = 0;
    int16_t periods[3] = {100, 10, 100};
    int16_t dutys[3] = {100, 1, 10};


    int pwm_period = 100; //10ms period-time = 100Hz
    int pwm_duty = 100;  //10ms dutycycle = 0,5% dutycycle
    int pwm_count = 0; //ms since period begin
  public:
    bool inverted;
    LedBar(){
      //empty default constructor
    }

    LedBar(int ledPins[4]){
      pins = ledPins;
      for(int i=0; i<4; i++){
        pinMode(ledPins[i], OUTPUT);
      }
      setBrightness(0);
    }

    void update(){
      int16_t mlls = millis();
      int16_t deltat = mlls - lastMillis;
      lastMillis = mlls;
      //call on every cycle
      if(blinkCounter > 0){
        blinkCounter -= deltat;        
      } else {
        ShowBinary(numericValue);
      }
      pwm_period = periods[brightness];
      pwm_duty = dutys[brightness];
      pwm_count += deltat;
      if(pwm_count < pwm_duty){
        ShowBinary(numericValue);
      } else if(pwm_count < pwm_period){
        setAll(false);
      } else {
        //period time has passed
        pwm_count = 0;
      }
    }

    void ShowBinary(int value){
      numericValue = value;
      bool bits[8];
      for (int i = 0; i < 4; i++) {
        bits[i] = ((value) >> (i)) & 0x01;
      }
      setLeds(bits);
    }

    void setLeds(bool values[4]){
      for(int i=0; i<4; i++){
        setLed(i, values[i]);
      }
    }

    void setLed(int led, bool value){
      // digitalWrite(pins[led], !(!value != inverted));
      digitalWrite(pins[led], !value != inverted);
    }

    void setAll(bool on){
      bool lds[4] = {on,on,on,on};
      setLeds(lds);
    }

    void setBrightness(int value){
      //set Brightness from 0 to 4
      brightness = value;
    }

    void blink(int duration){
      //blinks duration in ms all LEDs
      blinkCounter = duration;
      setAll(true);
    }

    void setValue(int value){
      //set the value to be displayed
      numericValue = value;
    }
};

class Preset{
  public:
    Note notes[5];
    Preset(){
      //empty default constructor
    }
    Preset(MidiWriter midi, int noteValues[5], int noteModes[5], int tremoloMode){
      for(int i=0; i<5; i++){
        notes[i] = Note(midi, noteValues[i], noteModes[i]);
      }
    }
};

class Config{
  //contains data and stores/retrieves to/from EEPROM
  private:
    int midiChannelAdd = 0;
    int firstPreset = 0;
  
  public:
    Config(){
    };
    
    int8_t readMidiChannel(){
      int8_t val = 0;
      EEPROM.get(midiChannelAdd, val);
      return val;
    }

    void writeMidiChannel(int8_t channel){
      EEPROM.put(midiChannelAdd, channel);
    }

    void readPreset(int number){

    }
};

const int pinNotes[5] = {6,7,8,9,10};
int pinLeds[4] = {2,5,4,3};

const int pinUp = 12;
const int pinDown = 11;
const int pinTremolo = A7;
const int pinSelect = A0;
const int pinDpadUp = A1;
const int pinDpadDown = A2;
const int pinDpadLeft = A3;
const int pinDpadRight = A4;
const int pinStart = 13;
const int pinDpadMid = A5;
const int pinSlide = A6;

Button btn_up(pinUp);
Button btn_down(pinDown);
Button btn_select(pinSelect);
Button btn_start(pinStart);
Button btn_dup(pinDpadUp);
Button btn_ddown(pinDpadDown);
Button btn_dleft(pinDpadLeft);
Button btn_dright(pinDpadRight);
Button btn_dmid(pinDpadMid);
Button btn_notes[5];

LedBar leds(pinLeds);

enum NoteNames{
  _C0, _Cs0, _D0, _Ds0, _E0, _F0, _Fs0, _G0, _Gs0, _A0, _As0, _B0,
  _C1, _Cs1, _D1, _Ds1, _E1, _F1, _Fs1, _G1, _Gs1, _A1, _As1, _B1,
  _C2, _Cs2, _D2, _Ds2, _E2, _F2, _Fs2, _G2, _Gs2, _A2, _As2, _B2,
  _C3, _Cs3, _D3, _Ds3, _E3, _F3, _Fs3, _G3, _Gs3, _A3, _As3, _B3,
  _C4, _Cs4, _D4, _Ds4, _E4, _F4, _Fs4, _G4, _Gs4, _A4, _As4, _B4,
  _C5, _Cs5, _D5, _Ds5, _E5, _F5, _Fs5, _G5, _Gs5, _A5, _As5, _B5,
  _C6, _Cs6, _D6, _Ds6, _E6, _F6, _Fs6, _G6, _Gs6, _A6, _As6, _B6
};

const int presetCount = 16;
//4 presets of 6 notes each. May be edited later by user
NoteNames presets[presetCount][5] = {{_C4, _F4, _G4, _A4, _C5}, // C F G A for chords
                             {_C4, _Ds4, _F4, _G4, _As4}, //c-minor pentatonik
                             {_C4, _D4, _E4, _G4, _A4}, //c-major pentatonik
                             {_A3, _B3, _D4, _E4, _A5},  // 4 Chords for Blitskrieg bob (A3-A3-D4-E4, Chorus D4-D4-A3-D4-A3 D4-D4-B3-D4-A3)
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
                             {_C4, _D4, _E4, _F4, _G4}, 
};


int presetModes[presetCount][5] = {{0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {1,1,1,1,0}, //chord-preset containing some chords
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0},
                                   {0,0,0,0,0}
};

int currPreset = 0;
int lastPreset = 1;

int tremoloRange = 1; //the range of the pitchbend in semitones (-12 to 12, 0 is skipped)
const int tremoloModeCount = 3; //number of different bend modes
int tremoloMode = 0; //counter for current bend mode (0..bendModes)

int ledCounter = 0; //used to blink the leds
int lastMillis = 0; //used to calculate delta t

// RollingAverage slideVal(5);

int stateTremolo = 0; //amount of tremolo pressed, 1023 when released, 0 when pressed down
uint32_t stateSlide = 0;

int lastStateTremolo = 0;
int lastStateSlide = 0;

bool resetEEPROM = true;

MidiWriter midi;
Note notes[5];
Preset preset;

// void readEEPROM(){
//   ConfigData cfg;
//   EEPROM.get(0, cfg);
//   midiChannel = cfg.midiChannel;
//   for(int i=0; i<16; i++){
//     for(int j=0; j<5; j++) {
//       presetList[i][j] = Note(midi, cfg.presets[i][j], cfg.presetModes[i][j], midiChannel);
//     }
//   }
// }

void setup() {
  if(resetEEPROM){

  }

  // Init midi
  midi = MidiWriter();
  //Init LEDs
  leds.setValue(currPreset);
  leds.setBrightness(0);
  //Init Note Buttons
  for(int i = 0; i < 5; i++){
    btn_notes[i] = Button(pinNotes[i]);
    notes[i] = Note(midi, presets[0][i], presetModes[0][i]);
  }
  //Other buttons are Initialized on declaration
}

void loop() {
      
  lastStateTremolo = stateTremolo;
  stateTremolo = analogRead(pinTremolo);

  //read buttons and update current states
  btn_up.update();
  btn_down.update();
  btn_select.update();
  btn_start.update();
  btn_dup.update();
  btn_ddown.update();
  btn_dleft.update();
  btn_dright.update();
  btn_dmid.update();

  for(int i = 0; i<5; i++){
    btn_notes[i].update(); 
  }

  //read slide bar value
  readSlideBar();

  if(btn_select.pressed){
    // hero power button pressed
    if(btn_start.state){
      // tuning/ shift pressed - change mode (single note, major chord, minor chord ..) for all active notes
      for (int i=0; i<5; i++){
        if (btn_notes[i].state){
          notes[i].changeMode(midi, true);
          notes[i].play(midi, true);
        }
      }
    } else {
      // switch preset
      ChangePreset(true);
    }
  }

  if(btn_up.pressed || btn_down.pressed){
    if(btn_dmid.state){
      //currently editing Midi Channel
      midi.changeMidiChannel(btn_up.state);
      leds.setValue(midi.channel);
    } else {
      if(btn_start.state){
        //Tune Notes up/down
        TuneNotes(btn_up.state);
      } else {
        //Normal strum, play notes
        PlayNotes();            
      }
    }
  }

  //check for Hammeron
  for(int i=0; i<5; i++){
    if(btn_notes[i].pressed){
      bool hammeron = false;
      for(int j=0; j<i; j++){
        hammeron = hammeron || notes[j].playing;
      }
      for(int j=0; j<5-i; j++){
        hammeron = hammeron && !notes[4-j].playing;
      }
      if(hammeron){
        for(int j=0; j<5; j++){
          notes[j].play(midi, false);
        }
        notes[i].play(midi, true);
      }
    }
  }

  //reset notes when note button is released
  for(int i = 0; i < 5; i++){
    if(btn_notes[i].released){
      //Mute every released note
      int pulloff = -1;
      for(int j = 0; j<i; j++){
        if(btn_notes[j].state && !notes[j].playing && notes[i].playing){
          pulloff = j;
        }
      }
      if(pulloff >= 0){
        for(int j = 0; j<5; j++){
          notes[j].play(midi, j==pulloff);
        }
      }
      notes[i].play(midi, false);

      UpdateTremolo();
    }
  }

  //read tremolo change
  if(abs(lastStateTremolo - stateTremolo) > 3){
      UpdateTremolo();
  }

  //read D-Pad LEFT
  if(btn_dleft.pressed){
    ChangeTremoloMode(true);    
  }

  //read D-Pad RIGHT
  if(btn_dright.pressed){
    ChangeTremoloMode(false);
  }

  //read dpad UP
  if(btn_dup.pressed){
    //dpad up pressed - change preset
    ChangeTremoloRange(true);
  }

  //read D-Pad DOWN
  if(btn_ddown.pressed){
    //cross down pressed
    ChangeTremoloRange(false);
  }

  leds.setValue(currPreset);
  leds.setBrightness(0);
  if(btn_dmid.state){

    if(btn_start.state && anyNote()){
      leds.setBrightness(2);
      leds.setValue(15);
    } else {
      leds.setBrightness(1);
      leds.setValue(midi.channel);
    }
  }

  // if(btn_start.released){
  //   if(btn_dmid.state && anyNote()){
  //     if(anyNote()){
  //       int adr = 0;
  //       bool btns[4] = {btn_notes[0].state, btn_notes[1].state, btn_notes[2].state, btn_notes[3].state};
  //       for (int i = 0; i < 4; ++i) {
  //         adr |= (btns[i] << (3 - i));
  //       }
  //       writePreset(adr, notes);        
  //     } else {
  //       writeSystem();
  //     }
  //   }
  // }

  leds.update();

  // Debug Code to print all Input states
  // Serial.print("N1: " + (String)btn_notes[0].state + 
  //             " N2: " + (String)btn_notes[1].state + 
  //             " N3: " + (String)btn_notes[2].state + 
  //             " N4: " + (String)btn_notes[3].state + 
  //             " N5: " + (String)btn_notes[4].state + 
  //             " UP: " + (String)btn_up.state + 
  //             " DOWN: " + (String)btn_down.state + 
  //             " START: " + (String)btn_start.state + 
  //             " SELECT: " + (String)btn_select.state + 
  //             " DUP: " + (String)btn_dup.state + 
  //             " DDOWN: " + (String)btn_ddown.state + 
  //             " DLEFT: " + (String)btn_dleft.state + 
  //             " DRIGHT: " + (String)btn_dright.state + 
  //             " DMID: " + (String)btn_dmid.state + 
  //             " TREMOLO: " + (String)stateTremolo + 
  //             " Slide: " + (String)stateSlide + "\n"
  //             );



}

int strobeCounter = 0;
bool strobeState = false;

void readSlideBar(){
  //read slide-bar TOBEDONE
  // lastStateSlide = stateSlide;
  // slideVal.add(analogRead(pinSlide));
  // stateSlide = slideVal.get();
  // int tolerance = 50;
  // int slideNote = 0;
  // if(abs(stateSlide - 140) <= tolerance){
  //   slideNote = 1;
  // } else if(abs(stateSlide - 350) <= tolerance){
  //   slideNote = 2;
  // } else if(abs(stateSlide - 600) <= tolerance){
  //   slideNote = 3;
  // } else if(abs(stateSlide - 770) <= tolerance){
  //   slideNote = 4;
  // } else if(abs(stateSlide - 1000) <= tolerance){
  //   slideNote = 5;
  // } else if(abs(stateSlide - 500) <= tolerance){
  //   slideNote = 6;
  // }  
  // Serial.print("SlideVal: ");
  // Serial.print(slideNote);
  // Serial.print("\n");
}

// void writePreset(int adress, Note notes[4]){
//   //writes a preset to EEPROM
//   ConfigData cfg;
//   EEPROM.get(0, cfg);
//   for(int i=0; i<5; i++){
//     cfg.presets[adress][i] = notes[i].getRoot();
//     cfg.presetModes[adress][i] = notes[i].getRoot();
//   }
//   EEPROM.put(0, cfg);
// }

// void writeSystem(){
//   //writes current midichannel and start preset to memory
//   ConfigData cfg;
//   EEPROM.get(0, cfg);
//   cfg.midiChannel = midiChannel;
//   EEPROM.put(0, cfg);
// }

bool anyNote(){
  //returns if any note button is pressed
  bool any = false;
  for(int i=0; i<5; i++){
    if(btn_notes[i].state){
      any = true;
    };
  }
  return any;
}

void PlayNotes(){
  //Plays the notes of the currently pressed note buttons
  for(int i = 0; i < 5; i++){
    if(btn_notes[i].state){
      notes[i].play(midi, true);
      UpdateTremolo();
    }
  }
}

void TuneNotes(bool up){
  //tunes the curently pressed note buttones one semitone up/down
  bool range = false;
  for(int i = 0; i < 5; i++){
    //make sure, that no note exceeds 0x00..0x77
    range = (((notes[i].getRoot() + 1) >= 0x77) && up) || (((notes[i].getRoot() - 1) < 0x00) && !up) || range;
  }
  for(int i = 0; i < 5; i++){
    if(btn_notes[i].state){
      //Mute all notes
      notes[i].play(midi, false);
      if(!range){
        notes[i].tune(up); //increment or decrement
      }
      //Play tuned notes
      notes[i].play(midi, true);
    }
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
  leds.blink(80);
}

void ChangeTremoloRange(bool up){
  //change tremoloRange
  tremoloRange += up?1:-1;
  if(tremoloRange > 12){
    tremoloRange = 12;
  } else if(tremoloRange <= 0){
    tremoloRange = 1;
  } else {
    leds.blink(80);
  }
  UpdateTremolo();
}

void ChangePreset(bool up){
  //changes the Preset one number up/down with overroll at 0..4
  //reset midi notes
  for(int i = 0; i < 5; i++){
    //mute all notes
    notes[i].play(midi, false);
  }
  //decrement currPreset
  lastPreset = currPreset;
  currPreset += up?1:-1;
  if(currPreset < 0){
    currPreset += 4;
  } else if(currPreset >=presetCount){
    currPreset -= presetCount;
  }
  for(int i = 0; i < 5; i++){
    notes[i].set(midi, presets[currPreset][i], presetModes[currPreset][i]);
  }
  //use leds to display currently selected preset
  leds.setValue(currPreset);
  leds.update();
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
      midi.pitchBend(tremoloRange * (-(tremoloValue * 8)/12));
      break;
    case 1:  //Pitch Up
      midi.pitchBend(tremoloRange * ((tremoloValue * 8)/12));
      break;
    case 2:  // Pressure
      for (int i=0; i<5; i++){
        if(btn_notes[i].state){
          midi.monoPressure(notes[i].getRoot(), ((tremoloRange * tremoloValue/8)/12));
        }
      }
      break;
    }
}
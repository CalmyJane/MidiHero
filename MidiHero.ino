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
      Serial.write(firstByte);
      Serial.write(data1);
      Serial.write(data2);
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
    
    void monoPressure(int noteValue, int value, int channel){
      //sends pressure message for monophonic aftertouch. there's also a message for polyphonic aftertoucht
      //value in 0..127
      //0xA0 = pressure message for aftertouch from 0..127
      // Serial.print("\n " + (String)value + " \n");
      write(0xA0, channel, noteValue, value);
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
    int brightTime = 100;
    int darkTime = 0;

  public:

    LedBar(){
      //empty default constructor
    }

    LedBar(int* ledPins){
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

      if(brightCounter > 0){
        brightCounter -= deltat;
        setAll(false);
        if(brightCounter <= 0){
          // brightnesscounter done, reset counter
          darkCounter = darkTime;

        }
      }
      if(darkCounter > 0){
        darkCounter -= deltat;
        if(darkCounter <= 0){
          brightCounter = brightTime;
        }
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
      digitalWrite(pins[led], !value);
    }

    void setAll(bool on){
      bool lds[4] = {on,on,on,on};
      setLeds(lds);
    }

    void setBrightness(int value){
      //set Brightness from 0 to 4
      brightness = value;
      int16_t brightTimes[5] = {20, 20, 100, 200, 500};
      int16_t darkTimes[5] = {0, 1, 100, 200, 500};
      darkTime = darkTimes[value];
      brightTime = brightTimes[value];
      brightCounter = brightTime;
      darkCounter = darkTime;
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

int stateTremolo = 0; //amount of tremolo pressed, 1023 when released, 0 when pressed down
uint32_t stateSlide = 0;

int lastStateTremolo = 0;
int lastStateSlide = 0;

MidiWriter midi;
Note notes[5];

void setup() {
  // Set MIDI baud rate:
  midi = MidiWriter();
  leds.setValue(currPreset);

  for(int i = 0; i < 5; i++){
    btn_notes[i] = Button(pinNotes[i]);
    notes[i] = Note(midi, getHexNote(presets[0][i]), presetModes[0][i], midiChannel);
  }
}

void loop() {
  lastStateTremolo = stateTremolo;
  stateTremolo = analogRead(pinTremolo);

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
  // Serial.print("SlideVal: ");
  // Serial.print(slideNote);
  // Serial.print("\n");

  if(btn_select.pressed){
    // hero power button pressed
    if(btn_start.state){
      // tuning/ shift pressed - change mode (single note, major chord, minor chord ..) for all active notes
      for (int i=0; i<5; i++){
        if (btn_notes[i].state){
          notes[i].changeMode(true);
          notes[i].play(true);
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
      ChangeMidiChannel(btn_up.state);
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

  //reset notes when note button is released
  for(int i = 0; i < 5; i++){
    if(btn_notes[i].released){
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

  if(btn_dmid.pressed){
    leds.setBrightness(2);
    leds.setValue(midiChannel);
  }
  if(btn_dmid.released){
    leds.setBrightness(0);
    leds.setValue(currPreset);
  }

  leds.update();
    
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
    if(btn_notes[i].state){
      notes[i].play(true);
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
  leds.setValue(midiChannel);
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
      midi.pitchBend(tremoloRange * (-(tremoloValue * 8)/12), midiChannel);
      break;
    case 1:  //Pitch Up
      midi.pitchBend(tremoloRange * ((tremoloValue * 8)/12), midiChannel);
      break;
    case 2:  // Pressure
      for (int i=0; i<5; i++){
        if(btn_notes[i].state){
          midi.monoPressure(notes[i].getRoot(), ((tremoloRange * tremoloValue/8)/12), midiChannel);
        }
      }
      break;
    }
}
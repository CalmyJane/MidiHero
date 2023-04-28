// ---- MIDI HERO FIRMWARE ----

// Project for ardution that reads several buttons, a potentiometer and a PWM-Signal (dutycycle) and converts them into MIDI-Messages
// uses Arduino nano to output MIDI messages on the Tx pin to a standart 5-pin-plug
// pwm dutycycle is read to get slider-value
// poti is read to get tremolo-value
// all other buttons on the guitar hero controller are read as digital inputs

// This code is written by Calmy Jane and based on various examples
//check out the readme for more information

#include <EEPROM.h>
#define buttonCount 5 //number of buttons on the guitar
#define presetCount 16

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

      //write midi to tx pin
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
      channel = ((((up?0x01:-0x01) + channel) % 0x10) + 0x10) % 0x10;
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
        
      Serial.print(mode);
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

    void tune(bool up, int semitones){
      // tune 1 semitone up/down
      root += up?semitones:-semitones;
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
    bool analogPin = false;

    Button(){
      
    }

    Button(int pinNumber){
      //Initialize button input
      pin = pinNumber;
      analogPin = (pinNumber >= A0 && pinNumber <= A7); //is analog pin?
      pinMode(pinNumber, INPUT);
    }

    void update(){
      //call periodically
      lastState = state;
      if(analogPin){
        state = analogRead(pin) > 400;
      } else {
        state = digitalRead(pin);
      }
      pressed = !lastState && state; //was pressed in this iteration
      released = lastState && !state; //was released in this iteration
    }
};

class Led{
  private:
    int pin;
    int brightness = 2;
    int16_t brightCounter = 0;
    int16_t brightPeriod = 10;
    uint32_t blinkRate = 0;
    uint32_t blinkDuty = 0;
    int16_t blinkBrightness = 0;
    int16_t prevBrightness = 0;
    int16_t blinkCount = 0;
    int16_t blinkCounter = 0;
    int lastMlls;
    bool state;
    bool timerPwm = false; //if a pwm-pin is used, use the pwm feature to get better resolution

    int invertBrightness(int brightness){
      return 255 - brightness;
    }

  public:
    bool inverted = false; //Set to true, depending on how you connected the LED
    Led(){
      
    }

    Led(int pin){
      //
      lastMlls = millis();
      this->pin = pin;
      pinMode(pin, OUTPUT);
      brightCounter = brightPeriod;
      switch(pin){
        case 3:
        case 5:
        case 6:
        case 9:
        case 10:
        case 11:
          //is PWM-Pin - use timer-pwm
          timerPwm = true;
        break;
        default:
          //not pwm-pin, use software timed pwm
          timerPwm = false;
        break;
      }
      setPin(false);
    }

    void update(){
      int mls = millis();
      int deltat = mls - lastMlls;
      lastMlls = mls;
      bool dimmed = false;
      //Dimming of the LED
      if(brightCounter > 0){
        brightCounter -= deltat;
        if(brightCounter < invertBrightness(brightness)/26){
          dimmed = false;
        } else {
          dimmed = true;
        }
        if(brightCounter <= 0){
          brightCounter = brightPeriod;
        }
      }
      //Blinking of the LED
      if(blinkCounter > 0 && blinkCount > 0){
        //blinking
        blinkCounter -= deltat;
        setPin(blinkCounter > (blinkRate - blinkDuty));
        if(blinkCounter <= 0){
          if(blinkCount != 0){
            if(blinkCount > 0){
              blinkCount--;
            }
            blinkCounter = blinkRate;
          } else {
            //blinkCount is 0 - blinking ended
            brightness = prevBrightness;
          }          
        }
      } else{
        //not blinking
        if(blinkCount != 0){
          //start another blink
          if(blinkCount > 0){
            //reduce if not blink forever
            blinkCount --;
          }
          blinkCounter = blinkRate;
          setPin(true);
        }
        setPin(state && (timerPwm || dimmed));
      }
    }

    void setPin(bool state){
      int onBright = timerPwm?brightness:255; //set brightness to 255 if not PWM-Pin, state will be toggled from update() to dim
      int offBright = 0;
      if(inverted){
        onBright = invertBrightness(onBright);
        offBright = 255;
      }
      analogWrite(pin, state?onBright:offBright);        
    }

    void setState(bool state){
      this->state = state;
      // setPin(state);
    }

    void blink(uint32_t rate, uint32_t duty, int count, int brightness){
      //sets to blink N times, count = -1 for infinite
      //rate in ms period time, 1000 slow rate, 10 fast rate
      //duty in 0..100 (%)
      blinkCount = 0;
      blinkCounter = 0;
      prevBrightness = this->brightness;
      setBrightness(brightness);
      blinkRate = rate;
      blinkDuty = (duty * rate)/100;
      blinkCount = count; //decrement, since first blink started
      blinkCounter = rate;
      setState(true);
    }

    void setBrightness(int brightness){
      //brightness in 0..10
      if(blinkCount > 0 && blinkCounter > 0){
        this->prevBrightness = brightness;
      } else {
        this->brightness = brightness;
      }
    }
};

class LedBar{
  //handles the 4 leds, blinks them, sets brightness through pwm
  private:
    int numericValue = 0; 
    Led leds[4];
  public:
    LedBar(){
      //empty default constructor
    }

    LedBar(int ledPins[4]){
      for(int i=0; i<4; i++){
        leds[i] = Led(ledPins[i]);
        leds[i].inverted = true;
      }
    }

    void update(){
      for(int i=0; i<4; i++){
        leds[i].update();
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
      leds[led].setState(value);
    }

    void setAll(bool on){
      bool lds[4] = {on,on,on,on};
      setLeds(lds);
    }

    void setBrightness(int value){
      //set Brightness from 0 to 4
      for(int i=0; i<4; i++){
        leds[i].setBrightness(value);
      }
    }

    void blink(uint32_t rate, uint32_t duty, int count){
      //blinks duration in ms all LEDs
      for(int i=0; i<4; i++){
        leds[i].setState(true);
        leds[i].blink(rate, duty, count, 100);
      }
    }

    void setValue(int value){
      //set the value to be displayed
      numericValue = value;
      ShowBinary(value);
    }
};

class Preset{
  public:
    Note notes[5];
    int tremoloMode = 0;
    int tremoloRange = 1;
    int tremoloModeCount = 3;
    bool hammeron = true; //will hammerons be played?
    Preset(){
      //empty default constructor
    }
    Preset(MidiWriter midi, int noteValues[5], int noteModes[5], int tremMode, int tremRange, bool hammeron){
      this->hammeron = hammeron;
      tremoloMode = tremMode;
      tremoloRange = tremRange;
      tremoloModeCount = 3;
      for(int i=0; i<5; i++){
        notes[i] = Note(midi, noteValues[i], noteModes[i]);
      }
    }

    void ChangeTremoloMode(bool up){
      //change tremoloMode
      tremoloMode += up?1:-1;
      if(tremoloMode < 0){
        tremoloMode = tremoloModeCount-1;
      }
      if(tremoloMode >= tremoloModeCount){
        tremoloMode = 0;
      }
    }
    
    bool ChangeTremoloRange(bool up){
      //change tremoloRange
      tremoloRange += up?1:-1;
      if(tremoloRange > 12){
        tremoloRange = 12;
      } else if(tremoloRange <= 0){
        tremoloRange = 1;
      } else {
        return true;
      }
      return false;
    }

    bool toggleHammeron(){
      hammeron = !hammeron;
      return hammeron;
    }
};

class Config{
  //contains data and stores/retrieves to/from EEPROM
  private:
    int midiChannelAdd = 0;
    int firstPreset = 1;
  
  public:
    Config(){
      // Serial.begin(9600);
    };
    
    int8_t readMidiCh(){
      int8_t val = 0;
      Serial.begin(9600);
      EEPROM.get(midiChannelAdd, val);
      Serial.begin(31250);
      return val;
    }

    void writeMidiCh(int8_t channel){
      Serial.begin(9600);
      EEPROM.put(midiChannelAdd, channel);
      Serial.begin(31250);
    }

    Preset readPreset(int index){
      Preset prst;
      Serial.begin(9600);
      EEPROM.get(firstPreset + index * sizeof(Preset), prst);
      Serial.begin(31250);
      return prst;
    }

    void writePreset(int index, Preset preset){
      Serial.begin(9600);
      EEPROM.put(firstPreset + index * sizeof(Preset), preset);
      Serial.begin(31250);
    }

    void writeAllPresets(Preset presets[16]){
      for(int i=0; i<16; i++){
        writePreset(i, presets[i]);
      }
    }

    bool updatePreset(int index, Preset preset){
      //overwrites a preset in the EEPROM, avoids to overwrite first 4 presets
      if(index >= 4){
        writePreset(index, preset);
        return true;
      }
      return false;
    }
};

enum NoteNames{
  _C0, _Cs0, _D0, _Ds0, _E0, _F0, _Fs0, _G0, _Gs0, _A0, _As0, _B0,
  _C1, _Cs1, _D1, _Ds1, _E1, _F1, _Fs1, _G1, _Gs1, _A1, _As1, _B1,
  _C2, _Cs2, _D2, _Ds2, _E2, _F2, _Fs2, _G2, _Gs2, _A2, _As2, _B2,
  _C3, _Cs3, _D3, _Ds3, _E3, _F3, _Fs3, _G3, _Gs3, _A3, _As3, _B3,
  _C4, _Cs4, _D4, _Ds4, _E4, _F4, _Fs4, _G4, _Gs4, _A4, _As4, _B4,
  _C5, _Cs5, _D5, _Ds5, _E5, _F5, _Fs5, _G5, _Gs5, _A5, _As5, _B5,
  _C6, _Cs6, _D6, _Ds6, _E6, _F6, _Fs6, _G6, _Gs6, _A6, _As6, _B6,
  _C7, _Cs7, _D7, _Ds7, _E7, _F7, _Fs7, _G7, _Gs7, _A7, _As7, _B7
};

class Poti{
  private:
    int16_t lastValue = 0;
  public:
    int pin = A0;
    int moveTolerance = 3; //turn up to reduce cpu load, keep on 1 to read poti as much as possible
    bool moved = false; //indicates if poti was moved this iteration by more than moveTolerance
    int16_t value = 0;
    bool inverted = false;

    Poti(){}

    Poti(int _pin){
      pin = _pin;
      pinMode(pin, INPUT);
    }

    void update(){
      lastValue = value;
      value = analogRead(pin);
      if(inverted){
        value = 1023 - value;
      }
      moved = abs(value-lastValue) >= moveTolerance; //detect movement greater than moveTolerance
    }
};

class Tremolo : public Poti {
  public:
    Tremolo(){}
    Tremolo(int pin) : Poti(pin) {
    }
    
    void reset(Preset preset, MidiWriter midi, Button btn_notes[buttonCount]){
      value = 0;
      moved = true;
      sendMidi(preset, midi, btn_notes);
    }

    void update(Preset preset, MidiWriter midi, Button btn_notes[buttonCount]){
      Poti::update();
      if(Poti::moved){
        sendMidi(preset, midi, btn_notes);
      }
    }

    void sendMidi(Preset preset, MidiWriter midi, Button btn_notes[buttonCount]){
      // Serial.print("\n");
      // Serial.print(preset.tremoloRange);
      // Serial.print("\n");
      switch(preset.tremoloMode){
        case 0:  // Pitch Down
          midi.pitchBend(preset.tremoloRange * (-(value * 8)/12));
          break;
        case 1:  //Pitch Up
          midi.pitchBend(preset.tremoloRange * ((value * 8)/12));
          break;
        case 2:  // Pressure
          for (int i=0; i<5; i++){
            if(btn_notes[i].state){
              midi.monoPressure(preset.notes[i].getRoot(), ((preset.tremoloRange * value/8)/12));
            }
          }
          break;
      }
    }
};

// Set this to true to write the default values (from the array constants) to EEPROM, only do this once, then turn off and re-write program to arduino
bool resetEEPROM = false;

const int SHORT_BLINK = 80;
const int LONG_BLINK = 200;
const int VERY_LONG_BLINK = 500;

const int BRIGHT_DEFAULT = 7;
const int BRIGHT_BRIGHT = 100;
const int BRIGHT_LOW = 5;

//PINOUTS FOR FIRST 5 BUTTON CONTROLLER
// // Adjust Pinouts here
// int buttonCount = 6; //number of buttons on the guitar
// const int pinNotes[5] = {6,7,8,9,10};
// int pinLeds[4] = {2,5,4,3};
// const int pinUp = 12;
// const int pinDown = 11;
// const int pinTremolo = A7;
// const int pinSelect = A0;
// const int pinDpadUp = A1;
// const int pinDpadDown = A2;
// const int pinDpadLeft = A3;
// const int pinDpadRight = A4;
// const int pinStart = 13;
// const int pinDpadMid = A5;
// const int pinSlide = A6;

// Adjust Pinouts here
const int pinNotes[buttonCount] = {A1,A2,A3,A4,A5};
int pinLeds[4] = {6,11,10,9};
const int pinUp = 12;
const int pinDown = 13;
const int pinTremolo = A7;
const int pinSelect = 2;
const int pinDpadUp = 4;
const int pinDpadDown = 5;
const int pinDpadLeft = 7;
const int pinDpadRight = 8;
const int pinStart = A6;
const int pinDpadMid = 3;
const int pinSlide = A0;

//4 presets of 6 notes each. May be edited later by user
NoteNames presetNotes[presetCount][buttonCount] = {{_C4, _F4, _G4, _A4, _C5}, // C F G A for chords
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

int presetModes[presetCount][buttonCount] = {{0,0,0,0,0},
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

// Presets for 6 Button Guitar
// //4 presets of 6 notes each. May be edited later by user
// NoteNames presetNotes[presetCount][6] = {{_C4, _F4, _G4, _A4, _C5, _F5}, // C F G A for chords
//                              {_C4, _Ds4, _F4, _G4, _As4, _C5}, //c-minor pentatonik
//                              {_C4, _D4, _E4, _G4, _A4, _C5}, //c-major pentatonik
//                              {_A3, _B3, _D4, _E4, _A5, _D5},  // 4 Chords for Blitskrieg bob (A3-A3-D4-E4, Chorus D4-D4-A3-D4-A3 D4-D4-B3-D4-A3)
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
//                              {_C4, _D4, _E4, _F4, _G4, _C5},
// };

// int presetModes[presetCount][6] = {{0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {1,1,1,1,0,0}, //chord-preset containing some chords
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0},
//                                    {0,0,0,0,0,0}
// };

int presetTremolos[presetCount] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}; //tremolo values for 16 presets
int presetTremRanges[presetCount] = {2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,}; //tremolo values for 16 presets
bool hammerons[presetCount] = {true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true};

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

Tremolo tremolo;

LedBar leds(pinLeds);

int currPreset = 1;
int previousMidiCh = 0;
int saveTimeout = 2000;
int octaveTimeout = 600;
int timeCounter = 0;
int lastMlls = 0;
int deltat = 0;

MidiWriter midi;
Preset preset;
Preset presets[16];
Config cfg;

void setup() {
  // Init midi
  Serial.begin(31250);
  midi = MidiWriter();
  if(resetEEPROM){
    cfg.writeMidiCh(0);
    for(int i=0; i<16; i++){
      // Reset all presets
      Preset prst(midi, (int*)presetNotes[i], presetModes[i], presetTremolos[i], presetTremRanges[i], hammerons[i]);
      presets[i] = prst;
    }
    cfg.writeAllPresets(presets);
  }
  //read config data
  midi.channel = cfg.readMidiCh();
  for(int i=0; i<16; i++){
    //Read presets from EEPROM
    presets[i] = cfg.readPreset(i);
  }
  preset = presets[currPreset];

  //Init LEDs
  leds.setValue(currPreset);
  leds.setBrightness(BRIGHT_DEFAULT);
  leds.blink(2000, 50, 1);
  //Init Note Buttons
  for(int i = 0; i < buttonCount; i++){
    btn_notes[i] = Button(pinNotes[i]);
  }
  tremolo = Tremolo(pinTremolo);
  tremolo.inverted = false;
  //Other buttons are Initialized on declaration
}

void loop() {
  int mls = millis();
  deltat = mls - lastMlls;
  lastMlls = mls;

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

  for(int i = 0; i<buttonCount; i++){
    btn_notes[i].update(); 
  }

  //read tremolo poti and send midi message on change
  tremolo.update(preset, midi, btn_notes);
  //read slide bar value
  readSlideBar();

  if(btn_select.pressed){
    // hero power button pressed
    if(btn_start.state){
      if(anyNote()){
        // tuning/ start pressed - change mode (single note, major chord, minor chord ..) for all active notes
        for (int i=0; i<buttonCount; i++){
          if (btn_notes[i].state){
            preset.notes[i].changeMode(midi, true);
            if(tremolo.value <= 500){
              preset.notes[i].play(midi, true);
            }
            leds.blink(LONG_BLINK, 50, 1);
          }
        }        
      } else{
        //change hammeron/pulloff enables
        preset.toggleHammeron();
      }

    } else {
      //change tremolo mode
      tremolo.reset(preset, midi, btn_notes);
      preset.ChangeTremoloMode(true); 
      leds.blink(SHORT_BLINK, 50, 1);
    }
  }

  //Trigger pressed
  if(btn_up.pressed || btn_down.pressed){
    if(btn_dmid.state){
      //currently editing Midi Channel
      midi.changeMidiChannel(btn_up.state);
      leds.setValue(midi.channel);
    } else {
      if(btn_start.state){
        //Tune Notes up/down
        TuneNotes(btn_up.state, 1);
        timeCounter = 0;
        leds.blink(SHORT_BLINK, 50, 1);
      } else {
        //Normal strum, play notes
        PlayNotes();          
      }
    }
  }

  //Trigger hold
  if(btn_up.state || btn_down.state){
    if(btn_start.state && timeCounter < octaveTimeout){
      timeCounter += deltat;
      if(timeCounter >= octaveTimeout){
        TuneNotes(btn_up.state, 11);
        leds.blink(LONG_BLINK, 50, 1);
      }
    }
  }

  //Start Button released
  if(btn_start.released){
    for(int i=0; i<buttonCount; i++){
      preset.notes[i].play(midi, false);
    }
  }

  //check for Hammeron
  for(int i=0; i<buttonCount; i++){
    if(btn_notes[i].pressed){
      bool hammeron = false;
      for(int j=0; j<i; j++){
        hammeron = hammeron || preset.notes[j].playing;
      }
      for(int j=0; j<buttonCount-i; j++){
        hammeron = hammeron && !preset.notes[buttonCount-1-j].playing;
      }
      if(hammeron && preset.hammeron){
        for(int j=0; j<buttonCount; j++){
          preset.notes[j].play(midi, false);
        }
        preset.notes[i].play(midi, true);
      }
    }
  }

  //reset notes when note button is released
  for(int i = 0; i < buttonCount; i++){
    if(btn_notes[i].released){
      //Mute notes on released button and check for pulloff
      int pulloff = -1;
      for(int j = 0; j<i; j++){
        if(btn_notes[j].state && !preset.notes[j].playing && preset.notes[i].playing){
          pulloff = j;
        }
      }
      preset.notes[i].play(midi, false);
      if(pulloff >= 0 && preset.hammeron){
        for(int j = 0; j<buttonCount; j++){
          preset.notes[j].play(midi, j==pulloff);
        }
      }
    }
  }

  //read D-Pad LEFT
  if(btn_dleft.pressed){  
    ChangePreset(true);
  }

  //read D-Pad RIGHT
  if(btn_dright.pressed){
    ChangePreset(false);
  }

  //read dpad UP
  if(btn_dup.pressed){
    //dpad up pressed - change preset
    if(preset.ChangeTremoloRange(true)){
      leds.blink(SHORT_BLINK, 50, 1);
    };
    tremolo.reset(preset, midi, btn_notes);
  }

  //read D-Pad DOWN
  if(btn_ddown.pressed){
    //cross down pressed
    if(preset.ChangeTremoloRange(false)){
      leds.blink(SHORT_BLINK, 50, 1);
    }; 
    tremolo.reset(preset, midi, btn_notes);
  }

  //read middle button of dpad pressed
  if(btn_dmid.pressed){
    previousMidiCh = midi.channel;
    if(anyNote()){
      //Start saving preset, start timeout
      timeCounter = 0;
      leds.setValue(15);
      leds.setBrightness(BRIGHT_DEFAULT);
    } else {
      //Change midi channel
      leds.setBrightness(BRIGHT_BRIGHT);  
      timeCounter = saveTimeout; //to prevent from first pressing menu and then pressing note buttons, notes should be pressed first
    }

  }

  //read middle button hold
  leds.setValue(currPreset);
  leds.setBrightness(BRIGHT_BRIGHT);
  if(btn_dmid.state){
    if(anyNote() && timeCounter < saveTimeout){
      leds.setValue(15);
      timeCounter += deltat;
      if(timeCounter >= saveTimeout){
        //timeout passed, save preset
        int sel = getNoteBinaryInput();
        cfg.updatePreset(sel, preset);
        presets[sel] = preset;
        leds.blink(VERY_LONG_BLINK, 50, 1);
      }
    } else if (!anyNote()){
      leds.setValue(midi.channel);
    }
  }

  //read middle button released
  if(btn_dmid.released){
    if(midi.channel != previousMidiCh){
      // WRITING TO EEPROM
      // writing midi channel to permanent memory if changed
      cfg.writeMidiCh(midi.channel);
      leds.blink(VERY_LONG_BLINK, 50, 1);
    }
    leds.setBrightness(BRIGHT_DEFAULT);
  }

  leds.update();

  // // Debug Code to print all Input states
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

bool anyNote(){
  //returns if any note button is pressed
  bool any = false;
  for(int i=0; i<buttonCount; i++){
    if(btn_notes[i].state){
      any = true;
    };
  }
  return any;
}

int getNoteBinaryInput(){
  //returns the first 4 buttons binary values as integer
  return (btn_notes[0].state << 3) | (btn_notes[1].state << 2) | (btn_notes[2].state << 1) | btn_notes[3].state;
}

void PlayNotes(){
  //Plays the notes of the currently pressed note buttons
  for(int i = 0; i < buttonCount; i++){
    if(btn_notes[i].state){
      preset.notes[i].play(midi, true);
      tremolo.update(preset, midi, btn_notes);
    }
  }
}

void TuneNotes(bool up, int semitones){
  //tunes the curently pressed note buttones one semitone up/down
  bool range = false;
  bool replay = tremolo.value <= 500; //don't replay if tremolo is pressed
  for(int i = 0; i < buttonCount; i++){
    //make sure, that no note exceeds 0x00..0x77
    range = (((preset.notes[i].getRoot() + semitones) >= 0x77) && up) || (((preset.notes[i].getRoot() - semitones) < 0x00) && !up) || range;
  }
  for(int i = 0; i < buttonCount; i++){
    if(btn_notes[i].state || !anyNote()){
      //Mute all notes
      preset.notes[i].play(midi, false);
      if(!range){
        preset.notes[i].tune(up, semitones); //increment or decrement
      }
      if(replay){
        //Play tuned notes
        preset.notes[i].play(midi, true);
      }
    }
  }
}

void ChangePreset(bool up){
  //changes the Preset one number up/down with overroll at 0..4
  //reset midi notes
  for(int i = 0; i < buttonCount; i++){
    //mute all notes
    preset.notes[i].play(midi, false);
  }
  presets[currPreset] = preset;
  //decrement currPreset
  currPreset += up?1:-1;
  if(currPreset < 0){
    currPreset += presetCount;
  } else if(currPreset >=presetCount){
    currPreset -= presetCount;
  }
  preset = presets[currPreset];
  //use leds to display currently selected preset
  leds.setValue(currPreset);
}
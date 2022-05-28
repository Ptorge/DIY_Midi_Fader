#include <Arduino.h>
#include <Control_Surface.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ezButton.h>
#include <Adafruit_SH110X.h>
#include <U8x8lib.h>


//---- setup OLED

U8X8_SH1106_128X64_NONAME_HW_I2C oledDisplay(/* reset=*/ U8X8_PIN_NONE);

const int  buttonPin = 7;    // the pin that the pushbutton for midi programing is attached to
const int fader1Pin = A0;
const int fader2Pin = A1;
const int fader3Pin = A2;
const int fader4Pin = A3;
const int joyXPin = A8;
const int joyYPin = A9;
//const int pot1Pin = A6;
//const int pot2Pin = A6;
//const int footPin = A7;
const int joyButtonPin = 5;
// create variable for fader and joystick state machine
int fader1state =0;
int fader2state =0;
int fader3state =0;
int fader4state =0;
int joyXstate =0;
int joyYstate =0;
int lastfader1state = 1;
int lastfader2state = 1;
int lastfader3state = 1;
int lastfader4state = 1;
int lastjoyXstate = 1;
int lastjoyYstate = 1;

ezButton button(buttonPin);
int buttonState = 0;         // current state of the button
int lastButtonState = 0; 



//------------Interface----------------------
//  for USB Midi interface
USBMIDI_Interface midi;

// For midi over serial 5 pin din
// HardwareSerialMIDI_Interface serialmidi {Serial, MIDI_BAUD};


//--- Variables---------------------
//For controller midi cc note and channel
MIDIAddress faderController1 = {MIDI_CC::Bank_Select, CHANNEL_1};
MIDIAddress faderController2 = {MIDI_CC::Bank_Select, CHANNEL_1};
MIDIAddress faderController3 = {MIDI_CC::Bank_Select, CHANNEL_1};
MIDIAddress faderController4 = {MIDI_CC::Bank_Select, CHANNEL_1};
MIDIAddress joyXController = {MIDI_CC::Bank_Select, CHANNEL_1};
MIDIAddress joyYController = {MIDI_CC::Bank_Select , CHANNEL_1};
//MIDIAddress pot1Controller = {MIDI_CC::Bank_Select , CHANNEL_1};
//MIDIAddress pot2Controller = {MIDI_CC::Bank_Select , CHANNEL_1};
//MIDIAddress footController = {MIDI_CC::Bank_Select , CHANNEL_1};


// The filtered value read when joystick is at the 0% position, this is calibrated to remove dead zones on joystick
constexpr analog_t minimumValue = 1200;
// The filtered value read when joystick is at the 100% position
constexpr analog_t maximumValue = 16383 - 2000;

//--------------------Banks-------------------------------

// Instantiate six Banks, with one track per bank. 
// Compare these numbers to the diagram above.
Bank<128> bankFader1(1);
//   │       └───── number of tracks per bank
//   └───────────── number of banks
//Bank for chaning address
Bank<128> bankFader2(1);
Bank<128> bankFader3(1);
Bank<128> bankFader4(1);
Bank<128> bankJoyX(1);
Bank<128> bankJoyY(1);
//Bank<128> bankpot1(1);
//Bank<128> bankpot2(1);
//Bank<128> bankFoot(1);


// Instantiate six potentiometers for the controls.
Bankable::CCPotentiometer fader1 {
  {bankFader1, BankType::CHANGE_ADDRESS},     // bank configuration
  fader1Pin,                                   // analog pin
  {faderController1}, // address
};

Bankable::CCPotentiometer fader2 {
  {bankFader2, BankType::CHANGE_ADDRESS},     // bank configuration
  fader2Pin,                                   // analog pin
  {faderController2}, // address
};

Bankable::CCPotentiometer fader3 {
  {bankFader3, BankType::CHANGE_ADDRESS},     // bank configuration
  fader3Pin,                                   // analog pin
 {faderController3}, // address
};
Bankable::CCPotentiometer fader4 {
  {bankFader4, BankType::CHANGE_ADDRESS},     // bank configuration
  fader4Pin,                                   // analog pin
  {faderController4}, // address
};

Bankable::CCPotentiometer joyX {
  {bankJoyX, BankType::CHANGE_ADDRESS},     // bank configuration
  joyXPin,                                   // analog pin
 {joyXController}, // address
};
Bankable::CCPotentiometer joyY {
  {bankJoyY, BankType::CHANGE_ADDRESS},     // bank configuration
  joyYPin,                                   // analog pin
  {joyYController}, // address
};
/*
Bankable::CCPotentiometer pot1 {
  {bankpot1, BankType::CHANGE_ADDRESS},     // bank configuration
  pot1Pin,                                   // analog pin
  {pot1Controller}, // address
};
Bankable::CCPotentiometer pot2 {
  {bankpot2, BankType::CHANGE_ADDRESS},     // bank configuration
  pot2Pin,                                   // analog pin
  {pot2Controller}, // address
};
Bankable::CCPotentiometer foot {
  {bankFoot, BankType::CHANGE_ADDRESS},     // bank configuration
  footPin,                                   // analog pin
  {footController}, // address
};

*/

// A mapping function to eliminate the dead zones of the potentiometer:
// Some potentiometers don't output a perfect zero signal when you move them to
// the zero position, they will still output a value of 1 or 2, and the same
// goes for the maximum position.
analog_t mappingFunction(analog_t raw) {
    // make sure that the analog value is between the minimum and maximum
    raw = constrain(raw, minimumValue, maximumValue);
    // map the value from [minimumValue, maximumValue] to [0, 16383]
    return map(raw, minimumValue, maximumValue, 0, 16383);
    // Note: 16383 = 2¹⁴ - 1 (the maximum value that can be represented by
    // a 14-bit unsigned number
}

//Clear the line for LCD screen, pass the line to start and the starting cursor pos
void clearLCDLine(int line, int startpos)
{               
        oledDisplay.setCursor(startpos,line);
        for(int n = 0; n < 3; n++) // 3 indicates symbols in line. For 2x16 LCD write - 16
        {
               oledDisplay.print(" ");
        }
}


//Update the display
void DisplayAddress(int ccFader, int faderID) 
{
      
      switch (faderID)
      {
      case 1:
        clearLCDLine(1,0);
        oledDisplay.drawString(0,1,String(ccFader).c_str());
        break;
      case 2:
        clearLCDLine(1,5);
        oledDisplay.drawString(4,1,String(ccFader).c_str());
       break;
      case 3:
        clearLCDLine(1,9);
        oledDisplay.drawString(8,1,String(ccFader).c_str());
        break;
      case 4:
        clearLCDLine(1,13);
        oledDisplay.drawString(12,1,String(ccFader).c_str());
        break;
      case 5:
        clearLCDLine(5,0);
        oledDisplay.drawString(0,5,String(ccFader).c_str());
        break;
      case 6:
        clearLCDLine(5,5);
        oledDisplay.drawString(4,5,String(ccFader).c_str());
       break;
      default:
        break;
      }



}
// Display the fader value to the oled
void DisplayFaderValue(int faderVal, int faderID) 
{
      
      switch (faderID)
      {
      case 1:
        clearLCDLine(2,0);
        oledDisplay.drawString(0,2,String(faderVal).c_str());
        break;
      case 2:
        clearLCDLine(2,5);
        oledDisplay.drawString(4,2,String(faderVal).c_str());
        break;
      case 3:
        clearLCDLine(2,9);
        oledDisplay.drawString(8,2,String(faderVal).c_str());
        break;
      case 4:
        clearLCDLine(2,13);
        oledDisplay.drawString(12,2,String(faderVal).c_str());
        break;
      case 5:
        clearLCDLine(6,0);
        oledDisplay.drawString(0,6,String(faderVal).c_str());
        break;
      case 6:
        clearLCDLine(6,5);
        oledDisplay.drawString(4,6,String(faderVal).c_str());
        break;
      default:
        break;
      }
      
}


// Change the midi CC address of the fader or control that is moved and it is saved to EEPROM
void ChangeAddress() // Change the CC address for a selected fader
{
  // get current value for each fader to determine if thefader moves
  int origFader1Value = fader1.getValue();
  int origFader2Value = fader2.getValue();
  int origFader3Value = fader3.getValue();
  int origFader4Value = fader4.getValue();
  int origJoyXValue = joyX.getValue();
  int origJoyYValue = joyY.getValue();
  int newFader1Value;
  int newFader2Value;
  int newFader3Value;
  int newFader4Value;
  int newJoyXValue;
  int newJoyYValue;


// loop while select button is held down
     int loopState = buttonState;
    while (loopState == LOW)
     {
    // get raw pot value between 0-127
    Control_Surface.loop();
    
    newFader1Value = fader1.getValue();
    newFader2Value = fader2.getValue();
    newFader3Value = fader3.getValue();
    newFader4Value = fader4.getValue();
    newJoyXValue = joyX.getValue();
    newJoyYValue = joyY.getValue();
    
      //if a fader moves, take the raw pot number between 0-127 and assign the new value to the moving fader
      if (newFader1Value != origFader1Value)
      {
        bankFader1.select(newFader1Value);
        origFader1Value = newFader1Value;
        DisplayAddress(newFader1Value, 1);
      }
      if (origFader2Value != newFader2Value)
      {
       bankFader2.select(newFader2Value);
       origFader2Value = newFader2Value;
       DisplayAddress(newFader2Value, 2);
      }
      
      if (origFader3Value != newFader3Value)
      {
        bankFader3.select(newFader3Value);
        origFader3Value = newFader3Value;
        DisplayAddress(newFader3Value, 3);
      }
      if (origFader4Value != newFader4Value)
      {
        bankFader4.select(newFader4Value);
        origFader4Value = newFader4Value;
        DisplayAddress(newFader4Value, 4);
      }

       if (origJoyXValue != newJoyXValue)
      {
        bankJoyX.select(newJoyXValue);
        origJoyXValue = newJoyXValue;
        DisplayAddress(newJoyXValue, 5);
      }
        if (origJoyYValue != newJoyYValue)
      {
        bankJoyY.select(newJoyYValue);
        origJoyYValue = newJoyYValue;
        DisplayAddress(newJoyYValue, 6);
      }
      
      //write values to NVM so they are available on restart
      EEPROM.update(1,bankFader1.getSelection());
      EEPROM.update(2,bankFader2.getSelection());
      EEPROM.update(3,bankFader3.getSelection());
      EEPROM.update(4,bankFader4.getSelection());
      EEPROM.update(5,bankJoyX.getSelection());
      EEPROM.update(6,bankJoyY.getSelection());
     
      button.loop();
      loopState = button.getState();

    }
    
  }

void GetFaderValue()  // get fader values to diplay
{

//Display fader value change if fader is moved
  fader1state = fader1.getValue();
  
  if (fader1state != lastfader1state)
  {
    DisplayFaderValue(fader1state,1);
  }
  lastfader1state = fader1state;

   fader2state = fader2.getValue();
  if (fader2state != lastfader2state)
  {
    DisplayFaderValue(fader2state,2);
  }
  lastfader2state = fader2state;

   fader3state = fader3.getValue();
  if (fader3state != lastfader3state)
  {
    DisplayFaderValue(fader3state,3);
  }
  lastfader3state = fader3state;

   fader4state = fader4.getValue();
  if (fader4state != lastfader4state)
  {
    DisplayFaderValue(fader4state,4);
  }
  lastfader4state = fader4state;

  joyXstate = joyX.getValue();
  if (joyXstate != lastjoyXstate)
  {
    DisplayFaderValue(joyXstate,5);
  }
  lastjoyXstate = joyXstate;

  joyYstate = joyY.getValue();
  if (joyYstate != lastjoyYstate)
  {
    DisplayFaderValue(joyYstate,6);
  }
  lastjoyYstate = joyYstate;

}

void setup() 
{
  
     // Add the mapping function to the joystick X potentiometer, this is calibrated to remove dead zones on joystick
    joyX.map(mappingFunction);
    // Add the mapping function to the joystick Y potentiometer, this is calibrated to remove dead zones on joystick
    joyY.map(mappingFunction);

//debounce swtich
  button.setDebounceTime(10);
    
    //eeprom program for first time only-------- optional
    //EEPROM.write(1,1);
    //EEPROM.write(2,11);
    //EEPROM.write(3,11);
    //EEPROM.write(4,11);
    //EEPROM.write(5,11);
    //EEPROM.write(6,11);

    //read cc fader value from eprom and assign to eachfader bank
    bankFader1.select(EEPROM.read(1));
    bankFader2.select(EEPROM.read(2));
    bankFader3.select(EEPROM.read(3));
    bankFader4.select(EEPROM.read(4));
    bankJoyX.select(EEPROM.read(5));
    bankJoyY.select(EEPROM.read(6));

    DisplayAddress(7,1);
// setup display and put the labels for each controller in columns
  oledDisplay.begin();  
  Control_Surface.begin();
  
  oledDisplay.setPowerSave(0);
 
  oledDisplay.setFont(u8x8_font_victoriabold8_r);
  oledDisplay.inverse();
  oledDisplay.drawString(0,0,"F1");
  oledDisplay.drawString(4,0,"F2");
  oledDisplay.drawString(8,0,"F3");
  oledDisplay.drawString(12,0,"F4");

  oledDisplay.drawString(0,4,"JX");
  oledDisplay.drawString(4,4,"JY");
  oledDisplay.drawString(8,4,"P1");
  oledDisplay.drawString(12,4,"P2");
  
 oledDisplay.setFont(u8x8_font_chroma48medium8_r);
 oledDisplay.noInverse();

// Read and display the current midi CC address the controll is set to
 DisplayAddress(EEPROM.read(1),1);
 DisplayAddress(EEPROM.read(2),2);
  DisplayAddress(EEPROM.read(3),3);
 DisplayAddress(EEPROM.read(4),4);
  DisplayAddress(EEPROM.read(5),5);
 DisplayAddress(EEPROM.read(6),6);

  }

void loop() 
{
   
  button.loop();
  buttonState = button.getState();
  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      ChangeAddress();
    } 
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;

  GetFaderValue();  // loop to get and display the fader values
  Control_Surface.loop();
  
}
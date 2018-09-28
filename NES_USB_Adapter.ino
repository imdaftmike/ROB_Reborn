/*  ================================================================================
    Dual NES Controller to USB gamepad/keyboard adapter for RetroPie - daftmike 2018

    uses code from 'NES Controller Test Code' by Joseph Corleto
    https://www.allaboutcircuits.com/projects/nes-controller-interface-with-an-arduino-uno/

    ================================================================================
  Initially I wrote this sketch to have two USB joysticks and it was nice and neat, but RetroPie
  didn't like having two joysticks with the same name and I couldn't get them working properly.
  Instead, I've got NES player 1 as a keyboard and NES player 2 as a joystick and that works ok.

*/
#include <Joystick.h>
#include <Keyboard.h>

const int numButtons = 8;   // NES has 8 buttons on it controller (including d-pad)

// Setup a gamepad with 8 buttons and an x/y axis (RetroPie won't recognise a controller without an x/y axis)
Joystick_ Joystick(0x03, 0x05, numButtons, 0, true, true, false, false, false, false, false, false, false, false, false);

/* Joystick Parameters
  uint8_t hidReportId       - Default: 0x03(don't use 0x01 or 0x02)
  uint8_t joystickType      - Default: 0x04 (0x05 = gamepad)
  uint8_t buttonCount       - Default: 32
  uint8_t hatSwitchCount    - Default: 2
  bool includeXAxis         - Default: true
  bool includeYAxis         - Default: true
  bool includeZAxis         - Default: true - (sometimes this is the right X Axis)
  bool includeRxAxis        - Default: true - (sometimes this is the right Y Axis)
  bool includeRyAxis        - Default: true - Y Axis Rotation
  bool includeRzAxis        - Default: true - Z Axis Rotation
  bool includeRudder        - Default: true
  bool includeThrottle      - Default: true
  bool includeAccelerator   - Default: true
  bool includeBrake         - Default: true
  bool includeSteering      - Default: true
*/

// NES Controller buttons
const int A_BUTTON         = 0;
const int B_BUTTON         = 1;
const int SELECT_BUTTON    = 2;
const int START_BUTTON     = 3;
const int UP_BUTTON        = 4;
const int DOWN_BUTTON      = 5;
const int LEFT_BUTTON      = 6;
const int RIGHT_BUTTON     = 7;


// Array to store all the button states
int lastButtonState[2][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

// NES controller pins
int p1Data       = 4;    // data
int p1Clock      = 3;    // clock
int p1Latch      = 2;    // latch
int p2Data       = 7;
int p2Clock      = 6;
int p2Latch      = 5;

/* NES controller pinout  

      GND    o   
      CLOCK  o  o  +5V
      LATCH  o  o  N/C
      DATA   o  o  N/C


/****************************************************************************************************************/

void setup()
{
  // Set pinmodes
  pinMode(p1Data, INPUT_PULLUP);
  pinMode(p1Clock, OUTPUT);
  pinMode(p1Latch, OUTPUT);
  pinMode(p2Data, INPUT_PULLUP);
  pinMode(p2Clock, OUTPUT);
  pinMode(p2Latch, OUTPUT);

  // Set initial states
  digitalWrite(p1Clock, LOW);
  digitalWrite(p1Latch, LOW);
  digitalWrite(p2Clock, LOW);
  digitalWrite(p2Latch, LOW);

  // I used pins 8/9 for 5V/GND to make the wiring easier (not necessary)
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);

  // Initialise keyboard and joystick
  Keyboard.begin();
  Joystick.begin();

  // Centre the joystick
  Joystick.setXAxis(511);
  Joystick.setYAxis(511);

}

/****************************************************************************************************************/

void loop()
{
  // use keyboard for player 1
  for (int i = 0; i < numButtons; i++)                              // loop through each NES button
  {
    int nesRegister = readNesController(p1Data, p1Clock, p1Latch);  // read NES controller 1
    bool currentButtonState = bitRead(nesRegister, i);              // get the appropriate bit
    if (currentButtonState != lastButtonState[0][i])                // check if the button state has changed since last time
    {
      if (currentButtonState == 0) {                                // press a key when a button is pressed
        Keyboard.press(47 + i);
      }
      if (currentButtonState == 1) {                                // release the key when that button is released
        Keyboard.release(47 + i);
      }
      lastButtonState[0][i] = currentButtonState;                   // store the button state for next time
    }
  }


  // use joystick for player 2
  setUSBgamepad(readNesController(p2Data, p2Clock, p2Latch), 1);

}

/****************************************************************************************************************/

void setUSBgamepad(int nesController, int j) {
  for (int i = 0; i < numButtons; i++)                          // loop through each NES button
  {
    bool currentButtonState = bitRead(nesController, i);        // get the appropriate bit from the NES controller
    if (currentButtonState != lastButtonState[j][i])            // check if the button state has changed since last time
    {
      Joystick.setButton(i, !currentButtonState);               // set the joystick button to the current button state
      lastButtonState[j][i] = currentButtonState;               // store the button state for next time
    }
  }
}

/****************************************************************************************************************/

byte readNesController(int nesData, int nesClock, int nesLatch)    // from 'NES Controller Test Code' by Joseph Corleto
{
  // Pre-load a variable with all 1's which assumes all buttons are not
  // pressed. But while we cycle through the bits, if we detect a LOW, which is
  // a 0, we clear that bit. In the end, we find all the buttons states at once.
  int tempData = 255;

  // Quickly pulse the nesLatch pin so that the register grabs what it see on
  // its parallel data pins.
  digitalWrite(nesLatch, HIGH);
  digitalWrite(nesLatch, LOW);

  // Upon latching, the first bit is available to look at, which is the state
  // of the A button. We see if it is low, and if it is, we clear out variable's
  // first bit to indicate this is so.
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, A_BUTTON);

  // Clock the next bit which is the B button and determine its state just like
  // we did above.
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, B_BUTTON);

  // Now do this for the rest of them!

  // Select button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, SELECT_BUTTON);

  // Start button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, START_BUTTON);

  // Up button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, UP_BUTTON);

  // Down button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, DOWN_BUTTON);

  // Left button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, LEFT_BUTTON);

  // Right button
  digitalWrite(nesClock, HIGH);
  digitalWrite(nesClock, LOW);
  if (digitalRead(nesData) == LOW)
    bitClear(tempData, RIGHT_BUTTON);

  return tempData;
}

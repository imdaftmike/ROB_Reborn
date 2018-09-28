/*  ================================================================================
    i2c control for ROBs 'body and hands' motors - daftmike 2018

    ================================================================================
*/

#include <avr/wdt.h>
#include <Wire.h>

#define SLAVE_ADDRESS 0x04

// Constants
const int switchA = 14;           // Limit switch (blue, green)
const int switchB = 15;
const int motorA1 = 8;            // Hands motor (red, brown)
const int motorA2 = 9;
const int motorB1 = 4;            // Body motor (orange, yellow)
const int motorB2 = 5;

const long maxHandsTime = 1800;   // max time to keep the hands motor running
const long maxBodyTime = 2500;    // max time to keep the body motor running

// Variables
bool handsStatus = 0;           // 1 = open, 0 = closed
bool bodyState = 0;             // current state of the body switch
bool lastBodyState = 0;         // previous state of the body switch
bool startupFinished = 0;       // keep track of the startup status
int bodyPosition = 0;           // 0 = bottom, 5 = top
int prevBodyPosition = 0;       // keep track of the body position
unsigned long handsTime;        // used to check how long since the hands started opening
unsigned long prevHandsTime;    // used to check how long since the hands started opening
unsigned long bodyTime;         // used to check how long the body motors have been running
unsigned long prevBodyTime;     // used to check how long the body motors have been running
int robMsg = 0;
int robStatus;


void setup() {

  MCUSR = 0;  // clear out any flags of prior resets.

  //Serial.begin(9600);

  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);

  // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // setup the switch
  pinMode(switchA, INPUT_PULLUP);
  pinMode(switchB, OUTPUT);
  digitalWrite(switchB, LOW);

  // setup the motor outputs
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);


  // start the body motors moving up
  digitalWrite(motorB1, HIGH);
  digitalWrite(motorB2, LOW);

  // open the hands
  digitalWrite(motorA1, LOW);
  digitalWrite(motorA2, HIGH);
  // save the time since we started to open the hands
  handsTime = millis();

}

void loop() {

  startupPosition();

  if (robMsg == 1) {
    openHands();
  }
  else if (robMsg == 2) {
    closeHands();
  }

  else if (robMsg == 3) {
    if (bodyPosition < 5) {
      bodyTime = millis();
      prevBodyPosition = bodyPosition;
      do {
        moveBodyUp();
      } while (prevBodyPosition == bodyPosition);
    }
  }
  else if (robMsg == 4) {
    if (bodyPosition > 0) {
      bodyTime = millis();
      prevBodyPosition = bodyPosition;
      do {
        moveBodyDown();
      } while (prevBodyPosition == bodyPosition);
    }
  }
  else if (robMsg == 5) {
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
  }

  robMsg = 0;
  robStatus = bodyPosition;
  if (handsStatus) {
    robStatus += 10;
  }


}


void openHands() {
  if (!handsStatus) {
    // open the hands
    digitalWrite(motorA1, LOW);
    digitalWrite(motorA2, HIGH);

    delay(maxHandsTime);

    // stop the motors
    digitalWrite(motorA1, LOW);
    digitalWrite(motorA2, LOW);

    // update the handStatus
    handsStatus = 1;
  }
}


void closeHands() {
  if (handsStatus) {
    // close the hands
    digitalWrite(motorA1, HIGH);
    digitalWrite(motorA2, LOW);

    delay(maxHandsTime);

    // stop the motors
    digitalWrite(motorA1, LOW);
    digitalWrite(motorA2, LOW);

    // update the handStatus
    handsStatus = 0;
  }
}

void startupPosition() {
  if (!startupFinished) {

    // check if the hand motors have been running longer than the maximum allowed time
    if (millis() - handsTime > maxHandsTime) {
      // stop the motors
      digitalWrite(motorA1, LOW);
      digitalWrite(motorA2, LOW);
      // update the handStatus
      handsStatus = 1;
    }

    // check if we've reached a notch (limit switch falling edge)
    bodyState = digitalRead(switchA);
    if (bodyState != lastBodyState) {
      if (bodyState == LOW) {
        bodyTime = millis();
      }
      delay(50);
    }
    lastBodyState = bodyState;

    //Serial.print("elasped time: "); Serial.print(millis());
    //Serial.print(" | time since last notch: "); Serial.println(millis() - bodyTime);

    if ((millis() - bodyTime > maxBodyTime) || (millis() > 12000)) {
      startupFinished = 1;
      bodyPosition = 5;
      prevBodyPosition = bodyPosition;

    }

  }
  else {
    stopBody();
  }
}

void moveBodyUp() {
  // start the body motors moving up
  digitalWrite(motorB1, HIGH);
  digitalWrite(motorB2, LOW);

  // check if the body motors have been running longer than the maximum allowed time
  if (millis() - bodyTime > maxBodyTime) {
    // stop the motors
    digitalWrite(motorB1, LOW);
    digitalWrite(motorB2, LOW);
    bodyPosition++;
  }

  // check if we've reached a notch (limit switch falling edge)
  bodyState = digitalRead(switchA);
  delay(50);
  if (bodyState != lastBodyState) {
    if (bodyState == LOW) {
      bodyPosition++;
      stopBody();
    }
    //delay(50);
  }
  lastBodyState = bodyState;

  //Serial.print("limit switch: "); Serial.print(bodyState);
  //Serial.print(" | body position: "); Serial.println(bodyPosition);
}

void moveBodyDown() {
  // start the body motors moving down
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, HIGH);

  // check if the body motors have been running longer than the maximum allowed time
  if (millis() - bodyTime > maxBodyTime) {
    // stop the motors
    digitalWrite(motorB1, LOW);
    digitalWrite(motorB2, LOW);
    bodyPosition--;
  }

  // check if we've reached a notch (limit switch falling edge)
  bodyState = digitalRead(switchA);
  delay(50);
  if (bodyState != lastBodyState) {
    if (bodyState == LOW) {
      bodyPosition--;
      stopBody();
    }
    //delay(50);
  }
  lastBodyState = bodyState;

  //Serial.print("limit switch: "); Serial.print(bodyState);
  //Serial.print(" | body position: "); Serial.println(bodyPosition);
}

void stopBody() {
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, LOW);
}


// callback for received data
void receiveData(int byteCount) {
  robMsg = Wire.read();
}

// callback for sending data
void sendData() {
  Wire.write(robStatus);
}

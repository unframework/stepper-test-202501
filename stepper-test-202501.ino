#include <Stepper.h>

// actually ~2036.8? per https://www.youtube.com/watch?v=B86nqDRskVU
const int stepsPerRevolution = 2038;

// IN1-IN4 = pin 8-11
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

// const int pinLeft = 2;
// const int pinRight = 3;
const int analogInPin = A0;

int dirState = 0;

void setup() {
  // input pins
  // pinMode(pinLeft, INPUT_PULLUP);
  // pinMode(pinRight, INPUT_PULLUP);
  pinMode(analogInPin, INPUT);

  // speed at 4 RPM (5+ starts being weak and stalling out)
  // myStepper.setSpeed(4);
}

void loop() {
  if (dirState < -1) {
    myStepper.step(-1);
  } else if (dirState > 1) {
    myStepper.step(1);
  }

  // check for button press
  if (analogRead(analogInPin) < 200) {
    dirState = max(-4, dirState - 1);
  } else if (analogRead(analogInPin) > 823) {
    dirState = min(4, dirState + 1);
  } else {
    // return to center
    if (dirState > 0) {
      dirState--;
    } else if (dirState < 0) {
      dirState++;
    }
  }

  delay(5); // 4rpm = ~7.5ms per step
}

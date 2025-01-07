#include <Stepper.h>

// actually ~2036.8? per https://www.youtube.com/watch?v=B86nqDRskVU
const int stepsPerRevolution = 2038;

// IN1-IN4 = pin 8-11
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

void setup() {
  // speed at 4 RPM (5+ starts being weak and stalling out)
  myStepper.setSpeed(4);
}

void loop() {
  // 90deg turn and delay
  myStepper.step(-stepsPerRevolution / 4);
  delay(500);
}

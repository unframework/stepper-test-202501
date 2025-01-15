// #include <Stepper.h>

// actually ~2036.8? per https://www.youtube.com/watch?v=B86nqDRskVU
const int stepsPerRevolution = 2038;

// IN1-IN4 = pin 8-11
const int motorPin1 = 8;
const int motorPin2 = 9;
const int motorPin3 = 10;
const int motorPin4 = 11;

// Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

// const int pinLeft = 2;
// const int pinRight = 3;
const int dirInPin = A0;
const int freqInPin = A1;

int dirState = 0;

unsigned long animCountdownUs = 0;
unsigned long lastNow = 0;

float posState = 0;
int stepOffset = 0; // actual position in steps

unsigned int stepIndex = 0; // 0-3
unsigned int stepsSinceMove = 0;

void setup() {
  // input pins
  // pinMode(pinLeft, INPUT_PULLUP);
  // pinMode(pinRight, INPUT_PULLUP);
  pinMode(dirInPin, INPUT);
  pinMode(freqInPin, INPUT);

  // driver pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  // speed at 4 RPM (5+ starts being weak and stalling out)
  // myStepper.setSpeed(4);

  lastNow = micros();
}

void loop() {
  stepsSinceMove = min(20, stepsSinceMove + 1);

  unsigned long now = micros();
  unsigned long deltaUs = min(50000, now - lastNow); // prevent overflow
  lastNow = now;

  if (animCountdownUs > 0) {
    float t = animCountdownUs / 1000000.0f;
    posState = sin(t * 3.14159f * 2.0f);

    animCountdownUs = deltaUs > animCountdownUs ? 0 : animCountdownUs - deltaUs;
  } else {
    posState = 0;
  }

  if (dirState < -1) {
    // myStepper.step(-1);
    stepsSinceMove = 0;
    stepIndex = (stepIndex - 1) % 4;
  } else if (dirState > 1) {
    // myStepper.step(1);
    stepsSinceMove = 0;
    stepIndex = (stepIndex + 1) % 4;
  } else {
    int diff = posState * 200 - stepOffset;
    if (diff > 5) {
      stepsSinceMove = 0;
      stepIndex = (stepIndex + 1) % 4;
      stepOffset++;
    } else if (diff < -5) {
      stepsSinceMove = 0;
      stepIndex = (stepIndex - 1) % 4;
      stepOffset--;
    }
  }

  // apply current step pins
  // (https://github.com/arduino-libraries/Stepper/blob/master/src/Stepper.cpp)
  switch (stepsSinceMove < 20 ? stepIndex : 99) {
  case 0: // 1010
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, HIGH);
    digitalWrite(motorPin4, LOW);
    break;
  case 1: // 0110
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    digitalWrite(motorPin3, HIGH);
    digitalWrite(motorPin4, LOW);
    break;
  case 2: // 0101
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, HIGH);
    break;
  case 3: // 1001
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, HIGH);
    break;
  default: // 0000 to cool down
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, LOW);
  }

  // check for button press
  if (analogRead(dirInPin) < 200) {
    dirState = max(-4, dirState - 1);
  } else if (analogRead(dirInPin) > 823) {
    dirState = min(4, dirState + 1);
  } else {
    // return to center
    if (dirState > 0) {
      dirState--;
    } else if (dirState < 0) {
      dirState++;
    }
  }

  // frequency in
  // float posRead = (analogRead(freqInPin) / 1023.0f) - 0.5f;
  // posState = posState * 0.6f + posRead * 0.4f; // smooth out the input

  // cheap trigger for animation
  if (abs(analogRead(freqInPin) - 512) > 200) {
    animCountdownUs = 1000000;
  }

  delay(2); // 4rpm = ~7.5ms per step
}

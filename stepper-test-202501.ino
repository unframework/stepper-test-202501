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

// 0 = off, period = 20 steps + 100 steps * N ie about 100ms to 600ms
float freqState = 0;

const int freqReadRangeMin = 100;
const int freqReadRangeAmount = (1023 - freqReadRangeMin);

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
}

int periodN = 0;

unsigned int stepIndex = 0; // 0-3

void loop() {
  if (dirState < -1) {
    // myStepper.step(-1);
    stepIndex = (stepIndex - 1) % 4;
  } else if (dirState > 1) {
    // myStepper.step(1);
    stepIndex = (stepIndex + 1) % 4;
  } else {
    if (freqState > 0.05f) {
      int period = 260.0f - 220.0f * freqState;
      periodN++;

      if (periodN > period) {
        periodN = 0;
      }

      int pos = periodN - period / 2;
      if (pos < 0) {
        // check for a small cooldown first
        if (periodN > 10) {
          // myStepper.step(-1);
          stepIndex = (stepIndex - 1) % 4;
        }
      } else {
        // check for a small cooldown first
        if (pos > 10) {
          // myStepper.step(1);
          stepIndex = (stepIndex + 1) % 4;
        }
      }
    }
  }

  // apply current step pins
  // (https://github.com/arduino-libraries/Stepper/blob/master/src/Stepper.cpp)
  switch (stepIndex) {
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
  float freqRead = max(0, analogRead(freqInPin) - freqReadRangeMin) /
                   (float)freqReadRangeAmount;
  freqState = freqState * 0.9f + 0.1f * freqRead; // smooth out the input

  delay(2); // 4rpm = ~7.5ms per step
}

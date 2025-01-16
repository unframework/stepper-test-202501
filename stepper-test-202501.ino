// #include <Stepper.h>

// actually ~2036.8? per https://www.youtube.com/watch?v=B86nqDRskVU
const int stepsPerRevolution = 2038;

const int holdStepMax = 20;
class SoftStep {
public:
  SoftStep(int pin1, int pin2, int pin3, int pin4)
      : pin1(pin1), pin2(pin2), pin3(pin3), pin4(pin4) {}

  void setup() {
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    pinMode(pin3, OUTPUT);
    pinMode(pin4, OUTPUT);
  }

  void setTarget(int target) { this->target = target; }
  void setSpeed(int speed) { this->speed = speed; }

  void applyStep() {
    holdStepCount = min(holdStepMax, holdStepCount + 1);

    // apply speed
    offset += speed;

    // move within range of target position
    // TODO check for underflow/overflow or use longs
    int realTarget = target + offset;
    if (position < realTarget - 2) {
      position++;
      holdStepCount = 0;
    } else if (position > realTarget + 2) {
      position--;
      holdStepCount = 0;
    }

    // apply current step pins
    // (https://github.com/arduino-libraries/Stepper/blob/master/src/Stepper.cpp)
    switch (holdStepCount < holdStepMax ? position & 0b11 : 99) {
    case 0: // 1010
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, LOW);
      digitalWrite(pin3, HIGH);
      digitalWrite(pin4, LOW);
      break;
    case 1: // 0110
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
      digitalWrite(pin3, HIGH);
      digitalWrite(pin4, LOW);
      break;
    case 2: // 0101
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
      digitalWrite(pin3, LOW);
      digitalWrite(pin4, HIGH);
      break;
    case 3: // 1001
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, LOW);
      digitalWrite(pin3, LOW);
      digitalWrite(pin4, HIGH);
      break;
    default: // 0000 to cool down
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
      digitalWrite(pin3, LOW);
      digitalWrite(pin4, LOW);
    }
  }

private:
  int pin1;
  int pin2;
  int pin3;
  int pin4;

  int target = 0;   // direct setting
  int offset = 0;   // changed by speed
  int position = 0; // runs towards target + offset
  int speed = 0;    // -1..1, affects offset only

  int holdStepCount = 0;
};

// IN1-IN4 = pin 8-11
class SoftStep stepperA(8, 9, 10, 11);

// const int pinLeft = 2;
// const int pinRight = 3;
const int dirInPin = A0;
const int freqInPin = A1;

int dirState = 0;

unsigned long animCountdownUs = 0;
unsigned long lastNow = 0;

void setup() {
  // input pins
  // pinMode(pinLeft, INPUT_PULLUP);
  // pinMode(pinRight, INPUT_PULLUP);
  pinMode(dirInPin, INPUT);
  pinMode(freqInPin, INPUT);

  // driver pins
  stepperA.setup();

  // speed at 4 RPM (5+ starts being weak and stalling out)
  // myStepper.setSpeed(4);

  lastNow = micros();
}

const float resoHz = 2.15f; // 3.3f;
const float cycles = 2.0f;
const float dur = cycles / resoHz;

void loop() {
  unsigned long now = micros();
  unsigned long deltaUs = min(50000, now - lastNow); // prevent overflow
  lastNow = now;

  if (animCountdownUs > 0) {
    // normalize to 0..1 (to fit predictable number of sin() cycles)
    float t = 1.0f - (animCountdownUs / 1000000.0f) / dur;
    animCountdownUs = deltaUs > animCountdownUs ? 0 : animCountdownUs - deltaUs;

    // up-down-up swings of sin() with a slow raise (and then release)
    float posState = sin(t * 3.14159f * 2.0f * cycles) * 0.4f - t * 2.0f;
    stepperA.setTarget(posState * 100);
  } else {
    stepperA.setTarget(0);
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

  if (dirState < -1) {
    stepperA.setSpeed(-1);
  } else if (dirState > 1) {
    stepperA.setSpeed(1);
  } else {
    stepperA.setSpeed(0);
  }

  // cheap trigger for animation
  if (abs(analogRead(freqInPin) - 512) > 200) {
    animCountdownUs = 1000000 * dur;
  }

  // delay(2); // 4rpm = ~7.5ms per step
  // @todo move into interrupt
  stepperA.applyStep();
  delayMicroseconds(1500);
}

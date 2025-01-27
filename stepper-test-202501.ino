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
  void setSpeed(int speed) {
    // cannot possibly go faster than 1 motor movement per step tick
    if (speed < -1)
      speed = -1;
    if (speed > 1)
      speed = 1;
    this->speed = speed;
  }

  void applyStep() {
    holdStepCount = min(holdStepMax, holdStepCount + 1);

    // apply speed
    offset += speed;

    // overflow intended offset and real position in tandem together
    // (i.e. preserving their relative diff amount)
    // this does not need to use the steps-per-revolution value, just be div
    // by 4 (# of coil steps)
    int overflowAdjust = -(offset & 0xff00);
    offset += overflowAdjust;
    position += overflowAdjust;

    // move within range of target position
    int realTarget = target + offset;
    int diff = realTarget - position;
    if (diff > 2) {
      position++;
      holdStepCount = 0;
    } else if (diff < -2) {
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

  lastNow = micros();

  // set up timer for 2ms interrupt (about as fast as the servo goes)
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 3999;            // compare match register 16MHz/8/500Hz
  TCCR1B |= (1 << WGM12);  // CTC mode
  TCCR1B |= (1 << CS11);   // 8 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

ISR(TIMER1_COMPA_vect) { stepperA.applyStep(); }

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

  // input/animation loop delay
  delay(1);
}

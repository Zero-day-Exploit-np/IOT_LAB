#include <Stepper.h>

#define IN1 5
#define IN2 4
#define IN3 14
#define IN4 12

const int stepsPerRevolution = 2048;

Stepper stepperMotor(stepsPerRevolution, IN1, IN3, IN2, IN4);

void setup() {
  stepperMotor.setSpeed(10);  // RPM
}

void loop() {
  stepperMotor.step(512);     // Clockwise
  delay(1000);
  stepperMotor.step(-512);    // Counter-clockwise
  delay(1000);
}

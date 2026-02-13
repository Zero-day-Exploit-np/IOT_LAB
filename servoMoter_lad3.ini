#include <Servo.h>

Servo myServo;
#define SERVO_PIN 5

void setup() {
  myServo.attach(SERVO_PIN);
}

void loop() {
  for(int pos = 0; pos <= 180; pos++) {
    myServo.write(pos);
    delay(15);
  }

  for(int pos = 180; pos >= 0; pos--) {
    myServo.write(pos);
    delay(15);
  }
}

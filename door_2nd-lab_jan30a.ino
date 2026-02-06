#define DOOR_PIN 4
#define BUZZER_PIN 18

void setup() {
  pinMode(DOOR_PIN, INPUT_PULLUP);  // Important
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  int doorState = digitalRead(DOOR_PIN);

  if (doorState == HIGH) {
    // Door OPEN or sensor DISCONNECTED
    digitalWrite(BUZZER_PIN, HIGH);   // Continuous beep
  } else {
    // Door CLOSED (sensor connected + magnet near)
    digitalWrite(BUZZER_PIN, LOW);
  }
}

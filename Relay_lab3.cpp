#define RELAY_PIN 7

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
  digitalWrite(RELAY_PIN, LOW);   // Relay ON (Most modules are LOW trigger)
  delay(2000);

  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
  delay(2000);
}

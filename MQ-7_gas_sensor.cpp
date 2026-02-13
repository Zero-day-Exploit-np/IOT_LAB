#define MQ7_PIN A0

void setup() {
  Serial.begin(9600);
}

void loop() {
  int value = analogRead(MQ7_PIN);

  Serial.print("CO Value: ");
  Serial.println(value);

  delay(1000);
}

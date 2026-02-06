#define IR_PIN 4
#define BUZZER_PIN 18

void setup() {
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int irValue = digitalRead(IR_PIN);

  if (irValue == LOW) {        // Object detected
    digitalWrite(BUZZER_PIN, HIGH);  // Buzzer ON
    Serial.println("Object Detected - Buzzer ON");
  } else {
    digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF
    Serial.println("No Object");
  }

  delay(100);
}

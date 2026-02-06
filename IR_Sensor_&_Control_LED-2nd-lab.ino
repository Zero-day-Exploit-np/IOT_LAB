#define IR_PIN 4      // IR sensor output pin
#define LED_PIN 2     // Built-in LED (usually GPIO 2)

void setup() {
  pinMode(IR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int irValue = digitalRead(IR_PIN);

  if (irValue == LOW) {        // Object detected (most IR sensors)
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Object Detected");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("No Object");
  }

  delay(200);
}

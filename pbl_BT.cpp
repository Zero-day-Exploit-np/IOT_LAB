#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Motor pins (as per your ESP32)
#define IN1 14   // Left motor
#define IN2 27
#define IN3 26   // Right motor
#define IN4 25

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_ROBOT"); // Bluetooth name

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors(); // Start stopped
}

void loop() {
  if (SerialBT.available()) {
    char cmd = SerialBT.read();
    Serial.println(cmd);

    switch (cmd) {
      case 'F': forward(); break;
      case 'B': backward(); break;
      case 'L': left(); break;
      case 'R': right(); break;
      case 'S': stopMotors(); break;
    }
  }
}

// 🔼 Forward
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// 🔽 Backward
void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ⬅ Left Turn
void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// ➡ Right Turn
void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ⏹ Stop
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

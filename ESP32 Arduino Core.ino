#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// ============================================================
// WIFI CONFIGURATION
// ============================================================

const char* ssid = "sonu";
const char* password = "123456789";

// Current Wi-Fi network
IPAddress local_IP(172, 17, 40, 50);
IPAddress gateway(172, 17, 40, 135);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// ============================================================
// L298N MOTOR PINS
// ============================================================

#define ENA 14
#define IN1 27
#define IN2 26

#define ENB 12
#define IN3 25
#define IN4 33

// ============================================================
// SENSOR PINS
// ============================================================

#define DHTPIN 4
#define DHTTYPE DHT11

#define TRIG_PIN 16
#define ECHO_PIN 17

#define MQ_PIN 35
#define WATER_PIN 34

DHT dht(DHTPIN, DHTTYPE);

// ============================================================
// MOTOR SPEED
// ============================================================

int motorSpeed = 180;

// ============================================================
// SENSOR VALUES
// ============================================================

float temperature = 0.0;
float humidity = 0.0;
float distance = 0.0;

int mqValue = 0;
int waterValue = 0;

// ============================================================
// CORS
// ============================================================

void enableCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

// ============================================================
// MOTOR STOP
// ============================================================

void stopMotors() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

// ============================================================
// MOTOR FORWARD
// ============================================================

void forward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  ledcWrite(ENA, motorSpeed);
  ledcWrite(ENB, motorSpeed);
}

// ============================================================
// MOTOR BACKWARD
// ============================================================

void backward() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  ledcWrite(ENA, motorSpeed);
  ledcWrite(ENB, motorSpeed);
}

// ============================================================
// MOTOR LEFT
// ============================================================

void left() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  ledcWrite(ENA, motorSpeed);
  ledcWrite(ENB, motorSpeed);
}

// ============================================================
// MOTOR RIGHT
// ============================================================

void right() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  ledcWrite(ENA, motorSpeed);
  ledcWrite(ENB, motorSpeed);
}

// ============================================================
// ULTRASONIC SENSOR
// ============================================================

float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  float d = duration * 0.0343 / 2.0;

  return d;
}

// ============================================================
// READ SENSORS
// ============================================================

void readSensors() {

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) {
    temperature = t;
  }

  if (!isnan(h)) {
    humidity = h;
  }

  distance = readDistance();

  mqValue = analogRead(MQ_PIN);

  waterValue = analogRead(WATER_PIN);
}

// ============================================================
// ROOT PAGE
// ============================================================

void handleRoot() {

  enableCORS();

  String html;

  html += "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32 Robot</title>";
  html += "</head>";

  html += "<body>";

  html += "<h1>ESP32 Robot Motor Controller</h1>";

  html += "<p>IP Address: ";
  html += WiFi.localIP().toString();
  html += "</p>";

  html += "<p>WiFi RSSI: ";
  html += String(WiFi.RSSI());
  html += " dBm</p>";

  html += "<p>Server: Running</p>";

  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

// ============================================================
// FORWARD
// ============================================================

void handleForward() {

  forward();

  enableCORS();

  server.send(
    200,
    "text/plain",
    "FORWARD"
  );
}

// ============================================================
// BACKWARD
// ============================================================

void handleBackward() {

  backward();

  enableCORS();

  server.send(
    200,
    "text/plain",
    "BACKWARD"
  );
}

// ============================================================
// LEFT
// ============================================================

void handleLeft() {

  left();

  enableCORS();

  server.send(
    200,
    "text/plain",
    "LEFT"
  );
}

// ============================================================
// RIGHT
// ============================================================

void handleRight() {

  right();

  enableCORS();

  server.send(
    200,
    "text/plain",
    "RIGHT"
  );
}

// ============================================================
// STOP
// ============================================================

void handleStop() {

  stopMotors();

  enableCORS();

  server.send(
    200,
    "text/plain",
    "STOP"
  );
}

// ============================================================
// MOTOR SPEED
// ============================================================

void handleSpeed() {

  if (server.hasArg("v")) {

    motorSpeed = constrain(
      server.arg("v").toInt(),
      0,
      255
    );
  }

  enableCORS();

  server.send(
    200,
    "text/plain",
    String(motorSpeed)
  );
}

// ============================================================
// SENSOR DATA
// ============================================================

void handleData() {

  readSensors();

  enableCORS();

  String json = "{";

  json += "\"distance\":";
  json += String(distance, 1);

  json += ",\"temperature\":";
  json += String(temperature, 1);

  json += ",\"humidity\":";
  json += String(humidity, 1);

  json += ",\"mq\":";
  json += String(mqValue);

  json += ",\"water\":";
  json += String(waterValue);

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// ============================================================
// STATUS
// ============================================================

void handleStatus() {

  enableCORS();

  String json = "{";

  json += "\"device\":\"ESP32-MOTOR\",";

  json += "\"type\":\"MOTOR_SENSOR\",";

  json += "\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",";

  json += "\"gateway\":\"";
  json += WiFi.gatewayIP().toString();
  json += "\",";

  json += "\"ssid\":\"";
  json += WiFi.SSID();
  json += "\",";

  json += "\"bssid\":\"";
  json += WiFi.BSSIDstr();
  json += "\",";

  json += "\"channel\":";
  json += String(WiFi.channel());

  json += ",";

  json += "\"rssi\":";
  json += String(WiFi.RSSI());

  json += ",";

  json += "\"uptime\":";
  json += String(millis() / 1000);

  json += ",";

  json += "\"motorSpeed\":";
  json += String(motorSpeed);

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// ============================================================
// WIFI CONNECTION
// ============================================================

void connectWiFi() {

  WiFi.mode(WIFI_STA);

  WiFi.setSleep(false);

  // Configure static IP
  if (!WiFi.config(
        local_IP,
        gateway,
        subnet
      )) {

    Serial.println(
      "Static IP configuration failed"
    );
  }

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32 MOTOR ROBOT");
  Serial.println("==============================");

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(
    ssid,
    password
  );

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 40
  ) {

    delay(500);

    Serial.print(".");

    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi connected");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.print("Channel: ");
    Serial.println(WiFi.channel());

  } else {

    Serial.println("WiFi connection FAILED");

  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  // ----------------------------------------------------------
  // MOTOR PINS
  // ----------------------------------------------------------

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ----------------------------------------------------------
  // PWM
  // ESP32 Arduino Core 3.x
  // ----------------------------------------------------------

  ledcAttach(
    ENA,
    1000,
    8
  );

  ledcAttach(
    ENB,
    1000,
    8
  );

  stopMotors();

  // ----------------------------------------------------------
  // DHT11
  // ----------------------------------------------------------

  dht.begin();

  // ----------------------------------------------------------
  // HC-SR04
  // ----------------------------------------------------------

  pinMode(
    TRIG_PIN,
    OUTPUT
  );

  pinMode(
    ECHO_PIN,
    INPUT
  );

  // ----------------------------------------------------------
  // MQ SENSOR
  // ----------------------------------------------------------

  pinMode(
    MQ_PIN,
    INPUT
  );

  // ----------------------------------------------------------
  // WATER SENSOR
  // ----------------------------------------------------------

  pinMode(
    WATER_PIN,
    INPUT
  );

  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  connectWiFi();

  // ----------------------------------------------------------
  // HTTP ROUTES
  // ----------------------------------------------------------

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/F",
    handleForward
  );

  server.on(
    "/B",
    handleBackward
  );

  server.on(
    "/L",
    handleLeft
  );

  server.on(
    "/R",
    handleRight
  );

  server.on(
    "/S",
    handleStop
  );

  server.on(
    "/speed",
    handleSpeed
  );

  server.on(
    "/data",
    handleData
  );

  server.on(
    "/status",
    handleStatus
  );

  // ----------------------------------------------------------
  // START SERVER
  // ----------------------------------------------------------

  server.begin();

  Serial.println();
  Serial.println("Motor server started");

  Serial.print("Open in browser: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  Serial.println("==============================");
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  server.handleClient();

  // Reconnect if WiFi disconnects
  if (WiFi.status() != WL_CONNECTED) {

    WiFi.reconnect();

    delay(1000);
  }
}

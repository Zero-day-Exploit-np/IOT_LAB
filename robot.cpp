#include <WiFi.h>
#include <WebServer.h>

// Replace with your WiFi
const char* ssid = "______";
const char* password = "_______";

WebServer server(80);

// Motor pins
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12

// HTML Page
String webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Car</title>
<style>
button {width:100px;height:50px;font-size:18px;margin:5px;}
</style>
</head>
<body style="text-align:center;">
<h2>ESP32 WiFi Car</h2>
<button onclick="send('F')">Forward</button><br>
<button onclick="send('L')">Left</button>
<button onclick="send('S')">Stop</button>
<button onclick="send('R')">Right</button><br>
<button onclick="send('B')">Backward</button>

<script>
function send(cmd){
  fetch("/" + cmd);
}
</script>
</body>
</html>
)rawliteral";

// Movement functions
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/", []() {
    server.send(200, "text/html", webpage);
  });

  server.on("/F", []() { forward(); server.send(200, "text/plain", "Forward"); });
  server.on("/B", []() { backward(); server.send(200, "text/plain", "Backward"); });
  server.on("/L", []() { left(); server.send(200, "text/plain", "Left"); });
  server.on("/R", []() { right(); server.send(200, "text/plain", "Right"); });
  server.on("/S", []() { stopCar(); server.send(200, "text/plain", "Stop"); });

  server.begin();
}

void loop() {
  server.handleClient();
}

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "sonu";
const char* password = "123456789";

// Static IP
IPAddress local_IP(192,168,1,50);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

// Motor pins
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25

// PWM pins
#define ENA 4
#define ENB 2

int speedValue = 200;

// ---------------- MOTOR FUNCTIONS ----------------
void setSpeed(int spd){
  speedValue = constrain(spd,0,255);
  ledcWrite(ENA, speedValue);
  ledcWrite(ENB, speedValue);
  Serial.print("Speed: ");
  Serial.println(speedValue);
}

void stopMotors(){
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
  Serial.println("STOP");
}

void forward(){
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  Serial.println("FORWARD");
}

void backward(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  Serial.println("BACKWARD");
}

void left(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  Serial.println("LEFT");
}

void right(){
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  Serial.println("RIGHT");
}

// ---------------- SETUP ----------------
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);

  // PWM setup (NEW ESP32 CORE)
  ledcAttach(ENA, 5000, 8);
  ledcAttach(ENB, 5000, 8);

  setSpeed(200);

  Serial.println("Connecting WiFi...");
WiFi.begin(ssid, password);

while(WiFi.status()!=WL_CONNECTED){
  Serial.print(".");
  delay(500);
}

Serial.println("\nWiFi Connected!");
Serial.print("IP: ");
Serial.println(WiFi.localIP());
  // ---------------- ROUTES ----------------
  server.on("/", [](){
    server.send(200, "text/html", R"rawliteral(
      <h2>Robot Control</h2>
      <button onclick="fetch('/F')">Forward</button><br><br>
      <button onclick="fetch('/L')">Left</button>
      <button onclick="fetch('/S')">Stop</button>
      <button onclick="fetch('/R')">Right</button><br><br>
      <button onclick="fetch('/B')">Backward</button><br><br>
      Speed:
      <input type="range" min="0" max="255" value="200"
      oninput="fetch('/speed?v='+this.value)">
    )rawliteral");
  });

  server.on("/F", [](){ forward(); server.send(200,"text/plain","OK"); });
  server.on("/B", [](){ backward(); server.send(200,"text/plain","OK"); });
  server.on("/L", [](){ left(); server.send(200,"text/plain","OK"); });
  server.on("/R", [](){ right(); server.send(200,"text/plain","OK"); });
  server.on("/S", [](){ stopMotors(); server.send(200,"text/plain","OK"); });

  server.on("/speed", [](){
    if(server.hasArg("v")){
      setSpeed(server.arg("v").toInt());
    }
    server.send(200,"text/plain","OK");
  });

  server.begin();
  Serial.println("Server started");
}

// ---------------- LOOP ----------------
void loop(){
  server.handleClient();
}

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

const char* ssid = "sonu";
const char* password = "123456789";

AsyncWebServer server(80);

// Static IP
IPAddress local_IP(192,168,137,50);
IPAddress gateway(192,168,137,1);
IPAddress subnet(255,255,255,0);

// Motor pins
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25
#define ENA 33
#define ENB 32

int speedValue = 200;

// -------- MOTOR --------
void setSpeed(int spd){
  speedValue = constrain(spd,0,255);
  ledcWrite(ENA, speedValue);
  ledcWrite(ENB, speedValue);
}

void stopMotors(){
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
}

void forward(){
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

void backward(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
}

void left(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
}

void right(){
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
}

// -------- SETUP --------
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);

  // PWM (ESP32 core v3+)
  ledcAttach(ENA, 5000, 8);
  ledcAttach(ENB, 5000, 8);
  setSpeed(200);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
  }

  Serial.println("Robot Ready");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/F", HTTP_GET, [](AsyncWebServerRequest *req){ forward(); req->send(200,"text/plain","OK"); });
  server.on("/B", HTTP_GET, [](AsyncWebServerRequest *req){ backward(); req->send(200,"text/plain","OK"); });
  server.on("/L", HTTP_GET, [](AsyncWebServerRequest *req){ left(); req->send(200,"text/plain","OK"); });
  server.on("/R", HTTP_GET, [](AsyncWebServerRequest *req){ right(); req->send(200,"text/plain","OK"); });
  server.on("/S", HTTP_GET, [](AsyncWebServerRequest *req){ stopMotors(); req->send(200,"text/plain","OK"); });

  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *req){
    if(req->hasParam("v")){
      setSpeed(req->getParam("v")->value().toInt());
    }
    req->send(200,"text/plain","OK");
  });

  server.begin();
}

void loop(){}

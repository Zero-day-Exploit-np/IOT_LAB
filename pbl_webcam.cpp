#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

WebServer server(80);

// Motor pins
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25

#define ENA 33
#define ENB 32

int speedValue = 200; // 0–255

// PWM setup
const int freq = 1000;
const int channelA = 0;
const int channelB = 1;
const int resolution = 8;

void setSpeed(int spd){
  speedValue = constrain(spd, 0, 255);
  ledcWrite(channelA, speedValue);
  ledcWrite(channelB, speedValue);
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

void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);

  // PWM setup
  ledcSetup(channelA, freq, resolution);
  ledcSetup(channelB, freq, resolution);

  ledcAttachPin(ENA, channelA);
  ledcAttachPin(ENB, channelB);

  setSpeed(speedValue);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  Serial.println(WiFi.localIP());

  // Fast control endpoints
  server.on("/F", [](){ forward(); server.send(200,"text",""); });
  server.on("/B", [](){ backward(); server.send(200,"text",""); });
  server.on("/L", [](){ left(); server.send(200,"text",""); });
  server.on("/R", [](){ right(); server.send(200,"text",""); });
  server.on("/S", [](){ stopMotors(); server.send(200,"text",""); });

  // Speed control
  server.on("/speed", [](){
    if(server.hasArg("v")){
      int v = server.arg("v").toInt();
      setSpeed(v);
    }
    server.send(200,"text","");
  });

  server.begin();
}

void loop(){
  server.handleClient();
}

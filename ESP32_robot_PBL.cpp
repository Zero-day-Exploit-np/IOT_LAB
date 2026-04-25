/*
 * ESP32 Robot — Motor Controller (ESP32 DevKit V1)
 * -------------------------------------------------
 * Controls 2 DC motors via L298N driver using hardware LEDC PWM.
 * Includes per-motor trim calibration for straight-line driving.
 *
 * LEDC API version:  ESP32 Arduino Core 2.x
 *   If you're on Core 3.x, see the comments marked "CORE 3.x" below.
 *
 * Wiring (L298N):
 *   ENA → GPIO 14  (PWM speed left)    — remove jumper on L298N!
 *   IN1 → GPIO 27  (left forward)
 *   IN2 → GPIO 26  (left backward)
 *   ENB → GPIO 12  (PWM speed right)   — remove jumper on L298N!
 *   IN3 → GPIO 25  (right forward)
 *   IN4 → GPIO 33  (right backward)
 *   Servo → GPIO 13
 *
 * Power:
 *   3S Li-ion (11.1V) → L298N "12V" terminal
 *   L298N GND → ESP32 GND  (COMMON GROUND — mandatory!)
 *   L298N 5V  → ESP32 VIN  (powers ESP32 from L298N regulator)
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// ===================================================================
//  WIFI CONFIGURATION
// ===================================================================
const char* ssid     = "sonu";
const char* password = "123456789";

IPAddress local_IP(192, 168, 137, 50);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);

// ===================================================================
//  MOTOR PINS (L298N)
// ===================================================================
//  Motor A = Left wheels
//  Motor B = Right wheels
#define ENA  14    // PWM speed left
#define IN1  27    // left forward
#define IN2  26    // left backward
#define ENB  12    // PWM speed right
#define IN3  25    // right forward
#define IN4  33    // right backward

// ===================================================================
//  LEDC PWM CONFIGURATION
// ===================================================================
//  5 kHz is the sweet spot for L298N (BJT H-bridge).
//  Too low = audible whine.  Too high = excessive heat in L298N.
#define PWM_FREQ        5000   // Hz
#define PWM_RESOLUTION  8      // 8-bit → duty 0–255

// Core 2.x: channels must be assigned manually
#define LEDC_CH_ENA  0   // channel for left motor
#define LEDC_CH_ENB  1   // channel for right motor
#define LEDC_CH_SERVO 2  // channel for servo

// ===================================================================
//  SERVO CONFIGURATION (raw LEDC — no library needed)
// ===================================================================
#define SERVO_PIN     13
#define SERVO_FREQ    50       // 50 Hz = 20 ms period (standard servo)
#define SERVO_RES     16       // 16-bit for fine angle control
// At 50 Hz / 16-bit:  1 tick = 20000 µs / 65536 ≈ 0.305 µs
// 500 µs  (0°)   → 500  / 0.305 ≈ 1638
// 2500 µs (180°) → 2500 / 0.305 ≈ 8192
#define SERVO_MIN_DUTY 1638
#define SERVO_MAX_DUTY 8192

volatile int targetAngle  = 90;
int          currentAngle = 90;
unsigned long lastServo   = 0;

// ===================================================================
//  MOTOR SPEED & TRIM CALIBRATION
// ===================================================================
//  Base speed set by slider (0–255)
volatile int motorSpeed = 200;

//  Trim multipliers: reduce the FASTER motor's trim below 1.0
//  until the robot drives perfectly straight.
//
//  HOW TO CALIBRATE:
//    1. Put robot on floor, send /F
//    2. If it drifts LEFT  → left motor is slower  → reduce RIGHT_TRIM
//    3. If it drifts RIGHT → right motor is slower → reduce LEFT_TRIM
//    4. Use /trim?l=0.92&r=1.0  to adjust live (no reflash needed)
//    5. Once found, hardcode the values here for permanent fix
volatile float leftTrim  = 1.00;
volatile float rightTrim = 1.00;

//  Anti-stall: motors typically can't spin below ~60 PWM.
//  Below this threshold we snap to 0 (full stop).
#define MIN_PWM 60

// ===================================================================
//  WATCHDOG — auto-stop if no command received
// ===================================================================
volatile unsigned long lastCmdMs = 0;
#define CMD_TIMEOUT 600   // ms

// ===================================================================
//  MOTOR FUNCTIONS (LEDC PWM)
// ===================================================================
void motorA(int dir, int spd) {
    // Apply left trim and clamp
    int trimmed = (int)(spd * leftTrim);
    trimmed = constrain(trimmed, 0, 255);
    if (trimmed > 0 && trimmed < MIN_PWM) trimmed = MIN_PWM;

    // ★ Set direction pins BEFORE applying PWM enable
    //   L298N requires stable IN1/IN2 before ENx is driven.
    digitalWrite(IN1, dir ==  1 ? HIGH : LOW);
    digitalWrite(IN2, dir == -1 ? HIGH : LOW);
    delayMicroseconds(50);  // let H-bridge direction settle
    ledcWrite(ENA, dir == 0 ? 0 : trimmed);
}

void motorB(int dir, int spd) {
    int trimmed = (int)(spd * rightTrim);
    trimmed = constrain(trimmed, 0, 255);
    if (trimmed > 0 && trimmed < MIN_PWM) trimmed = MIN_PWM;

    // ★ Set direction pins BEFORE applying PWM enable
    digitalWrite(IN3, dir ==  1 ? HIGH : LOW);
    digitalWrite(IN4, dir == -1 ? HIGH : LOW);
    delayMicroseconds(50);  // let H-bridge direction settle
    ledcWrite(ENB, dir == 0 ? 0 : trimmed);
}

void forward()   {
    Serial.printf("FWD: spd=%d L_trim=%.2f R_trim=%.2f\n", motorSpeed, leftTrim, rightTrim);
    motorA( 1, motorSpeed); motorB( 1, motorSpeed);
}
void backward()  { motorA(-1, motorSpeed); motorB(-1, motorSpeed); }
void turnLeft()  { motorA(-1, motorSpeed); motorB( 1, motorSpeed); }
void turnRight() { motorA( 1, motorSpeed); motorB(-1, motorSpeed); }
void stopAll()   {
    // Explicitly clear direction pins AND PWM for a clean stop
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    ledcWrite(ENA, 0);
    ledcWrite(ENB, 0);
}

// ===================================================================
//  SERVO — smooth non-blocking movement via raw LEDC
// ===================================================================
int angleToDuty(int angle) {
    // Map 0–180° to SERVO_MIN_DUTY–SERVO_MAX_DUTY
    angle = constrain(angle, 0, 180);
    return map(angle, 0, 180, SERVO_MIN_DUTY, SERVO_MAX_DUTY);
}

void servoWrite(int angle) {
    // Core 3.x:  ledcWrite(pin, duty)
    ledcWrite(SERVO_PIN, angleToDuty(angle));
}

void updateServo() {
    if (millis() - lastServo > 12) {
        if      (currentAngle < targetAngle) currentAngle++;
        else if (currentAngle > targetAngle) currentAngle--;
        servoWrite(currentAngle);
        lastServo = millis();
    }
}

// ===================================================================
//  COMMAND HANDLER
// ===================================================================
void doCmd(const String& c) {
    lastCmdMs = millis();
    Serial.println("CMD: " + c);

    if      (c == "F") { forward();   targetAngle = 90;  }
    else if (c == "B") { backward();  targetAngle = 90;  }
    else if (c == "L") { turnLeft();  targetAngle = 140; }
    else if (c == "R") { turnRight(); targetAngle = 40;  }
    else if (c == "S") { stopAll();   targetAngle = 90;  }
}

// ===================================================================
//  WATCHDOG TASK (Core 0) — auto-stop on disconnect
// ===================================================================
void watchdogTask(void* pv) {
    while (true) {
        if (millis() - lastCmdMs > CMD_TIMEOUT) {
            stopAll();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// ===================================================================
//  SETUP
// ===================================================================
void setup() {
    Serial.begin(115200);

    // ---- Direction pins (digital output) ----
    //  Initialize direction pins FIRST and set LOW before enabling PWM.
    //  This prevents motor glitches on boot.
    pinMode(IN1, OUTPUT); digitalWrite(IN1, LOW);
    pinMode(IN2, OUTPUT); digitalWrite(IN2, LOW);
    pinMode(IN3, OUTPUT); digitalWrite(IN3, LOW);
    pinMode(IN4, OUTPUT); digitalWrite(IN4, LOW);

    // ---- ENA/ENB pins: ensure LOW before LEDC takes over ----
    pinMode(ENA, OUTPUT); digitalWrite(ENA, LOW);
    pinMode(ENB, OUTPUT); digitalWrite(ENB, LOW);
    delay(10);  // let pins settle

    // ---- LEDC PWM setup (Core 3.x API) ----
    //  Core 3.x uses simplified API: ledcAttach(pin, freq, resolution)
    //  No manual channel assignment needed
    ledcAttach(ENA, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(ENB, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(ENA, 0);
    ledcWrite(ENB, 0);

    // ---- Servo via LEDC (no library — avoids channel conflicts) ----
    ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RES);
    servoWrite(90);

    stopAll();

    Serial.println("Motor pins initialized:");
    Serial.printf("  ENA=%d IN1=%d IN2=%d (Left)\n", ENA, IN1, IN2);
    Serial.printf("  ENB=%d IN3=%d IN4=%d (Right)\n", ENB, IN3, IN4);

    // ---- WiFi ----
    WiFi.setSleep(false);                   // disable modem sleep for fast response
    WiFi.setTxPower(WIFI_POWER_19_5dBm);   // max range
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Robot IP: ");
    Serial.println(WiFi.localIP());

    // ---- HTTP ROUTES ----

    // Movement commands — MIT App Inventor compatible
    server.on("/F", HTTP_GET, [](AsyncWebServerRequest* req) {
        doCmd("F"); req->send(200, "text/plain", "OK");
    });
    server.on("/B", HTTP_GET, [](AsyncWebServerRequest* req) {
        doCmd("B"); req->send(200, "text/plain", "OK");
    });
    server.on("/L", HTTP_GET, [](AsyncWebServerRequest* req) {
        doCmd("L"); req->send(200, "text/plain", "OK");
    });
    server.on("/R", HTTP_GET, [](AsyncWebServerRequest* req) {
        doCmd("R"); req->send(200, "text/plain", "OK");
    });
    server.on("/S", HTTP_GET, [](AsyncWebServerRequest* req) {
        doCmd("S"); req->send(200, "text/plain", "OK");
    });

    // Speed slider:  /speed?v=150
    server.on("/speed", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (req->hasParam("v")) {
            int v = req->getParam("v")->value().toInt();
            motorSpeed = constrain(v, MIN_PWM, 255);
            Serial.println("Speed: " + String(motorSpeed));
        }
        req->send(200, "text/plain", "OK");
    });

    // ★ NEW: Live trim adjustment:  /trim?l=0.92&r=1.0
    //   Adjust until robot drives straight, then hardcode the values.
    server.on("/trim", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (req->hasParam("l")) {
            leftTrim = constrain(req->getParam("l")->value().toFloat(), 0.5, 1.0);
        }
        if (req->hasParam("r")) {
            rightTrim = constrain(req->getParam("r")->value().toFloat(), 0.5, 1.0);
        }
        String msg = "Trim L=" + String(leftTrim, 2) + " R=" + String(rightTrim, 2);
        Serial.println(msg);
        req->send(200, "text/plain", msg);
    });

    // Status (JSON)
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        String json = "{\"speed\":" + String(motorSpeed) +
                      ",\"leftTrim\":" + String(leftTrim, 2) +
                      ",\"rightTrim\":" + String(rightTrim, 2) +
                      ",\"servo\":" + String(currentAngle) +
                      ",\"uptime\":" + String(millis() / 1000) + "}";
        req->send(200, "application/json", json);
    });

    // Root
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/plain", "Robot OK - " + WiFi.localIP().toString());
    });

    server.begin();
    Serial.println("HTTP server started");

    // ---- Watchdog on Core 0 ----
    xTaskCreatePinnedToCore(watchdogTask, "wdog", 2048, NULL, 1, NULL, 0);
}

// ===================================================================
//  LOOP (Core 1 — servo smoothing only)
// ===================================================================
void loop() {
    updateServo();
}

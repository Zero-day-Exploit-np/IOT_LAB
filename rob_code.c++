#include <WiFi.h>
#include <WebServer.h>

// ================================================================
// WIFI CONFIGURATION
// ================================================================

const char* ssid     = "sonu";
const char* password = "123456789";

// Confirmed network
IPAddress local_IP(10, 78, 24, 50);
IPAddress gateway(10, 78, 24, 221);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// ================================================================
// MOTOR PINS
// ================================================================

// LEFT MOTOR
#define ENA 14
#define IN1 27
#define IN2 26

// RIGHT MOTOR
#define ENB 12
#define IN3 25
#define IN4 33

// ================================================================
// PWM
// ================================================================

#define PWM_FREQ 20000
#define PWM_RES  8

// ================================================================
// MOTOR SETTINGS
// ================================================================

int motorSpeed = 180;

// Motor balancing
float leftFactor  = 1.00;
float rightFactor = 1.00;

// ================================================================
// CORS
// ================================================================

void addCORS()
{
    server.sendHeader(
        "Access-Control-Allow-Origin",
        "*"
    );

    server.sendHeader(
        "Access-Control-Allow-Methods",
        "GET, OPTIONS"
    );

    server.sendHeader(
        "Access-Control-Allow-Headers",
        "Content-Type"
    );
}

// ================================================================
// LEFT MOTOR
// ================================================================

void leftMotor(int direction, int speed)
{
    speed = constrain(speed, 0, 255);

    int pwm = (int)(speed * leftFactor);
    pwm = constrain(pwm, 0, 255);

    if (direction > 0)
    {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
    }
    else if (direction < 0)
    {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
    }
    else
    {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        pwm = 0;
    }

    ledcWrite(ENA, pwm);
}

// ================================================================
// RIGHT MOTOR
// ================================================================

void rightMotor(int direction, int speed)
{
    speed = constrain(speed, 0, 255);

    int pwm = (int)(speed * rightFactor);
    pwm = constrain(pwm, 0, 255);

    if (direction > 0)
    {
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    }
    else if (direction < 0)
    {
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    }
    else
    {
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        pwm = 0;
    }

    ledcWrite(ENB, pwm);
}

// ================================================================
// MOVEMENT
// ================================================================

void forward()
{
    leftMotor(1, motorSpeed);
    rightMotor(1, motorSpeed);
}

void backward()
{
    leftMotor(-1, motorSpeed);
    rightMotor(-1, motorSpeed);
}

void turnLeft()
{
    leftMotor(-1, motorSpeed);
    rightMotor(1, motorSpeed);
}

void turnRight()
{
    leftMotor(1, motorSpeed);
    rightMotor(-1, motorSpeed);
}

void stopRobot()
{
    leftMotor(0, 0);
    rightMotor(0, 0);
}

// ================================================================
// HTTP COMMAND HANDLERS
// ================================================================

void handleForward()
{
    addCORS();

    forward();

    server.send(
        200,
        "text/plain",
        "F"
    );
}

void handleBackward()
{
    addCORS();

    backward();

    server.send(
        200,
        "text/plain",
        "B"
    );
}

void handleLeft()
{
    addCORS();

    turnLeft();

    server.send(
        200,
        "text/plain",
        "L"
    );
}

void handleRight()
{
    addCORS();

    turnRight();

    server.send(
        200,
        "text/plain",
        "R"
    );
}

void handleStop()
{
    addCORS();

    stopRobot();

    server.send(
        200,
        "text/plain",
        "S"
    );
}

// ================================================================
// SPEED
// ================================================================

void handleSpeed()
{
    addCORS();

    if (server.hasArg("v"))
    {
        motorSpeed = constrain(
            server.arg("v").toInt(),
            0,
            255
        );
    }

    server.send(
        200,
        "text/plain",
        String(motorSpeed)
    );
}

// ================================================================
// STATUS
// ================================================================

void handleStatus()
{
    String json = "{";

    json += "\"ip\":\"";
    json += WiFi.localIP().toString();
    json += "\",";

    json += "\"gateway\":\"";
    json += WiFi.gatewayIP().toString();
    json += "\",";

    json += "\"subnet\":\"";
    json += WiFi.subnetMask().toString();
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

    json += "\"speed\":";
    json += String(motorSpeed);
    json += ",";

    json += "\"uptime\":";
    json += String(millis());

    json += "}";

    addCORS();

    server.send(
        200,
        "application/json",
        json
    );
}

// ================================================================
// ROOT
// ================================================================

void handleRoot()
{
    addCORS();

    server.send(
        200,
        "text/plain",
        "ESP32 Robot Controller OK"
    );
}

// ================================================================
// OPTIONS / CORS PREFLIGHT
// ================================================================

void handleOptions()
{
    addCORS();

    server.send(
        204,
        "text/plain",
        ""
    );
}

// ================================================================
// ROUTES
// ================================================================

void setupRoutes()
{
    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );

    server.on(
        "/F",
        HTTP_GET,
        handleForward
    );

    server.on(
        "/B",
        HTTP_GET,
        handleBackward
    );

    server.on(
        "/L",
        HTTP_GET,
        handleLeft
    );

    server.on(
        "/R",
        HTTP_GET,
        handleRight
    );

    server.on(
        "/S",
        HTTP_GET,
        handleStop
    );

    server.on(
        "/speed",
        HTTP_GET,
        handleSpeed
    );

    server.on(
        "/status",
        HTTP_GET,
        handleStatus
    );

    server.on(
        "/status",
        HTTP_OPTIONS,
        handleOptions
    );

    server.onNotFound(
        []()
        {
            addCORS();

            server.send(
                404,
                "text/plain",
                "Not Found"
            );
        }
    );
}

// ================================================================
// WIFI CONNECTION
// ================================================================

bool connectWiFi()
{
    WiFi.mode(WIFI_STA);

    // Disable WiFi sleep for faster response
    WiFi.setSleep(false);

    // Static IP
    if (!WiFi.config(
        local_IP,
        gateway,
        subnet
    ))
    {
        Serial.println(
            "Static IP configuration FAILED"
        );
    }
    else
    {
        Serial.println(
            "Static IP configuration OK"
        );
    }

    Serial.println();
    Serial.print(
        "Connecting to WiFi: "
    );
    Serial.println(ssid);

    WiFi.begin(
        ssid,
        password
    );

    unsigned long startTime = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - startTime < 20000
    )
    {
        delay(250);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(
            "WiFi connection FAILED"
        );

        return false;
    }

    Serial.println();
    Serial.println(
        "================================"
    );
    Serial.println(
        "       WIFI CONNECTED"
    );
    Serial.println(
        "================================"
    );

    Serial.print("SSID:     ");
    Serial.println(WiFi.SSID());

    Serial.print("BSSID:    ");
    Serial.println(WiFi.BSSIDstr());

    Serial.print("Channel:  ");
    Serial.println(WiFi.channel());

    Serial.print("IP:       ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway:  ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet:   ");
    Serial.println(WiFi.subnetMask());

    Serial.print("RSSI:     ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.println(
        "================================"
    );

    return true;
}

// ================================================================
// SETUP
// ================================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println(
        "================================"
    );
    Serial.println(
        "      ESP32 ROBOT CONTROLLER"
    );
    Serial.println(
        "================================"
    );

    // ------------------------------------------------------------
    // Motor direction pins
    // ------------------------------------------------------------

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    // ------------------------------------------------------------
    // PWM
    // ESP32 Arduino Core 3.x
    // ------------------------------------------------------------

    ledcAttach(
        ENA,
        PWM_FREQ,
        PWM_RES
    );

    ledcAttach(
        ENB,
        PWM_FREQ,
        PWM_RES
    );

    // ------------------------------------------------------------
    // Safety: motors OFF
    // ------------------------------------------------------------

    stopRobot();

    // ------------------------------------------------------------
    // WiFi
    // ------------------------------------------------------------

    connectWiFi();

    // ------------------------------------------------------------
    // HTTP server
    // ------------------------------------------------------------

    setupRoutes();

    server.begin();

    Serial.println();
    Serial.println(
        "HTTP SERVER STARTED"
    );

    Serial.print(
        "Robot URL: http://"
    );
    Serial.println(
        WiFi.localIP()
    );

    Serial.println();
    Serial.println(
        "Available commands:"
    );

    Serial.println(
        "  /F          Forward"
    );

    Serial.println(
        "  /B          Backward"
    );

    Serial.println(
        "  /L          Left"
    );

    Serial.println(
        "  /R          Right"
    );

    Serial.println(
        "  /S          Stop"
    );

    Serial.println(
        "  /speed?v=   Speed"
    );

    Serial.println(
        "  /status     Status"
    );

    Serial.println(
        "================================"
    );
}

// ================================================================
// LOOP
// ================================================================

void loop()
{
    server.handleClient();

    // ------------------------------------------------------------
    // WiFi lost = STOP ROBOT
    // ------------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        static unsigned long lastReconnect = 0;

        // Safety first
        stopRobot();

        if (
            millis() - lastReconnect > 5000
        )
        {
            lastReconnect = millis();

            Serial.println();
            Serial.println(
                "WiFi disconnected."
            );

            Serial.println(
                "Reconnecting..."
            );

            WiFi.disconnect();

            WiFi.begin(
                ssid,
                password
            );
        }
    }
}

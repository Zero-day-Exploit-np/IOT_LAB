#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"
#include <ESP32Servo.h>

// ============================================================
// WIFI
// ============================================================

const char* ssid = "sonu";
const char* password = "123456789";

// Static IP
IPAddress local_IP(172, 17, 40, 60);
IPAddress gateway(172, 17, 40, 135);
IPAddress subnet(255, 255, 255, 0);

// ============================================================
// SERVERS
// ============================================================

WebServer server(80);
WebSocketsServer webSocket(81);
WiFiServer streamServer(82);

// ============================================================
// SERVO
// ============================================================

#define SERVO_PIN 13

Servo cameraServo;

int servoAngle = 90;

// ============================================================
// AI-THINKER ESP32-CAM CAMERA PINS
// ============================================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============================================================
// CAMERA STATUS
// ============================================================

bool cameraOK = false;

// ============================================================
// WIFI CONNECTION
// ============================================================

void connectWiFi() {

  WiFi.mode(WIFI_STA);

  // Disable WiFi sleep for better robot response
  WiFi.setSleep(false);

  if (!WiFi.config(
        local_IP,
        gateway,
        subnet
      )) {

    Serial.println("Static IP configuration failed");
  }

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32-CAM ROBOT");
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
// CAMERA INITIALIZATION
// ============================================================

bool initCamera() {

  camera_config_t config;

  // ----------------------------------------------------------
  // Camera clock
  // ----------------------------------------------------------

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  // ----------------------------------------------------------
  // Camera data pins
  // ----------------------------------------------------------

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  // ----------------------------------------------------------
  // Camera control pins
  // ----------------------------------------------------------

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // ----------------------------------------------------------
  // Camera settings
  // ----------------------------------------------------------

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;

  // ----------------------------------------------------------
  // PSRAM configuration
  // ----------------------------------------------------------

  if (psramFound()) {

    Serial.println("PSRAM detected");

    config.frame_size = FRAMESIZE_VGA;

    config.jpeg_quality = 10;

    config.fb_count = 2;

    config.fb_location = CAMERA_FB_IN_PSRAM;

  } else {

    Serial.println("PSRAM NOT detected");

    config.frame_size = FRAMESIZE_QVGA;

    config.jpeg_quality = 12;

    config.fb_count = 1;

    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  config.grab_mode = CAMERA_GRAB_LATEST;

  // ----------------------------------------------------------
  // Initialize camera
  // ----------------------------------------------------------

  Serial.println("Initializing camera...");

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {

    Serial.print("Camera initialization failed: 0x");

    Serial.println(
      err,
      HEX
    );

    return false;
  }

  // ----------------------------------------------------------
  // Camera sensor configuration
  // ----------------------------------------------------------

  sensor_t* sensor = esp_camera_sensor_get();

  if (sensor != nullptr) {

    sensor->set_framesize(
      sensor,
      psramFound()
        ? FRAMESIZE_VGA
        : FRAMESIZE_QVGA
    );

    sensor->set_quality(
      sensor,
      psramFound()
        ? 10
        : 12
    );
  }

  Serial.println("Camera initialized successfully");

  return true;
}

// ============================================================
// SERVO
// ============================================================

void setServoAngle(int angle) {

  angle = constrain(
    angle,
    20,
    160
  );

  servoAngle = angle;

  cameraServo.write(
    servoAngle
  );

  // Send updated angle to WebSocket clients
  webSocket.broadcastTXT(
    "SERVO:" + String(servoAngle)
  );
}

// ============================================================
// WEBSOCKET EVENTS
// ============================================================

void webSocketEvent(
  uint8_t clientNum,
  WStype_t type,
  uint8_t* payload,
  size_t length
) {

  // ----------------------------------------------------------
  // Connected
  // ----------------------------------------------------------

  if (type == WStype_CONNECTED) {

    Serial.print(
      "WebSocket client connected: "
    );

    Serial.println(
      clientNum
    );

    webSocket.sendTXT(
      clientNum,
      "CONNECTED"
    );

    webSocket.sendTXT(
      clientNum,
      "SERVO:" + String(servoAngle)
    );

    return;
  }

  // ----------------------------------------------------------
  // Disconnected
  // ----------------------------------------------------------

  if (type == WStype_DISCONNECTED) {

    Serial.print(
      "WebSocket client disconnected: "
    );

    Serial.println(
      clientNum
    );

    return;
  }

  // ----------------------------------------------------------
  // Text command
  // ----------------------------------------------------------

  if (type == WStype_TEXT) {

    String command =
      String(
        (char*)payload
      );

    command.trim();

    Serial.print(
      "WebSocket command: "
    );

    Serial.println(
      command
    );

    // --------------------------------------------------------
    // LEFT
    // --------------------------------------------------------

    if (command == "LEFT") {

      setServoAngle(
        servoAngle - 10
      );
    }

    // --------------------------------------------------------
    // CENTER
    // --------------------------------------------------------

    else if (command == "CENTER") {

      setServoAngle(90);
    }

    // --------------------------------------------------------
    // RIGHT
    // --------------------------------------------------------

    else if (command == "RIGHT") {

      setServoAngle(
        servoAngle + 10
      );
    }

    // --------------------------------------------------------
    // ANGLE
    // Example: ANGLE:120
    // --------------------------------------------------------

    else if (
      command.startsWith("ANGLE:")
    ) {

      int angle =
        command.substring(6).toInt();

      setServoAngle(angle);
    }

    // --------------------------------------------------------
    // PING
    // --------------------------------------------------------

    else if (command == "PING") {

      webSocket.sendTXT(
        clientNum,
        "PONG"
      );
    }
  }
}

// ============================================================
// ROOT PAGE
// ============================================================

void handleRoot() {

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  String html;

  html += "<!DOCTYPE html>";

  html += "<html>";

  html += "<head>";

  html +=
    "<meta name='viewport' "
    "content='width=device-width,initial-scale=1'>";

  html +=
    "<title>ESP32-CAM Robot</title>";

  html += "</head>";

  html += "<body>";

  html += "<h1>ESP32-CAM Robot</h1>";

  html += "<p>IP: ";

  html +=
    WiFi.localIP().toString();

  html += "</p>";

  html += "<p>RSSI: ";

  html +=
    String(WiFi.RSSI());

  html += " dBm</p>";

  html += "<p>Servo: ";

  html +=
    String(servoAngle);

  html += " degrees</p>";

  // ----------------------------------------------------------
  // Camera status
  // ----------------------------------------------------------

  html += "<p>Camera: ";

  if (cameraOK) {

    html += "OK";

  } else {

    html += "FAILED";
  }

  html += "</p>";

  // ----------------------------------------------------------
  // Servo controls
  // ----------------------------------------------------------

  html +=
    "<p><a href='/left'>LEFT</a></p>";

  html +=
    "<p><a href='/center'>CENTER</a></p>";

  html +=
    "<p><a href='/right'>RIGHT</a></p>";

  // ----------------------------------------------------------
  // Camera stream
  // ----------------------------------------------------------

  html += "<p>";

  html += "<a href='http://";

  html +=
    WiFi.localIP().toString();

  html += ":82/stream'>";

  html += "CAMERA STREAM";

  html += "</a>";

  html += "</p>";

  html += "</body>";

  html += "</html>";

  server.send(
    200,
    "text/html",
    html
  );
}

// ============================================================
// SERVO HTTP CONTROL
// ============================================================

void handleServo() {

  if (server.hasArg("angle")) {

    int angle =
      server.arg("angle").toInt();

    setServoAngle(angle);
  }

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  server.send(
    200,
    "text/plain",
    String(servoAngle)
  );
}

// ============================================================
// LEFT
// ============================================================

void handleLeft() {

  setServoAngle(
    servoAngle - 10
  );

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  server.send(
    200,
    "text/plain",
    "LEFT"
  );
}

// ============================================================
// CENTER
// ============================================================

void handleCenter() {

  setServoAngle(90);

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  server.send(
    200,
    "text/plain",
    "CENTER"
  );
}

// ============================================================
// RIGHT
// ============================================================

void handleRight() {

  setServoAngle(
    servoAngle + 10
  );

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  server.send(
    200,
    "text/plain",
    "RIGHT"
  );
}

// ============================================================
// STATUS
// ============================================================

void handleStatus() {

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  String json = "{";

  json += "\"device\":\"ESP32-CAM\",";

  json += "\"type\":\"CAMERA_SERVO\",";

  json += "\"ip\":\"";

  json +=
    WiFi.localIP().toString();

  json += "\",";

  json += "\"gateway\":\"";

  json +=
    WiFi.gatewayIP().toString();

  json += "\",";

  json += "\"ssid\":\"";

  json +=
    WiFi.SSID();

  json += "\",";

  json += "\"bssid\":\"";

  json +=
    WiFi.BSSIDstr();

  json += "\",";

  json += "\"channel\":";

  json +=
    String(WiFi.channel());

  json += ",";

  json += "\"rssi\":";

  json +=
    String(WiFi.RSSI());

  json += ",";

  json += "\"uptime\":";

  json +=
    String(millis() / 1000);

  json += ",";

  json += "\"servoAngle\":";

  json +=
    String(servoAngle);

  json += ",";

  json += "\"camera\":";

  json +=
    cameraOK ? "true" : "false";

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// ============================================================
// CAMERA STREAM
// ============================================================
//
// IMPORTANT:
// This function receives the already-connected client.
// It does NOT call streamServer.available() again.
// ============================================================

void handleStream(WiFiClient client) {

  Serial.println(
    "Camera client connected"
  );

  // ----------------------------------------------------------
  // HTTP response
  // ----------------------------------------------------------

  client.println(
    "HTTP/1.1 200 OK"
  );

  client.println(
    "Content-Type: multipart/x-mixed-replace; boundary=frame"
  );

  client.println(
    "Cache-Control: no-cache"
  );

  client.println(
    "Pragma: no-cache"
  );

  client.println(
    "Access-Control-Allow-Origin: *"
  );

  client.println();

  // ----------------------------------------------------------
  // Send frames continuously
  // ----------------------------------------------------------

  while (
    client.connected()
  ) {

    // --------------------------------------------------------
    // Capture frame
    // --------------------------------------------------------

    camera_fb_t* fb =
      esp_camera_fb_get();

    if (!fb) {

      Serial.println(
        "Camera capture failed"
      );

      break;
    }

    // --------------------------------------------------------
    // MJPEG frame header
    // --------------------------------------------------------

    client.print(
      "--frame\r\n"
    );

    client.print(
      "Content-Type: image/jpeg\r\n"
    );

    client.print(
      "Content-Length: "
    );

    client.print(
      fb->len
    );

    client.print(
      "\r\n\r\n"
    );

    // --------------------------------------------------------
    // Send JPEG
    // --------------------------------------------------------

    size_t written =
      client.write(
        fb->buf,
        fb->len
      );

    // --------------------------------------------------------
    // Release camera frame
    // --------------------------------------------------------

    esp_camera_fb_return(
      fb
    );

    // --------------------------------------------------------
    // End frame
    // --------------------------------------------------------

    client.print(
      "\r\n"
    );

    // --------------------------------------------------------
    // Check write
    // --------------------------------------------------------

    if (
      written == 0
    ) {

      Serial.println(
        "Camera client write failed"
      );

      break;
    }

    // --------------------------------------------------------
    // Small delay
    // --------------------------------------------------------

    delay(30);

    // --------------------------------------------------------
    // Keep WebSocket alive
    // --------------------------------------------------------

    webSocket.loop();

    // --------------------------------------------------------
    // Keep HTTP server responsive
    // --------------------------------------------------------

    server.handleClient();
  }

  // ----------------------------------------------------------
  // Close client
  // ----------------------------------------------------------

  client.stop();

  Serial.println(
    "Camera client disconnected"
  );
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  // ==========================================================
  // SERVO
  // ==========================================================

  cameraServo.setPeriodHertz(50);

  cameraServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  cameraServo.write(90);

  servoAngle = 90;

  Serial.println(
    "Servo initialized"
  );

  // ==========================================================
  // CAMERA
  // ==========================================================

  cameraOK =
    initCamera();

  if (!cameraOK) {

    Serial.println();

    Serial.println(
      "WARNING: Camera initialization FAILED!"
    );

    Serial.println(
      "WiFi and WebSocket will still start."
    );
  }

  // ==========================================================
  // WIFI
  // ==========================================================

  connectWiFi();

  // ==========================================================
  // HTTP SERVER - PORT 80
  // ==========================================================

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/servo",
    handleServo
  );

  server.on(
    "/left",
    handleLeft
  );

  server.on(
    "/center",
    handleCenter
  );

  server.on(
    "/right",
    handleRight
  );

  server.on(
    "/status",
    handleStatus
  );

  server.begin();

  // ==========================================================
  // WEBSOCKET SERVER - PORT 81
  // ==========================================================

  webSocket.begin();

  webSocket.onEvent(
    webSocketEvent
  );

  // ==========================================================
  // CAMERA STREAM SERVER - PORT 82
  // ==========================================================

  streamServer.begin();

  // Allow socket reuse
  streamServer.setNoDelay(true);

  // ==========================================================
  // READY
  // ==========================================================

  Serial.println();

  Serial.println(
    "=============================="
  );

  Serial.println(
    "ESP32-CAM READY"
  );

  Serial.println(
    "=============================="
  );

  Serial.print(
    "Web page:       http://"
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.print(
    "Status:         http://"
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.println(
    "/status"
  );

  Serial.print(
    "WebSocket:      ws://"
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.println(
    ":81"
  );

  Serial.print(
    "Camera stream:  http://"
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.println(
    ":82/stream"
  );

  Serial.println(
    "=============================="
  );
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  // ==========================================================
  // NORMAL HTTP SERVER - PORT 80
  // ==========================================================

  server.handleClient();

  // ==========================================================
  // WEBSOCKET SERVER - PORT 81
  // ==========================================================

  webSocket.loop();

  // ==========================================================
  // CAMERA STREAM - PORT 82
  // ==========================================================

  WiFiClient client =
    streamServer.available();

  if (client) {

    handleStream(client);
  }

  // ==========================================================
  // WIFI RECONNECT
  // ==========================================================

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    Serial.println(
      "WiFi disconnected. Reconnecting..."
    );

    WiFi.reconnect();

    delay(1000);
  }
}

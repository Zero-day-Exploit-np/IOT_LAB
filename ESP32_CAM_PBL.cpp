/*
 * ESP32-CAM — Video Streamer (WebSocket MJPEG)
 * ----------------------------------------------
 * Streams JPEG frames from OV2640 camera over WebSocket.
 * Serves an HTML page that displays the live feed on a canvas.
 *
 * Board: AI Thinker ESP32-CAM
 * IP:    192.168.137.60 (static)
 *
 * Optimizations over original code:
 *   - More aggressive WebSocket client cleanup
 *   - Configurable frame delay
 *   - Memory-safe frame buffer handling
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// ===================================================================
//  WIFI CONFIGURATION
// ===================================================================
const char* ssid     = "sonu";
const char* password = "123456789";

IPAddress local_IP(192, 168, 137, 60);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);

// ===================================================================
//  CAMERA PINS (AI Thinker ESP32-CAM)
// ===================================================================
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// ===================================================================
//  STREAM SETTINGS
// ===================================================================
#define FRAME_DELAY_MS   80   // 80ms ≈ 12fps — realistic for ESP32-CAM WiFi throughput.
                              // Going faster causes WebSocket buffer overflow → disconnect loop.
                              // 80ms gives stable connection; 50ms may work on strong WiFi.

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ===================================================================
//  WEB PAGE — full-screen canvas with FPS HUD
// ===================================================================
const char PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>
<title>Robot Cam</title>
<style>
  *{margin:0;padding:0;box-sizing:border-box;}
  html,body{width:100%;height:100%;background:#000;}
  #c{display:block;width:100%;height:100vh;object-fit:contain;}
  #hud{position:fixed;top:6px;left:8px;color:#0f0;font-size:12px;
       font-family:monospace;pointer-events:none;
       text-shadow:1px 1px 3px #000;}
</style>
</head>
<body>
<canvas id='c'></canvas>
<div id='hud'>⏳ connecting...</div>
<script>
  const canvas = document.getElementById('c');
  const ctx    = canvas.getContext('2d');
  const hud    = document.getElementById('hud');

  function resize(){
    canvas.width  = window.innerWidth;
    canvas.height = window.innerHeight;
  }
  resize();
  window.addEventListener('resize', resize);

  let fps=0, frames=0, fpsTimer=Date.now();

  function connect(){
    const ws = new WebSocket('ws://' + location.host + '/ws');
    ws.binaryType = 'arraybuffer';

    ws.onopen  = () => hud.textContent = '📡 connected';
    ws.onclose = () => { hud.textContent = '🔴 reconnecting...'; setTimeout(connect, 800); };
    ws.onerror = () => ws.close();

    ws.onmessage = e => {
      if(!(e.data instanceof ArrayBuffer)) return;
      frames++;
      createImageBitmap(new Blob([e.data],{type:'image/jpeg'}))
        .then(bmp => { ctx.drawImage(bmp,0,0,canvas.width,canvas.height); bmp.close(); });
      const now = Date.now();
      if(now - fpsTimer >= 1000){
        hud.textContent = '📷 ' + frames + ' fps';
        frames = 0; fpsTimer = now;
      }
    };
  }
  connect();
</script>
</body>
</html>
)rawhtml";

// ===================================================================
//  CAMERA STREAM TASK (pinned to Core 0)
// ===================================================================
void camTask(void* pv) {
    while (true) {
        // No clients connected — sleep longer to save resources
        if (ws.count() == 0) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(5 / portTICK_PERIOD_MS);
            continue;
        }

        // Only send to clients that have finished consuming the previous frame.
        // This is THE key fix: binaryAll() blindly queues frames for every client.
        // If a client is slow, the queue grows until the library kills the connection.
        // By checking queueLen(), we skip slow clients instead of disconnecting them.
        if (fb->len > 0 && fb->len < 30000) {
            for (auto& c : ws.getClients()) {
                if (c.status() == WS_CONNECTED && c.queueLen() == 0) {
                    c.binary(fb->buf, fb->len);
                }
            }
        }
        esp_camera_fb_return(fb);

        vTaskDelay(FRAME_DELAY_MS / portTICK_PERIOD_MS);
    }
}

// ===================================================================
//  SETUP
// ===================================================================
void setup() {
    Serial.begin(115200);

    // ---- Camera config ----
    camera_config_t cfg;
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0       = Y2_GPIO_NUM;
    cfg.pin_d1       = Y3_GPIO_NUM;
    cfg.pin_d2       = Y4_GPIO_NUM;
    cfg.pin_d3       = Y5_GPIO_NUM;
    cfg.pin_d4       = Y6_GPIO_NUM;
    cfg.pin_d5       = Y7_GPIO_NUM;
    cfg.pin_d6       = Y8_GPIO_NUM;
    cfg.pin_d7       = Y9_GPIO_NUM;
    cfg.pin_xclk     = XCLK_GPIO_NUM;
    cfg.pin_pclk     = PCLK_GPIO_NUM;
    cfg.pin_vsync    = VSYNC_GPIO_NUM;
    cfg.pin_href     = HREF_GPIO_NUM;
    cfg.pin_sscb_sda = SIOD_GPIO_NUM;
    cfg.pin_sscb_scl = SIOC_GPIO_NUM;
    cfg.pin_pwdn     = PWDN_GPIO_NUM;
    cfg.pin_reset    = RESET_GPIO_NUM;
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.frame_size   = FRAMESIZE_QVGA;   // 320×240
    cfg.jpeg_quality = 25;               // higher number = more compression = smaller frames
                                         // 10=best quality  63=most compressed
                                         // 25 = prioritize small frames to prevent disconnect
    cfg.fb_count     = 2;                // double-buffer for smoother stream
    cfg.grab_mode    = CAMERA_GRAB_LATEST; // always send the latest frame, skip stale ones

    if (esp_camera_init(&cfg) != ESP_OK) {
        Serial.println("Camera FAIL");
        return;
    }

    // Fine-tune image sensor
    sensor_t* s = esp_camera_sensor_get();
    s->set_brightness(s, 1);
    s->set_saturation(s, 0);
    s->set_sharpness(s, 1);

    // ---- WiFi ----
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("CAM IP: ");
    Serial.println(WiFi.localIP());

    // ---- WebSocket ----
    ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type,
                  void*, uint8_t*, size_t) {
        // Optional: log connect/disconnect events
        if (type == WS_EVT_CONNECT)    Serial.println("WS client connected");
        if (type == WS_EVT_DISCONNECT) Serial.println("WS client disconnected");
    });
    server.addHandler(&ws);

    // ---- HTTP routes ----
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", PAGE);
    });

    server.begin();
    Serial.println("Camera server started");

    // ---- Stream task on Core 0 ----
    xTaskCreatePinnedToCore(camTask, "cam", 8192, NULL, 5, NULL, 0);
}

// ===================================================================
//  LOOP — WebSocket housekeeping
// ===================================================================
void loop() {
    // Clean up stale/dead WebSocket clients frequently to prevent
    // buffer buildup and memory exhaustion that causes disconnects
    static unsigned long lastClean = 0;
    if (millis() - lastClean > 1000) {
        ws.cleanupClients(2);  // keep max 2 concurrent clients
        lastClean = millis();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include <ESPmDNS.h>

// ตั้งค่า WiFi
const char* ssid = "ปลี่ยนเป็น WiFi SSID ของคุณ";          // เปลี่ยนเป็น WiFi SSID ของคุณ
const char* password = "เปลี่ยนเป็น WiFi Password ของคุณ";  // เปลี่ยนเป็น WiFi Password ของคุณ

// ตั้งค่า mDNS hostname
const char* hostname = "camera01";  // เข้าถึงผ่าน http://camera01.local

// กำหนด Camera Pins สำหรับ Freenove ESP32-S3 WROOM (OV2640)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

// LED Pin
#define LED_PIN           2

httpd_handle_t camera_httpd = NULL;
bool wifiConnected = false;

// HTML สำหรับหน้าเว็บ
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-S3 Camera Stream</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        h1 {
            color: #333;
        }
        img {
            max-width: 100%;
            height: auto;
            border: 3px solid #333;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .info {
            margin-top: 20px;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-S3 Camera Stream</h1>
        <img src="/stream" alt="Camera Stream">
        <div class="info">
            <p>กำลังแสดงภาพสดจากกล้อง ESP32-S3</p>
        </div>
    </div>
</body>
</html>
)rawliteral";

// Handler สำหรับหน้าแรก
static esp_err_t index_handler(httpd_req_t *req) {
    Serial.println("Index page requested");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

// Handler สำหรับ Stream วิดีโอ
static esp_err_t stream_handler(httpd_req_t *req) {
    Serial.println("Stream requested");
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
    static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
    static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        return res;
    }

    while(true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        if(fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            esp_camera_fb_return(fb);
            fb = NULL;
            if(!jpeg_converted) {
                Serial.println("JPEG compression failed");
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if(fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if(res != ESP_OK) {
            break;
        }
    }

    return res;
}

// เริ่มต้น Web Server
void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &stream_uri);
        Serial.println("Camera server started successfully");
    } else {
        Serial.println("Failed to start camera server");
    }
}

// ตั้งค่ากล้อง
bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // ถ้ามี PSRAM ให้ใช้คุณภาพสูง
    if(psramFound()) {
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    // เริ่มต้นกล้อง
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    // ปรับแต่งค่ากล้อง
    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    Serial.println("Camera initialized successfully");
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // รอให้ Serial พร้อม
    Serial.println();
    Serial.println("=================================");
    Serial.println("ESP32-S3 Camera Server Starting...");
    Serial.println("=================================");

    // ตั้งค่า LED Pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // เปิด LED ติดค้างเมื่อเริ่มทำงาน
    Serial.println("[1/3] LED initialized on GPIO 2");

    // เริ่มต้นกล้อง
    Serial.println("[2/3] Initializing camera...");
    if(!initCamera()) {
        Serial.println("ERROR: Camera initialization failed!");
        Serial.println("System halted. Please check camera connection.");
        while(1) {
            // กระพริบ LED เร็วมากเมื่อ Camera ล้มเหลว
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    }
    Serial.println("SUCCESS: Camera initialized");

    // เชื่อมต่อ WiFi
    Serial.println("[3/3] Connecting to WiFi...");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    Serial.print("Status: ");
    unsigned long wifiTimeout = millis();
    bool ledState = false;
    int dotCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
        // กระพริบ LED ระหว่างรอเชื่อมต่อ WiFi
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        delay(250);
        Serial.print(".");
        dotCount++;

        // ขึ้นบรรทัดใหม่ทุก 40 จุด
        if (dotCount % 40 == 0) {
            Serial.println();
            Serial.print("Status: ");
        }

        // ถ้าเชื่อมต่อ WiFi ไม่สำเร็จภายใน 30 วินาที
        if (millis() - wifiTimeout > 30000) {
            Serial.println();
            Serial.println("ERROR: WiFi connection timeout!");
            Serial.println("Please check:");
            Serial.println("  1. WiFi SSID is correct");
            Serial.println("  2. WiFi password is correct");
            Serial.println("  3. WiFi router is powered on");
            wifiConnected = false;
            digitalWrite(LED_PIN, LOW);
            return;
        }
    }

    Serial.println();
    Serial.println("=================================");
    Serial.println("SUCCESS: WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.println("=================================");
    wifiConnected = true;

    // เริ่มต้น mDNS
    Serial.println("Starting mDNS responder...");
    if (MDNS.begin(hostname)) {
        Serial.print("mDNS started: http://");
        Serial.print(hostname);
        Serial.println(".local");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("Error setting up mDNS responder!");
    }

    // เริ่มต้น Web Server
    Serial.println("Starting camera web server...");
    startCameraServer();

    Serial.println();
    Serial.println("=================================");
    Serial.println("CAMERA READY!");
    Serial.print("Open browser: http://");
    Serial.print(hostname);
    Serial.println(".local");
    Serial.print("Or use IP: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=================================");
    Serial.println();
}

void loop() {
    if (wifiConnected) {
        // ถ้าเชื่อมต่อ WiFi ได้ - กระพริบ 1 วินาที
        digitalWrite(LED_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_PIN, LOW);
        delay(1000);
    } else {
        // ถ้าเชื่อมต่อ WiFi ไม่ได้ - กระพริบ 0.5 วินาที 3 ครั้ง แล้วหยุด 1 วินาที
        for(int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(500);
            digitalWrite(LED_PIN, LOW);
            delay(500);
        }
        delay(1000);  // หยุด 1 วินาที
    }
}

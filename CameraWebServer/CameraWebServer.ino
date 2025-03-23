#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <esp_camera.h>
#include "base64.h"

const char* ssid = "ROG_Phone6_2775";
const char* password = "qq123456789";
const char* websocket_server = "192.168.57.230";  // WebSocket 伺服器 IP
const uint16_t websocket_port = 3000;

using namespace websockets;
WebsocketsClient client;

// 相機 GPIO 設定 (根據你的配置)
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     34
#define SIOD_GPIO_NUM     15
#define SIOC_GPIO_NUM     16

#define Y9_GPIO_NUM       14
#define Y8_GPIO_NUM       13
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       11
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       9
#define Y3_GPIO_NUM       8
#define Y2_GPIO_NUM       7
#define VSYNC_GPIO_NUM    36
#define HREF_GPIO_NUM     35
#define PCLK_GPIO_NUM     37

void startCamera() {
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
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.println("❌ 相機初始化失敗");
        return;
    }
    Serial.println("✅ 相機初始化成功");
}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("🔄 連線 WiFi");

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        retry++;
        if (retry > 20) {
            Serial.println("\n❌ WiFi 連線失敗，請檢查 SSID 和密碼");
            return;
        }
    }
    Serial.println("\n✅ WiFi 連線成功！");
    Serial.print("📶 IP 地址: ");
    Serial.println(WiFi.localIP());
}

void connectToWebSocket() {
    Serial.print("🔄 連線 WebSocket");
    if (client.connect(websocket_server, websocket_port, "/")) {
        Serial.println("\n✅ WebSocket 連線成功！");
    } else {
        Serial.println("\n❌ WebSocket 連線失敗，將在 5 秒後重試...");
        delay(5000);
        connectToWebSocket();
    }
}

void setup() {
    Serial.begin(115200);
    
    connectToWiFi();
    startCamera();
    connectToWebSocket();
}

void loop() {
    if (client.available()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("❌ 無法獲取影像");
            return;
        }

        // 傳送影像數據 (二進制)
        client.sendBinary((const char*)fb->buf, fb->len);
        Serial.printf("📷 已發送影像 (%d bytes)\n", fb->len);

        esp_camera_fb_return(fb);
        delay(100);
    } else {
        Serial.println("⚠️ WebSocket 連線中斷，重新連接...");
        connectToWebSocket();
    }
}

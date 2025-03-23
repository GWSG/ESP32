#include <WiFi.h>
#include <WebSocketsClient.h>
#include <TinyGPSPlus.h>

// WiFi 設定
const char* ssid = "ROGPhone6";  
const char* password = "qq123456789";  

// WebSocket 伺服器
const char* websocket_server = " 192.168.229.230";  // 你的電腦 IP
const int websocket_port = 3000;

// GPS 設定 (ESP32 UART2)
#define RXD2 16  // GPS TX → ESP32 RX2 (GPIO 16)
#define TXD2 17  // GPS RX → ESP32 TX2 (GPIO 17)
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// WebSocket 物件
WebSocketsClient webSocket;

// 記錄時間
unsigned long lastReconnectAttempt = 0;
unsigned long lastGPSUpdate = 0;

// WebSocket 事件處理
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("⚠️ WebSocket 斷線，嘗試重新連接...");
            break;
        case WStype_CONNECTED:
            Serial.println("✅ WebSocket 已連線！");
            break;
        case WStype_TEXT:
            Serial.printf("📩 收到伺服器消息: %s\n", payload);
            break;
    }
}

// 嘗試重新連接 WebSocket
void reconnectWebSocket() {
    if (!webSocket.isConnected()) {
        Serial.println("🔄 嘗試重新連接 WebSocket...");
        webSocket.begin(websocket_server, websocket_port, "/");
        webSocket.onEvent(webSocketEvent);
    }
}

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);  // GPS 串列埠

    Serial.println("⏳ 初始化 WiFi 連線...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi 連接成功！");

    // 連接 WebSocket
    reconnectWebSocket();

    // **等待 GPS 啟動後發送 PMTK 設定**
    delay(3000);
    Serial.println("⏳ 設定 GPS 參數...");
    gpsSerial.println("$PMTK104");  // 重新啟動 GPS
    delay(100);
    gpsSerial.println("$PMTK220,1000");  // 設定 1Hz 更新率
    delay(100);
    gpsSerial.println("$PMTK353,1,1,1,1,1");  // 啟用 GPS + GLONASS + Galileo + BeiDou
    delay(100);
    gpsSerial.println("$PMTK301,2");  // 啟用 WAAS
    Serial.println("✅ GPS 設定完成！");
}

void loop() {
    webSocket.loop();

    // **自動重新連接 WebSocket**
    if (!webSocket.isConnected() && millis() - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = millis();
        reconnectWebSocket();
    }

    // **讀取 GPS 數據**
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        Serial.print(c);  // 顯示 GPS 原始 NMEA 資料
        gps.encode(c);
    }

    // **只要有新的 GPS 數據，就立即發送**
    if (gps.location.isUpdated()) {
        float latitude = gps.location.lat();
        float longitude = gps.location.lng();
        String gpsData = "{\"lat\":" + String(latitude, 6) + ",\"lng\":" + String(longitude, 6) + "}";
        Serial.println("📡 發送 GPS 位置: " + gpsData);
        webSocket.sendTXT(gpsData);
        lastGPSUpdate = millis();  // 記錄最後一次發送時間
    }

    // **避免 GPS 資料長時間未更新**
    if (millis() - lastGPSUpdate > 30000) {
        Serial.println("⚠️ 30 秒內未獲取到 GPS 數據...");
        lastGPSUpdate = millis();
    }
}

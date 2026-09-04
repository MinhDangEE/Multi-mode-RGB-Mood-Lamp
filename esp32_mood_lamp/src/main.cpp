// ============================================================================
//  Cầu nối WiFi ↔ UART cho RGB Mood Lamp
//
//  ESP32-S3 ở đây KHÔNG điều khiển đèn. Toàn bộ logic vẫn nằm ở STM32F103 —
//  4 nấc, gamma, cầu vồng, hẹn giờ, OLED. ESP chỉ làm một việc: chuyển tiếp
//  byte giữa internet và cổng UART của STM32, đúng như một sợi dây USB-TTL
//  nhưng dài vô hạn.
//
//      internet ──MQTT──> ESP32 ──UART──> STM32  (lệnh đi xuống)
//      internet <──MQTT── ESP32 <──UART── STM32  (log đi lên)
//
//  Nhờ vậy trang web gửi đúng những ký tự mà firmware STM32 vốn đã hiểu
//  ('1','2','3','0','n'), không phải học giao thức mới.
//
//  Vì sao MQTT chứ không cho ESP làm web server: đèn nằm sau router NAT ở
//  nhà, người ngoài không gọi thẳng vào được. ESP chủ động nối RA broker và
//  giữ kết nối đó mở, nên không cần mở port hay IP tĩnh.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

WiFiClient   net;
PubSubClient mqtt(net);
HardwareSerial stm(1);              // UART1 của ESP32, nối sang STM32

static char topic_cmd[96], topic_uart[96], topic_online[96], topic_status[96];
static String rx_line;              // gom byte từ STM32 cho tới khi đủ một dòng

// ---------------------------------------------------------------------------
//  Đẩy một dòng log của STM32 lên internet
// ---------------------------------------------------------------------------
static void publishLine(const String &line) {
    if (!mqtt.connected() || line.isEmpty()) return;
    JsonDocument doc;
    doc["line"] = line;
    doc["t"]    = millis() / 1000;
    char buf[320];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(topic_uart, (const uint8_t *)buf, n, false);
}

static void publishStatus() {
    if (!mqtt.connected()) return;
    JsonDocument doc;
    doc["rssi"]   = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["ip"]     = WiFi.localIP().toString();
    char buf[160];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(topic_status, (const uint8_t *)buf, n, true);
}

// ---------------------------------------------------------------------------
//  Lệnh từ web -> gửi thẳng xuống STM32
//
//  {"tx":"1"}            gõ ký tự '1'  (nấc 1)
//  {"tx":"C:255,0,0\n"}  gửi cả chuỗi, nếu firmware STM32 có hỗ trợ
// ---------------------------------------------------------------------------
static void onMessage(char *topic, byte *payload, unsigned int len) {
    JsonDocument doc;
    if (deserializeJson(doc, payload, len)) {
        publishLine("[ESP] Lenh JSON hong, bo qua");
        return;
    }
    // Kiểm tra thông mạch: tách UART, ép chân TX xuống 0 trong N mili-giây.
    // Bên kia đo PA10 — tụt theo là dây (và mát chung) đều tốt.
    if (doc["pintest"].is<int>()) {
        int ms = constrain((int)doc["pintest"], 100, 10000);
        stm.end();
        // Tự đọc lại chân sau khi ghi: phân biệt "chân ESP hỏng" với "đứt dây".
        pinMode(UART_TX_PIN, OUTPUT);
        digitalWrite(UART_TX_PIN, HIGH); delayMicroseconds(200);
        int rh = digitalRead(UART_TX_PIN);
        digitalWrite(UART_TX_PIN, LOW);  delayMicroseconds(200);
        int rl = digitalRead(UART_TX_PIN);
        Serial.printf("[TEST] GPIO%d tu doc lai: ghi CAO->doc %d, ghi THAP->doc %d  %s\n",
                      UART_TX_PIN, rh, rl,
                      (rh == 1 && rl == 0) ? "=> CHAN ESP TOT" : "=> CHAN ESP CO VAN DE");
        Serial.printf("[TEST] Giu GPIO%d = LOW trong %d ms\n", UART_TX_PIN, ms);
        publishLine("[ESP] Dang giu chan TX o muc THAP de do thong mach");
        delay(ms);
        digitalWrite(UART_TX_PIN, HIGH);
        stm.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
        Serial.println("[TEST] Da tra chan TX ve UART");
        return;
    }

    const char *tx = doc["tx"];
    if (!tx) return;

    size_t n = strlen(tx);
    if (n == 0 || n > 64) return;           // chặn gói rác, UART không cần dài
    stm.write((const uint8_t *)tx, n);

    // Cho web biết đã đẩy đi, kể cả khi STM32 im lặng không phản hồi.
    String echo = "[ESP] -> STM32: ";
    for (size_t i = 0; i < n; i++) {
        char c = tx[i];
        echo += (c == '\n') ? "\\n" : (c == '\r') ? "\\r" : String(c);
    }
    publishLine(echo);
}

static void mqttConnect() {
    // Last Will: broker tự báo offline nếu ESP mất điện hay rớt mạng.
    if (mqtt.connect(DEVICE_ID, topic_online, 1, true, "offline")) {
        mqtt.publish(topic_online, "online", true);
        mqtt.subscribe(topic_cmd);
        publishStatus();
        publishLine("[ESP] Cau noi WiFi-UART da san sang");
        Serial.printf("[MQTT] Da noi. Lenh vao: %s\n", topic_cmd);
    }
}

void setup() {
    Serial.begin(115200);                       // cổng USB, để bạn xem log
    stm.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    delay(300);

    snprintf(topic_cmd,    sizeof(topic_cmd),    "mliot/%s/cmd",    DEVICE_ID);
    snprintf(topic_uart,   sizeof(topic_uart),   "mliot/%s/uart",   DEVICE_ID);
    snprintf(topic_online, sizeof(topic_online), "mliot/%s/online", DEVICE_ID);
    snprintf(topic_status, sizeof(topic_status), "mliot/%s/status", DEVICE_ID);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);                       // đừng ngủ, lệnh phải xuống ngay
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("\n[WiFi] SSID dat trong config: \"%s\" (%d byte:", WIFI_SSID, (int)strlen(WIFI_SSID));
    for (size_t i = 0; i < strlen(WIFI_SSID); i++) Serial.printf(" %02X", (uint8_t)WIFI_SSID[i]);
    Serial.println(")");
    Serial.printf("[WiFi] Dang noi toi %s", WIFI_SSID);
    for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] OK, IP %s, RSSI %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    else {
        Serial.println("[WiFi] That bai. Dang quet xem co nhung mang nao...");
        WiFi.disconnect(true);          // phải ngắt hẳn, không thì quét trả về 0
        delay(200);
        WiFi.mode(WIFI_STA);
        delay(200);
        int n = WiFi.scanNetworks(false, true);   // hiện cả mạng an SSID
        if (n <= 0) {
            Serial.println("[WiFi] Khong thay mang nao — anten hoac vi tri co van de");
        } else {
            bool found = false;
            for (int i = 0; i < n; i++) {
                bool match = WiFi.SSID(i) == WIFI_SSID;
                if (match) found = true;
                Serial.printf("   %c %-24s  %d dBm  kenh %d  %s\n",
                              match ? '>' : ' ', WiFi.SSID(i).c_str(),
                              WiFi.RSSI(i), WiFi.channel(i),
                              WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "mo" : "co mat khau");
            }
            Serial.printf("[WiFi] %s\n", found
                ? "THAY mang can noi -> nhieu kha nang SAI MAT KHAU"
                : "KHONG thay mang can noi -> sai ten, hoac mang la 5GHz (S3 chi bat 2.4GHz)");
        }
    }

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMessage);
    mqtt.setBufferSize(512);
    mqttConnect();

    Serial.println("\n================================================");
    Serial.printf("  Ma thiet bi: %s\n", DEVICE_ID);
    Serial.printf("  Mo web:      ...vercel.app/?id=%s\n", DEVICE_ID);
    Serial.printf("  UART sang STM32: TX=GPIO%d  RX=GPIO%d  @%d\n",
                  UART_TX_PIN, UART_RX_PIN, UART_BAUD);
    Serial.println("================================================\n");
}

void loop() {
    uint32_t now = millis();

    // --- giữ kết nối, không chặn việc chuyển tiếp UART ---
    static uint32_t last_try = 0, last_status = 0;
    if (WiFi.status() != WL_CONNECTED) {
        // WiFi.reconnect() trên ESP32 hay không ăn khi phiên cũ đã hỏng hẳn.
        // Ngắt sạch rồi begin() lại từ đầu thì chắc chắn hơn nhiều.
        if (now - last_try > 8000) {
            last_try = now;
            Serial.printf("[WiFi] Mat ket noi (status=%d), dang noi lai...\n",
                          (int)WiFi.status());
            WiFi.disconnect(true);
            delay(100);
            WiFi.mode(WIFI_STA);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    } else if (!mqtt.connected()) {
        if (now - last_try > 3000) { last_try = now; mqttConnect(); }
    } else {
        mqtt.loop();
        if (now - last_status > 5000) { last_status = now; publishStatus(); }
    }

    // --- STM32 nói gì thì bắn thẳng lên web, gom theo dòng ---
    // Đừng echo từng byte thô ra USB: chân RX hở hoặc sai baud sẽ đổ rác
    // liên tục và nhấn chìm mọi log khác. Chỉ in ra khi đã đủ một dòng.
    static uint32_t junk = 0, last_junk_report = 0;
    while (stm.available()) {
        char c = (char)stm.read();
        if (c == '\n' || c == '\r') {
            rx_line.trim();
            if (rx_line.length()) {
                publishLine(rx_line);
                Serial.printf("[STM32] %s\n", rx_line.c_str());
                rx_line = "";
            }
        } else if ((uint8_t)c >= 0x20 && (uint8_t)c < 0x7F && rx_line.length() < 240) {
            rx_line += c;                       // chỉ nhận ký tự in được
        } else {
            junk++;                             // byte rác: đếm chứ không in
            rx_line = "";
        }
    }
    // Trạng thái định kỳ ra cổng USB, để chẩn đoán mà không cần bắt đúng lúc khởi động.
    static uint32_t last_beat = 0;
    if (now - last_beat > 5000) {
        last_beat = now;
        Serial.printf("[STATUS] WiFi=%s(%d) IP=%s RSSI=%d | MQTT=%s | id=%s\n",
                      WiFi.status() == WL_CONNECTED ? "OK" : "CHUA", (int)WiFi.status(),
                      WiFi.localIP().toString().c_str(), (int)WiFi.RSSI(),
                      mqtt.connected() ? "OK" : "CHUA", DEVICE_ID);
    }

    // Báo định kỳ nếu đường UART toàn rác — dấu hiệu sai dây hoặc chưa chung mát.
    // Đường UART lúc rảnh phải im hoàn toàn: STM32 chỉ nói khi đổi nấc. Rác đổ
    // liên tục nghĩa là chân RX đang hở, sai baud, hoặc hai board chưa chung mát.
    if (junk && now - last_junk_report > 3000) {
        last_junk_report = now;
        Serial.printf("[UART] %lu byte rac/3s — kiem tra TX/RX va GND chung\n",
                      (unsigned long)junk);
        junk = 0;
    }
}

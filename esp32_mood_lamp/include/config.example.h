#pragma once
// ============================================================================
//  SỬA FILE NÀY TRƯỚC KHI NẠP
// ============================================================================

// --- WiFi ---------------------------------------------------------------
// ESP32-S3 chỉ bắt được WiFi 2.4 GHz. Router phát 5 GHz riêng thì phải nối
// vào băng 2.4 GHz, không thấy tên mạng là do chỗ này.
#define WIFI_SSID       "TEN_WIFI_CUA_BAN"
#define WIFI_PASSWORD   "MAT_KHAU_WIFI"

// --- Mã thiết bị --------------------------------------------------------
// Chuỗi này là "địa chỉ" của đèn trên internet. Ai biết chuỗi này thì điều
// khiển được đèn, nên hãy đổi thành một chuỗi khó đoán của riêng bạn.
// Web dùng đúng chuỗi này: https://...vercel.app/?id=moodlamp-nhan-7f3a9c
#define DEVICE_ID       "moodlamp-doi-chuoi-nay-di"

// --- UART nối sang STM32 ------------------------------------------------
// ESP32-S3 và STM32F103 đều dùng mức logic 3.3 V nên nối thẳng, không cần
// mạch chuyển mức. Nhớ bắt CHÉO: TX bên này sang RX bên kia.
#define UART_TX_PIN     17      // -> PA10 (RX của STM32)
#define UART_RX_PIN     18      // <- PA9  (TX của STM32)
#define UART_BAUD       115200  // khớp MX_USART1_UART_Init() bên STM32

// --- MQTT ---------------------------------------------------------------
// Broker công cộng miễn phí, không cần tài khoản. ESP32 nối ra cổng 1883,
// trình duyệt nối vào cùng broker qua WSS cổng 8084.
#define MQTT_HOST       "broker.emqx.io"
#define MQTT_PORT       1883

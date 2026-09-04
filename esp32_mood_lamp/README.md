# Cầu nối WiFi ↔ UART (ESP32-S3) cho RGB Mood Lamp

ESP32-S3 ở đây **không điều khiển đèn**. Toàn bộ logic vẫn nằm ở STM32F103
ở thư mục gốc repo này — 4 nấc, gamma, cầu vồng, hẹn giờ, OLED.
ESP chỉ chuyển tiếp byte giữa internet và cổng UART của STM32, đúng như một
sợi dây USB-TTL nhưng dài vô hạn.

```
internet ──MQTT──> ESP32 ──UART──> STM32     lệnh đi xuống
internet <──MQTT── ESP32 <──UART── STM32     log đi lên
```

Nhờ vậy trang web gửi đúng những ký tự mà firmware STM32 vốn đã hiểu
(`1` `2` `3` `0` `n`) — không phải sửa gì bên STM32, và khung nhật ký trên web
hiện đúng những dòng STM32 thật sự in ra.

## Đấu dây — 3 sợi

| ESP32-S3 | STM32F103 | |
|---|---|---|
| GPIO17 (TX) | **PA10** (RX) | bắt chéo |
| GPIO18 (RX) | **PA9** (TX) | bắt chéo |
| GND | GND | bắt buộc chung mát |

Cả hai đều chạy logic 3.3 V nên nối thẳng, **không cần mạch chuyển mức**.

**Không phải đấu lại gì khác.** LED, biến trở, OLED vẫn giữ nguyên trên
STM32 như cũ. Module USB-TTL thì tháo ra được — ESP32 thay chỗ nó.

Đổi chân trong `include/config.h` nếu GPIO17/18 đang bận.

## Cấu hình

Sửa `include/config.h`:

```c
#define WIFI_SSID     "TenWiFi"        // ESP32-S3 chỉ bắt 2.4 GHz
#define WIFI_PASSWORD "MatKhau"
#define DEVICE_ID     "moodlamp-cua-ban-7f3a9c"   // đổi thành chuỗi khó đoán
```

`DEVICE_ID` là "địa chỉ" của đèn trên internet. **Ai biết chuỗi này thì điều
khiển được đèn**, nên đừng để mặc định và đừng commit lên GitHub công khai.

## Nạp và dùng

```bash
pio run -t upload      # nạp
pio device monitor     # xem log qua cổng USB, 115200
```

Rồi mở `https://<ten-app>.vercel.app/?id=<DEVICE_ID>`. Share đúng link đó cho
ai cũng được — họ không cần cùng mạng WiFi với bạn.

## Giao thức

Web gửi vào `mliot/<DEVICE_ID>/cmd`, ESP đẩy nguyên si xuống UART:

```json
{"tx": "1"}                  // nấc 1 — trắng tĩnh
{"tx": "2"}                  // nấc 2 — thở
{"tx": "3"}                  // nấc 3 — cầu vồng
{"tx": "0"}                  // nấc 4 — tắt
{"tx": "n"}                  // nấc kế tiếp
{"tx": "C:255,0,0\n"}        // màu tự chọn — xem giới hạn bên dưới
```

### Chẩn đoán khi đấu dây

```json
{"pintest": 5000}
```

ESP tách chân TX khỏi UART và **ép nó xuống mức 0** trong 5 giây, đồng thời tự
đọc lại chân để xác nhận chính nó không hỏng. Trong lúc đó, đo chân PA10 bên
STM32 (bằng ST-Link hoặc đồng hồ): **PA10 phải tụt xuống 0**. Nếu PA10 vẫn ở
mức cao thì sợi dây TX không dẫn — đứt, cắm nhầm lỗ, hoặc thiếu mát chung.

Ngoài ra ESP tự đếm byte rác trên đường UART và in ra cổng USB mỗi 3 giây.
Lúc rảnh, đường này phải **im hoàn toàn** vì STM32 chỉ nói khi đổi nấc — thấy
rác đổ liên tục là chân RX đang hở hoặc hai board chưa chung mát.

ESP bắn ngược lên `mliot/<DEVICE_ID>/uart` mỗi dòng STM32 in ra:

```json
{"line": "Nac 3: RAINBOW SPECTRUM (...)", "t": 42}
```

Còn `mliot/<DEVICE_ID>/online` là `online`/`offline` (dùng Last Will nên
broker tự báo offline khi mất điện), và `.../status` mang IP, RSSI, uptime.

## Giới hạn hiện tại

Firmware STM32 chỉ đọc **từng ký tự đơn**, nên:

- **Màu tự chọn chưa chạy** — chuỗi `C:R,G,B` bị bỏ qua. Muốn dùng phải thêm
  bộ gom chuỗi vào `HAL_UART_RxCpltCallback()` bên STM32.
- **Độ sáng** chỉnh bằng biến trở trên board, UART không có lệnh nào đụng tới.
- **Hẹn giờ** chỉ kích được bằng nút PB13, không có lệnh UART.

Ba thứ này đã bị ẩn/đánh dấu rõ trên web thay vì bày ra nút bấm vô tác dụng.

## Vì sao MQTT

Đèn nằm sau router NAT ở nhà nên không ai từ internet gọi thẳng vào được, kể
cả khi biết IP. Cách đi vòng là ESP **chủ động nối ra** một broker công cộng
rồi giữ kết nối đó mở; trang web nối vào cùng broker. Không cần mở port,
không cần IP tĩnh, không cần trả tiền.

Broker dùng `broker.emqx.io`. HiveMQ và test.mosquitto.org đã thử nhưng cổng
WSS của chúng không nối được từ mạng thử nghiệm.

**Lưu ý bảo mật:** broker công cộng nghĩa là dữ liệu không mã hoá riêng cho
bạn, và ai đoán trúng `DEVICE_ID` cũng điều khiển được. Với đèn trang trí thì
chấp nhận được; đừng dùng kiểu này cho ổ khoá hay thiết bị nguy hiểm.

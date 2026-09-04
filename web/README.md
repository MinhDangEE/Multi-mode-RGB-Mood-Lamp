# RGB Mood Lamp — Dashboard

Giao diện điều khiển đèn RGB Mood Lamp. Một file HTML duy nhất, không
dependency, không bước build.

**Bản đang chạy:** https://web-tikoocs-projects.vercel.app/?id=moodlamp-a3781afd

## Ba đường tới đèn, trang tự chọn

Lúc tải, trang dò theo thứ tự sau và dùng đường đầu tiên khả dụng:

| Điều kiện | Đường dùng | Cần gì |
|---|---|---|
| URL có `?id=<DEVICE_ID>` | **MQTT** qua ESP32-S3 | ESP32 đang online, ở bất kỳ đâu |
| `GET /api/state` trả lời | **Cầu SWD** qua ST-Link | chạy `tools/lamp_web.py` ở localhost |
| không có gì ở trên | **Web Serial** | module USB–TTL cắm vào máy |

Cả ba gửi **cùng một byte** xuống STM32, nên firmware không biết lệnh đến từ
đường nào. Khác nhau chỉ ở đoạn đường đi tới cổng UART.

## Chạy tại máy

```bash
# xem giao diện, hoặc dùng Web Serial (cần HTTPS hoặc localhost)
cd web && python3 -m http.server 5173     # rồi mở http://localhost:5173

# hoặc chạy kèm cầu SWD, tự phục vụ luôn trang này
python3 ../tools/lamp_web.py              # rồi mở http://localhost:8000
```

Web Serial chỉ có trên Chrome / Edge / Opera bản desktop. Firefox và Safari
chưa hỗ trợ — trang sẽ hiện banner báo rõ thay vì im lặng hỏng.

## Deploy

Site tĩnh, không có bước build. Chạy **từ trong thư mục này**:

```bash
cd web
npx vercel --prod
```

## Giao thức UART — 115200 8-N-1

Khớp đúng `HAL_UART_RxCpltCallback()` trong `../Core/Src/main.c`.

| Nút trên web | Byte gửi đi | State trong firmware |
|---|---|---|
| Màu Trắng Tĩnh | `1` (hoặc `w`) | `STATE_SOLID` |
| Thở Màu Trắng | `2` (hoặc `b`) | `STATE_BREATHING` |
| Cầu Vồng 6 Màu | `3` (hoặc `r`) | `STATE_RAINBOW` |
| Chế Độ Ngủ | `0` (hoặc `s`) | `STATE_SLEEP` |
| (phím tắt `n`) | `n` | chuyển sang nấc kế tiếp |

Board echo lại ký tự vừa nhận rồi in tên nấc mới. Ở chế độ MQTT, ESP32 bắn
những dòng đó ngược lên nên khung log hiện **đúng dữ liệu STM32 in ra** —
ký tự đứng đầu dòng `3Nac 3: RAINBOW...` chính là phần echo.

## Màu tự chọn chưa chạy

Panel màu gửi chuỗi `C:R,G,B\n`. Firmware hiện tại **chỉ đọc từng ký tự đơn**
nên bỏ qua chuỗi này — mỗi ký tự rơi vào nhánh `else` và không làm gì. Muốn
dùng được, cần thêm bộ gom chuỗi vào `HAL_UART_RxCpltCallback()` và một
`STATE_CUSTOM` đặt trực tiếp 3 kênh PWM (PA6 / PA7 / PB0).

Độ sáng và hẹn giờ cũng chưa có lệnh UART — đang điều khiển bằng biến trở và
nút PB13 trên board.

# RGB Mood Lamp — Web Serial Dashboard

Giao diện web điều khiển đèn RGB Mood Lamp chạy trên STM32F103C8T6
(`final_project_RGB_mood_lamp/`). Trình duyệt nói chuyện thẳng với board qua
**Web Serial API**, không cần cài driver hay app trung gian.

## Chạy thử tại máy

Web Serial chỉ hoạt động trên **HTTPS** hoặc **localhost**, nên không mở file
trực tiếp bằng `file://` được:

```bash
cd web
python3 -m http.server 5173
# mở http://localhost:5173
```

Yêu cầu: Chrome / Edge / Opera trên desktop. Firefox và Safari chưa hỗ trợ
Web Serial.

## Deploy

Đây là site tĩnh một file, không có bước build:

```bash
npx vercel --cwd web            # preview
npx vercel --cwd web --prod     # production
```

## Giao thức UART — 115200 8-N-1

Bảng lệnh dưới đây khớp đúng `HAL_UART_RxCpltCallback()` trong
`final_project_RGB_mood_lamp/Core/Src/main.c`.

| Nút trên web | Byte gửi đi | State trong firmware |
|---|---|---|
| Màu Trắng Tĩnh | `1` (hoặc `w`) | `STATE_SOLID` |
| Thở Màu Trắng | `2` (hoặc `b`) | `STATE_BREATHING` |
| Cầu Vồng 6 Màu | `3` (hoặc `r`) | `STATE_RAINBOW` |
| Chế Độ Ngủ | `0` (hoặc `s`) | `STATE_SLEEP` |
| (phím tắt `n`) | `n` | chuyển sang nấc kế tiếp |

Board echo lại ký tự vừa nhận rồi in tên nấc mới; các dòng đó hiện trực tiếp
trong khung **Nhật Ký Dữ Liệu UART**.

### Lệnh màu tùy chọn chưa có trong firmware

Panel màu gửi chuỗi `C:R,G,B\n`. Firmware hiện tại **chỉ đọc từng ký tự đơn**
nên sẽ bỏ qua chuỗi này — mỗi ký tự rơi vào nhánh `else` và không làm gì. Muốn
dùng được, cần thêm bộ gom chuỗi vào `HAL_UART_RxCpltCallback()` và một
`STATE_CUSTOM` đặt trực tiếp 3 kênh PWM (PA6 / PA7 / PB0).

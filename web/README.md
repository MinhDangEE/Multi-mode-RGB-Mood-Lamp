# 🌈 RGB Mood Lamp - Web Controller Dashboard

Ứng dụng web điều khiển trực quan Đèn LED RGB qua giao tiếp **Web Serial API** (USB UART).

## 🚀 Cách sử dụng:

1. **Yêu cầu trình duyệt:** Google Chrome, Microsoft Edge, Brave, hoặc Cốc Cốc trên máy tính (hỗ trợ Web Serial API).
2. **Khởi chạy:**
   * Chỉ cần nhấp đúp chuột để mở trực tiếp file `index.html` bằng trình duyệt web.
3. **Kết nối STM32:**
   * Nhấp nút **"Kết nối STM32 (USB)"** ở góc phải màn hình.
   * Chọn cổng COM của STM32 / USB-UART (ví dụ: `COM3`, `COM4`...).
   * Trình duyệt sẽ tự động kết nối ở Baudrate **115200**.
4. **Điều khiển:**
   * **Bảng màu (Color Picker):** Nhấp chọn bất kỳ màu nào trên bảng màu hoặc thanh trượt RGB -> Đèn LED STM32 sẽ sáng ngay màu đó theo thời gian thực!
   * **Chế độ nhanh:** Chọn Nấc 1 (Trắng tĩnh), Nấc 2 (Thở trắng Gamma 2.2), Nấc 3 (Cầu vồng), Nấc 0 (Tắt đèn).
   * **Serial Monitor:** Theo dõi dữ liệu phản hồi từ STM32 ngay trong giao diện web.

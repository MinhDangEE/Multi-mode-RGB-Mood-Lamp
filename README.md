# Multi-mode RGB Mood Lamp with Auto-off Timer based on STM32F103C8T6

He thong den trang tri thong minh da che do (Multi-mode RGB Mood Lamp) tich hop hen gio tu dong tat (Auto-off Timer), dieu khien do sang bang bien tro (ADC), hen gio bang encoder xoay KY-040 (EXTI), hien thi trang thai qua man hinh OLED SSD1306 va dong bo thoi gian thuc voi giao dien Web Serial Dashboard.

---

## 1. Tong quan du an

Du an duoc xay dung tren nen tang vi dieu khien STM32F103C8T6 (ARM Cortex-M3), ket hop cac ngoai vi phan cung bao gom Timer PWM, ADC, ngat ngoai EXTI, I2C va UART nham tao nen mot he thong chieu sang trang tri hoan chinh voi kha nang tuong tac truc quan:

- Dieu khien mau sac LED RGB thong qua 3 kenh PWM 16-bit doc lap.
- Tich hop 5 che do hoat dong: Mau trang tinh, Tho mau trang (Gamma 2.2), Cau vong 6 giai doan, Mau tu chon (Custom RGB), va Che do ngu (Sleep Mode).
- Chinh do sang tong the bang bien tro xoay 10k (ADC1).
- Hen gio tu dong tat den bang module encoder KY-040 voi buoc nhay 10 giay moi nac, ho tro huy hen gio nhanh bang nut nhan tich hop.
- Hien thi truc quan thong tin che do va bo dem nguoc tren man hinh OLED SSD1306 128x64.
- Giao dien Web Serial Controller phong cach Glassmorphism, cho phep chon mau truc tiep tu Color Picker va truyen nhan du lieu UART 115200 baud thoi gian thuc.

---

## 2. So do ket noi phan cung (Pinout Configuration)

| Thiet bi / Module | Chan tren Module | Chan tren STM32 | Chuc nang ngoai vi |
| :--- | :--- | :--- | :--- |
| **LED RGB** | Red (Kenh Do) | PA6 | TIM3_CH1 (PWM 1kHz) |
| | Green (Kenh Xanh la) | PA7 | TIM3_CH2 (PWM 1kHz) |
| | Blue (Kenh Xanh duong) | PB0 | TIM3_CH3 (PWM 1kHz) |
| | Cathode / Anode | GND / 3.3V | Cuc chung (Common Cathode/Anode) |
| **Bien tro 10k** | Chan giua (Wiper) | PA1 | ADC1_IN1 (Doc dien ap do sang) |
| | 2 chan ngoai | 3.3V / GND | Nguon cap dien ap chuan |
| **Encoder KY-040** | CLK (Phase A) | PB15 | EXTI15 (Ngat suon xuong dem buoc) |
| | DT (Phase B) | PB14 | GPIO Input (Doc chieu xoay Thuan/Nghich) |
| | SW (Push Button) | PB13 | EXTI13 (Nut nhan Tat/Huy hen gio) |
| | + (VCC) / GND | 3.3V / GND | Nguon cap module |
| **Nut bam che do** | Nut bam Mode | PB12 | EXTI12 (Chuyen che do hoat dong) |
| **OLED SSD1306** | SCL | PB6 | I2C1_SCL |
| | SDA | PB7 | I2C1_SDA |
| | VCC / GND | 3.3V / GND | Nguon man hinh |
| **UART USB Bridge** | TXD (Module USB) | PA10 | USART1_RX (Nhan lenh tu PC/Web) |
| | RXD (Module USB) | PA9 | USART1_TX (Truyen log len PC/Web) |
| | GND | GND | Mass chung |
| **ST-Link V2** | SWDIO / SWCLK / GND | PA13 / PA14 / GND | Debug va nap chuong trinh SWD |

---

## 3. Cac che do hoat dong (Operating Modes)

1. **Nac 1 - Solid White (Mau trang tinh 100%):**
   - Ba kenh mau R, G, B cung phat PWM o muc cuc dai, tao ra anh sang trang on dinh.
   - Do sang duoc dieu chinh truc tiep thong qua bien tro PA1.

2. **Nac 2 - Breathing White (Tho mau trang Gamma 2.2):**
   - Hieu ung tang/giam do sang tuan hoan theo duong cong Gamma 2.2, mang lai cam giac chuyen dong anh sang tu nhien, mem mai.

3. **Nac 3 - Rainbow Spectrum (Cau vong 6 giai doan):**
   - Chuyen mau lien tuc qua 6 vung quang pho: Do -> Vang -> Xanh la -> Cyan -> Xanh duong -> Magenta -> Do.
   - Su dung thuat toan crossfade PWM giup mau sac bien doi tuyet doi khong bi giat.

4. **Nac 4 - Custom RGB (Mau tu chon tu Web):**
   - Nhan gia tri RGB tu Web Serial Dashboard hoac Terminal qua giao thuc `C:R,G,B\n` hoac `#RRGGBB\n`.
   - Xuat PWM chinh xac theo dung ma mau nguoi dung da chon tren bang mau.

5. **Nac 0 - Sleep Mode (Che do ngu / Tat den):**
   - Tat toan bo 3 kenh PWM (Duty Cycle = 0%).
   - He thong tu dong chuyen ve che do nay khi bo dem nguoc hen gio ket thuc.

---

## 4. Co che hen gio (Auto-off Countdown Timer)

- **Xoay theo chieu kim dong ho (CW):** Moi nac xoay tang them 10 giay vao bo nho dem hen gio (`+10s`).
- **Xoay nguoc chieu kim dong ho (CCW):** Moi nac xoay giam di 10 giay (`-10s`). Khi giam ve 0 giay, che do hen gio tu dong tat.
- **Nhan vao dau num xoay (SW - PB13):** Ngay lap tuc huy va tat che do hen gio (`Timer: OFF`).
- **Tu dong dem nguoc:** He thong tu dong giam thoi gian moi 1 giay. Khi thoi gian dem ve `00:00`, he thong tu dong tat toan bo den LED va chuyen sang Sleep Mode.
- **Hien thi thoi gian thuc:** Thoi gian hen gio duoc cap nhat dong thoi tren man hinh OLED va gui log ve Serial Monitor.

---

## 5. Giao tiep Web Serial Dashboard

Giao dien dieu khien Web duoc thiet ke theo phong cach Glassmorphism ket hop cung Web Serial API:

- **Ket noi truc tiep:** Ket noi toi STM32 qua cong COM USB voi Baudrate 115200 (khong can cai dat driver phuc tap).
- **Bang mau Color Picker:** Chon mau truc quan va truyen ma mau RGB thoi gian thuc.
- **Thanh truot RGB Sliders:** Tinh chinh chi tiet tung kenh Red, Green, Blue tu 0 den 255.
- **Nut chon che do nhanh:** Chuyen doi giua cac che do Solid, Breathing, Rainbow, Custom, Sleep chi bang mot cu nhap chuot.
- **Serial Monitor tich hop:** Bo dem nhan dong (Line Stream Buffer) giup hien thi nhat ky du lieu tu STM32 ro rang, khong bi ngat quang hoac vo dong.

### Giao thuc truyen nhan UART (UART Protocol)

- **Chon che do:**
  - `1\n` hoac `w\n`: Chuyen Nac 1 (Solid White)
  - `2\n` hoac `b\n`: Chuyen Nac 2 (Breathing White)
  - `3\n` hoac `r\n`: Chuyen Nac 3 (Rainbow)
  - `4\n` hoac `c\n`: Chuyen Nac 4 (Custom Color)
  - `0\n` hoac `s\n`: Chuyen Nac 0 (Sleep / Standby)
  - `n\n` hoac `Space`: Chuyen sang che do tiep theo
- **Chon mau sac tuy chinh:**
  - `C:R,G,B\n` (vi du: `C:255,100,50\n`)
  - `#RRGGBB\n` (vi du: `#FF6432\n`)

---

## 6. Cau truc thu muc du an

```text
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── ssd1306.h
│   │   ├── ssd1306_fonts.h
│   │   ├── stm32f1xx_hal_conf.h
│   │   └── stm32f1xx_it.h
│   └── Src/
│       ├── main.c              # Chuong trinh chinh, State Machine, Timer, UART parser
│       ├── ssd1306.c           # Driver I2C man hinh OLED SSD1306
│       ├── ssd1306_fonts.c     # Bo font chu hien thi
│       ├── stm32f1xx_hal_msp.c # Khoi tao phan cung ngoai vi (MSP)
│       └── stm32f1xx_it.c      # Trinh phuc vu ngat EXTI va UART
├── Drivers/                    # Thu vien STM32F1xx HAL Driver & CMSIS
├── cmake/                      # Cau hinh build he thong CMake & STM32CubeMX
├── docs/                       # Tai lieu du an va file mo phong Proteus
├── web/
│   ├── index.html              # Giao dien Web Serial Dashboard
│   ├── style.css               # Dinh dang Glassmorphism va animation
│   ├── app.js                  # Xu ly ket noi Web Serial API va truyen goi tin RGB
│   └── README.md               # Huong dan su dung Web Dashboard
└── README.md                   # Tai lieu huong dan tong quan du an
```

---

## 7. Huong dan bien dich va nap chuong trinh

### Yeu cau moi truong
- **Toolchain:** Arm GNU Toolchain (`arm-none-eabi-gcc` 14.x tro len)
- **Build System:** CMake (>= 3.22) va Ninja
- **Debug / Flash Tool:** ST-Link V2 va STM32CubeProgrammer CLI

### Cac buoc bien dich

1. **Cau hinh du an bang CMake:**
   ```bash
   cmake --preset Debug
   ```

2. **Tien hanh bien dich:**
   ```bash
   cmake --build --preset Debug
   ```
   File thuc thi se duoc tao tai `build/Debug/STM32.bin` va `build/Debug/STM32.elf`.

3. **Nap chuong trinh xuong STM32 bang STM32CubeProgrammer:**
   ```bash
   STM32_Programmer_CLI -c port=SWD mode=UR -w build/Debug/STM32.bin 0x08000000 -v -rst
   ```

---

## 8. Huong dan su dung Web Controller

1. Mo trinh duyet Google Chrome, Microsoft Edge hoac Coc Coc tren may tinh.
2. Mo truc tiep file `web/index.html`.
3. Nhap vao nut **Ket noi STM32**, chon cong COM tuong ung cua mach USB-UART (toc do 115200 baud).
4. Thao tac chon mau tren Color Picker hoac nhap cac nut che do de dieu khien den LED RGB thoi gian thuc.
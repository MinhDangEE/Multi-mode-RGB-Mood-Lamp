# Multi-Mode RGB Mood Lamp with Auto-Off Timer on STM32F103C8T6

This project implements an embedded ambient lighting system using the STM32F103C8T6 (ARM Cortex-M3) microcontroller. It combines 3-channel 16-bit Timer PWM color blending, real-time potentiometer brightness regulation via ADC, rotary encoder countdown scheduling with instant cancellation, an SSD1306 OLED interface, and a browser-based Web Serial control dashboard communicating over UART at 115200 baud.

---

## 1. System Overview

The firmware leverages hardware peripherals of the STM32F103C8T6 MCU to drive a common-cathode RGB LED and manage interactive controls:

TIM3 generates 1 kHz PWM across channels 1, 2, and 3 (PA6, PA7, PB0) for independent color mixing. Analog input from a 10k potentiometer on PA1 is converted through ADC1 to scale total luminous output from 0% to 100%. User control is handled via an external mode toggle button on PB12 and a KY-040 rotary encoder on PB14 and PB15, which adjusts the auto-off timer in 10-second increments with step debouncing. The timer push button (PB13) cancels active countdowns. Status information is rendered on a 128x64 SSD1306 OLED over software I2C.

---

## 2. Hardware Pinout & Peripheral Mapping

| Device / Module | Module Pin | STM32 Pin | Peripheral Function | Electrical Role |
| :--- | :--- | :--- | :--- | :--- |
| RGB LED | Red Channel | PA6 | TIM3_CH1 | 1 kHz PWM Output |
| | Green Channel | PA7 | TIM3_CH2 | 1 kHz PWM Output |
| | Blue Channel | PB0 | TIM3_CH3 | 1 kHz PWM Output |
| | Common Terminal | GND | Ground | Common Cathode Reference |
| 10k Potentiometer | Wiper Pin | PA1 | ADC1_IN1 | Analog Voltage Input |
| | Outer Rails | 3.3V / GND | Power Rails | Voltage Divider Reference |
| KY-040 Encoder | CLK (Phase A) | PB15 | EXTI15 | Dual-Edge Step Interrupt |
| | DT (Phase B) | PB14 | EXTI14 | Direction Logic Input |
| | SW (Push Switch) | PB13 | EXTI13 | Falling-Edge Timer Cancel |
| | Power Rails | 3.3V / GND | Power Rails | 3.3V Supply |
| Mode Button | Tactile Switch | PB12 | EXTI12 | Falling-Edge Mode Toggle |
| SSD1306 OLED | SCL | PB6 | GPIO Output PP | Software I2C Clock |
| | SDA | PB7 | GPIO Output PP | Software I2C Data |
| | Power Rails | 3.3V / GND | Power Rails | Display Power |
| USB-UART Bridge | TXD / RXD | PA10 / PA9 | USART1 (RX/TX) | 115200 8N1 Serial Protocol |
| ST-Link V2 | SWDIO / SWCLK | PA13 / PA14 | SWD Debug | Firmware Flash & Debug |

---

## 3. Operational Lighting Modes

The lamp implements five discrete operational states:

1. **Solid White:** Drives Red, Green, and Blue channels at full duty cycle. Overall luminous intensity is dynamically scaled by the PA1 potentiometer.
2. **Breathing White:** Modulates intensity through a 3000 ms periodic cycle applying a non-linear Gamma 2.2 curve ($\text{Duty} = \text{MaxBright} \times \text{Progress}^{2.2}$) to match human eye lightness perception.
3. **Rainbow Spectrum:** Cycles through six chromatic spectrum zones (Red $\rightarrow$ Yellow $\rightarrow$ Green $\rightarrow$ Cyan $\rightarrow$ Blue $\rightarrow$ Magenta $\rightarrow$ Red) over a 6000 ms period using linear PWM crossfading without color jumping.
4. **Custom RGB:** Directly applies color coordinates transmitted over UART from the Web Serial palette.
5. **Sleep / Standby:** Sets all PWM compare registers to zero, turning off the LED completely upon manual selection or timer expiration.

---

## 4. Auto-Off Countdown Timer

Turning the KY-040 encoder clockwise increases timer duration by 10 seconds per detent click, while counter-clockwise rotation decreases duration by 10 seconds. Pressing the encoder shaft switch (PB13) clears the timer and disables countdown tracking.

A background timer tick decrements remaining time once per second. When the counter reaches zero, the firmware automatically switches to Sleep Mode (Mode 0) and blanks the LED output. Remaining time is continuously updated on the OLED display and echoed over UART.

---

## 5. Web Serial Control & UART Protocol

The browser interface connects directly to the STM32 USART1 port via the Web Serial API at 115200 baud. It includes a native color picker, individual R/G/B sliders, mode preset buttons, and a line-stream buffered serial log.

| Command Packet | Target Action | Example / Details |
| :--- | :--- | :--- |
| `1\n` or `w\n` | Switch to Solid White | Constant white illumination |
| `2\n` or `b\n` | Switch to Breathing White | Gamma 2.2 breathing cycle |
| `3\n` or `r\n` | Switch to Rainbow Spectrum | 6-phase chromatic crossfade |
| `4\n` or `c\n` | Switch to Custom RGB | Uses current custom RGB registers |
| `0\n` or `s\n` | Switch to Sleep Mode | Turns off PWM channels |
| `n\n` or `Space` | Next Mode | Cycles sequentially through states |
| `C:R,G,B\n` | Set Custom RGB Coordinates | Example: `C:255,120,40\n` |
| `#RRGGBB\n` | Set Custom Hex Color | Example: `#FF7828\n` |

To operate the web controller, open `web/index.html` in Chrome or Edge, click Connect STM32, choose the active COM port, and select colors or modes in real time.

---

## 6. Demo Video

Functional testing footage, hardware validation, potentiometer brightness control, encoder timer countdown, and web serial communication can be reviewed here:

[Demo Video](https://drive.google.com/drive/folders/1vPH_NVBRHYjKIKNvcpQxKeFuqSAPMJKs?usp=sharing)

---

## 7. Limitations and Future Improvements

While the current system operates reliably, several hardware and software limitations offer clear opportunities for future development:

1. **State Persistence:** Current configuration parameters (active mode, custom RGB values, timer states) reside in volatile RAM and reset upon power loss. Emulating EEPROM in Flash memory (internal Flash pages) would enable persistent state recovery across reboot cycles.
2. **Wireless Connectivity:** Communication currently requires a physical USB-UART cable. Integrating an ESP32 or BLE module (e.g., nRF52) would allow wireless Web Bluetooth or MQTT/Home Assistant IoT integration.
3. **Encoder Sensor Upgrade:** The mechanical KY-040 encoder has finite mechanical contact life. Upgrading to a magnetic Hall-effect rotary encoder or optical encoder would eliminate mechanical wear and long-term contact degradation entirely.
4. **Acoustic / Sound Sync Mode:** Adding an analog electret microphone with an operational amplifier or an I2S digital microphone (e.g., INMP441) would allow hardware FFT analysis on the STM32 to create audio-reactive music visualization.
5. **Power Management:** In Sleep Mode, the microcontroller remains in full run mode with peripherals active. Implementing STM32 Stop Mode or Standby Mode with EXTI wakeup would significantly reduce idle current consumption for battery-powered operation.

---

## 8. Authors

- Dang Quang Minh
- Duong Minh Trong
- Ho Thanh Nhan

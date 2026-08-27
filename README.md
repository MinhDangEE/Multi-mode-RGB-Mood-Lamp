# Multi-Mode RGB Mood Lamp with Auto-Off Timer on STM32F103C8T6

This project implements an embedded ambient lighting system using the STM32F103C8T6 (ARM Cortex-M3) microcontroller. It combines 3-channel 16-bit Timer PWM color blending, real-time potentiometer brightness regulation via ADC, rotary encoder countdown scheduling with instant cancellation, an SSD1306 OLED interface, and a browser-based Web Serial control dashboard communicating over UART at 115200 baud.

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Hardware Bill of Materials](#2-hardware-bill-of-materials)
3. [System Flowchart & Control Logic](#3-system-flowchart--control-logic)
4. [Hardware Pinout & Peripheral Mapping](#4-hardware-pinout--peripheral-mapping)
5. [Operational Lighting Modes](#5-operational-lighting-modes)
6. [Auto-Off Countdown Timer](#6-auto-off-countdown-timer)
7. [Web Serial Control & UART Protocol](#7-web-serial-control--uart-protocol)
8. [Demo Video](#8-demo-video)
9. [Limitations and Future Improvements](#9-limitations-and-future-improvements)
10. [Authors](#10-authors)

---

## 1. System Overview

The firmware leverages hardware peripherals of the STM32F103C8T6 MCU to drive a common-cathode RGB LED and manage interactive controls:

TIM3 generates 1 kHz PWM across channels 1, 2, and 3 (PA6, PA7, PB0) for independent color mixing. Analog input from a 10k potentiometer on PA1 is converted through ADC1 to scale total luminous output from 0% to 100%. User control is handled via an external mode toggle button on PB12 and a KY-040 rotary encoder module on PB14 and PB15, which adjusts the auto-off timer in 10-second increments with step debouncing. The timer push button (PB13) cancels active countdowns. Status information is rendered on a 128x64 SSD1306 OLED module over software I2C.

---

## 2. Hardware Bill of Materials

The complete hardware prototype is constructed using the following components and breakout modules:

| Component / Module | Specification / Model | Quantity | Role in System |
| :--- | :--- | :---: | :--- |
| **Microcontroller Board** | STM32F103C8T6 Blue Pill (ARM Cortex-M3, 72 MHz, 64KB Flash) | 1 | Master embedded processing unit |
| **RGB LED Module** |  Common-Cathode RGB LED Breakout Module | 1 | 3-channel PWM mood light emitter |
| **Rotary Encoder Module** | KY-040 Quadrature Rotary Encoder Breakout Module | 1 | Auto-off timer adjustment and instant cancel push switch |
| **OLED Display Module** | 0.96 inch SSD1306 I2C OLED Module (128x64 pixels, Blue/White) | 1 | Real-time status, mode, and countdown visualization |
| **Potentiometer** | 10k Linear Rotary Potentiometer (B10K) | 1 | Analog 0% to 100% master brightness regulator |
| **Mode Push Button** | 6x6mm Tactile Momentary Push Button | 1 | External hardware mode toggle trigger |
| **USB-to-UART Adapter** | CP2102 USB to Serial TTL Adapter | 1 | Bidirectional PC/Web Serial interface (115200 baud) |
| **Debugger / Programmer** | ST-Link V2 USB Dongle | 1 | SWD firmware flashing and hardware debugging |
| **Prototyping Accessories** | Solderless Breadboard and Jumper Wires | 1 set | Circuit interconnects and power distribution |

---

## 3. System Flowchart & Control Logic

The execution flow, mode transition logic, potentiometer ADC brightness mapping, and countdown timer routine are illustrated in the architecture flowchart below:

<div align="center">
  <img src="docs/images/RGB_lamp_general.png" alt="System Flowchart" width="550" />
</div>

---

## 4. Hardware Pinout & Peripheral Mapping

| Device / Module | Module Pin | STM32 Pin | Peripheral Function | Electrical Role |
| :--- | :--- | :--- | :--- | :--- |
| RGB LED Module | Red Channel (R) | PA6 | TIM3_CH1 | 1 kHz PWM Output |
| | Green Channel (G) | PA7 | TIM3_CH2 | 1 kHz PWM Output |
| | Blue Channel (B) | PB0 | TIM3_CH3 | 1 kHz PWM Output |
| | Common Ground (-) | GND | Ground | Common Cathode Reference |
| 10k Potentiometer | Wiper Pin | PA1 | ADC1_IN1 | Analog Voltage Input (0V to 3.3V) |
| | Outer Rails | 3.3V / GND | Power Rails | Voltage Divider Reference |
| KY-040 Encoder Module | CLK (Phase A) | PB15 | EXTI15 | Dual-Edge Step Interrupt |
| | DT (Phase B) | PB14 | EXTI14 | Direction Logic Input |
| | SW (Push Switch) | PB13 | EXTI13 | Falling-Edge Timer Cancel |
| | Power (+ / GND) | 3.3V / GND | Power Rails | Module 3.3V Supply |
| Mode Button | Tactile Switch | PB12 | EXTI12 | Falling-Edge Mode Toggle |
| SSD1306 OLED Module | SCL | PB6 | GPIO Output PP | Software I2C Clock |
| | SDA | PB7 | GPIO Output PP | Software I2C Data |
| | Power (VCC / GND)| 3.3V / GND | Power Rails | Module Display Power |
| USB-UART Bridge | TXD / RXD | PA10 / PA9 | USART1 (RX/TX) | 115200 8N1 Serial Protocol |
| ST-Link V2 | SWDIO / SWCLK | PA13 / PA14 | SWD Debug | Firmware Flash & Debug |

---

## 5. Operational Lighting Modes

The lamp implements five discrete operational states:

1. **Solid White:** Drives Red, Green, and Blue channels at full duty cycle. Overall luminous intensity is dynamically scaled by the PA1 potentiometer.
2. **Breathing White:** Modulates intensity through a 3000 ms periodic cycle applying a non-linear Gamma 2.2 curve (`Duty = MaxBright * Progress^2.2`) to match human eye lightness perception.
3. **Rainbow Spectrum:** Cycles through six chromatic spectrum zones (Red -> Yellow -> Green -> Cyan -> Blue -> Magenta -> Red) over a 6000 ms period using linear PWM crossfading without color jumping.
4. **Custom RGB:** Directly applies color coordinates transmitted over UART from the Web Serial palette.
5. **Sleep / Standby:** Sets all PWM compare registers to zero, turning off the LED completely upon manual selection or timer expiration.

---

## 6. Auto-Off Countdown Timer

Turning the KY-040 encoder clockwise increases timer duration by 10 seconds per detent click, while counter-clockwise rotation decreases duration by 10 seconds. Pressing the encoder shaft switch (PB13) clears the timer and disables countdown tracking.

A background timer tick decrements remaining time once per second. When the counter reaches zero, the firmware automatically switches to Sleep Mode (Mode 0) and blanks the LED output. Remaining time is continuously updated on the OLED display and echoed over UART.

---

## 7. Web Serial Control & UART Protocol

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

## 8. Demo Video

Functional testing footage, hardware validation, potentiometer brightness control, encoder timer countdown, and web serial communication can be reviewed here:

[Demo Video](https://drive.google.com/drive/folders/1vPH_NVBRHYjKIKNvcpQxKeFuqSAPMJKs?usp=sharing)

---

## 9. Limitations and Future Improvements

While the current system operates reliably, several hardware and software limitations offer clear opportunities for future development:

1. **State Persistence:** Current configuration parameters (active mode, custom RGB values, timer states) reside in volatile RAM and reset upon power loss. Emulating EEPROM in Flash memory (internal Flash pages) would enable persistent state recovery across reboot cycles.
2. **Wireless Connectivity:** Communication currently requires a physical USB-UART cable. Integrating an ESP32 or BLE module (e.g., nRF52) would allow wireless Web Bluetooth or MQTT/Home Assistant IoT integration.
3. **Encoder Sensor Upgrade:** The mechanical KY-040 encoder has finite mechanical contact life. Upgrading to a magnetic Hall-effect rotary encoder or optical encoder would eliminate mechanical wear and long-term contact degradation entirely.
4. **Acoustic / Sound Sync Mode:** Adding an analog electret microphone with an operational amplifier or an I2S digital microphone (e.g., INMP441) would allow hardware FFT analysis on the STM32 to create audio-reactive music visualization.
5. **Power Management:** In Sleep Mode, the microcontroller remains in full run mode with peripherals active. Implementing STM32 Stop Mode or Standby Mode with EXTI wakeup would significantly reduce idle current consumption for battery-powered operation.

---

## 10. Authors

- Dang Quang Minh
- Duong Minh Trong
- Ho Thanh Nhan

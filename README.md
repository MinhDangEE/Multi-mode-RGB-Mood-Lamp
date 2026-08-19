# Multi-Mode RGB Mood Lamp with Auto-Off Timer on STM32F103C8T6

An intelligent embedded ambient lighting controller developed on the STM32F103C8T6 (ARM Cortex-M3) microcontroller. The system features 3-channel hardware Timer PWM for dynamic color generation, ADC-based analog brightness regulation, a KY-040 rotary encoder for auto-off countdown scheduling (10s increments), an SSD1306 I2C OLED display for real-time status monitoring, and a Glassmorphism Web Serial Dashboard for real-time color picking and control via UART (115200 baud).

---

## 1. Project Overview

This project implements a multi-functional mood lamp utilizing core hardware peripherals of the STM32F103C8T6 MCU:

- **16-bit Hardware Timer PWM (TIM3):** Generates 1 kHz PWM on 3 independent channels for smooth RGB color mixing and brightness fading.
- **5 Operational Lighting Modes:** Solid White, Breathing White (Gamma 2.2 corrected), 6-Phase Rainbow Spectrum Crossfade, Custom RGB Color, and Sleep / Standby Mode.
- **Analog Brightness Control (ADC1):** Continuously reads a 10k potentiometer on PA1 to scale the overall luminous intensity from 0% to 100%.
- **KY-040 Rotary Encoder Timer (EXTI):** Configured with 10-second step increments/decrements per detent click, complete with instant timer cancellation via the integrated push button.
- **OLED Display Interface (I2C1):** Visualizes active mode names, custom RGB coordinates, and countdown timer in real time on a 128x64 SSD1306 screen.
- **Glassmorphism Web Serial Controller:** Web-based control panel built on the Web Serial API (Chrome / Edge), featuring a Color Picker, discrete RGB sliders, and a Line Stream Buffered UART monitor.

---

## 2. Hardware Pinout & Wiring Configuration

The system is deployed on an STM32F103C8T6 "Blue Pill" board with the following pin assignments:

| Peripheral / Module | Module Pin | STM32 Pin | Hardware Function | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **RGB LED** | Red Channel | PA6 | TIM3_CH1 | 1 kHz PWM output |
| | Green Channel | PA7 | TIM3_CH2 | 1 kHz PWM output |
| | Blue Channel | PB0 | TIM3_CH3 | 1 kHz PWM output |
| | Common Terminal | GND / 3.3V | Common Cathode / Anode | Common Cathode default |
| **10k Potentiometer** | Wiper (Middle) | PA1 | ADC1_IN1 | Analog voltage reading |
| | Outer Pins | 3.3V / GND | Power Rails | Voltage divider reference |
| **KY-040 Encoder** | CLK (Phase A) | PB15 | EXTI15 | Falling edge interrupt (step counter) |
| | DT (Phase B) | PB14 | GPIO Input | Direction detection (CW / CCW) |
| | SW (Push Button) | PB13 | EXTI13 | Falling edge interrupt (Timer Cancel) |
| | + (VCC) / GND | 3.3V / GND | Power Supply | 3.3V or 5V compatible |
| **Mode Button** | Push Button | PB12 | EXTI12 | Falling edge interrupt (Mode toggle) |
| **SSD1306 OLED** | SCL | PB6 | I2C1_SCL | 400 kHz Fast I2C Clock |
| | SDA | PB7 | I2C1_SDA | Fast I2C Data Line |
| | VCC / GND | 3.3V / GND | Power Supply | Display power |
| **USB-to-UART Bridge** | TXD (USB Bridge)| PA10 | USART1_RX | Command reception (115200 8N1) |
| | RXD (USB Bridge)| PA9 | USART1_TX | Log transmission (115200 8N1) |
| | GND | GND | Common Ground | Required for signal ground |
| **ST-Link V2 Debugger**| SWDIO / SWCLK / GND | PA13 / PA14 / GND | Serial Wire Debug (SWD) | Programming and debugging |

---

## 3. Operational Lighting Modes

1. **Mode 1 - Solid White (100% Constant Intensity):**
   - Drives all three channels (Red, Green, Blue) at maximum duty cycle to generate uniform neutral white light.
   - Master brightness is scaled continuously via the PA1 potentiometer.

2. **Mode 2 - Breathing White (Gamma 2.2 Corrected):**
   - Applies a smooth periodic brightness pulse using a non-linear Gamma 2.2 correction curve to match human eye luminous sensitivity.

3. **Mode 3 - Rainbow Spectrum (6-Phase Crossfade):**
   - Seamlessly transitions through 6 spectrum zones: Red -> Yellow -> Green -> Cyan -> Blue -> Magenta -> Red.
   - Utilizes linear PWM crossfading for artifact-free color transitions.

4. **Mode 4 - Custom RGB Color (Web Color Picker):**
   - Receives RGB color coordinates directly from the Web Serial Dashboard or UART Terminal via `C:R,G,B\n` or `#RRGGBB\n` packets.
   - Directly maps RGB values (0-255) to TIM3 compare registers.

5. **Mode 0 - Sleep Mode (Standby / Off):**
   - Disables all PWM outputs (0% duty cycle) to turn off the lamp completely.
   - The system automatically enters this mode when the auto-off timer reaches zero.

---

## 4. Auto-Off Countdown Timer Mechanism

- **Clockwise Rotation (CW):** Each physical detent click adds 10 seconds to the timer memory (`+10s`).
- **Counter-Clockwise Rotation (CCW):** Each click subtracts 10 seconds (`-10s`). Reducing the time to 0 seconds turns off the timer.
- **Push Button Action (SW - PB13):** Instantly cancels the countdown and resets the timer (`Timer: OFF`).
- **Automatic Countdown Execution:** A non-blocking 1-second system tick decrements the timer. When the remaining time reaches `00:00`, the system automatically transitions to **Sleep Mode (Mode 0)**, extinguishing all LEDs.
- **Real-Time Visualization:** Remaining time (`MM:SS`) is rendered on the OLED display and transmitted via UART.

---

## 5. Web Serial Dashboard & UART Protocol

The web interface is engineered with a Glassmorphism aesthetic and connects directly to the microcontroller through the browser's Web Serial API.

### Features
- **Direct USB Serial Connection:** Native browser-to-MCU serial communication at 115200 baud without third-party drivers.
- **Live Color Picker:** Interactive color palette and discrete RGB sliders for real-time color syncing.
- **Quick Preset Swatches:** Single-click selection of popular mood colors (Pastel Pink, Warm Amber, Neon Blue, Emerald, Purple).
- **Mode Switching Cards:** Instant toggle buttons for all operating modes.
- **Line Stream Buffered Terminal:** Prevents fragmented text and displays incoming serial telemetry neatly with timestamps.

### UART Communication Protocol

- **Mode Commands:**
  - `1\n` or `w\n`: Select Mode 1 (Solid White)
  - `2\n` or `b\n`: Select Mode 2 (Breathing White)
  - `3\n` or `r\n`: Select Mode 3 (Rainbow Spectrum)
  - `4\n` or `c\n`: Select Mode 4 (Custom RGB)
  - `0\n` or `s\n`: Select Mode 0 (Sleep / Standby)
  - `n\n` or `Space`: Step to next mode
- **Custom Color Packets:**
  - `C:R,G,B\n` (e.g., `C:255,100,50\n` for custom RGB)
  - `#RRGGBB\n` (e.g., `#FF6432\n` for hex format)

---

## 6. Repository Directory Structure

```text
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── ssd1306.h
│   │   ├── ssd1306_fonts.h
│   │   ├── stm32f1xx_hal_conf.h
│   │   └── stm32f1xx_it.h
│   └── Src/
│       ├── main.c              # Core logic, State Machine, Timer, UART parser
│       ├── ssd1306.c           # SSD1306 OLED display driver
│       ├── ssd1306_fonts.c     # Display font bitmaps
│       ├── stm32f1xx_hal_msp.c # MCU peripheral MSP initialization
│       └── stm32f1xx_it.c      # EXTI and UART interrupt service routines
├── Drivers/                    # STM32F1xx HAL Driver & CMSIS libraries
├── cmake/                      # CMake build definitions and toolchain scripts
├── docs/                       # Documentation and Proteus simulation files
├── web/
│   ├── index.html              # Glassmorphism Web Serial Dashboard UI
│   ├── style.css               # Frosted glass styling and ambient animations
│   ├── app.js                  # Web Serial API handler and line stream buffer
│   └── README.md               # Web Dashboard documentation
└── README.md                   # Project overview and technical specification
```

---

## 7. Build and Flash Instructions

### Prerequisites
- **Toolchain:** Arm GNU Toolchain (`arm-none-eabi-gcc` 14.x or later)
- **Build System:** CMake (>= 3.22) and Ninja
- **Programmer:** ST-Link V2 and STM32CubeProgrammer CLI

### Compilation Steps

1. **Configure the build using CMake Presets:**
   ```bash
   cmake --preset Debug
   ```

2. **Compile the binary targets:**
   ```bash
   cmake --build --preset Debug
   ```
   Compiled output artifacts will be generated in `build/Debug/STM32.bin` and `build/Debug/STM32.elf`.

3. **Flash the firmware via SWD using STM32CubeProgrammer:**
   ```bash
   STM32_Programmer_CLI -c port=SWD mode=UR -w build/Debug/STM32.bin 0x08000000 -v -rst
   ```

---

## 8. Web Controller Quick Start

1. Open Google Chrome, Microsoft Edge, or any Web Serial-compatible browser.
2. Open the file `web/index.html`.
3. Click **Connect STM32** and select the active USB-to-UART COM port (115200 baud).
4. Use the color picker, RGB sliders, or mode buttons to control the mood lamp in real time.

---

## 9. Authors & Contributors

- **Dang Quang Minh**
- **Duong Minh Trong**
- **Ho Thanh Nhan**
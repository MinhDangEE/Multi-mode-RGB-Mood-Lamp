# RGB Mood Lamp Web Controller Dashboard

This web application provides a real-time control interface for the STM32F103C8T6 RGB Mood Lamp system via the browser-native Web Serial API. Built with a Glassmorphism design and animated ambient background lighting, it allows users to adjust colors, switch lighting modes, and inspect live serial telemetry directly through a USB-to-UART connection at 115200 baud.

---

## 1. Requirements and Quick Start

1. Open Google Chrome, Microsoft Edge, Brave, or Opera on a desktop computer with Web Serial API support enabled.
2. Launch the application by opening `index.html` directly in the browser. No local web server or package installation is required.
3. Click the Connect STM32 button located in the top navigation bar.
4. Select the corresponding USB-to-UART serial port (such as COM3, COM4, or `/dev/ttyUSB0`) from the browser prompt.
5. The dashboard automatically connects at 115200 baud and establishes two-way communication with the microcontroller.

---

## 2. Interface Controls and Features

The application is structured into three primary control panels:

### Color Management
Users can select custom colors using the native HTML5 color palette, adjust discrete 8-bit Red, Green, and Blue sliders (range 0 to 255), or click preset mood swatches. Changes can be transmitted in real time or applied as a single batch update to configure Mode 4 (Custom RGB).

### Lighting Mode Selection
The right panel contains dedicated mode cards to trigger predefined firmware routines. Clicking any card dispatches the corresponding single-byte or multi-byte command to switch between Solid White, Breathing White, Rainbow Spectrum, Custom Color, and Sleep Mode.

### Real-Time Terminal Monitor
The embedded console displays bidirectional UART communication. Incoming serial telemetry from the STM32 is processed through a line-stream buffer to prevent line fragmentation, timestamped with millisecond precision, and color-coded by message type (system info, transmitted packets, received telemetry, and error notifications).

---

## 3. Serial Communication Protocol

The web client communicates with the STM32 firmware using standard ASCII command packets:

| Command Packet | Action Triggered | Target Mode | Description |
| :--- | :--- | :--- | :--- |
| `1\n` or `w\n` | Activate Mode 1 | Solid White | Drives all channels at full duty cycle |
| `2\n` or `b\n` | Activate Mode 2 | Breathing White | Applies Gamma 2.2 breathing cycle |
| `3\n` or `r\n` | Activate Mode 3 | Rainbow Spectrum | 6-phase chromatic spectrum crossfade |
| `4\n` or `c\n` | Activate Mode 4 | Custom RGB | Applies stored custom color registers |
| `0\n` or `s\n` | Activate Mode 0 | Sleep / Standby | Disables all PWM outputs |
| `n\n` or `Space` | Next Mode | Sequential | Steps forward to the next state |
| `C:R,G,B\n` | Custom RGB Coordinates | Mode 4 | Formats 8-bit values (e.g., `C:255,128,64\n`) |
| `#RRGGBB\n` | Custom Hex Color | Mode 4 | Formats 24-bit hex code (e.g., `#FF8040\n`) |

---

## 4. Architecture and Technology Stack

The dashboard is built entirely with vanilla web standards without external build dependencies. Styling is defined in `style.css` using modern CSS tokens, backdrop filter blurs, and spring-physics transition curves. Serial stream management and UI synchronization are implemented in `app.js` using asynchronous reader and writer streams. Vector icons are rendered using the Lucide icon library.

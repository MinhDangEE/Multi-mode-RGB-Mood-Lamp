/**
 * RGB Mood Lamp Controller - Glassmorphism Web Serial Controller
 * Designed for STM32F103C8T6 (115200 Baud)
 * Lucide Icons | Zero Emojis | Line Stream Buffer
 */

let port = null;
let reader = null;
let keepReading = false;
let serialRxAccumulator = '';
let flushTimeout = null;

let currentR = 255;
let currentG = 255;
let currentB = 255;

// DOM Elements
const btnConnect = document.getElementById('btn-connect');
const btnConnectLabel = document.getElementById('btn-connect-label');
const connectionBadge = document.getElementById('connection-badge');
const connectionText = document.getElementById('connection-text');
const statusOrb = document.getElementById('status-orb');

const colorPreview = document.getElementById('color-preview');
const hexVal = document.getElementById('hex-val');
const nativePicker = document.getElementById('native-color-picker');

const sliderR = document.getElementById('slider-r');
const sliderG = document.getElementById('slider-g');
const sliderB = document.getElementById('slider-b');
const valR = document.getElementById('r-val');
const valG = document.getElementById('g-val');
const valB = document.getElementById('b-val');

const btnApplyColor = document.getElementById('btn-apply-color');
const modeButtons = document.querySelectorAll('.mode-btn');
const swatches = document.querySelectorAll('.swatch');

const terminalOutput = document.getElementById('terminal-output');
const btnClearConsole = document.getElementById('btn-clear-console');

// Initialize Lucide Icons
function refreshIcons() {
    if (window.lucide) {
        lucide.createIcons();
    }
}
document.addEventListener('DOMContentLoaded', refreshIcons);

// Check Web Serial API support
if (!('serial' in navigator)) {
    logTerminal('[LỖI] Trình duyệt không hỗ trợ Web Serial API. Vui lòng sử dụng Chrome, Edge hoặc Cốc Cốc trên máy tính.', 'err');
    btnConnect.disabled = true;
    btnConnectLabel.textContent = 'Không hỗ trợ';
}

// ----------------------------------------------------
// Terminal Logger with Clean Timestamp Formatting
// ----------------------------------------------------
function logTerminal(message, type = 'rx') {
    if (!message || message.trim() === '') return;
    
    const line = document.createElement('div');
    line.className = `term-line ${type}`;
    
    const timestamp = new Date().toLocaleTimeString('vi-VN');
    
    line.innerHTML = `
        <span class="term-timestamp">[${timestamp}]</span>
        <span class="term-text">${escapeHtml(message.trim())}</span>
    `;
    
    terminalOutput.appendChild(line);
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
}

function escapeHtml(str) {
    return str.replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;");
}

btnClearConsole.addEventListener('click', () => {
    terminalOutput.innerHTML = '';
});

// ----------------------------------------------------
// Web Serial Connect / Disconnect
// ----------------------------------------------------
btnConnect.addEventListener('click', async () => {
    if (port) {
        await disconnectSerial();
    } else {
        await connectSerial();
    }
});

async function connectSerial() {
    try {
        logTerminal('[SYS] Đang mở hộp thoại chọn cổng COM USB...', 'info');
        port = await navigator.serial.requestPort();
        await port.open({ baudRate: 115200 });

        connectionBadge.className = 'status-badge connected';
        connectionText.textContent = 'Đã kết nối COM (115200)';
        
        btnConnect.classList.add('is-connected');
        btnConnect.innerHTML = `
            <i data-lucide="power" class="btn-icon"></i>
            <span>Ngắt kết nối</span>
        `;
        refreshIcons();

        logTerminal('[SYS] Kết nối thành công với vi điều khiển STM32!', 'info');

        keepReading = true;
        readSerialLoop();
    } catch (err) {
        logTerminal(`[LỖI] Kết nối thất bại: ${err.message}`, 'err');
        port = null;
    }
}

async function disconnectSerial() {
    try {
        logTerminal('[SYS] Đang ngắt kết nối cổng COM...', 'info');
        
        // 1. Tắt cờ đọc
        keepReading = false;

        // 2. Hủy reader đang chờ
        if (reader) {
            try {
                await reader.cancel();
            } catch (e) {
                console.warn('Reader cancel warning:', e);
            }
        }

        // 3. Đợi giải phóng Stream Lock
        await new Promise(resolve => setTimeout(resolve, 60));

        // 4. Đóng cổng COM
        if (port) {
            try {
                await port.close();
            } catch (e) {
                console.warn('Port close warning:', e);
            }
            port = null;
        }

        connectionBadge.className = 'status-badge disconnected';
        connectionText.textContent = 'Chưa kết nối';
        
        btnConnect.classList.remove('is-connected');
        btnConnect.innerHTML = `
            <i data-lucide="usb" class="btn-icon"></i>
            <span>Kết nối STM32</span>
        `;
        refreshIcons();

        logTerminal('[SYS] Đã ngắt kết nối an toàn với STM32.', 'info');
    } catch (err) {
        logTerminal(`[LỖI] Ngắt kết nối thất bại: ${err.message}`, 'err');
        port = null;
    }
}

// ----------------------------------------------------
// Vòng lặp đọc dữ liệu UART với Line Stream Buffer
// Ghép đầy đủ các mẩu vụn cho đến khi gặp dấu xuống dòng \n
// ----------------------------------------------------
async function readSerialLoop() {
    const textDecoder = new TextDecoder();
    serialRxAccumulator = '';

    while (port && port.readable && keepReading) {
        try {
            reader = port.readable.getReader();
            while (keepReading) {
                const { value, done } = await reader.read();
                if (done || !keepReading) {
                    break;
                }
                if (value) {
                    const chunk = textDecoder.decode(value, { stream: true });
                    serialRxAccumulator += chunk;

                    // Kiểm tra có dấu xuống dòng không
                    if (serialRxAccumulator.includes('\n')) {
                        const lines = serialRxAccumulator.split('\n');
                        // Giữ lại phần chưa kết thúc bởi \n vào accumulator
                        serialRxAccumulator = lines.pop();

                        for (const line of lines) {
                            const cleanLine = line.replace('\r', '').trim();
                            if (cleanLine.length > 0) {
                                logTerminal(cleanLine, 'rx');
                            }
                        }
                    }

                    // Tự động xả dữ liệu nếu không có ký tự \n sau 150ms
                    if (flushTimeout) clearTimeout(flushTimeout);
                    flushTimeout = setTimeout(() => {
                        if (serialRxAccumulator.trim().length > 0) {
                            logTerminal(serialRxAccumulator.replace('\r', '').trim(), 'rx');
                            serialRxAccumulator = '';
                        }
                    }, 150);
                }
            }
        } catch (err) {
            break;
        } finally {
            if (reader) {
                try {
                    reader.releaseLock();
                } catch (e) {}
                reader = null;
            }
        }
    }
}

// Gửi lệnh UART tới STM32
async function sendCommand(cmd) {
    if (!port || !port.writable) {
        logTerminal(`[LỆNH NỘI BỘ] ${cmd.trim()}`, 'tx');
        return;
    }

    try {
        const textEncoder = new TextEncoder();
        const writer = port.writable.getWriter();
        await writer.write(textEncoder.encode(cmd));
        writer.releaseLock();
        logTerminal(`[TX] -> ${cmd.trim()}`, 'tx');
    } catch (err) {
        logTerminal(`[LỖI GỬI] ${err.message}`, 'err');
    }
}

// ----------------------------------------------------
// Color Picker & RGB Calculations
// ----------------------------------------------------
function updateColorUI(r, g, b, fromInput = 'rgb') {
    currentR = parseInt(r);
    currentG = parseInt(g);
    currentB = parseInt(b);

    const hex = rgbToHex(currentR, currentG, currentB);

    if (fromInput !== 'slider') {
        sliderR.value = currentR;
        sliderG.value = currentG;
        sliderB.value = currentB;
    }
    
    if (fromInput !== 'native') {
        nativePicker.value = hex;
    }

    valR.textContent = currentR;
    valG.textContent = currentG;
    valB.textContent = currentB;
    hexVal.textContent = hex.toUpperCase();

    // Solid Flat Color Fill (No Gradients)
    colorPreview.style.backgroundColor = `rgb(${currentR}, ${currentG}, ${currentB})`;
    
    // Status Orb Flat Accent Indicator
    statusOrb.style.borderColor = `rgb(${currentR}, ${currentG}, ${currentB})`;
    statusOrb.style.color = `rgb(${currentR}, ${currentG}, ${currentB})`;
}

function hexToRgb(hex) {
    const bigint = parseInt(hex.replace('#', ''), 16);
    return {
        r: (bigint >> 16) & 255,
        g: (bigint >> 8) & 255,
        b: bigint & 255
    };
}

function rgbToHex(r, g, b) {
    return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
}

// Event Listeners for Sliders
[sliderR, sliderG, sliderB].forEach(slider => {
    slider.addEventListener('input', () => {
        updateColorUI(sliderR.value, sliderG.value, sliderB.value, 'slider');
    });
});

// Event Listener for Native Color Picker
nativePicker.addEventListener('input', (e) => {
    const { r, g, b } = hexToRgb(e.target.value);
    updateColorUI(r, g, b, 'native');
});

// Event Listeners for Swatches
swatches.forEach(swatch => {
    swatch.addEventListener('click', () => {
        const hex = swatch.getAttribute('data-color');
        const { r, g, b } = hexToRgb(hex);
        updateColorUI(r, g, b);
        sendColorToSTM32(r, g, b);
    });
});

// Apply Color Button
btnApplyColor.addEventListener('click', () => {
    sendColorToSTM32(currentR, currentG, currentB);
});

function sendColorToSTM32(r, g, b) {
    setActiveModeButton('custom');
    const packet = `C:${r},${g},${b}\n`;
    sendCommand(packet);
}

// ----------------------------------------------------
// Mode Selection Handling
// ----------------------------------------------------
modeButtons.forEach(btn => {
    btn.addEventListener('click', () => {
        const mode = btn.getAttribute('data-mode');
        setActiveModeButton(mode);

        if (mode === 'custom') {
            sendColorToSTM32(currentR, currentG, currentB);
        } else {
            sendCommand(`${mode}\n`);
        }
    });
});

function setActiveModeButton(mode) {
    modeButtons.forEach(btn => {
        if (btn.getAttribute('data-mode') === mode) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}

// Initialize default color (White) & render icons
updateColorUI(255, 255, 255);
refreshIcons();

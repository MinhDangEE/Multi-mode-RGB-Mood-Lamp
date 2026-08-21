let port = null;
let reader = null;
let keepReading = false;
let serialRxAccumulator = '';
let flushTimeout = null;

let currentR = 255;
let currentG = 255;
let currentB = 255;

// DOM references
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

function refreshIcons() {
    if (window.lucide) {
        lucide.createIcons();
    }
}
document.addEventListener('DOMContentLoaded', refreshIcons);

if (!('serial' in navigator)) {
    logTerminal('[ERROR] Web Serial API not supported in this browser.', 'err');
    btnConnect.disabled = true;
    btnConnectLabel.textContent = 'Unsupported';
}

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

// Connection Handlers
btnConnect.addEventListener('click', async () => {
    if (port) {
        await disconnectSerial();
    } else {
        await connectSerial();
    }
});

async function connectSerial() {
    try {
        logTerminal('[SYS] Requesting COM port...', 'info');
        port = await navigator.serial.requestPort();
        await port.open({ baudRate: 115200 });

        connectionBadge.className = 'status-badge connected';
        connectionText.textContent = 'Connected (115200)';
        
        btnConnect.classList.add('is-connected');
        btnConnect.innerHTML = `
            <i data-lucide="power" class="btn-icon"></i>
            <span>Ngắt kết nối</span>
        `;
        refreshIcons();

        logTerminal('[SYS] Connected to STM32 successfully', 'info');

        keepReading = true;
        readSerialLoop();
    } catch (err) {
        logTerminal(`[ERROR] Connection failed: ${err.message}`, 'err');
        port = null;
    }
}

async function disconnectSerial() {
    try {
        logTerminal('[SYS] Disconnecting port...', 'info');
        keepReading = false;

        if (reader) {
            try {
                await reader.cancel();
            } catch (e) {
                console.warn('Reader cancel warning:', e);
            }
        }

        await new Promise(resolve => setTimeout(resolve, 60));

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

        logTerminal('[SYS] Disconnected.', 'info');
    } catch (err) {
        logTerminal(`[ERROR] Disconnect failed: ${err.message}`, 'err');
        port = null;
    }
}

// Serial Receiver
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

                    if (serialRxAccumulator.includes('\n')) {
                        const lines = serialRxAccumulator.split('\n');
                        serialRxAccumulator = lines.pop();

                        for (const line of lines) {
                            const cleanLine = line.replace('\r', '').trim();
                            if (cleanLine.length > 0) {
                                logTerminal(cleanLine, 'rx');
                            }
                        }
                    }

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

// Send Command
async function sendCommand(cmd) {
    if (!port || !port.writable) {
        logTerminal(`[LOCAL] ${cmd.trim()}`, 'tx');
        return;
    }

    try {
        const textEncoder = new TextEncoder();
        const writer = port.writable.getWriter();
        await writer.write(textEncoder.encode(cmd));
        writer.releaseLock();
        logTerminal(`[TX] ${cmd.trim()}`, 'tx');
    } catch (err) {
        logTerminal(`[TX ERROR] ${err.message}`, 'err');
    }
}

// UI & Color logic
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

    colorPreview.style.backgroundColor = `rgb(${currentR}, ${currentG}, ${currentB})`;
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

[sliderR, sliderG, sliderB].forEach(slider => {
    slider.addEventListener('input', () => {
        updateColorUI(sliderR.value, sliderG.value, sliderB.value, 'slider');
    });
});

nativePicker.addEventListener('input', (e) => {
    const { r, g, b } = hexToRgb(e.target.value);
    updateColorUI(r, g, b, 'native');
});

swatches.forEach(swatch => {
    swatch.addEventListener('click', () => {
        const hex = swatch.getAttribute('data-color');
        const { r, g, b } = hexToRgb(hex);
        updateColorUI(r, g, b);
        sendColorToSTM32(r, g, b);
    });
});

btnApplyColor.addEventListener('click', () => {
    sendColorToSTM32(currentR, currentG, currentB);
});

function sendColorToSTM32(r, g, b) {
    setActiveModeButton('custom');
    const packet = `C:${r},${g},${b}\n`;
    sendCommand(packet);
}

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

updateColorUI(255, 255, 255);
refreshIcons();

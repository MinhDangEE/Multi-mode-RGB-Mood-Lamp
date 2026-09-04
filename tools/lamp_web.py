#!/usr/bin/env python3
"""Cầu nối web ↔ ST-Link cho RGB Mood Lamp.

ST-Link không tạo ra cổng COM nên trình duyệt không dùng Web Serial được.
Script này chạy OpenOCD ở chế độ server, mở một HTTP server ở localhost vừa
phục vụ trang dashboard vừa nhận lệnh từ nó, rồi đọc/ghi thẳng RAM và thanh
ghi của MCU qua SWD.

Trang được phục vụ từ http://localhost nên không dính lỗi mixed-content như
khi mở bản đặt trên Vercel.

    python3 tools/lamp_web.py            # rồi mở http://localhost:8000
"""
import json
import os
import re
import socket
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent
PROJ = HERE.parent
REPO = PROJ.parent
WEB_DIR = Path(os.environ.get("LAMP_WEB", REPO / "web"))
ELF = Path(os.environ.get("LAMP_ELF", PROJ / "build" / "STM32.elf"))
NM = os.environ.get("NM", "arm-none-eabi-nm")
PORT = int(os.environ.get("LAMP_PORT", "8000"))

TCL_HOST, TCL_PORT, SEP = "127.0.0.1", 6666, b"\x1a"
TIM3_CCR1, ADC1_DR = "0x40000434", "0x4001244C"
PWM_MAX, ADC_MAX = 999, 4095
STATE_CUSTOM = 4          # ngoài dải enum -> switch trong main loop bỏ qua, CCR do ta giữ


def die(msg):
    print(f"Lỗi: {msg}", file=sys.stderr)
    sys.exit(1)


def symbols():
    if not ELF.exists():
        die(f"không thấy {ELF} — build firmware trước (cd build && ninja)")
    try:
        out = subprocess.run([NM, str(ELF)], capture_output=True, text=True,
                             check=True).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        die(f"không chạy được {NM}: {exc}")
    table = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3:
            table[parts[2]] = "0x" + parts[0]
    need = ["current_state", "mode_changed_flag", "screen_dirty",
            "timer_remaining_sec", "timer_last_tick", "uwTick"]
    missing = [n for n in need if n not in table]
    if missing:
        die(f"thiếu biến trong ELF: {', '.join(missing)}")
    return {n: table[n] for n in need}


class Ocd:
    """Nói chuyện với OpenOCD qua cổng TCL. Một khoá cho mọi truy cập vì
    HTTP server chạy đa luồng còn socket thì không chia sẻ được."""

    def __init__(self):
        self.lock = threading.Lock()
        self.proc = subprocess.Popen(
            ["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f1x.cfg"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.sock = None
        for _ in range(60):
            if self.proc.poll() is not None:
                die("OpenOCD thoát sớm — ST-Link đã cắm chưa?")
            try:
                self.sock = socket.create_connection((TCL_HOST, TCL_PORT), timeout=3)
                break
            except OSError:
                time.sleep(0.2)
        if self.sock is None:
            self.close()
            die("OpenOCD không mở được cổng TCL 6666")

    def cmd(self, text):
        with self.lock:
            self.sock.sendall(text.encode() + SEP)
            buf = b""
            while not buf.endswith(SEP):
                chunk = self.sock.recv(8192)
                if not chunk:
                    raise ConnectionError("OpenOCD đóng kết nối")
                buf += chunk
            return buf[:-1].decode(errors="replace").strip()

    def read(self, addr, width, count=1):
        raw = self.cmd(f"read_memory {addr} {width} {count}")
        vals = [int(t, 0) for t in raw.split()]
        if len(vals) != count:
            raise ValueError(f"đọc {addr} trả về {raw!r}")
        return vals

    def write(self, addr, values, width=32):
        self.cmd(f"write_memory {addr} {width} {{{' '.join(str(v) for v in values)}}}")

    def close(self):
        try:
            if self.sock:
                self.sock.close()
        except OSError:
            pass
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()


class Lamp:
    def __init__(self, ocd, sym):
        self.o, self.s = ocd, sym
        self.custom = None          # (r, g, b) thang 0–255, None = firmware tự lo

    def _max_bright(self):
        return self.o.read(ADC1_DR, 32)[0] * PWM_MAX // ADC_MAX

    def state(self):
        mode = self.o.read(self.s["current_state"], 8)[0]
        r, g, b = self.o.read(TIM3_CCR1, 32, 3)
        adc = self.o.read(ADC1_DR, 32)[0]
        timer = self.o.read(self.s["timer_remaining_sec"], 32)[0]
        # Ở chế độ màu tự chọn, main loop không đụng CCR nữa nên ta phải tự áp
        # lại mỗi lần polling thì núm vặn độ sáng mới còn tác dụng.
        if mode == STATE_CUSTOM and self.custom:
            self._apply(self.custom)
        elif mode != STATE_CUSTOM:
            self.custom = None
        return {"ok": True, "mode": mode, "r": r, "g": g, "b": b,
                "adc": adc, "timer": timer, "custom": self.custom}

    def set_mode(self, mode):
        if mode not in (0, 1, 2, 3):
            raise ValueError("nấc phải nằm trong 0–3")
        self.custom = None
        self.o.write(self.s["current_state"], [mode], 8)
        self.o.write(self.s["mode_changed_flag"], [1], 8)

    def _apply(self, rgb):
        scale = self._max_bright()
        self.o.write(TIM3_CCR1, [v * scale // 255 for v in rgb])

    def set_color(self, r, g, b):
        for v in (r, g, b):
            if not 0 <= v <= 255:
                raise ValueError("mỗi kênh màu phải nằm trong 0–255")
        self.custom = (r, g, b)
        # Đẩy state ra ngoài dải enum: switch trong main loop không khớp case nào
        # nên nó thôi ghi đè CCR, màu ta đặt mới giữ được.
        self.o.write(self.s["current_state"], [STATE_CUSTOM], 8)
        self._apply(self.custom)

    def set_timer(self, minutes):
        if not 0 <= minutes <= 600:
            raise ValueError("số phút phải nằm trong 0–600")
        now = self.o.read(self.s["uwTick"], 32)[0]
        self.o.write(self.s["timer_last_tick"], [now])
        self.o.write(self.s["timer_remaining_sec"], [minutes * 60])
        self.o.write(self.s["screen_dirty"], [1], 8)


def make_handler(lamp):
    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *_):
            pass

        def _send(self, code, body, ctype):
            data = body if isinstance(body, bytes) else body.encode()
            self.send_response(code)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

        def _json(self, code, obj):
            self._send(code, json.dumps(obj, ensure_ascii=False), "application/json; charset=utf-8")

        def do_GET(self):
            path = self.path.split("?")[0]
            if path == "/api/state":
                try:
                    return self._json(200, lamp.state())
                except Exception as exc:
                    return self._json(503, {"ok": False, "error": str(exc)})
            name = "index.html" if path == "/" else path.lstrip("/")
            if not re.fullmatch(r"[\w.-]+(/[\w.-]+)*", name):
                return self._json(404, {"ok": False, "error": "không tìm thấy"})
            f = (WEB_DIR / name).resolve()
            if not str(f).startswith(str(WEB_DIR.resolve())) or not f.is_file():
                return self._json(404, {"ok": False, "error": "không tìm thấy"})
            types = {".html": "text/html; charset=utf-8", ".css": "text/css",
                     ".js": "text/javascript", ".json": "application/json"}
            self._send(200, f.read_bytes(), types.get(f.suffix, "application/octet-stream"))

        def do_POST(self):
            try:
                n = int(self.headers.get("Content-Length", 0))
                body = json.loads(self.rfile.read(n) or "{}")
            except (ValueError, json.JSONDecodeError):
                return self._json(400, {"ok": False, "error": "JSON không hợp lệ"})
            try:
                if self.path == "/api/mode":
                    lamp.set_mode(int(body["mode"]))
                elif self.path == "/api/color":
                    lamp.set_color(int(body["r"]), int(body["g"]), int(body["b"]))
                elif self.path == "/api/timer":
                    lamp.set_timer(int(body["minutes"]))
                else:
                    return self._json(404, {"ok": False, "error": "không có endpoint này"})
                return self._json(200, lamp.state())
            except (KeyError, TypeError):
                return self._json(400, {"ok": False, "error": "thiếu tham số"})
            except ValueError as exc:
                return self._json(400, {"ok": False, "error": str(exc)})
            except Exception as exc:
                return self._json(503, {"ok": False, "error": str(exc)})
    return Handler


def main():
    if not WEB_DIR.is_dir():
        die(f"không thấy thư mục web: {WEB_DIR}")
    sym = symbols()
    print(f"Địa chỉ biến đọc từ {ELF.name}: "
          + ", ".join(f"{k}={v}" for k, v in list(sym.items())[:3]) + " …")
    ocd = Ocd()
    lamp = Lamp(ocd, sym)
    try:
        st = lamp.state()
        print(f"ST-Link OK — nấc hiện tại {st['mode']}, PWM {st['r']}/{st['g']}/{st['b']}")
    except Exception as exc:
        ocd.close()
        die(f"không đọc được MCU: {exc}")

    srv = ThreadingHTTPServer(("127.0.0.1", PORT), make_handler(lamp))
    print(f"\n  Mở trình duyệt:  http://localhost:{PORT}\n  Ctrl+C để dừng\n")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nĐang dừng…")
    finally:
        srv.server_close()
        ocd.close()


if __name__ == "__main__":
    main()

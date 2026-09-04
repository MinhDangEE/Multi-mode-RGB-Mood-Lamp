#!/usr/bin/env python3
"""Theo dõi trạng thái RGB Mood Lamp qua cổng TCL của OpenOCD.

Không dùng vòng lặp `while` trong TCL vì OpenOCD kẹt trong đó sẽ không nhận
tín hiệu dừng, để lại tiến trình treo giữ luôn ST-Link. Thay vào đó OpenOCD
chạy như một server bình thường, còn vòng lặp nằm ở đây nên Ctrl+C thoát sạch.
"""
import os
import socket
import sys

HOST, PORT = "127.0.0.1", 6666
SEP = b"\x1a"

MODES = ("Trắng tĩnh", "Thở trắng", "Cầu vồng", "Tắt (Sleep)")


def rpc(sock, cmd):
    sock.sendall(cmd.encode() + SEP)
    buf = b""
    while not buf.endswith(SEP):
        chunk = sock.recv(8192)
        if not chunk:
            raise ConnectionError("OpenOCD đã đóng kết nối")
        buf += chunk
    return buf[:-1].decode(errors="replace").strip()


def words(sock, addr, width, count):
    raw = rpc(sock, f"read_memory {addr} {width} {count}")
    return [int(t, 0) for t in raw.split()]


def main():
    a_state = os.environ["A_STATE"]
    a_trem = os.environ["A_TREM"]
    ccr = os.environ.get("TIM3_CCR1", "0x40000434")
    adc = os.environ.get("ADC1_DR", "0x4001244C")

    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        print("Đang theo dõi — Ctrl+C để dừng\n")
        while True:
            state = words(sock, a_state, 8, 1)[0]
            r, g, b = words(sock, ccr, 32, 3)
            pot = words(sock, adc, 32, 1)[0]
            left = words(sock, a_trem, 32, 1)[0]

            name = MODES[state] if state < len(MODES) else f"? ({state})"
            filled = min(25, max(r, g, b) * 25 // 999)
            bar = "█" * filled + "·" * (25 - filled)
            clock = f"{left // 60}:{left % 60:02d}" if left else "--:--"

            sys.stdout.write(
                f"\r\033[K{name:<12} R{r:4d} G{g:4d} B{b:4d}  {bar}"
                f"  VR {pot * 100 // 4095:3d}%  ⏱ {clock}"
            )
            sys.stdout.flush()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
    except (ConnectionError, OSError) as exc:
        print(f"\nMất kết nối tới OpenOCD: {exc}", file=sys.stderr)
        sys.exit(1)

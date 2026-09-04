#!/usr/bin/env bash
#
# lamp.sh — điều khiển RGB Mood Lamp qua ST-Link (SWD), không cần USB-TTL.
#
# Script ghi thẳng vào biến trạng thái trong RAM của MCU, đúng những biến mà
# vòng lặp chính đọc mỗi chu kỳ. Địa chỉ được tra từ file .elf lúc chạy nên
# build lại firmware xong vẫn đúng, không cần sửa script.
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ="$(dirname "$HERE")"
ELF="${LAMP_ELF:-$PROJ/build/STM32.elf}"
BIN="${LAMP_BIN:-$PROJ/build/STM32.bin}"
NM="${NM:-arm-none-eabi-nm}"
OOCD_IF="${OOCD_IF:-interface/stlink.cfg}"
OOCD_TGT="${OOCD_TGT:-target/stm32f1x.cfg}"

# Thanh ghi ngoại vi (cố định theo datasheet STM32F103)
TIM3_CCR1=0x40000434   # CCR1/2/3 nằm liền nhau: đỏ, lục, lam
ADC1_DR=0x4001244C

die() { printf '\033[31mLỗi:\033[0m %s\n' "$*" >&2; exit 1; }

command -v openocd >/dev/null || die "chưa cài openocd"
command -v "$NM"   >/dev/null || die "không tìm thấy $NM"
[[ -f "$ELF" ]] || die "không thấy $ELF — build firmware trước đã (cd build && ninja)"

# --- Tra địa chỉ biến từ ELF -------------------------------------------------
sym() {
  local a
  a=$("$NM" "$ELF" | awk -v s="$1" '$3==s {print $1; exit}')
  [[ -n "$a" ]] || die "không tìm thấy biến '$1' trong $ELF"
  printf '0x%s' "$a"
}
A_STATE=$(sym current_state)        # 1 byte: 0 solid, 1 breath, 2 rainbow, 3 sleep
A_FLAG=$(sym mode_changed_flag)     # 1 byte: đặt 1 để firmware in UART + vẽ lại OLED
A_DIRTY=$(sym screen_dirty)         # 1 byte: báo vẽ lại OLED
A_TREM=$(sym timer_remaining_sec)   # 4 byte: giây còn lại của hẹn giờ
A_TLAST=$(sym timer_last_tick)      # 4 byte: mốc đếm giây
A_TICK=$(sym uwTick)                # 4 byte: HAL_GetTick()

# --- Chạy lệnh TCL trên OpenOCD ----------------------------------------------
oocd() {
  local args=(-f "$OOCD_IF" -f "$OOCD_TGT" -c init)
  for c in "$@"; do args+=(-c "$c"); done
  args+=(-c shutdown)
  openocd "${args[@]}" 2>&1
}

MODE_NAMES=("Trắng tĩnh" "Thở trắng" "Cầu vồng" "Tắt (Sleep)")

set_mode() {
  local n="$1"
  oocd "mwb $A_STATE $n" "mwb $A_FLAG 1" >/dev/null
  printf '\033[32m✓\033[0m Nấc %d — %s\n' "$((n + 1))" "${MODE_NAMES[$n]}"
}

cmd_status() {
  local out
  out=$(oocd \
    "set s  [read_memory $A_STATE 8 1]" \
    "set c  [read_memory $TIM3_CCR1 32 3]" \
    "set a  [read_memory $ADC1_DR 32 1]" \
    "set t  [read_memory $A_TREM 32 1]" \
    "echo \"OUT \$s \$c \$a \$t\"")
  echo "$out" | awk -v names="${MODE_NAMES[0]}|${MODE_NAMES[1]}|${MODE_NAMES[2]}|${MODE_NAMES[3]}" '
    function h2d(s,   i, c, n, d) {
      sub(/^0[xX]/, "", s); n = 0
      for (i = 1; i <= length(s); i++) {
        d = index("0123456789abcdef", tolower(substr(s, i, 1))) - 1
        if (d >= 0) n = n * 16 + d
      }
      return n
    }
    /^OUT /{
      split(names, nm, "|")
      st = h2d($2); r = h2d($3); g = h2d($4); b = h2d($5)
      adc = h2d($6); tmr = h2d($7)
      printf "  Chế độ        %d — %s\n", st+1, nm[st+1]
      printf "  PWM  R/G/B    %d / %d / %d   (thang 0–999)\n", r, g, b
      printf "  Biến trở      %d / 4095  →  %d%%\n", adc, int(adc*100/4095)
      if (tmr > 0) printf "  Hẹn giờ       còn %d:%02d\n", int(tmr/60), tmr%60
      else         printf "  Hẹn giờ       tắt\n"
      found = 1
    }
    END { if (!found) { print "  (không đọc được — kiểm tra ST-Link đã cắm chưa)"; exit 1 } }'
}

cmd_timer() {
  local min="${1:-}"
  [[ "$min" =~ ^[0-9]+$ ]] || die "cú pháp: lamp.sh timer <số phút>"
  local sec=$((min * 60))
  # Phải đồng bộ timer_last_tick với uwTick hiện tại, nếu không vòng lặp chính
  # sẽ thấy đã trễ hàng chục giây và trừ dồn một lúc cho tới khi bắt kịp.
  oocd \
    "set now [read_memory $A_TICK 32 1]" \
    "mww $A_TLAST \$now" \
    "mww $A_TREM $sec" \
    "mwb $A_DIRTY 1" >/dev/null
  if [[ $sec -eq 0 ]]; then
    printf '\033[32m✓\033[0m Đã hủy hẹn giờ\n'
  else
    printf '\033[32m✓\033[0m Hẹn giờ %d phút — hết giờ sẽ tự chuyển sang Sleep\n' "$min"
  fi
}

cmd_next() {
  local cur
  cur=$(oocd "set s [read_memory $A_STATE 8 1]" "echo \"OUT \$s\"" \
        | awk '
          function h2d(s,   i, d, n) {
            sub(/^0[xX]/, "", s); n = 0
            for (i = 1; i <= length(s); i++) {
              d = index("0123456789abcdef", tolower(substr(s, i, 1))) - 1
              if (d >= 0) n = n * 16 + d
            }
            return n
          }
          /^OUT /{ print h2d($2); exit }')
  [[ -n "${cur:-}" ]] || die "không đọc được trạng thái hiện tại"
  set_mode $(((cur + 1) % 4))
}

cmd_monitor() {
  command -v python3 >/dev/null || die "cần python3 cho lệnh monitor"
  if ss -ltn 2>/dev/null | grep -q ":6666 "; then
    die "cổng 6666 đang bận — có openocd khác đang chạy? (ps -e | grep openocd)"
  fi

  # OpenOCD chạy như server bình thường: nó xử lý tín hiệu dừng đúng cách.
  # Đừng dùng 'while' trong TCL — OpenOCD kẹt trong đó sẽ bỏ qua SIGTERM và
  # để lại tiến trình treo giữ luôn ST-Link.
  openocd -f "$OOCD_IF" -f "$OOCD_TGT" >/dev/null 2>&1 &
  local pid=$!
  # shellcheck disable=SC2064
  trap "kill $pid 2>/dev/null; wait $pid 2>/dev/null; printf '\n'" EXIT INT TERM

  local ready=0 i
  for i in $(seq 40); do
    if ss -ltn 2>/dev/null | grep -q ":6666 "; then ready=1; break; fi
    kill -0 "$pid" 2>/dev/null || die "OpenOCD thoát sớm — ST-Link đã cắm chưa?"
    sleep 0.2
  done
  [[ $ready -eq 1 ]] || die "OpenOCD không mở được cổng TCL 6666"

  A_STATE="$A_STATE" A_TREM="$A_TREM" TIM3_CCR1="$TIM3_CCR1" ADC1_DR="$ADC1_DR" \
    python3 "$HERE/lamp_monitor.py"
}

cmd_flash() {
  [[ -f "$BIN" ]] || die "không thấy $BIN"
  command -v st-flash >/dev/null || die "chưa cài st-flash"
  st-flash --reset write "$BIN" 0x8000000
}

usage() {
  cat <<EOF
lamp.sh — điều khiển RGB Mood Lamp qua ST-Link (SWD)

  lamp.sh solid          Nấc 1 — trắng tĩnh 100%
  lamp.sh breath         Nấc 2 — thở trắng (gamma 2.2)
  lamp.sh rainbow        Nấc 3 — cầu vồng 6 màu
  lamp.sh sleep          Nấc 4 — tắt hết đèn
  lamp.sh next           Chuyển sang nấc kế tiếp

  lamp.sh status         Đọc trạng thái hiện tại một lần
  lamp.sh monitor        Theo dõi liên tục (thay cho terminal UART)

  lamp.sh timer <phút>   Đặt hẹn giờ tự tắt (0 = hủy)
  lamp.sh flash          Nạp lại firmware từ build/STM32.bin

Địa chỉ biến tra từ: $ELF
EOF
}

case "${1:-}" in
  solid)   set_mode 0 ;;
  breath)  set_mode 1 ;;
  rainbow) set_mode 2 ;;
  sleep)   set_mode 3 ;;
  next)    cmd_next ;;
  status)  cmd_status ;;
  monitor) cmd_monitor ;;
  timer)   cmd_timer "${2:-}" ;;
  flash)   cmd_flash ;;
  ""|-h|--help|help) usage ;;
  *) die "lệnh không hợp lệ: $1  (chạy 'lamp.sh --help')" ;;
esac

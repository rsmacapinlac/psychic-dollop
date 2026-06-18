#!/usr/bin/env bash
#
# set-time.sh — set the scribr device clock over USB serial.
#
# scribr's firmware listens on its USB CDC serial port for a line:
#     TIME <unix-epoch-seconds-UTC>
# and writes that to the PCF85063 RTC + system clock (see clock::pollSerialTimeSet).
# This pushes the host's *current* UTC, so there is no snapshot drift or SD-card
# editing — unlike the one-shot rtc_set_utc= line in /scribr.cfg.
#
# Usage:
#   tools/set-time.sh [PORT] [EPOCH]
#     PORT   serial device (default: /dev/ttyACM0)
#     EPOCH  unix epoch seconds, UTC (default: now)
#
# Examples:
#   tools/set-time.sh                      # set /dev/ttyACM0 to now
#   tools/set-time.sh /dev/ttyACM1         # different port, now
#   tools/set-time.sh /dev/ttyACM0 1750257060
#
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
EPOCH="${2:-$(date -u +%s)}"

if [ ! -e "$PORT" ]; then
  echo "error: serial port '$PORT' not found (is the device plugged in?)" >&2
  exit 1
fi

# Native USB CDC does not reset on open, but -hupcl avoids any hang-up-on-close
# reset behavior; raw/-echo keeps the line we send byte-exact.
stty -F "$PORT" 115200 raw -echo -hupcl

printf 'TIME %s\n' "$EPOCH" > "$PORT"

human="$(date -u -d "@$EPOCH" +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || echo "epoch $EPOCH")"
echo "sent: TIME $EPOCH  ($human)"
echo "confirm with: arduino-cli monitor -p $PORT -c baudrate=115200"
echo "  (expect: 'scribr: RTC set from serial; epoch=$EPOCH UTC')"

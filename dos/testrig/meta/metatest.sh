#!/bin/bash
# metatest.sh - host unit test of the TSR's M record code (dos/flmeta.h): gcc build of metatest.c, bytes compared with
# serial-bench flproto.encode_meta and decoded with flproto.MetaText. SB=<serial-bench> (default /home/claude/serial-bench)
set -eu
H=$(cd "$(dirname "$0")" && pwd); SB=${SB:-/home/claude/serial-bench}
gcc -std=gnu99 -O2 -Wall -Wextra -fsanitize=undefined,address -o /tmp/metatest "$H/metatest.c"
printf 'one\tgpu=VGA|\nedge15\ta=bcdefghijklm|\nedge14\ta=bcdefghijkl|\nedge30\ta=bcdefghijklm|b=cdefghijklm|\nempty\t\nlong\tgpu=%s|cpu=y|\n' "$(printf 'G%.0s' $(seq 200))" \
  | /tmp/metatest | python3 -I "$H/metacheck.py" "$SB"

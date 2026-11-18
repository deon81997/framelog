#!/bin/sh
# r86.sh GO.BAT INI [FILE...] - priavte store-w98qi-86box (acpi, P3B-F PII-450, Cirrus 5446) in 86Box: Win98 runs
# D:\GO.BAT at login (WIN.INI run=), COM1 = socat pty -> serial-bench fllog.py --suite INI -> $R/csv, D:\ -> $R/out.
# env: T (timeout s, default 900), GFX (86Box gfxcard, default the VM's Cirrus), SB, VM.
set -e
H=$(cd "$(dirname "$0")/.." && pwd); SB=${SB:-/home/claude/serial-bench}; V=${VMS:-/home/claude/vms}
R=${R:-/tmp/r86}; W=$R/vm; GO=$1; INI=$2; shift 2
pkill -f "86Box -P $W" 2>/dev/null || true; pkill -f "socat.*vcom1" 2>/dev/null || true; sleep 1
rm -rf $R; mkdir -p $W $R/out
cp -r --sparse=always ${VM:-$V/w98-86box/acpi}/* $W/
cp --sparse=always $V/shared/test.img $W/test.img
C="-i $W/win98.img@@32256"; D="-i $W/test.img@@32256"
mtype $C ::/WINDOWS/WIN.INI | sed 's/^run=.*/run=D:\\GO.BAT/' | sed 's/\r*$/\r/' > $R/win.ini
mcopy -o $C $R/win.ini ::/WINDOWS/WIN.INI
mdel $D '::/*' 2>/dev/null || true
sed 's/\r*$/\r/' $GO > $R/GO.BAT; mcopy -o $D $R/GO.BAT ::/
for f in "$@"; do mcopy -o $D "$f" ::/; done
sed -i "s|^hdd_02_fn = .*|hdd_02_fn = $W/test.img|" $W/86box.cfg
[ -n "$GFX" ] && sed -i "s|^gfxcard = .*|gfxcard = $GFX|" $W/86box.cfg
printf '\n[Ports (COM & LPT)]\nserial1_enabled = 1\nserial1_device = pipe\n\n[Named Pipe (COM) #1]\npath = /tmp/vcom1\nmode = 0\n' >> $W/86box.cfg
rm -f /tmp/vcom1 /tmp/hcom1
setsid socat pty,raw,echo=0,link=/tmp/vcom1 pty,raw,echo=0,link=/tmp/hcom1 < /dev/null > /dev/null 2>&1 & sleep 1
ROMS=${ROMS:-/home/claude/86box/roms} sh $V/bin/86run.sh $W
timeout ${T:-900} python3 $SB/fllog.py /tmp/hcom1 --suite $INI -o $R/csv > $R/fllog.log 2>&1 &
FL=$!
i=0; while pgrep -f "86Box -P $W" > /dev/null && [ $i -lt ${T:-900} ]; do sleep 5; i=$((i + 5)); done
sleep 2; kill $FL 2>/dev/null || true; pkill -f "86Box -P $W" 2>/dev/null || true; pkill -f "socat.*vcom1" 2>/dev/null || true
sleep 2
mcopy -s -n $D '::/*' $R/out/ 2>/dev/null || true
echo "r86: ${i}s"; tail -15 $R/fllog.log

#!/bin/sh
# boot.sh GO.BAT [FILE|DIR...] - boot the persistent Voodoo3 VM (priavte store-w98qi-86box-v3: vms/w98-86box-v3/acpi,
# P3B-F PII-450, Voodoo3 3000 AGP + v3w9x 1.02.18, DX9.0c) once: WIN.INI run=D:\GO.BAT, D: = fresh copy of
# vms/shared/test.img in /tmp/v3d.img with GO.BAT + files, waits until Windows powers off (T s, default 1800),
# then copies D:\ to /tmp/v3out. Changes to C: STAY (that is the point: setup + game installs). Env: SHOTS=1 = png/60 s.
set -e
V=${VM:-/home/claude/vms/w98-86box-v3/acpi}; B=/home/claude/vms/bin; GO=$1; shift
ps aux | grep -q "[b]uild/src/86Box -P $V" && { echo "VM already running"; exit 1; }
C="-i $V/win98.img@@32256"; D="-i /tmp/v3d.img@@32256"
cp --sparse=always /home/claude/vms/shared/test.img /tmp/v3d.img; mdel $D '::/*' 2>/dev/null || true
mtype $C ::/WINDOWS/WIN.INI | sed 's/^run=.*/run=D:\\GO.BAT/' | sed 's/\r*$/\r/' > /tmp/v3win.ini; mcopy -o $C /tmp/v3win.ini ::/WINDOWS/WIN.INI
sed 's/\r*$/\r/' $GO > /tmp/v3go.bat; mcopy -o $D /tmp/v3go.bat ::/GO.BAT
for f in "$@"; do mcopy -s -o -Q $D "$f" ::/; done
sed -i "s|^hdd_02_fn = .*|hdd_02_fn = /tmp/v3d.img|" $V/86box.cfg
grep -q '^serial1_enabled' $V/86box.cfg || printf '\n[Ports (COM & LPT)]\nserial1_enabled = 1\nserial1_device = pipe\n\n[Named Pipe (COM) #1]\npath = /tmp/vcom1\nmode = 0\n' >> $V/86box.cfg
pkill -f "socat.*vcom1" 2>/dev/null || true; rm -f /tmp/vcom1 /tmp/hcom1
setsid socat pty,raw,echo=0,link=/tmp/vcom1 pty,raw,echo=0,link=/tmp/hcom1 < /dev/null > /dev/null 2>&1 & sleep 1
sh $B/86run.sh $V; sleep 3
i=0; while ps aux | grep -q "[b]uild/src/86Box -P $V" && [ $i -lt ${T:-1800} ]; do sleep 10; i=$((i + 10))
  [ -n "$SHOTS" ] && [ $((i % 60)) = 0 ] && sh $B/86run.sh $V shot /tmp/v3shot-$i.png 2>/dev/null; done
sh $B/86run.sh $V stop 2>/dev/null || true
mtype $C ::/WINDOWS/WIN.INI | sed 's/^run=.*/run=/' | sed 's/\r*$/\r/' > /tmp/v3win.ini; mcopy -o $C /tmp/v3win.ini ::/WINDOWS/WIN.INI
rm -rf /tmp/v3out; mkdir -p /tmp/v3out; mcopy -s -n $D '::/*' /tmp/v3out/ 2>/dev/null || true
echo "boot.sh: ${i}s"

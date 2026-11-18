#!/bin/sh
# gl86.sh SECS 'DOS line' ['DOS line'...] - DOS Glide rig: MS-DOS 7.1 (priavte store-dos71) on 86Box P3B-F PII-450 with
# a Voodoo3 3000 AGP (128 MB, COM1 = socat pty /tmp/vcom1 <-> /tmp/hcom1). C:\GL holds GLIDE2X.OVL (V=, default the
# Voodoo3 one from store-w98qi-86box-v3 C:\WINDOWS), DOS4GW, HDPMI32I, GLFLIP, FRAMELOG; T.BAT = the given lines (run
# from C:\GL by AUTOEXEC). After SECS: screenshot /tmp/glv/LAST.png, VM stopped, C:\GL\*.TXT copied to /tmp/glv/out/.
# env: W=/tmp/glv, V=ovl, LOG=name (fllog.py -> $W/name.csv, SB=serial-bench), SBZ=unzipped SBEMU zip (HDPMI32I), EXTRA="files to copy into C:\GL".
H=$(cd "$(dirname "$0")" && pwd); W=${W:-/tmp/glv}; SECS=$1; shift
V=${V:-/tmp/dl/v3ovl/GLIDE2X.OVL}; SBZ=${SBZ:-/tmp/dl/sbz/NUKEDSB/FILES}; R=/home/claude/vms
mkdir -p $W/vm $W/out
if [ ! -f $W/dos.img ] || [ -n "${REBUILD:-}" ]; then
  cp $R/dos71/msdos71.img $W/dos.img && mmd -i $W/dos.img@@32256 ::/GL
  printf '@ECHO OFF\r\nPROMPT $P$G\r\nPATH C:\\DOS;C:\\GL\r\nSET TEMP=C:\\\r\nC:\r\nCD \\GL\r\nIF EXIST C:\\T.BAT CALL C:\\T.BAT\r\n' | mcopy -o -i $W/dos.img@@32256 - ::/AUTOEXEC.BAT
fi
sed -e 's/^gfxcard = .*/gfxcard = voodoo3_3k_agp/' -e "s#^hdd_01_fn = .*#hdd_01_fn = $W/dos.img#" -e '/^hdd_02/d' \
    -e '/Cirrus/,/^bios/d' -e 's/^mem_size = .*/mem_size = 131072/' $R/dos71/86box/86box.cfg > $W/vm/86box.cfg
printf '\n[Ports (COM & LPT)]\nserial1_enabled = 1\nserial1_device = pipe\n\n[Named Pipe (COM) #1]\npath = /tmp/vcom1\nmode = 0\n' >> $W/vm/86box.cfg
cp -r $R/dos71/86box/nvr $W/vm/
mdel -i $W/dos.img@@32256 '::/GL/*.TXT' 2>/dev/null
mcopy -o -i $W/dos.img@@32256 $V ::/GL/GLIDE2X.OVL
mcopy -o -i $W/dos.img@@32256 /tmp/dl/ow/binw/dos4gw.exe $H/../output/glflip.exe $H/../output/framelog.exe $SBZ/HDPMI/HDPMI32I.EXE ${EXTRA:-} ::/GL/
for l in "$@"; do printf '%s\r\n' "$l"; done | mcopy -o -i $W/dos.img@@32256 - ::/T.BAT
pkill -f "socat.*vcom1"; rm -f /tmp/vcom1 /tmp/hcom1
setsid socat pty,raw,echo=0,link=/tmp/vcom1 pty,raw,echo=0,link=/tmp/hcom1 < /dev/null > /dev/null 2>&1 &
sleep 1
sh $R/bin/86run.sh $W/vm
SB=${SB:-/home/claude/serial-bench}
if [ -n "${LOG:-}" ]; then   # LOG=name: serial-bench fllog.py on COM1 -> $W/LOG.csv (+ .fllog = its stderr)
  python3 $SB/fllog.py /tmp/hcom1 -o $W/$LOG.csv --seconds $((SECS - 5)) --label "86box-PII450-v3-$LOG" -q 2> $W/$LOG.fllog &
fi
sleep $SECS
sh $R/bin/86run.sh $W/vm shot $W/LAST.png
sh $R/bin/86run.sh $W/vm stop
pkill -f "socat.*vcom1"
rm -f $W/out/*
mcopy -o -i $W/dos.img@@32256 '::/GL/*.TXT' $W/out/ 2>/dev/null
for f in $W/out/*; do [ -f "$f" ] && { echo "== $(basename $f)"; tr -d '\r' < $f; }; done

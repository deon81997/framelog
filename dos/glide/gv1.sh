#!/bin/sh
# gv1.sh SECS GAMEDIR DOSDIR 'DOS line' [...] - DOS Glide game rig on 86Box Voodoo 1: MS-DOS 7.1 (priavte store-dos71),
# P3B-F PII-450, Cirrus 5446 + 3Dfx Voodoo Graphics 4 MB (2+2), 64 MB, COM1 = socat pty /tmp/vcom1 <-> /tmp/hcom1.
# GAMEDIR (host, an extracted demo) is copied to C:\DOSDIR once per fresh image; C:\GL = FRAMELOG + HDPMI32I.
# T.BAT = the given lines (AUTOEXEC calls it from C:\). After SECS: shot $W/LAST.png, stop, C:\GL\*.TXT -> $W/out.
# env: W=/tmp/gv1, LOG=name (fllog -> $W/name.csv), REBUILD=1 fresh image, KEYSEQ="25:space 40:Return,Down" (xdotool keys at those seconds, multiple of 5),
#      KEYSEQ "+Key" = held 0.4 s (polling games miss taps), e.g. "150:+Escape";
#      MOUSESEQ="30:abs=480,470;click 35:rel=-20,0;click" (xdotool mouse, ; = steps),
#      SHOTS="t1 t2 ..." extra screenshots at those seconds, OVL=file to copy over C:\DOSDIR\...\GLIDE2X.OVL (OVLDST).
H=$(cd "$(dirname "$0")" && pwd); W=${W:-/tmp/gv1}; SECS=$1; GD=$2; DD=$3; shift 3
SBZ=${SBZ:-/tmp/dl/sbz/NUKEDSB/FILES}; R=/home/claude/vms; I="$W/dos.img@@32256"
mkdir -p $W/vm $W/out
if [ ! -f $W/dos.img ] || [ -n "${REBUILD:-}" ]; then
  cp $R/dos71/msdos71.img $W/dos.img && mmd -i $I ::/GL
  printf '@ECHO OFF\r\nPROMPT $P$G\r\nPATH C:\\DOS;C:\\GL\r\nSET TEMP=C:\\\r\nC:\r\nCD \\\r\nIF EXIST C:\\T.BAT CALL C:\\T.BAT\r\n' | mcopy -o -i $I - ::/AUTOEXEC.BAT
fi
mdir -i $I ::/$DD >/dev/null 2>&1 || { mmd -i $I ::/$DD; mcopy -s -i $I "$GD"/* ::/$DD/; }
[ -n "${OVL:-}" ] && mcopy -o -i $I "$OVL" "::/$DD/${OVLDST:-GLIDE2X.OVL}"
sed -e "s#^hdd_01_fn = .*#hdd_01_fn = $W/dos.img#" -e '/^hdd_02/d' $R/dos71/86box/86box.cfg > $W/vm/86box.cfg
sed -i 's/^\[Video\]/[Video]\nvoodoo = 1/' $W/vm/86box.cfg
printf '\n[3Dfx Voodoo Graphics]\ntype = 0\nframebuffer_memory = 2\ntexture_memory = 2\nbilinear = 1\nrender_threads = 2\nrecompiler = 1\nsli = 0\n' >> $W/vm/86box.cfg
printf '\n[Ports (COM & LPT)]\nserial1_enabled = 1\nserial1_device = pipe\n\n[Named Pipe (COM) #1]\npath = /tmp/vcom1\nmode = 0\n' >> $W/vm/86box.cfg
cp -r $R/dos71/86box/nvr $W/vm/
mdel -i $I '::/GL/*.TXT' 2>/dev/null
mcopy -o -i $I $H/../output/framelog.exe $SBZ/HDPMI/HDPMI32I.EXE $SBZ/MOUSE/CTMOUSE.EXE ${EXTRA:-} ::/GL/
for l in "$@"; do printf '%s\r\n' "$l"; done | mcopy -o -i $I - ::/T.BAT
pkill -f "socat.*vcom1"; rm -f /tmp/vcom1 /tmp/hcom1
setsid socat pty,raw,echo=0,link=/tmp/vcom1 pty,raw,echo=0,link=/tmp/hcom1 < /dev/null > /dev/null 2>&1 &
sleep 1
sh $R/bin/86run.sh $W/vm
SB=${SB:-/home/claude/serial-bench}
[ -n "${LOG:-}" ] && python3 $SB/fllog.py /tmp/hcom1 -o $W/$LOG.csv --seconds $((SECS - 5)) --label "86box-PII450-v1-$LOG" --gpu "Voodoo Graphics (86Box)" -q 2> $W/$LOG.fllog &
t=0
while [ $t -lt $SECS ]; do
  sleep 5; t=$((t + 5))
  for s in ${SHOTS:-}; do [ $s = $t ] && sh $R/bin/86run.sh $W/vm shot $W/s$t.png; done
  for kk in ${MOUSESEQ:-}; do [ "${kk%%:*}" = $t ] && { MW=$(DISPLAY=:86 xdotool search --name "^vm - 86Box" | tail -1); for c in $(echo "${kk#*:}" | tr ';' ' '); do
      case $c in click) DISPLAY=:86 xdotool windowactivate --sync $MW click 1;; rel=*) DISPLAY=:86 xdotool mousemove_relative -- $(echo ${c#rel=} | tr , ' ');; abs=*) DISPLAY=:86 xdotool mousemove --window $MW $(echo ${c#abs=} | tr , ' ');; esac; sleep 0.5; done; }; done
  for kk in ${KEYSEQ:-}; do [ "${kk%%:*}" = $t ] && for k in $(echo "${kk#*:}" | tr , ' '); do case $k in
      +*) KW=$(DISPLAY=:86 xdotool search --name "^vm - 86Box" | tail -1); DISPLAY=:86 xdotool windowactivate --sync $KW keydown ${k#+}; sleep 0.4; DISPLAY=:86 xdotool keyup ${k#+};;
      *) sh $R/bin/86run.sh $W/vm key $k;; esac; done; done
done
sh $R/bin/86run.sh $W/vm shot $W/LAST.png
sh $R/bin/86run.sh $W/vm stop
pkill -f "socat.*vcom1"
rm -f $W/out/*
mcopy -o -i $I '::/GL/*.TXT' $W/out/ 2>/dev/null
for f in $W/out/*; do [ -f "$f" ] && { echo "== $(basename $f)"; tr -d '\r' < $f | tail -15; }; done
grep -iE "voodoo|glide|error|illegal" $W/vm/86box.log | tail -5

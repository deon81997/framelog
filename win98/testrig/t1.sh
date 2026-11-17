#!/bin/sh
# t1.sh - QEMU Win98: FRAMELOG + the test programs (DDraw flip/blt, GL gdi/wgl, fake Glide 2), fllog suite -> /tmp/t1
H=$(cd "$(dirname "$0")/.." && pwd); SB=${SB:-/home/claude/serial-bench}; R=/tmp/t1
rm -rf $R; mkdir -p $R
sed 's/$/\r/' $H/testrig/${BAT:-T1.BAT} > $R/GO.BAT
cp $H/FRAMELOG.EXE $H/FLHOOK.DLL $H/test/*.EXE $H/test/glide2x.dll $R/
( T=${T:-500} $H/testrig/q98.sh $R/GO.BAT $R/*.EXE $R/*.DLL $R/glide2x.dll > $R/q98.log 2>&1 ) &
sleep 3
timeout ${T:-500} python3 $SB/fllog.py socket://127.0.0.1:4598 --suite $H/testrig/${INI:-t1.ini} -o $R/csv > $R/fllog.log 2>&1
wait
tail -3 $R/q98.log; tail -20 $R/fllog.log
for f in /tmp/q98out/*.TXT /tmp/q98out/*.LOG; do echo "== $f"; cat "$f"; done 2>/dev/null | head -60

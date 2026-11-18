#!/bin/sh
# run.sh GO.BAT FILES... - boot.sh on the Voodoo3 VM; fllog.py --suite logs COM1 the whole boot (20 run sections,
# split at B/E only: one CSV per FRAMELOG run, game-gpu-res.csv) -> /tmp/v3out/csv. T = timeout s.
H=$(cd "$(dirname "$0")" && pwd); SB=${SB:-/home/claude/serial-bench}
printf '[suite]\nname = v3-86box\ngap = 0\nmin_frames = 30\ncpu = Pentium II 450 (86Box)\n' > /tmp/v3suite.ini
for i in $(seq 10 29); do printf '[r%s]\n' $i >> /tmp/v3suite.ini; done
rm -rf /tmp/v3csv
sh $H/boot.sh "$@" > /tmp/v3boot.log 2>&1 &
BP=$!; sleep 4
timeout ${T:-1800} python3 $SB/fllog.py /tmp/hcom1 --suite /tmp/v3suite.ini -o /tmp/v3csv > /tmp/v3fllog.log 2>&1 &
FP=$!; wait $BP; sleep 2; kill -INT $FP 2>/dev/null; sleep 3; kill $FP 2>/dev/null
cp -r /tmp/v3csv /tmp/v3out/csv 2>/dev/null; cat /tmp/v3boot.log; ls /tmp/v3out/csv 2>/dev/null

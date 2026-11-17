#!/bin/bash
# e2e.sh ZIP [plain|sbemu|rm|rmsbemu] [FPS] [FLIPFLAGS] - (rm*: rm.img with JEMMEX, FRAMELOG /RM, real-mode RMFLIP.COM 70 fps) FRAMELOG end to end in QEMU MS-DOS 7.1: FLIP flips at FPS (every 10th frame
# double) for 12 s, COM1 -> TCP -> serial-bench fllog.py -> CSV -> flstats.py. ZIP: NUKED SB-EMU zip with ROMs.
# SB=<serial-bench checkout> (default /home/claude/serial-bench). Prints the DOS log, stats, and checks the frame pattern.
set -u
H=$(cd "$(dirname "$0")" && pwd); W=/tmp/fle2e; MUX=${MUXED:-/home/claude/deon81997/nuked-sbemu-muxed}/sc55-pack/testrig
SB=${SB:-/home/claude/serial-bench}; ZIP=$1; MODE=${2:-plain}; FPS=${3:-70}; FF=${4:-}   # FF=B: FLIP flips with VESA INT 10h 4F07h (VBE mode 101h)
mkdir -p $W; sh $MUX/menustat/mkimg.sh "$ZIP" $W >/dev/null || exit 2
mcopy -o -i $W/pm.img@@32256 $H/../output/framelog.exe $H/../output/flip.exe $H/../output/rmflip.com $H/E2E.BAT ::/ || exit 2
printf '@ECHO OFF\r\nCALL C:\\E2E.BAT %s %s %s\r\n' $MODE $FPS "$FF" > $W/T.BAT
port=$((4600 + RANDOM % 300))
export QAUDIO=${QAUDIO:-none,id=snd,timer-period=2000}
IMG=pm; case $MODE in rm*) IMG=rm; mcopy -o -i $W/rm.img@@32256 $H/../output/framelog.exe $H/../output/flip.exe $H/../output/rmflip.com $H/E2E.BAT ::/ || exit 2;; esac
$MUX/common/run.sh -t 240 $W/$IMG.img $W/T.BAT -- -chardev socket,id=c1,host=127.0.0.1,port=$port,server=on,wait=off -serial chardev:c1 > $W/dos.log 2>&1 &
rp=$!
for i in $(seq 100); do sleep 0.3; python3 $SB/fllog.py socket://127.0.0.1:$port -o $W/run.csv --idle 4 --label "qemu-$MODE-$FPS" -q 2> $W/fllog.err && break; grep -q "Connection refused\|could not open" $W/fllog.err || break; done
wait $rp
tr -d '\r' < $W/dos.log
python3 $SB/flstats.py $W/run.csv --png $W/run.png
python3 - "$W/run.csv" "$FPS" <<'P'
import sys
rows=[l.split(',') for l in open(sys.argv[1]) if l[0].isdigit()]; fps=float(sys.argv[2]); per=1000/fps
ft=[float(r[2]) for r in rows]
lng=[i for i,v in enumerate(ft) if v>1.5*per]; short=[v for v in ft if v<=1.5*per]
short.sort(); med=short[len(short)//2] if short else 0
ok=len(ft)>=fps*10 and short and abs(med-per)<0.02*per
gaps=[b-a for a,b in zip(lng,lng[1:])]
ok=ok and gaps and sum(g==10 for g in gaps)>=0.8*len(gaps)   # QEMU host stalls: FLIP catches up
print(f"check: {len(ft)} frames, normal median {med:.3f} ms (want {per:.3f}), long every 10th: {sum(g==10 for g in gaps)}/{len(gaps)} -> {'PASS' if ok else 'FAIL'}")
P

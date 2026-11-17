#!/bin/bash
# perf86.sh ZIP [none idle log] - 1d no-slowdown check on 86Box P3B-F PII-450 + ES1371 (MUXED vesawc/86box):
# SC55LOG speed (x real time) and SBDIAG load of SBEMU + SC-55 while FLIP flips at 70 fps with MIDI, per case.
set -u
H=$(cd "$(dirname "$0")" && pwd); W=/tmp/flperf; MUX=${MUXED:-/home/claude/deon81997/nuked-sbemu-muxed}/sc55-pack/testrig
ZIP=$1; shift; CASES=${*:-none idle log}; FAST=${FAST:-}   # FAST=fast skips SC55LOG
mkdir -p $W; sh $MUX/menustat/mkimg.sh "$ZIP" $W >/dev/null || exit 2
mcopy -o -i $W/pm.img@@32256 $H/../output/framelog.exe $H/../output/flip.exe $H/PERF.BAT ::/ || exit 2
for c in $CASES; do
  printf '@ECHO OFF\r\nCALL C:\\PERF.BAT %s %s\r\n' $c "$FAST" > $W/T_$c.BAT
  $MUX/common/run.sh -e 86box -c $MUX/vesawc/86box/86box.cfg -k -t ${TMO:-1500} $W/pm.img $W/T_$c.BAT > $W/$c.result 2> $W/$c.err
  d=$(sed -n 's/^work dir: //p' $W/$c.err); mkdir -p $W/$c
  for f in SC55LOG.OUT SBD.TXT FLIP.TXT FLST.TXT FLOUT.TXT SC55/SC55LOG.TXT; do mcopy -o -i $d/disk.img@@32256 ::/$f $W/$c/ 2>/dev/null; done
  rm -rf "$d"; echo "== $c: $(tr -d '\r' < $W/$c.result | tail -1)"
  tr -d '\r' < $W/$c/SC55LOG.TXT 2>/dev/null | grep -aiE "x real time" | head -3 | cut -c1-150
  tr -d '\r' < $W/$c/SBD.TXT 2>/dev/null | grep -aE "Measured|interrupts \(|uses .*CPU|lost:|needs .*real time" | cut -c1-150
  cat $W/$c/FLIP.TXT $W/$c/FLST.TXT 2>/dev/null | tr -d '\r'
done

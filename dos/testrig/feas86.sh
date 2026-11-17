#!/bin/bash
# feas86.sh ZIP [N B C] - 1a sound check on 86Box P3B-F PII-450 + ES1371 (MUXED testrig/vesawc/86box): SBEMU + SC-55
# with and without FLTSR; prints FLTEST verdicts, trap cost in TSC ticks and SBDIAG's load/output lines. ZIP needs ROMs.
set -u
H=$(cd "$(dirname "$0")" && pwd); W=/tmp/feas86; MUX=${MUXED:-/home/claude/deon81997/nuked-sbemu-muxed}/sc55-pack/testrig
ZIP=$1; shift; MODES=${*:-N B C}
mkdir -p $W; sh $MUX/menustat/mkimg.sh "$ZIP" $W >/dev/null || exit 2
python3 $MUX/midplay/genmid.py $W/T.MID 60 2 >/dev/null
mcopy -o -i $W/pm.img@@32256 $H/../output/fltsr.exe $H/../output/fltest.exe $H/FEAS86.BAT $W/T.MID ::/ || exit 2
for m in $MODES; do
  printf '@ECHO OFF\r\nCALL C:\\FEAS86.BAT %s\r\n' $m > $W/T_$m.BAT
  $MUX/common/run.sh -e 86box -c $MUX/vesawc/86box/86box.cfg -k -t ${TMO:-900} $W/pm.img $W/T_$m.BAT > $W/$m.result 2> $W/$m.err; echo "rc=$?" >> $W/$m.result
  d=$(sed -n 's/^work dir: //p' $W/$m.err); mkdir -p $W/$m
  for f in FT0 FT1 FT2 SBD SBOUT; do mcopy -o -i $d/disk.img@@32256 ::/$f.TXT $W/$m/ 2>/dev/null; done
  rm -rf "$d"; echo "== mode $m: $(tail -1 $W/$m.result)"
  cat $W/$m/FT1.TXT $W/$m/FT2.TXT 2>/dev/null | tr -d '\r' | grep -E "^(PASS|FAIL)|FAIL|ticks|DSP"
  tr -d '\r' < $W/$m/SBD.TXT 2>/dev/null | grep -aE "Measured|output|needs .*real time|lost|interrupts \(|uses .*CPU" | cut -c1-160
done

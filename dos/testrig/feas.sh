#!/bin/bash
# feas.sh ZIP [A B C] - milestone 1a in QEMU MS-DOS 7.1 (priavte store-dos71). ZIP = a NUKED SB-EMU zip WITH ROMs
# (personal pack: the public zip sends MPU MIDI to USB). Needs MUXED=<NUKED-SBEMU-MUXED checkout> for testrig/common.
set -u
H=$(cd "$(dirname "$0")" && pwd); W=/tmp/feas; MUX=${MUXED:-/home/claude/deon81997/nuked-sbemu-muxed}/sc55-pack/testrig
ZIP=$1; shift; MODES=${*:-A B C}
mkdir -p $W; sh $MUX/menustat/mkimg.sh "$ZIP" $W >/dev/null || exit 2
mcopy -o -i $W/pm.img@@32256 $H/../output/FLTSR.EXE $H/../output/FLTEST.EXE $H/FEAS.BAT ::/ 2>/dev/null || \
mcopy -o -i $W/pm.img@@32256 $H/../output/fltsr.exe $H/../output/fltest.exe $H/FEAS.BAT ::/
export QAUDIO=${QAUDIO:-none,id=snd,timer-period=2000}
for m in $MODES; do
  printf '@ECHO OFF\r\nCALL C:\\FEAS.BAT %s\r\n' $m > $W/T_$m.BAT
  $MUX/common/run.sh -k -t 240 $W/pm.img $W/T_$m.BAT > $W/$m.result 2> $W/$m.err; echo "rc=$?" >> $W/$m.result
  d=$(sed -n 's/^work dir: //p' $W/$m.err); mkdir -p $W/$m
  for f in FT0 FT1 FT2 MPU MPUERR SBD SBOUT; do mcopy -o -i $d/disk.img@@32256 ::/$f.TXT $W/$m/ 2>/dev/null; done
  rm -rf "$d"; echo "== mode $m: $(tail -1 $W/$m.result)"; cat $W/$m/FT0.TXT $W/$m/FT1.TXT 2>/dev/null | tr -d '\r'
done

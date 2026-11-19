#!/bin/sh
# gstest.sh [DEMOS]: host test of glstatic.c on synthetic files + every EXE/BDD/OVL of the store-dosglide-demos
# (DEMOS = dir with carm/ (splatdem.zip), xa/ (xa_demo.exe), ad/ (addemo.exe), tr1/ (tomb3dem.zip),
# tsd/ (tsddemo_3dfx.zip), scr2/ (SCREAMER2.zip) extracted; default /tmp/dl). Optional patches (no demo data, host
# scan only; checked when present): tomb/ (tomb3dfx.exe, Tomb Raider 3dfx 2.1.1, LE, fence), d2v/ (D2VOODOO.ZIP >
# VOODOO.ZIP, Descent II 2.2, LE, fence), tsdup/ (TSDUPGRD.ZIP, Toshinden retail upgrade, 2.03h2).
# Expected: grBufferSwap found in CARM3DFX's 3DFX_DOS.BDD, the SST-1 OVLs (OVL offsets = their export,
# dos/glide/leinfo.py), TOMB.EXE (Glide 2.03h3, start) and TSD.EXE (2.03h2, end), nowhere else. Screamer 2 demo
# (STARTL.EXE) has no Glide (software only). Redguard ships no Glide (uses AD's OVL): not listed.
set -e
d=$(cd "$(dirname "$0")" && pwd); D=${1:-/tmp/dl}
cc -O2 -Wall -o /tmp/gstest "$d/gstest.c"
set --
for f in "$D"/carm/*.EXE "$D"/carm/*.BDD "$D"/xa/ROOT/*.EXE "$D"/ad/*.EXE "$D"/tr1/tomb3dem/*.EXE "$D"/tsd/*.EXE \
         "$D"/scr2/*.EXE "$D"/scr2/*/*.EXE; do
    case $f in *3DFX_DOS.BDD|*/TOMB.EXE|*/TSD.EXE) ;; *) set -- "$@" "$f=none";; esac; done
[ -f "$D/tomb/tomb3dfx.exe" ] && set -- "$@" "$D/tomb/tomb3dfx.exe=0x7b2fc"
[ -f "$D/d2v/d2voodoo.exe" ] && set -- "$@" "$D/d2v/d2voodoo.exe=0x139b64"
[ -f "$D/tsdup/TSD.EXE" ] && set -- "$@" "$D/tsdup/TSD.EXE=end:0x861bd" "$D/tsdup/SETUP.EXE=none"
/tmp/gstest "$@" "$D/carm/3DFX_DOS.BDD=0x53bc" "$D/xa/ROOT/GLIDE2X.OVL=0xf5f8" "$D/ad/GLIDE2X.OVL=0x10708" \
    "$D/ad/VRUSH/GLIDE2X.OVL=none" "$D/tr1/tomb3dem/TOMB.EXE=0x5f874" "$D/tsd/TSD.EXE=end:0x856ed" > /tmp/gstest.out 2>&1 && r=0 || r=1
grep -c "^ok  " /tmp/gstest.out | sed 's/$/ files ok/'; grep -v " none want none$" /tmp/gstest.out | sed "s#$D/##g"
exit $r

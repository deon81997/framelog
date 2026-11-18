#!/bin/sh
# gstest.sh [DEMOS]: host test of glstatic.c on synthetic files + every EXE/BDD/OVL of the 4 store-dosglide-demos
# (DEMOS = dir with carm/ (splatdem.zip), xa/ (xa_demo.exe), ad/ (addemo.exe) extracted; default /tmp/dl).
# Expected: grBufferSwap found in CARM3DFX's 3DFX_DOS.BDD and the SST-1 OVLs only (OVL offsets = their export,
# dos/glide/leinfo.py), nowhere else. Redguard ships no Glide (uses AD's OVL): not listed.
set -e
d=$(cd "$(dirname "$0")" && pwd); D=${1:-/tmp/dl}
cc -O2 -Wall -o /tmp/gstest "$d/gstest.c"
set --
for f in "$D"/carm/*.EXE "$D"/carm/*.BDD "$D"/xa/ROOT/*.EXE "$D"/ad/*.EXE; do case $f in *3DFX_DOS.BDD) ;; *) set -- "$@" "$f=none";; esac; done
/tmp/gstest "$@" "$D/carm/3DFX_DOS.BDD=0x53bc" "$D/xa/ROOT/GLIDE2X.OVL=0xf5f8" "$D/ad/GLIDE2X.OVL=0x10708" \
    "$D/ad/VRUSH/GLIDE2X.OVL=none" > /tmp/gstest.out 2>&1 && r=0 || r=1
grep -c "^ok  " /tmp/gstest.out | sed 's/$/ files ok/'; grep -v " none want none$" /tmp/gstest.out | sed "s#$D/##g"
exit $r

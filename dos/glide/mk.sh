#!/bin/sh
# Build GLFLIP.EXE with OpenWatcom v2 (snapshot: curl -L github.com/open-watcom/open-watcom-v2/releases/download/
# Current-build/ow-snapshot.tar.xz, untar to WATCOM=/tmp/dl/ow). Output: dos/output/glflip.exe
set -e
: ${WATCOM:=/tmp/dl/ow}
export WATCOM PATH=$WATCOM/binl64:$PATH INCLUDE=$WATCOM/h
d=$(cd "$(dirname "$0")" && pwd)
o=$d/../output
mkdir -p $o
cd $o
for f in glflip glload; do wcc386 -q -bt=dos -5r -ox -zq -i=$d $d/$f.c -fo=$f.obj; done
wlink system dos4g option quiet file glflip.obj file glload.obj name glflip.exe libpath $WATCOM/lib386 libpath $WATCOM/lib386/dos
ls -la glflip.exe

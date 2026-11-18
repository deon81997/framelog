#!/bin/sh
# mkzip.sh [SUFFIX] - build release/FRAMELOG-<FL_VER><SUFFIX>.zip (e.g. ./pack/mkzip.sh -test1)
#   FRAMELOG/ FRAMELOG.EXE FLIP.EXE FRAMELOG.TXT   (game PC, DOS 8.3 names)
#   WIN98/    FRAMELOG.EXE FLHOOK.DLL FRAMEL98.TXT, TEST/ DDTEST D3DTEST GLTEST G2TEST   (Win98 game PC)
#   HOST/     fllog.py flstats.py flproto.py flmeta.py flreport.py README.md LOG.BAT   (host PC)
# Rebuilds the DOS exes first (DJGPP i586-pc-msdosdjgpp-gcc must be on PATH) and the Win98 files (make -C win98:
# i686-w64-mingw32-gcc, msvcrt; the import check win98/tools/chkimp98.py must pass).
# Host files come from serial-bench: SB=<path> (default ../serial-bench next to this repo), taken from its
# committed HEAD when it is a git repo (SB_WORKTREE=1 = use the working tree instead).
# Only the files listed above go in: no ROMs, HDPMI, SBEMU or private files.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SB=${SB:-$ROOT/../serial-bench}
SB=$(cd "$SB" && pwd)
VER=$(sed -n 's/^#define FL_VER *"\([^"]*\)".*/\1/p' "$ROOT/dos/flstate.h")
[ -n "$VER" ] || { echo "mkzip: FL_VER not found in dos/flstate.h" >&2; exit 1; }
NAME=FRAMELOG-$VER${1:-}
ZIP=$ROOT/release/$NAME.zip

make -C "$ROOT/dos" output/framelog.exe output/flip.exe
make -C "$ROOT/win98" check

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/FRAMELOG" "$TMP/HOST" "$TMP/WIN98/TEST" "$ROOT/release"
cp "$ROOT/dos/output/framelog.exe" "$TMP/FRAMELOG/FRAMELOG.EXE"
cp "$ROOT/dos/output/flip.exe" "$TMP/FRAMELOG/FLIP.EXE"
cp "$ROOT/pack/FRAMELOG.TXT" "$TMP/FRAMELOG/FRAMELOG.TXT"
cp "$ROOT/pack/LOG.BAT" "$TMP/HOST/LOG.BAT"
cp "$ROOT/win98/FRAMELOG.EXE" "$ROOT/win98/FLHOOK.DLL" "$TMP/WIN98/"
cp "$ROOT/pack/FRAMEL98.TXT" "$TMP/WIN98/FRAMEL98.TXT"
for t in DDTEST D3DTEST GLTEST G2TEST; do cp "$ROOT/win98/test/$t.EXE" "$TMP/WIN98/TEST/"; done
for f in fllog.py flstats.py flproto.py flmeta.py flreport.py README.md; do
    if [ "${SB_WORKTREE:-0}" != 1 ] && git -C "$SB" rev-parse --git-dir >/dev/null 2>&1; then
        git -C "$SB" show "HEAD:$f" > "$TMP/HOST/$f"
    else
        cp "$SB/$f" "$TMP/HOST/$f"
    fi
done

rm -f "$ZIP"
(cd "$TMP" && TZ=UTC touch -t 202601010000 FRAMELOG/* HOST/* WIN98/* WIN98/TEST/* && zip -X -q -r "$ZIP" FRAMELOG HOST WIN98)
echo "built $ZIP"

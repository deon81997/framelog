#!/bin/sh
# mkzip.sh [SUFFIX] - build release/FRAMELOG-<FL_VER><SUFFIX>.zip (e.g. ./pack/mkzip.sh -test1)
#   FRAMELOG/ FRAMELOG.EXE FLIP.EXE FRAMELOG.TXT   (game PC, DOS 8.3 names)
#   HOST/     fllog.py flstats.py flproto.py README.md LOG.BAT   (host PC)
# Rebuilds the DOS exes first (DJGPP i586-pc-msdosdjgpp-gcc must be on PATH).
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

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/FRAMELOG" "$TMP/HOST" "$ROOT/release"
cp "$ROOT/dos/output/framelog.exe" "$TMP/FRAMELOG/FRAMELOG.EXE"
cp "$ROOT/dos/output/flip.exe" "$TMP/FRAMELOG/FLIP.EXE"
cp "$ROOT/pack/FRAMELOG.TXT" "$TMP/FRAMELOG/FRAMELOG.TXT"
cp "$ROOT/pack/LOG.BAT" "$TMP/HOST/LOG.BAT"
for f in fllog.py flstats.py flproto.py README.md; do
    if [ "${SB_WORKTREE:-0}" != 1 ] && git -C "$SB" rev-parse --git-dir >/dev/null 2>&1; then
        git -C "$SB" show "HEAD:$f" > "$TMP/HOST/$f"
    else
        cp "$SB/$f" "$TMP/HOST/$f"
    fi
done

rm -f "$ZIP"
(cd "$TMP" && TZ=UTC touch -t 202601010000 FRAMELOG/* HOST/* && zip -X -q -r "$ZIP" FRAMELOG HOST)
echo "built $ZIP"

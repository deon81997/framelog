#!/bin/sh
# mkhost.sh DIR [ZIP] - the host-PC folder (DIR/HOST) and, with ZIP, the stand-alone host pack zip (HOST/ only):
#   HOST/  FLLOG.BAT CARDS.BAT LINKTEST.BAT REPORT.BAT LOG.BAT HOST.TXT  fllog.py flstats.py flproto.py flmeta.py flreport.py flcap.py README.md
#          90s/ star.gif bar.gif vee.gif (flreport's page art)
# Python files come from serial-bench: SB=<path> (default ../serial-bench), its committed HEAD when it is a git repo
# (SB_WORKTREE=1 = the working tree). Called by mkzip.sh; runs alone too (no compilers needed).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SB=${SB:-$ROOT/../serial-bench}
SB=$(cd "$SB" && pwd)
H=$1/HOST
mkdir -p "$H/90s"
for f in FLLOG.BAT CARDS.BAT LINKTEST.BAT REPORT.BAT LOG.BAT HOST.TXT; do cp "$ROOT/pack/$f" "$H/$f"; done
for f in fllog.py flstats.py flproto.py flmeta.py flreport.py flcap.py README.md \
         docs/90s/star.gif docs/90s/bar.gif docs/90s/vee.gif; do
    d=$H/${f#docs/}; case $f in *.py|*.md) d=$H/$f;; esac
    if [ "${SB_WORKTREE:-0}" != 1 ] && git -C "$SB" rev-parse --git-dir >/dev/null 2>&1; then
        git -C "$SB" show "HEAD:$f" > "$d"
    else
        cp "$SB/$f" "$d"
    fi
done
TZ=UTC touch -t 202601010000 "$H"/* "$H"/90s/*
if [ -n "${2:-}" ]; then
    Z=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
    rm -f "$Z"
    (cd "$1" && zip -X -q -r "$Z" HOST)
    echo "built $Z"
fi

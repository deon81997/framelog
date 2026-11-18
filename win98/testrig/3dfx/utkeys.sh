#!/bin/sh
# utkeys.sh [RUNS] [SECS] - drive Unreal Tournament's own timedemo while an 86Box VM runs GO.BAT (mkbat.sh ut):
# when the 86Box window shrinks to a 640x480 guest mode (UT fullscreen), wait 45 s (load), Tab "timedemo 1",
# wait SECS (60), Tab "timedemo 0" (UT logs "N frames rendered in S seconds. X FPS average." to its log), Tab "exit".
# Repeats for RUNS (2: without + with FRAMELOG) mode switches.
export DISPLAY=:86; R=${1:-2}; S=${2:-60}
k() { W=$(xdotool search --name "86Box" | tail -1); xdotool windowactivate --sync $W; xdotool "$@"; }
cmd() { k key Tab; sleep 2; k type --delay 120 "$1"; sleep 1; k key Return; sleep 2; }
w() { W=$(xdotool search --name "86Box" | tail -1); xdotool getwindowgeometry $W 2>/dev/null | sed -n 's/.*Geometry: \([0-9]*\)x.*/\1/p'; }
n=0; while [ $n -lt $R ] && ps aux | grep -q "[b]uild/src/86Box"; do
  x=$(w); if [ -n "$x" ] && [ "$x" -lt 760 ]; then
    sleep 45; cmd "timedemo 1"; echo "utkeys: run $n timedemo on $(date +%T)"; sleep $S; cmd "timedemo 0"; sleep 3; cmd "exit"
    echo "utkeys: run $n done $(date +%T)"; n=$((n + 1)); while [ "$(w)" -lt 760 ] 2>/dev/null; do sleep 5; done
  fi; sleep 5; done

#!/bin/sh
# autook.sh - while an 86Box VM runs: every 15 s, if Expendable's "Timedemo '1' mode activated!" box is up
# (crop matches expinfo.png), press Enter. Run in the background next to v3/run.sh.
H=$(cd "$(dirname "$0")" && pwd); export DISPLAY=:86
sleep 30
while ps aux | grep -q "[b]uild/src/86Box"; do
  W=$(xdotool search --name "86Box" | tail -1)
  if [ -n "$W" ] && timeout 15 import -window "$W" /tmp/ao.png 2>/dev/null; then
    G=$(identify -format '%h' /tmp/ao.png); convert /tmp/ao.png -crop 834x$((G-76))+0+52 +repage -crop 170x100+333+272 +repage /tmp/ao2.png 2>/dev/null
    AE=$(compare -metric AE -fuzz 10% /tmp/ao2.png "$H/expinfo.png" null: 2>&1 | cut -d' ' -f1)
    case $AE in ''|*[!0-9.]*) ;; *) [ "${AE%.*}" -lt 800 ] && { xdotool windowactivate --sync $W key Return; echo "autook: Enter $(date +%T)"; };; esac
  fi; sleep 15; done

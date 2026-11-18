#!/bin/sh
# go.sh CARD TAG PARTS... - one VM boot of the 3dfx game batch (mkbat.sh) on vms/w98-86box-CARD/acpi with the
# FRAMELOG build in win98/, autook.sh (+ utkeys.sh if ut) on the host; results -> /home/claude/res/CARD-TAG
# (D:\ files + csv/). Detached-safe: run with setsid nohup; T = time-out s (default 3600).
H=$(cd "$(dirname "$0")" && pwd); W=$(cd "$H/../.." && pwd); C=$1; TAG=$2; shift 2
mkdir -p /tmp/rig && cp -r "$H/../v3" /tmp/rig/ && sh "$H/mkbat.sh" $C "$@" > /tmp/rig/G.BAT
(sleep 20; sh "$H/autook.sh") > /tmp/autook.log 2>&1 &
case " $* " in *" ut "*) (sleep 20; sh "$H/utkeys.sh") > /tmp/utkeys.log 2>&1 & ;; esac
cd /tmp/rig && VM=/home/claude/vms/w98-86box-$C/acpi T=${T:-3600} sh v3/run.sh G.BAT $W/FRAMELOG.EXE $W/FLHOOK.DLL > /tmp/run-$C.log 2>&1
R=/home/claude/res/$C-$TAG; rm -rf $R; mkdir -p /home/claude/res; cp -r /tmp/v3out $R; cp /tmp/autook.log /tmp/utkeys.log /tmp/run-$C.log $R/ 2>/dev/null
echo "go.sh done $(date +%T)" >> $R/run-$C.log

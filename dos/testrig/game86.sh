#!/bin/bash
# game86.sh ZIP GAME [SECONDS] [nofl] - a real game's attract demo on 86Box P3B-F PII-450 + ES1371 with SBEMU + SC-55 and
# FRAMELOG; COM1 = socat pty pair (/tmp/vcom1 <-> /tmp/hcom1) -> serial-bench fllog.py -> /tmp/flgame/GAME.csv + stats.
# GAME: duke heretic rott (unpacked in G=/tmp/g, see dos/testrig/README.md). Screenshot at the end: /tmp/flgame/GAME.png.
set -u
H=$(cd "$(dirname "$0")" && pwd); W=/tmp/flgame; MUX=${MUXED:-/home/claude/deon81997/nuked-sbemu-muxed}/sc55-pack/testrig
SB=${SB:-/home/claude/serial-bench}; G=${G:-/tmp/g}; ZIP=$1; GAME=$2; SECS=${3:-90}; NOFL=${4:-}
mkdir -p $W
if [ ! -f $W/pm.img ] || [ -n "${REBUILD:-}" ]; then
  sh $MUX/menustat/mkimg.sh "$ZIP" $W >/dev/null || exit 2
  mmd -i $W/pm.img@@32256 ::/G 2>/dev/null
  for g in duke heretic rott; do [ -d $G/$g ] && mcopy -o -s -i $W/pm.img@@32256 $G/$g ::/G/; done
fi
mcopy -o -i $W/pm.img@@32256 $H/../output/framelog.exe $H/../output/flip.exe $H/GAME.BAT ::/ || exit 2
# configs: SB 220/7/1/5 (SBEMU default A220 I7 D1 H5), General MIDI on the MPU-401 330h (SC-55)
printf '[Setup]\nSetupVersion = "1.3D"\n\n[Screen Setup]\nScreenMode = %s\nScreenWidth = 320\nScreenHeight = 200\n\n[Sound Setup]\nFXDevice = 0\nMusicDevice = 4\nFXVolume = 220\nMusicVolume = 200\nNumBits = 16\nMixRate = 22000\nNumChannels = 2\nNumVoices = 8\nSoundToggle = 1\nMusicToggle = 1\nVoiceToggle = 1\nAmbienceToggle = 1\nReverseStereo = 0\nBlasterAddress = 0x220\nBlasterInterrupt = 7\nBlasterDma8 = 1\nBlasterDma16 = 5\nBlasterType = 6\nBlasterEmu = 0x620\nMidiPort = 0x330\n\n[Comm Setup]\nUserPath = ""\nRTSName = "DUKE.RTS"\n' "${DUKEMODE:-2}" | sed 's/$/\r/' > $W/DUKE3D.CFG
printf 'snd_musicdevice 6\nsnd_sfxdevice 3\nsnd_sbport 544\nsnd_sbirq 7\nsnd_sbdma 1\nsnd_mport 816\n' | sed 's/$/\r/' > $W/HERETIC.CFG
mcopy -o -i $W/pm.img@@32256 $W/DUKE3D.CFG ::/G/DUKE/; mcopy -o -i $W/pm.img@@32256 $W/HERETIC.CFG ::/G/HERETIC/
mkdir -p $W/vm; cp -r $MUX/vesawc/86box/* $W/vm/
printf '\n[Ports (COM & LPT)]\nserial1_enabled = 1\nserial1_device = pipe\n\n[Named Pipe (COM) #1]\npath = /tmp/vcom1\nmode = 0\n' >> $W/vm/86box.cfg
pkill -f "socat.*vcom1"; rm -f /tmp/vcom1 /tmp/hcom1
setsid socat pty,raw,echo=0,link=/tmp/vcom1 pty,raw,echo=0,link=/tmp/hcom1 < /dev/null > /dev/null 2>&1 & sleep 1
printf '@ECHO OFF\r\nCALL C:\\GAME.BAT %s %s\r\n' $GAME "$NOFL" > $W/T.BAT
L=$GAME${NOFL:+-nofl}
$MUX/common/run.sh -e 86box -c $W/vm/86box.cfg -t $((SECS + 60)) -k -s $W/$L.png $W/pm.img $W/T.BAT > $W/$L.result 2>&1 &
rp=$!
python3 $SB/fllog.py /tmp/hcom1 -o $W/$L.csv --seconds $((SECS + 55)) --label "86box-PII450-$GAME" -q 2> $W/$L.fllog
wait $rp; pkill -f "socat.*vcom1"
d=$(sed -n 's/^work dir: //p' $W/$L.result); [ -n "$d" ] && { for f in FLST.TXT FLOUT.TXT SBOUT.TXT; do mcopy -o -i $d/disk.img@@32256 ::/$f $W/$L-$f 2>/dev/null; done; rm -rf $d; }
python3 $SB/flstats.py $W/$L.csv --skip ${SKIP:-0} --png $W/$L-graph.png
grep "^#" $W/$L.csv | tr '\n' ' '; echo

# FRAMELOG test rigs

Need: DJGPP on PATH (`make -C dos`), priavte store-dos71 + store-86rig, a NUKED SB-EMU zip WITH ROMs
(priavte store-personal-1.0.0-beta.130-usb), NUKED-SBEMU-MUXED checkout (MUXED=, its testrig/common/run.sh),
serial-bench (SB=). One VM rig at a time (shared /tmp/testrig-run-latest, 2-core box).

| Script | Emulator | What |
|---|---|---|
| feas.sh ZIP A B C | QEMU | 1a: FLTSR + FLTEST (trap selectors, word OUT, SB DSP still answers) |
| feas86.sh ZIP N B C | 86Box PII-450 | 1a: SBDIAG with/without FLTSR while the SC-55 plays |
| e2e.sh ZIP plain/sbemu FPS | QEMU | FRAMELOG + FLIP, COM1 -> TCP -> fllog.py, frame pattern check |
| perf86.sh ZIP none idle log log3da | 86Box PII-450 | 1d: SC55LOG speed, SBDIAG, FLIP spare-CPU loops (FAST=fast skips SC55LOG) |
| game86.sh ZIP duke/heretic/rott/flip/flipv/flipw [SECS] | 86Box PII-450 | real game attract demo, COM1 -> socat pty -> fllog.py, CSV + graph + screenshot (DUKEMODE=0/2, FLOPT=/3DA[:ms], ROTTMUS/ROTTBITS); flipv = FLIP 35 fps vsync+MIDI, flipw = vsync only |

Games for game86.sh (G=/tmp/g): from priavte store-personal `nuked-sbemu-muxed/testgames/`:
`unzip 3dduke13.zip; unzip DN3DSW13.SHR -d /tmp/g/duke`, `cat HTIC_V12.1 HTIC_V12.2 > h.zip; unzip h.zip -d /tmp/g/heretic`.
game86.sh writes DUKE3D.CFG / HERETIC.CFG (SB 220/7/1/5 = SBEMU's default IRQ 7, GM on 330h).

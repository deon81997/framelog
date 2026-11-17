# HANDOFF — FRAMELOG

## RESUME HERE (2026-11-17) - MILESTONE 1 DONE (1a-1d), next: milestone 2

Done: 1a feasibility, 1b FRAMELOG.EXE TSR (dos/framelog.c), 1c host tools (deon81997/serial-bench: fllog.py,
flstats.py, flproto.py + tests), 1d overhead on 86Box PII-450. Not tested yet: real hardware (D810, P5PE-VM),
a real game, a real null-modem cable to a host PC. 3DAh-vs-SC-55 test skipped (3DAh mode = milestone 2).

1d numbers (86Box P3B-F PII-450 dynarec + ES1371, SBEMU + SC-55 mk1 resident, FLIP 30 s 70 fps + MIDI chord changes):
| case | SC55LOG | SBDIAG CPU / lost ticks / longest | FLIP spare loops |
|---|---|---|---|
| none | 0.48x real time | 61% / 0.0% / 3.55-3.61 ms | 578.69M, 578.67M |
| FRAMELOG idle | 0.48x | 61% / 0.0% / 3.47 ms | (not measured) |
| FRAMELOG logging (/AUTO, COM1 115200) | 0.48x | 61% / 0.0% / 3.56-3.61 ms | 576.79M, 576.60M (-0.33..0.36%) |
-> game-visible cost ~0.35% of its spare CPU at 70 fps (4 trapped CRTC writes per flip, ~20 us per flip);
SBEMU/SC-55 unchanged. Under 1%: CRTC logging stays on by default. 1921/1921 frames, 0 lost, 0 record drops.

QEMU e2e (dos/testrig/e2e.sh, COM1 -> TCP -> fllog.py): SBEMU + SC-55 resident, 852/852 frames, median 14.278 ms
at 70 fps (want 14.286); QEMU host stalls make FLIP catch up (runt frames), judge medians there.

Next 3 steps (milestone 2):
1. Real-hardware check list for the user (one list): D810 + P5PE-VM, FRAMELOG after SC55 in AUTOEXEC, null-modem
   to the host, fllog.py + flstats.py with Duke 3D / Doom (PM games). Compare with SBDIAG off/on.
2. /3DA mode (opt-in): trap 3DAh, frame = vsync edge seen by the game; measure vs SC-55 underruns on PII-450.
3. VESA 4F07h hook (INT 10h in PM via DPMI? or the RM vector) + /RM via JEMM/QEMM (qemm.c from SBEMU).
Known limits: only protected-mode games are trapped (HDPMI traps PM only); S/X take effect at the next flip
(no pump without CRTC writes - by design, no IRQ0/INT 1Ch); state found via 0040:00F0 (BIOS ICA) - move to AMIS
INT 2Dh later if a program clashes.

Findings (keep):
- HDPMI calls each client's handler with that client's own CS/DS (BX at install); our wrapper's stack = our DS.
  FLTEST: bad_ds = bad_ss = 0 over 8000+ calls, SBEMU's DSP still answers from the same program.
- OUT 3D4h,AX arrives as TWO byte calls (3D4h, then 3D5h). The 3D5h half's value = the handler's RETURN value
  shifted right 8 -> an OUT handler must return EAX unchanged. Byte calls carry stale EAX upper bits: use AL only.
- Trap cost on 86Box P3B-F PII-450 (dynarec): ~1700-1840 TSC ticks per trapped CRTC write idle,
  3300-4300 while the SC-55 renders (its ISR lands inside). A flip = 2-4 trapped writes -> ~0.1-0.3% CPU at 70 fps.
- SBDIAG with SC-55 + MIDPLAY 30 s: N (no FLTSR) / B (SBEMU then FLTSR) / C (FLTSR then SBEMU) identical:
  59% CPU, 344 int/s, longest 3.1-3.7 ms, 0.0% ticks lost, output 22050 Hz.
- QEMU's AC97 rig renders no audio (SBDIAG output 0 Hz): sound checks only on 86Box.
- Never call uclock() in test tools: it reprograms the PIT, which SBEMU watches. Use RDTSC.

Resume:
- export PATH=/tmp/dl/djgpp/djgpp/bin:$PATH; make -C dos   (DJGPP: curl github.com/andrewwutw/build-djgpp
  releases/download/v3.4/djgpp-linux64-gcc1220.tar.bz2; `gh release download` is 403 for unattached repos)
- VMs: priavte store-dos71 + store-86rig (86Box build). ROM zip: priavte store-personal-1.0.0-beta.130-usb
  (/tmp/sp/store/*.zip; never in this repo). NUKED-SBEMU-MUXED cloned read-only at /home/claude/deon81997/nuked-sbemu-muxed.
- QEMU: dos/testrig/feas.sh ZIP A B C, e2e.sh ZIP plain|sbemu 70 (~1 min). 86Box (B86=/home/claude/86box/src/build/src/86Box):
  feas86.sh ZIP N B C, perf86.sh ZIP none idle log (FAST=fast skips SC55LOG; ~2.5 min/case, ~1 min fast).


Test downloads: /tmp/dl/djgpp (DJGPP gcc 12.2, andrewwutw v3.4).

## Status

| Milestone | State |
|---|---|
| 1a feasibility (2 trap clients under HDPMI32i) | DONE 2026-11-17: feasible (QEMU A/B/C + 86Box N/B/C) |
| 1b DOS TSR | DONE: dos/framelog.c (FRAMELOG.EXE /COM: /AUTO /STATUS) |
| 1c Python host logger | DONE: serial-bench fllog.py / flstats.py |
| 1d overhead measurement | DONE: ~0.35% game CPU at 70 fps, SBEMU/SC-55 unchanged |

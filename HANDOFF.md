# HANDOFF — FRAMELOG

## RESUME HERE (2026-11-17) - 1a DONE, feasible

1a result: two resident HDPMI32i trap clients work. FLTSR (dos/feas) traps 3D4h/3D5h next to SBEMU + SC-55,
both load orders. Next: 1b (real TSR), 1c (serial-bench logger), 1d (86Box overhead).

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
- QEMU: dos/testrig/feas.sh ZIP A B C (~1 min each). 86Box: B86=<86Box> dos/testrig/feas86.sh ZIP N B C (~1.5 min each).

Next 3 steps:
1. 1b: dos/framelog.c from fltsr.c: ring buffer of TSC, polled UART pump from inside the trap (sbemu/serial.c, one FIFO
   load per pump), start/stop byte from the host, /COM: option, protocol header with TSC calibration.
   No PIT, no IRQ0, no port 60h. Write docs/PROTOCOL.md first.
2. 1c: serial-bench: logger (pyserial / QEMU socket://), CSV, stats (avg, min, 1%/0.1% lows), graph.
3. 1d: 86Box PII-450 SBDIAG + SC55LOG with/without FRAMELOG and a 70 fps flipper; >1% = off by default.

Test downloads: /tmp/dl/djgpp (DJGPP gcc 12.2, andrewwutw v3.4).

## Status

| Milestone | State |
|---|---|
| 1a feasibility (2 trap clients under HDPMI32i) | DONE 2026-11-17: feasible (QEMU A/B/C + 86Box N/B/C) |
| 1b DOS TSR | not started |
| 1c Python host logger | not started |
| 1d overhead measurement | not started — needs VMs |

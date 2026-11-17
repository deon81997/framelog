# FRAMELOG DOS side

- `lib/` - borrowed from NUKED-SBEMU-MUXED sc55-pack (GPL): hdpmipt.c/.h (port traps, nested-trap stack fix;
  FPU save and SBEMU diag hooks removed), qemm.h, dpmi/ (DJGPP DPMI helpers + TSR), untrapio, pic, serial.
- `feas/` - milestone 1a: FLTSR.EXE (resident CRTC trap, stats block) + FLTEST.EXE (checker).
- `framelog.c` - the TSR; `flmeta.h` - 'M' record (run info) text builders + encoder, plain C (host-tested by
  `testrig/meta/metatest.sh` against serial-bench flproto.encode_meta).
- `testrig/` - feas.sh (QEMU), feas86.sh (86Box PII-450 + ES1371), FEAS*.BAT. e2e/game86/perf86: see testrig/README.md.

Build: DJGPP gcc 12.2 (andrewwutw/build-djgpp v3.4) on PATH, `make`.

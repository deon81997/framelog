# FRAMELOG

Frame-time logger for DOS and Windows 98 games. Frame timestamps are sent over
serial to a second (host) PC. Goal: lower overhead than FRAPS, no on-screen
overlay. All logging, stats and graphs happen on the host.

**No per-game patching:** load once and every game is logged. No system or
game files are ever replaced or modified.

## Components

1. **DOS TSR** (DJGPP, resident under HDPMI32i, same model as SBEMU)
   - Loaded once from AUTOEXEC.BAT or a launch batch file; system-wide.
   - Traps CRTC 3D4h/3D5h; writes to index 0Ch/0Dh (start address) = page flip.
     Handles 8-bit and 16-bit `OUT 3D4h,AX` forms.
   - Optional modes: 3DAh vsync-poll trap (mode 13h games, opt-in, expensive);
     INT 10h AX=4F07h hook (VESA flips); `/RM` via JEMM/QEMM for real-mode games.
   - The trap handler only stores a TSC timestamp in a ring buffer.
2. **Win98 background loader + hook DLL** (FRAPS-style)
   - Loader installs a global window hook (SetWindowsHookEx) so the hook DLL
     loads into every GUI process. Closing the loader turns it off.
   - Hook DLL patches in memory: DirectDraw/D3D Flip, OpenGL wglSwapBuffers,
     Glide grBufferSwap (glide2x/glide3x).
   - On each swap: RDTSC -> lock-free queue -> worker thread writes to COM.
   - Fallback: `framelog game.exe` starts the game suspended, injects the DLL,
     resumes it.
   - No proxy DLLs; ddraw.dll / opengl32.dll / glide*.dll are never replaced.
3. **Shared binary serial protocol**: header (CPU clock from TSC calibration,
   source DOS/Win98), per-frame record (sync byte, frame count, TSC delta),
   start/stop markers.
4. **Host tools (Python)**: COM logger -> CSV; stats (avg, min, 1% / 0.1% lows,
   frame-time graphs, multi-machine comparison). Later: OpenCV analyzer for
   Datapath VGA capture recordings (frame changes + tear lines), merged with
   the serial timeline to show dropped/runt frames.

**Out of scope:** overlays, video recording, NT/2000, Win98 DOS-box games
(possible later as a VxD milestone).

## Coexistence with Nuked SB-EMU + Nuked-SC55 (DOS)

- Never hooks IRQ0 or touches the PIT (SBEMU learns the game's timer rate from
  the PIT and hooks INT 1Ch). Serial data is pumped from inside the CRTC trap.
- No keyboard hotkeys, no port 60h trap (SBEMU `/KEYTRAP` owns it).
  Start/stop is a byte sent from the host over serial (same on Win98).
- Own, configurable COM port; must not clash with SBEMU `/DBG` or serial MIDI.
- Traps VGA ports only; never anything SBEMU traps.

## Repositories

- **framelog** (this repo): DOS TSR, Win98 loader + hook DLL, serial protocol
  spec, project README/HANDOFF.
- **serial-bench**: host tools (Python logger, stats, graphs, capture analyzer).

## Borrowed code

From `sc55-pack/` in NUKED-SBEMU-MUXED (GPL, same author): `hdpmipt.c/.h`,
`qemm.h` (port traps incl. nested-trap stack switch fix), untrapped I/O
helpers, polled UART (`sbemu/serial.*`, `sbemu/dbglink.c`), TSC calibration,
TSR residency under HDPMI32i (`main.c`). Only what is needed; the TSR stays
small.

## Milestones

1. a) Feasibility: two resident HDPMI32i trap clients (FRAMELOG + SBEMU).
   b) DOS TSR: CRTC flip logging + TSC + serial. c) Python logger + stats/graph.
   d) Overhead measurement (SBDIAG, SC55LOG on 86Box PII-450); >1% = off by default.
2. 3DAh mode, VESA hook, `/RM`.
3. Win98 loader + hook DLL (DirectDraw/D3D + Glide first, launch fallback).
4. OpenGL hook, capture analyzer, merged timelines.

## License

GPL (inherits from borrowed SBEMU code).

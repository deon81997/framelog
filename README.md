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
2. **Win98 loader + hook DLL** (`win98/`: FRAMELOG.EXE + FLHOOK.DLL, Windows 95/98/ME)
   - Loader installs global window hooks (WH_CBT + WH_GETMESSAGE) so the hook DLL loads into every GUI
     process; the first process that presents is logged until it exits. Q / Esc / `FRAMELOG /STOP` unhooks.
   - DirectDraw/D3D 5-7: `Flip` and `Blt` to the primary through the surface vtables (DDRAW.DLL's shared,
     writable vtables on Win9x) -> stubs in shared memory -> the process's wrapper. Glide 2/3 `grBufferSwap`
     and `wglSwapBuffers` (opengl32, 3dfx MiniGL): 5-byte JMP in the process's own copy; gdi32 `SwapBuffers`
     and `LoadLibrary` through the importers' IAT (gdi32/kernel32 code is shared and read-only on Win9x).
   - Per present: RDTSC -> lock-free shared ring -> the loader's loop writes the COM port; 'M' run info
     (game, gpu/driver, res/depth, api, cpu) once per start.
   - `framelog game.exe [args]`: that game only; FLHOOK.DLL is also patched in at its entry point
     (CreateProcess suspended + a JMP to a shared-memory stub that LoadLibrary's it - no CreateRemoteThread on 9x).
   - No proxy DLLs; ddraw.dll / opengl32.dll / glide*.dll are never replaced. Cost on 86Box PII-450 within
     noise (+-0.5% spare CPU) for DirectDraw, D3D 6, OpenGL and Glide test programs (win98/test).
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

## Using it (milestone 1, DOS)

Game PC, AUTOEXEC.BAT or the game's batch file, after HDPMI32I (or after SC55 when NUKED SB-EMU is used):
`FRAMELOG /COM:1` (COM1-4 or a hex port, optional `,baud`; default COM1,115200). `FRAMELOG /STATUS` shows counters.
Host PC (serial-bench): `python3 fllog.py COM3 -o run.csv --gpu "Voodoo3 3000" --res 640x480 --game Duke3D`, play,
Ctrl+C, then `python3 flstats.py run.csv --png run.png`. Benchmark batch: `python3 fllog.py COM3 --suite suite.ini
--report report/` logs one CSV per game (game-gpu-res.csv) and writes HTML pages per GPU + a side-by-side page
(`flreport.py results/ -o report/` for all GPUs). Games no trap sees: VGA capture + `flcap.py`. Coverage: serial-bench docs/GAMES.md. Logs protected-mode games (DOS4GW, DJGPP, PMODE/W...); real-mode games need /RM (milestone 2).
Measured cost on a Pentium II 450: ~0.35% of the game's CPU at 70 fps, no change to SBEMU / SC-55 load.

## Goal and order

Close to a one-stop shop for benchmarking retro GPUs (DOS + Win98).
Done: DOS TSR (CRTC trap, /3DA, VESA hook, 'M' run info), Win98 hook DLL (DirectDraw/D3D 5-7, Glide 2/3,
OpenGL, 'M' run info), host logger + stats, run metadata, suites, HTML reports, capture analyzer (flcap.py, needs
a real capture to confirm).
Next: hardware results (DOS + Win98 games: Quake II, Forsaken, 3DMark99), D3D8/9 Present, DOS Glide, /RM.

## Repositories

- **framelog** (this repo): DOS TSR, Win98 loader + hook DLL, serial protocol
  spec, project README/HANDOFF.
- **serial-bench**: host tools (Python logger with metadata + suites, stats, HTML reports, capture analyzer,
  game coverage list docs/GAMES.md).

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

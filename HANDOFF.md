# HANDOFF — FRAMELOG

## RESUME HERE (2026-11-17 handoff) - GOAL: one-stop retro GPU benchmark. Next: Win98 hook DLL

Repos: deon81997/framelog (this repo: DOS TSR, Win98 loader/DLL in win98/, protocol, HANDOFF = the only handoff),
deon81997/serial-bench (host tools: logger, stats, reports, later capture analyzer; no handoff of its own),
deon81997/priavte (PLAYBOOK, LESSONS, VM store), NUKED-SBEMU-MUXED read-only (borrowed code + testrig).
State: milestone 1 done, /3DA done. No hardware results yet - continue without them.

Priorities (user 11/17: wider game coverage before any public post; aim = close to a one-stop shop for retro GPUs):
1. Win98 loader + hook DLL (was milestone 3): global SetWindowsHookEx loader, DLL patches in memory DirectDraw
   Flip/Blt (D3D DX5-7 presents through it), Glide grBufferSwap (glide2x/glide3x), then OpenGL wglSwapBuffers/
   SwapBuffers incl. 3dfx MiniGL in the game folder; RDTSC -> lock-free queue -> worker thread -> COM (same protocol,
   source=1 Win98, modes bit 4); "framelog game.exe" launch fallback (CreateProcess suspended + inject). No proxy DLLs,
   no replaced files. Build: MinGW i686 win32 (Ubuntu g++-mingw-w64-i686-win32 = msvcrt; chkimp2-style import check
   for Win98, see priavte LESSONS: no _strtoui64, CreateThread needs a thread-id pointer). Test: priavte
   store-w98qi-86box (86Box PII-450, Voodoo/S3 + DirectX) and store-w98qi-qemu; COM1 -> socat pty -> fllog.py.
   Test apps: write tiny DDraw-flip / D3D / Glide / OpenGL test programs first, then demos (Quake 2 / Forsaken /
   3DMark99 if obtainable as free downloads - list what's needed from the user once).
2. Run metadata: GPU, driver, resolution, depth, CPU in the header/CSV (new 'M' text record, host --label fields).
3. VESA 4F07h hook for DOS (design below), then suites (one batch of timedemos, host logs one CSV per run) and a
   report generator in serial-bench (per-GPU page + side-by-side), and a game coverage list (docs/GAMES.md).
4. DOS Glide research (Voodoo MMIO: in-memory grBufferSwap hook in the loaded glide2x.ovl, or a Voodoo2
   swap-history register - unverified), capture-card analyzer (milestone 4), /RM last.

VESA design (for step 3): a 28-byte real-mode stub in our DOS block (seg+1:0) on the RM INT 10h vector (PM games'
INT 10h is reflected to RM): cmp ax,4F07h / jne chain / test bl,7Dh (00h,02h,80h,82h = set/schedule) / jnz chain /
call far [cs:cb] / chain: jmp far [cs:old]   bytes: 3D 07 4F 75 0A F6 C3 7D 75 05 2E FF 1E 14 00 2E FF 2E 18 00,
cb@20 old@24; cb = _go32_dpmi_allocate_real_mode_callback_retf -> rdtsc + CRTC burst merge + frame_event. VBE 2.0
PM-interface flips on VGA-compatible cards already hit the CRTC trap. Test: Duke ScreenMode = 1 640x480 on 86Box.
/RM: SBEMU qemm.c via JEMM + QPIEMU (SC55JEM.BAT), same handlers.

Restore: DJGPP (curl github.com/andrewwutw/build-djgpp/releases/download/v3.4/djgpp-linux64-gcc1220.tar.bz2 ->
/tmp/dl/djgpp), priavte store-* branches (README table), ROM zip store-personal-1.0.0-beta.130-usb, test games
store-personal (dos/testrig/README.md). apt: qemu-system-x86 zstd mtools socat xvfb xdotool openbox imagemagick
librtmidi6; pip: pyserial matplotlib. One VM rig at a time (2 cores).

## 2026-11-17 night - VESA hook done; /RM + milestone-2 checkpoint in progress (user-asked chat, dos/ only)

VESA hook (default on, /NOVESA; header modes bit 2): INT 10h 4F07h via RM stub + RMCB (QEMU FLIP B: 836/836 calls,
e2e PASS with SBEMU) AND the VBE 2.0 PM interface (4F0Ah). Correction to the design above: Duke 3D VESA 640x480 on
86Box Voodoo3 never calls 4F07h and writes no CRTC - it takes the PM table (VESADBG log: 4F00, 4F01 x2, 4F02 4101h,
4F0Ah). Now we copy the table to seg+5:0 with a 19-byte thunk in front of SetDisplayStart (IN 3D4h with
EAX=464Cxxxxh -> crtc_trap -> flip; the code is position-independent per VBE 2.0). Duke: 596 frames, median 54.6 ms
(~18 fps), stop=marker, game fine. Stub source dos/tools/vesastub.asm -> dos/vesastub.h (tools/bin2h.py);
make VESADBG=1 = every 4F07h/4F0Ah call as text on the UART (read with cat /tmp/hcom1, not fllog).

## 2026-11-17 late - /3DA mode + ROTT done, waiting for hardware results (test2 zip)

Test build: release/FRAMELOG-0.1-test2.zip (pack/mkzip.sh -test2), replaces test1.
/3DA[:ms] (dos/framelog.c vsync_trap): traps 3DAh; frame = first retrace start the game reads after >= 6 ms away
from 3DAh (one vsync wait = one frame even over 2+ retraces). CRTC trap then only pumps; pump also on every retrace edge.
86Box PII-450 + SBEMU + SC-55:
- FLIP V (render half a frame, wait 2 retraces): 3/5-refresh frames exactly (10% long), 0 lost.
- With MIDI playing, 4 ms re-arm gave ~10% runt double counts (ISR 3.6 ms + trap time mid-wait); 6 ms: none -> default 6.
- Cost (perf86.sh FAST=fast FLIPARGS="MV 35" none log3da): SBDIAG 61% / 0.0% lost both; FLIP frames 395 vs 394;
  3.19M 3DAh traps in 30 s; FLIP spare loops 323M -> 113M (its wait loop is slower: every poll trapped). Opt-in.
- ROTT 1.3 (SOUND.ROT written by game86.sh: Version 13, FXMode 2, MusicMode 8, NumBits 8; 16-bit FX failed on DMA):
  page flips, 1073 frames, ~30 fps demo with regular ~65 ms hitches (not investigated).
- Heretic /3DA: 20 retrace waits in 41 s; Duke ScreenMode 2 /3DA: 65 in 52 s -> these copy-to-screen games neither
  flip nor vsync per frame: no port trap sees them. Options: capture card analyzer (milestone 4) or a memory-watch
  research item (DPMI 0B00h debug watchpoint on A0000h is per-client in HDPMI -> probably needs HDPMI/ring-0 help).

Next 3 steps: hardware results -> fixes; VESA 4F07h hook; /RM (JEMM/QEMM, qemm.c from SBEMU).

## 2026-11-17 evening - pre-hardware checks done, waiting for the user's hardware results

Test build (superseded by test2): FRAMELOG-0.1-test1.zip (pack/mkzip.sh -test1; FRAMELOG.TXT has the hardware checklist).
Tags/releases are refused here (403): the zip on main is the download.
Real games on 86Box PII-450 + SBEMU + SC-55 (dos/testrig/game86.sh, COM1 -> socat pty -> fllog.py):
- FLIP 40 s: 1839/1839 frames through 86Box's real UART timing, 0 junk, stop marker received.
- Duke 3D 1.3D ScreenMode = 0: 2508 frames, 0 lost, gameplay 10-20 ms, level-load gap 7.6 s (/tmp/flgame/duke-mode0.csv).
- Duke ScreenMode = 2 (screen buffered) and Heretic 1.2 (mode 13h copy): NO start-address writes -> 0 frames,
  the host never gets a header (S is read only at flips). These need /3DA (milestone 2). ROTT: needs SOUND.ROT
  (SNDSETUP), not run.
- fllog.py now streams rows, writes stop=marker|host (host = normal for DOS), sends X+S if the game PC was still logging.

## 2026-11-17 - MILESTONE 1 DONE (1a-1d), next: milestone 2

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

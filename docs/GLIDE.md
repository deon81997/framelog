# DOS Glide (3dfx Glide 2.x under DOS/4GW) - how FRAMELOG times it

Status: prototype (wip/dosglide-2026-11-18), tested on 86Box Voodoo3 3000 with the Voodoo3 GLIDE2X.OVL (Glide
2.61.00.0439) and a test program; not yet with a real game. Default on; `/NOGLIDE` turns it off.

## How DOS Glide games swap buffers

- **GLIDE2X.OVL games (most from 1997 on).** The OVL is an LE DLL (MZ + LE, `system dos4g dll`, Watcom). The game
  links 3dfx's `glide2x.lib`: an import library whose entries are 5-byte `call __loadme` thunks plus Tenberry's DLL
  loader (plain DOS/4GW 1.97 has no DLL support: "DOS/4GW error (2302): DLL modules not supported"). The first Glide
  call loads `glide2x` (.EXE/.DLL/.OVL) from the game folder/PATH with ordinary DOS reads, resolves the exports by
  name (`_GRBUFFERSWAP@4`, upper case in 3dfx OVLs, `__stdcall`) and patches the thunks. `grBufferSwap(interval)` is
  the frame: on a Voodoo3 it queues a swap packet in the command FIFO (no port I/O, no CRTC write; the display start
  is a Voodoo register), so the CRTC trap never sees a Glide frame.
- **Statically linked games (Voodoo 1 era, Glide 2.1/2.2).** Glide is inside the game EXE; no OVL. Not covered yet.
- Voodoo3 Glide sets the screen mode with VESA (INT 10h 4F02h 111h, 640x480x16), so the M record already shows
  `res=640x480 depth=16 mode=VESA 111h`.

## The hook (like the VBE PM-table thunk)

The VESA hook could not see Duke 3D's PM SetDisplayStart calls, so it hands the game a copy of the table whose
entry first runs a thunk that does `IN AL,3D4h` with `'FL'` in EAX's upper half (the CRTC trap sees it = a frame).
The Glide hook does the same to grBufferSwap, while the OVL is being read:

1. A real-mode INT 21h stub (`dos/glide/glstub.asm`, own 4.3 KB DOS block) watches open (3Dh/6Ch/716Ch), read (3Fh)
   and close (3Eh). Other DOS calls cost a few compares.
2. Open of a file named `GLIDE2X.OVL` by a protected-mode program (the caller's real-mode CS = the DPMI host's, learnt
   at load; COPY or other real-mode tools are left alone so no patched copy can land on disk): `gl_open_cb` opens the
   same name itself and parses the LE: resident name `_GRBUFFERSWAP@4` -> entry table -> object/offset -> file
   offset; the first instructions (>= 6 bytes; push/pop/mov/sub esp/lea... no relative operands) and the fixup records
   of that page (none may touch the patched bytes). It writes a 32-bit thunk into the DOS block:
   `pop dword [ret]; push eax; push edx; mov eax,474C0000h; mov dx,3D4h; in al,dx; pop edx; pop eax;
   <original instructions>; jmp dword [ret]` and arms the game's file handle.
3. Each read on that handle whose range covers grBufferSwap's first bytes: `gl_read_cb` overwrites them in the
   transfer buffer with `FF 15 <ptr>` (CALL DWORD [ptr], ptr -> thunk) + NOPs, before the extender copies the page up
   and applies its fixups.
4. The game calls grBufferSwap -> thunk -> `IN 3D4h` with `'GL'` (474Ch) in EAX's upper half -> crtc_trap ->
   `gl_flip` (same 0.5 ms merge as VESA) -> the original prologue -> back. A frame = the moment the game asks for the
   swap (same as the Win98 Glide hook).

Needs a zero-based flat model (DOS/4GW: CS/DS base 0) - the thunk sits in conventional memory and is reached with
absolute addresses. `FRAMELOG /STATUS` shows opens, patched loads, grBufferSwap calls and the last parse result:

| result | meaning |
|---|---|
| 0 | hooked |
| 1-2 | not an LE file / loader section > 64 KB |
| 3 | no `_GRBUFFERSWAP@4` export |
| 4-5 | entry table type or object not understood |
| 6 | prologue crosses a page |
| 7 | read error |
| 8 | prologue instruction not in the copy list |
| 9 | fixup table read error |
| 10 | a fixup touches the patched bytes |
| 11 | could not open the file again |

Known OVLs: Voodoo3 2.61.00.0439 `53 56 57 55 83 EC 0C` (7 bytes copied); Voodoo1 SST1 OVL from
efliks/nostalgic-glide-demo `53 56 57 55 8B 5C 24 14` (8 bytes) - both pass the checks (dos/glide/leinfo.py).

## Test program and rig

- `GLFLIP.EXE [N] [FPS] [interval]` (dos/glide/glflip.c, OpenWatcom, `dos/glide/mk.sh`): loads GLIDE2X.OVL with
  `glload.c`, a small LE loader standing in for Tenberry's (same DOS read path), opens 640x480, N x (grBufferClear +
  grBufferSwap), paced to FPS by RDTSC with spare-loop counting. `GLOVL=other.ovl` = a name FRAMELOG doesn't hook (A/B).
  The OVL's Watcom start code aborts under HDPMI32i ("Run-time library initialization failure: DLL load aborted": it
  asks INT 21h 35E9h for a loader hand-over that DOS/4GW answers itself); Glide 2.61 works without it, so GLFLIP only
  calls it with `GLINIT` set. Whether real games' loader gets past this under HDPMI32i is the first thing to check.
- `dos/glide/gl86.sh SECS 'DOS line'...`: MS-DOS 7.1 image (priavte store-dos71) on 86Box P3B-F PII-450 with
  `gfxcard = voodoo3_3k_agp`, 128 MB, C:\GL = OVL + DOS4GW + HDPMI32I + GLFLIP + FRAMELOG, `LOG=name` = fllog.py CSV.

## Results (86Box PII-450, Voodoo3 3000, HDPMI32i)

- GLFLIP 600 @ 50 fps, interval 0, fllog.py: 599 frames logged, 50.00 fps, min 20.00 / max 20.25 ms, 0 lost, header
  modes 37 (CRTC + VESA + Glide).
- Cost, one boot, FRAMELOG /AUTO, 4 x hooked vs 4 x not hooked (GLX.OVL) alternating, 600 frames @ 50 fps: spare loops
  385.375M vs 385.659M = -0.07 % (includes sending 600 frames over COM1).
- `COPY GLIDE2X.OVL X.OVL` with FRAMELOG loaded: copy unpatched (caller-CS check).
- SBEMU + SC-55 (testrig/perf86.sh idle, FLIP 70 fps + MIDI): same as the exe without the hook - SC55LOG 0.48x,
  SBDIAG 61 % CPU, 0.0 % lost, longest interrupt 3.57 ms.

## Real games (86Box PII-450, Voodoo Graphics, HDPMI32i), 11/18

- The OVL's Watcom DLL start code needs DOS/4GW as host (INT 21h 35E9h hand-over, FF00h check) and aborts under HDPMI32i
  with or without FRAMELOG; glhook.c gl_dllfix patches both while the OVL is read (thunks at GL_DLL, /STATUS "DLL start
  fixes 2"). Extreme Assault 1.1.1 demo (OVL 2.31): 3706/3706 frames; Archimedean Dynasty demo (2.42): 6552/6552;
  Redguard demo: logs; Carmageddon (static Glide): no OVL, 0 frames, no crash. Rig: dos/glide/gv1.sh.

## Open

- A real DOS Glide game with Tenberry's loader (does its DLL init run under HDPMI32i? does it read the OVL page by
  page or in big blocks - the hook copes with either, a read only has to cover the bytes).
- Statically linked Glide games: find grBufferSwap in the EXE by byte signature (per Glide version) and patch it the
  same way on the EXE's reads.
- glide3x (DOS Glide 3 is rare) and Voodoo2/Banshee OVLs: run leinfo.py on them.

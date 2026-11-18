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
- **Statically linked games (Voodoo 1 era, Glide 2.1/2.2).** Glide is inside the game EXE or a driver file it loads;
  no OVL. Covered by a byte signature, see "Static Glide" below.
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
  Redguard demo: logs; Carmageddon (static Glide): see "Static Glide". Rig: dos/glide/gv1.sh.
- Cost on Voodoo Graphics (test8 exe 03b84a18, AD's OVL 2.42, HDPMI32i, FRAMELOG /AUTO, GLFLIP 600 @ 50 fps, hooked
  GLIDE2X.OVL vs unhooked GLX.OVL alternating, 4 pairs per boot, 2 boots in opposite order): spare loops 385.402M vs
  385.694M = -0.076 %, 385.432M vs 385.668M = -0.061 % (logging 600 frames included), 0 lost; gl_dllfix has no runtime
  part. SBEMU + SC-55 (perf86.sh idle) test7 exe vs test8 exe: SC55LOG 0.48x, SBDIAG 61 % CPU, 0.0 % lost, longest
  interrupt 3.57-3.61 ms on both. GLFLIP's swap count includes ~74 per run that grSstWinOpen/Close make themselves.
- Redguard quit (11/18, 2 tries, gv1.sh +Key): Esc #1 closes the intro letter, Esc #2 opens the book menu (SAVE
  highlighted), Return selects; 86Box dropped 2 of 6 held Downs 5 s apart (landed on MOVIES), so no clean quit and no
  /STATUS swaps-vs-frames yet. Next try: in the rig's copy of MENU.INI put `default[6] = 1` (QUIT) instead of
  `default[0]`, then Esc, Esc, Return - or read each Down from a screenshot.

## Static Glide (Glide linked into the game, no GLIDE2X.OVL), 11/18

Carmageddon (Splat Pack demo) links Glide 2.11 into its BRender driver **3DFX_DOS.BDD**: a PE file ("This is a
BRender driver"), loaded by CARM3DFX.EXE's own PE loader with plain DOS reads (fopen/fread through DOS/4GW's transfer
buffer), base relocations applied after the read. CARM3DFX.EXE (LE, DOS/4GW) has no Glide in it. So the OVL method
works unchanged: patch the bytes in the read buffer before the loader copies them up.

- **Which files:** glstub.asm now also calls back on opens of `*.EXE` and `*.BDD` by a protected-mode program (same
  host-CS check, so COPY/real-mode tools never see patched bytes). glstatic.c gs_parse scans the file with our own
  handle at open time (4 KB reads, load time only; MZ files without an LE/PE header are skipped after one read).
  GLIDE2X.OVL keeps its own export-based path, so a game with an OVL is never counted twice (its EXE is scanned, no hit).
- **Signature** (Glide 2.x SST-1 grBufferSwap; the same function in OVL 2.31, OVL 2.42 and BDD 2.11):
  - fence: `87 05 xx xx xx xx  89 /r 28 01 00 00  87 05` = the write of swapbufferCMD (SST-1 register 128h) between
    two `xchg [p6fence],eax` (Glide's P6 write fence) - exactly once per Glide, never in game code;
  - start: the nearest `53 56 57 55 8B 5C 24 14` (push ebx/esi/edi/ebp; mov ebx,[esp+14h] = swapInterval) at most
    180h bytes before it; end: `5D 5F 5E 5B C2 04 00` (pop x4, ret 4) at most 40h bytes after it.
  - Exactly one hit per file, else nothing is patched (/STATUS "last error": 20 none, 21 several, 22 format, 23
    relocation/fixup in the 8 prologue bytes, 24 read error).
- **Checks:** PE: no base relocation in the 8 replaced bytes (the BDD's nearest is 2 bytes after them); LE: pages in
  order, no fixup in [start-6, start+8). Then the OVL thunk: `FF 15 ptr` + 2 NOPs over the prologue, thunk (own
  64-byte DOS block) = IN 3D4h with 'GL' -> gl_flip, the 8 original bytes, jmp back.
- **One call = one frame:** gl_flip no longer merges calls < 0.5 ms apart (the merge is for 2-byte CRTC writes).
  Carmageddon swaps twice back to back at screen changes; merged, /STATUS showed 5790 swaps vs 5782 frames.
- **Host test:** `sh dos/glide/test/gstest.sh /tmp/dl` (synthetic PE: relocs in/near the prologue, two hits, window
  edges, broken epilogue; all 35 EXE/BDD/OVL files of the 4 demos: hit only in 3DFX_DOS.BDD 0x53BC and the two SST-1
  OVLs at their export, none in VRUSH's Rush OVL or any EXE).

Results (86Box PII-450, Voodoo Graphics 4 MB, HDPMI32i; keys: priavte testrig/framelog/carmkeys.py, a screen-state
driver: Return until the race, wait for the demo timer, Return on the ad, Esc in the race menu, Down/Up until the red
lamp is on QUIT CARMAGEDDON, Return x2):

| Game | /STATUS | Log |
| --- | --- | --- |
| Carmageddon Splat Pack demo | patched in 1 (3 files scanned), grBufferSwap 6932 = flips 6932, 0 lost | 6931 frames (the last record is still in the ring when the game quits); race 41-52 fps (17 ms), menus 29.5 fps |
| Extreme Assault 1.1.1 demo | OVL 1, static scanned 1, patched 0; 4149 = 4149, 0 lost | exact |
| Archimedean Dynasty demo | OVL 1, static scanned 1, patched 0; 14307 = 14307, 0 lost | exact |

Cost (86Box PII-450, Archimedean Dynasty mission, fps 5-55 s after the mission starts, old = DOS Glide build without
static, alternating old/new x3): old 151.0 / 157.8 / 148.8, new 155.4 / 153.4 / 147.4 -> mean 152.5 vs 152.1 (-0.3 %,
runs spread +-3 %): within noise. Runtime path = the OVL thunk (measured -0.07 % before); the stub only adds compares
after a successful open. Load time: the scan, once per EXE/BDD opened by the game: 178 ms for FIGHT3.EXE (1.28 MB),
@@CARMSCAN@@ (/STATUS "scan time").

## Open

- A real DOS Glide game with Tenberry's loader (does its DLL init run under HDPMI32i? does it read the OVL page by
  page or in big blocks - the hook copes with either, a read only has to cover the bytes).
- Static Glide in an LE EXE (the code path exists and is host-tested, no real game found yet: Carmageddon keeps its
  Glide in a PE driver); static Glide 2.1x builds without the P6 fence (signature = fence + 128h, see below).
- glide3x (DOS Glide 3 is rare) and Voodoo2/Banshee OVLs: run leinfo.py on them.

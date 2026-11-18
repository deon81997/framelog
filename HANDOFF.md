# HANDOFF — FRAMELOG

## 2026-11-17 Q2 software renderer FIXED (BltFast hook, main 7d006ac); 3DMark99 on 86Box = emulator dead end

Q2 soft (vid_ref soft, sw_mode 3 640x480): ref_soft in a linear mode presents with BltFast(front, 0,0, sysmem
offscreen) - no Flip, no Blt. FRAMELOG.EXE now patches slot 7 (BltFast) too (3 slots per surface vtable, FL_MAXSLOT
24, shm/stop names _2); BltFast to the primary = a frame (api ddraw-blt, only if no Flip seen). /LOG detach line
counts BltFast. DDTEST "fast" = the Q2 pattern (exclusive 640x480x8, BltFast back + front). Results v3 rig (86Box
PII-450 Voodoo3): DDTEST fast 299/300 at 50.2 fps; Q2 soft 743 presents (632 timedemo + load), Q2 itself 39.9 fps
without FRAMELOG, 39.9 / 39.8 with it (632-frame window in the CSV = 39.88). ModeX path (Flip) was already covered.
3DMark99 Max: 640x480 triple (unregistered) = same garbage + 86Box crash ("Bad CMDFIFO0", Qt segfault). Registered as
Max Pro (free Futuremark key from philscomputerlab.com/futuremark-3dmark.html: Futuremark / 78TR2-8DBDU-BMXXS ->
frame buffer + test choice unlock), 640x480 DOUBLE, Game 1+2 only: the guest hung before the run (86box.log
"Illegal instruction 00008B55" at 0147:B9BD, black windows, clock stops). Both = 86Box Voodoo3 emulation, not
FRAMELOG (fails without it too). Store VM NOT changed (v3 image here got the registration but was hard-killed).
Next 3: (1) 3DMark99 on real hardware (D810 Radeon X300 / P5PE-VM GeForce 6800, Pro key above, Game 1/2 fps vs
FRAMELOG); (2) test6 zip with the BltFast hook + D810/P5PE-VM run of Q2 soft + GL; (3) DOS Glide / capture / /RM.

## 2026-11-17 Win98 real games + D3D8/9 Present hook DONE (test5 zip; Voodoo3 VM = priavte store-w98qi-86box-v3)

D3D8/9 (win98/flhook.c): Direct3DCreate8/9 inline hook -> IDirect3D8/9 vtable CreateDevice slot (15 / 16, private
.rdata, VirtualProtect) -> on the first device inline-hook IDirect3DDevice8::Present (slot 15) / Device9::Present (17)
+ IDirect3DSwapChain9::Present (3). Meta: GetAdapterIdentifier (gpu/driver), back buffer res/format, windowed=1,
api d3d8/d3d9, d3d=8/9. One nesting guard g_ptid for every API (outermost present per thread counts); Glide swaps in a
process with an OpenGL DLL hooked are not frames (3dfx ICD init swaps). Tests: test/d3dxtest.c -> D3D8TEST/D3D9TEST
([win] [sc] [spin] N FPS), DDDYN (DDTEST without a DDRAW import), DDTEST modex. /LOG: "first Flip hr" + detach line
"Flip N Blt N nested N" per process.
VM: priavte store-w98qi-86box-v3 (vms/w98-86box-v3/acpi: P3B-F PII-450, Voodoo3 3000 AGP voodoo3_3k_agp, v3w9x 1.02.18
installed by PnP from C:\DRIVER (its VOODOO3.INF swapped, msvcrt line dropped), DX9.0c, SB16 only, 800x600x16;
C:\QUAKE2 demo, Forsaken demo installed, C:\3DMARK99 unpacked). Rig: win98/testrig/v3 (boot.sh GO.BAT FILES = one
boot, C: changes stay; run.sh = + fllog --suite 20 sections -> /tmp/v3out/csv; split.py; mkpak.py). Interactive
games: drive with vms/bin/86run.sh DIR key/shot while boot.sh runs.
Results (86Box PII-450, Voodoo3): all test programs N-1 frames at 25.00 ms on the real 3dfx driver (DDTEST flip/blt,
D3DTEST hal, GLTEST gdi/wgl, G2TEST real glide2x = +155 frames of the 3dfx splash, D3D8TEST full/win). D3D9 fails on
1.02.18 (E_OUTOFMEMORY = DX6 driver); with QuickInstall's Voodoo3 driver (4.12.1.1225, 6/2000; /tmp copy of
store-w98qi-86box + Voodoo3, not stored) D3D9TEST win / win sc 199/199, fullscreen NOTAVAILABLE. Cost spare loops:
D3D8 -0.5%, D3D9 -0.6..+2% (noise).
Quake II 3.14 demo GL: Q2 "632 frames 10.8 s 58.3 fps", FRAMELOG 742 presents incl. load/console, 56.7 fps whole run;
w/o FRAMELOG 56.1 fps. Command line that quits after the timedemo (exec of a cfg does NOT work in the demo, +set is
applied before the map and demomap clears nextserver): +set logfile 2 +set vid_fullscreen 1 +timedemo 1 +alias fin
"killserver; quit" +alias go "demomap q2demo1.dm2; set nextserver fin" +go.
Forsaken demo (PLAY DEMO, biodome.dmo, 100%): game AVG 85.19 FPS 19162 frames 3.44; FRAMELOG last 19162 frames before
Esc 225.96 s = 84.80 fps. The demo is long (~11 min, loops); Esc returns to the menu, Play Demo page shows AVG.
OPEN BUGS: (1) Q2 software renderer (vid_ref soft, DDraw 8-bit, ALLOWMODEX) logs nothing: FLHOOK attached, but zero
Flip/Blt reach the wrappers, while DDTEST/DDDYN/DDTEST modex (same flags) work - Q2's front buffer vtable is not one of
the 5 FRAMELOG.EXE patches; next: debug hook IDirectDraw::CreateSurface (DD vtable slot 6) to log the surface vtables
Q2 gets. Repro: v3 rig, FRAMELOG then start /w QUAKE2.EXE +set vid_ref soft +set sw_mode 3 (+ Q2 line above).
(2) 3DMark99 Max on 86Box Voodoo3 (default 800x600x16 triple buffer) shows garbage, then an error box, also WITHOUT
FRAMELOG -> emulator/driver issue; try 640x480 double buffer in its Display settings (GUI only, no command line).
Next 3: (1) Q2 soft bug above; (2) 3DMark99 at 640x480 (+ its per-test fps vs FRAMELOG); (3) hardware: test5 on the
D810/P5PE-VM (D3D8/9 games), DOS Glide / capture from the old list.

## 2026-11-17 DOS 'M' record (run info) DONE - shipped in test4 (DOS exe byte-identical to main 74daa21)

FRAMELOG.EXE sends M after H (and a fresh block, seq 0, before the next frame after INT 10h AH=00h / 4F02h while
logging): gpu (VBE 4F00h OEM + VBE2 vendor/product, else VGA), driver (VBE x.y, OEM rev, product rev), res/depth/mode
(VESA: mode list + 4F01h read at load, 4F03h current, tracked via the stub's 4F02h; VGA: 40:49 + CRTC 01/07/09/12h,
mode=X when chain-4 off), cpu (CPUID brand or vendor family/model/stepping + TSC MHz). /NOMETA = off (no mode hook).
Code: dos/flmeta.h (pure C builders + encoder), framelog.c meta_init (load) / meta_build (once per block: a few untrapped
VGA port reads, no INT 10h) / mode_cb (stub RMCB after the BIOS; reads AX/BX the stub pushed behind 'FL' - the Voodoo3
BIOS nests INT 10h AH=00h 5Fh inside 4F02h, so no shared variables). Stub now 96 bytes, VBE PM table copy at seg+8:0.
Tests: dos/testrig/meta/metatest.sh 14/14 (bytes == flproto.encode_meta); QEMU e2e plain B + sbemu PASS (SeaBIOS VBE
3.0 lines); 86Box Voodoo3: Duke ScreenMode 0 = res 320x200 mode=X, ScreenMode 1 = 640x480 mode=VESA 101h;
game86.sh flipmix (FLIP B then text FLIP) -> '# change@956=mode=text 03h'. serial-bench 44735ca: fllog writes a mid-run
block as '# change@N=k=v ...' ('k=' = gone), 67 tests; PROTOCOL-M.md marked done, table lives in docs/PROTOCOL.md.
Cost 86Box PII-450 perf86 FLIPARGS="MB 70", spare loops / FLIP's TSC Hz, test3 vs new, 3 runs each: 1.01147 1.00573
1.01095 vs 1.01015 1.00881 1.01133 (+0.07%, run spread 0.6%); SBDIAG 59% / 0.0% lost, SC55LOG 0.48x both.
Next 3: (1) hardware: CSV header lines vs the real card (D810 X300, P5PE-VM 6800) - checklist item in FRAMELOG.TXT;
(2) if a game's mode isn't in the VBE list -> res missing (only mode=VESA xxxh): add a 4F01h in mode_cb then;
(3) suite file names use the first block's res: a game that logs from a 320x200 intro gets the wrong name (host idea:
res of the longest stretch).

## 2026-11-17 Win98 hook DLL DONE (test4 zip): FRAMELOG.EXE + FLHOOK.DLL, all APIs pass in QEMU + 86Box

Code: win98/ (framelog.c loader + COM worker, flhook.c DLL, lde.c, flshared.h, makefile, tools/chkimp98.py +
w98exports.txt from the VM's system DLLs), docs: pack/FRAMEL98.TXT (in the zip as WIN98/), PROTOCOL.md (Win98 frame
definition + M keys), serial-bench GAMES.md rows. release/FRAMELOG-0.1-test4.zip = DOS (incl. the M record from the
parallel DOS chat, rebuilt with DJGPP) + WIN98/ + HOST/ (now also flmeta.py, flreport.py: fllog imports flmeta).
How (Win98 facts from win98/test/probe.c): DDRAW.DLL, GDI32, KERNEL32 load in the shared arena, code read-only
(VirtualProtect/WriteProcessMemory fail) but DDRAW's vtables are shared + writable -> loader points Flip/Blt slots of
Surface1-7 vtables at stubs in a named file mapping (same address in every process); a stub looks up fs:[30h] in the
procs table -> that process's FLHOOK wrapper, else the original. DDRAW rewrites its vtables whenever a process loads it
-> loader re-hooks every 5 ms + DLL at attach/CBT. opengl32/glide/3dfxgl are private -> 5-byte JMP + trampoline (LDE);
gdi32 SwapBuffers + kernel32 LoadLibraryA/ExA via IAT. Launch mode: CREATE_SUSPENDED, JMP at the exe entry -> shared
stub: LoadLibrary(FLHOOK) + VirtualProtect + copy the 5 bytes back.
Tests (win98/testrig/README.md): QEMU T1 6/6 (ddflip, ddblt, d3d6 RGB, GL gdi, GL wgl, Glide2 stand-in) 199/199 at the
set rate; T3 launch mode incl. a windowless Glide program; T5 host X then S mid-game (67 + 302 frames); T6 FRAMELOG /STOP
while 3 games present (all finish). 86Box PII-450 Cirrus (P1): spare loops with vs without: flip -0.5%, blt +0.5%,
D3D6 +1.5%, GL +0.06%, Glide +0.35% (noise ~0.5%) -> on, no switch needed.
Not done / next 3 steps: (1) user's hardware results (FRAMEL98.TXT checklist; real Voodoo Glide never run - only the
stand-in; no 3dfx driver binaries in the VMs, only INFs); (2) real games: Quake II demo, Forsaken demo, 3DMark99 not
downloadable here (only github.com reachable) - asked the user; (3) D3D8/9 (IDirect3DDevice8/9::Present vtable, same
stub idea) + 86Box Voodoo3 with the user's 3dfx drivers. Known limits: windowless games only in launch mode; FRAMELOG
killed (End Task) while a DDraw game runs -> last FLHOOK instance restores the vtables (untested path).
Rigs: q98.sh/t1.sh (QEMU, ~40 s boot), r86.sh (86Box, ~4 min); restore store-w98qi-qemu, store-w98qi-86box,
store-86rig, store-86box-roms (86Box needs apt librtmidi6).

## 2026-11-17 serial-bench (host side, parallel chat) - metadata, suites, reports, M record, capture analyzer done

serial-bench main d761b5f (66 unit tests, `python3 -m unittest discover -s tests`), framelog untouched except this section:
- Metadata: `fllog.py --gpu --driver --res --depth --cpu --game --notes [--meta file.ini]` -> `# key=value` CSV
  lines; `flstats.py --label-by/--group-by/--where`.
- 'M' record, host side: serial-bench docs/PROTOCOL-M.md (proposal: M = seq u8 + 15 text bytes, 'key=value\n' lines,
  NUL ends, after H before B). fllog merges it (user's values win, pc_<key> on mismatch). TO DO game-PC side: DOS
  TSR (VESA 4F00h OEM string, mode, CPUID) + Win98 DLL (DDraw GetDeviceIdentifier, mode); then move the table to PROTOCOL.md.
- Suites: `fllog.py [PORT] --suite suite.ini [--report DIR --report-src results/]`: one CSV per run `game-gpu-res.csv`
  (split at B/E or gaps, fragments < min_frames dropped), HTML report at the end.
- Reports: `flreport.py FOLDER -o report/` -> gpu-*.html + compare.html + index.html (`--default gpu=...` for old CSVs).
- Capture analyzer (milestone 4, host part): `flcap.py capture.avi -o run.csv [--serial fllog.csv] [--multi-tear 3]`:
  frame times / tear lines / repeats / runts from a VGA capture at the game's refresh, same CSV format. Tested on a
  scan-out simulator only (tests/capsynth.py) - needs one real capture (Heretic or Duke ScreenMode 2, 70 Hz).
- serial-bench docs/GAMES.md: coverage list, now with the VESA results (Duke ScreenMode 1, FLIP B) and CAPTURE rows.
Next: hardware suite run -> flreport; a real capture for flcap; M record in the DOS TSR / Win98 DLL.
Later (user 11/17, after the main goals): optional local logging on the game PC (no 2nd PC): DOS = frames in XMS,
  FRAMELOG.BIN written at stop/exit when InDOS=0 (never mid-game); Win98 = worker thread file; host `fllog.py --file`.
  Serial stays the default (live, survives crashes).
Later (user 11/17, when serial is done and fully working): network logging - Win98 DLL worker thread sends UDP
  (~64 frames per packet, same per-frame cost), host fllog.py UDP listener; DOS via packet driver after that
  (per-NIC testing, IRQ clashes with SBEMU).

## RESUME HERE (2026-11-17 handoff) - GOAL: one-stop retro GPU benchmark. Win98 hook DLL DONE (top section)

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

## 2026-11-17 night - VESA hook DONE + checked, test3 zip; /RM parked on wip/rm-2026-11-17

Build: release/FRAMELOG-0.1-test3.zip (pack/mkzip.sh -test3; replaces test2). FRAMELOG.TXT: VESA section, /NOVESA,
Duke VESA line, FLIP B, VESA hardware checklist items.
Cost, 86Box PII-450 + SBEMU + SC-55, FLIP 30 s 70 fps flipping via INT 10h 4F07h + MIDI (FLIPARGS="MB 70" perf86.sh):
none / idle / log = spare loops 456.85M / 457.61M / 456.46M (-0.09% = noise), SBDIAG 59% / 0.0% lost / 3.5-3.6 ms
in all three, SC55LOG 0.48x, 1910/1910 frames, 0 drops. VESA stays on by default. Duke VESA with/without A/B not run
(user: only if quick); its 596-frame run is below.
/RM: parked (user 11/17) on wip/rm-2026-11-17 (dos/RM-WIP.md): QPI traps fire from V86 (2282 calls) but OUT 3D4h,AX
loses the data byte (one QPI callback per word, qemm.c passes AL only) -> only half the RMFLIP frames count. Removed
from main (qemm.c not linked; FLSTATE keeps 2 reserved words). RMFLIP.COM (tools/rmflip.asm) stays on main.
Next 3 steps: user's hardware results with test3; then the priorities in the RESUME section above (Win98 hook DLL);
/RM from the wip branch last.


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

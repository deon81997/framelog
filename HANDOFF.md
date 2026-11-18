# HANDOFF — FRAMELOG

## 2026-11-18 Static Glide (Carmageddon): grBufferSwap by signature in the EXE/BDD reads - works on 86Box Voodoo Graphics

Done: glide/glstatic.c (+ .h; #included by glhook.c): the stub also reports opens of *.EXE / *.BDD by a PM program;
gs_parse scans the file (load time only) for Glide 2.x SST-1 grBufferSwap = the P6-fenced swapbufferCMD write
`87 05 .. 89 /r 28 01 00 00 87 05`, nearest prologue `53 56 57 55 8B 5C 24 14` <= 180h before, `5D 5F 5E 5B C2 04 00`
<= 40h after; one hit, no PE reloc / LE fixup in the 8 bytes -> same FF 15 thunk as the OVL. Carmageddon's Glide 2.11
lives in 3DFX_DOS.BDD (PE BRender driver, read with plain DOS reads, file 0x53BC). gl_flip no longer merges swaps
< 0.5 ms apart (Carmageddon swaps twice at screen changes: 5790 vs 5782). Stub grew to 324 bytes: GL_THUNK 336,
GL_DLL 400. /STATUS "Static Glide" line (files scanned, patched, last error 20-24, scan time). docs/GLIDE.md
"Static Glide" section.
Tests: host `sh dos/glide/test/gstest.sh /tmp/dl` PASS (synthetic + all 35 EXE/BDD/OVL files of the 4 demos: hits only
in the BDD and the 2.31/2.42 OVLs). 86Box PII-450 Voodoo Graphics (gv1.sh): Carmageddon menus + race 6887 swaps =
6887 frames, 0 lost, race 41-52 fps; Extreme Assault 4149 = 4149; Archimedean Dynasty 14307 = 14307 (OVL only, their EXEs
scanned, no hit). Cost: AD fps old/new x3 alternating 152.5 vs 152.1 (-0.3 %, spread +-3 %) = noise; scan 178 ms
(FIGHT3.EXE) / 322 ms (Carmageddon, 3 files) once at load. Keys: priavte testrig/framelog/carmkeys.py (screen-state
driver; fixed KEYSEQ missed: race start drifts 30-40 s, 86Box drops/doubles arrows). XA quit: Esc, y, Esc, y, Esc.
Open: LE-EXE static path has no real game yet (host-tested only); Glide 2.1x without the P6 fence not covered; the
last frame record can wait in the ring until the next trap (seen once: 6932 swaps, 6931 logged, 0 lost).
Resume: restore as in the DOS Glide section below, `sh dos/glide/test/gstest.sh /tmp/dl`, Carmageddon:
`W=/tmp/gv1 LOG=c sh dos/glide/gv1.sh 440 /tmp/dl/carm CARM 'C:\GL\HDPMI32I.EXE -r -x > NUL' 'C:\GL\FRAMELOG /AUTO > C:\GL\LOAD.TXT'
'CD \CARM' 'CARM3DFX' 'C:\GL\FRAMELOG /STATUS > C:\GL\ST.TXT'` + `python3 priavte/testrig/framelog/carmkeys.py /tmp/gv1/vm REFS /tmp/ck`
(REFS = priavte testrig/framelog/carmref).

## 2026-11-18 test9: live status line in the host logger (fllog.py)

Done: fllog.py class Live: one status line redrawn once a second (\r, stderr, console only by default; --live forces it,
--no-live / -q turn it off): frames, fps over the last second (game-PC TSC deltas, not host time) and since the start,
last and slowest frame of the last second, lost (D records); "no frame in the last second (loading? paused?)" when the
game stalls; "connected, waiting for the first frame". Single run and suite (say() ends the line first, so "header" /
"run 1/1" / "port closed" print on their own lines). Counts reset on H and B only (suite: they run on across a gap split).
Tested with a fake game PC on socket:// (H, B, 60 fps F records with a 50 ms frame every 30, 1.3 s pause, D 2, E on X):
single run, suite (gap 1), piped run = no status line (rigs unchanged). CSV output unchanged (same rows as without it).
Docs: pack/FRAMELOG.TXT host step 2, pack/LOG.BAT usage, HOST/README.md fllog row, README news + counter 9.
test9 zip = test8 zip with HOST/fllog.py, HOST/README.md, HOST/LOG.BAT, FRAMELOG/FRAMELOG.TXT swapped (game PC and
WIN98 files byte-identical to test8).
serial-bench main 872162c has the same fllog.py/README.md + tests/test_live.py (3 tests; full suite 69, the one
failure test_games "Forsaken demo" was there before). Tag v0.1-test9 + GitHub release NOT made yet (tag push hangs,
API refused, no browser): make both in a browser on main ee01bc3 with release/FRAMELOG-0.1-test9.zip.
Next 3: as below (Redguard quit, real Voodoo 1/2 run, static Glide), plus the test9 tag/release.

## 2026-11-18 test8: DOS Glide release (cost checked, Redguard quit still open)

Done: main 049f4af already held the merge (wip/dosglide2 7992ec1 + priavte pending/framelog-dosglide3.patch, dos/ =
patched tip byte for byte, test7 section kept); DJGPP v3.4 build clean (only the old lib/dpmi warnings). Cost (86Box
PII-450 Voodoo Graphics, docs/GLIDE.md): GLFLIP hooked vs unhooked -0.076 % / -0.061 % (2 boots x 4 pairs, logging
included, 0 lost); SBEMU perf86.sh idle test7 vs test8 exe identical (0.48x, 61 %, 0.0 % lost). Redguard: 2 quit tries
failed (Esc, Esc = book menu, Return selects, 86Box dropped 2 of 6 Downs) -> GAMES.md stays partial. gv1.sh got the
+Key (held) support the docs already described. test8 zip = test8 DOS exe 03b84a18 + FRAMELOG.TXT (DOS GLIDE section,
/NOGLIDE, cost, Voodoo 1/2 PC checklist) + test7's WIN98 files (win98/ unchanged; a fresh mingw build differs in bytes)
+ serial-bench HEAD host files. README news + counter 8.
Released: tag v0.1-test8 = 46a9a1a + GitHub release with the zip (made in Chrome: tag push "hung up" and did not land);
wip/dosglide-2026-11-18, wip/dosglide2-2026-11-18, priavte wip/framelog-dosglide3-2026-11-18 deleted (branches page).
Wiki not reposted for test8 (Game Coverage page still says test7).
Next 3: (1) Redguard: rig copy of MENU.INI default[6] = 1 (QUIT), Esc, Esc, Return -> /STATUS swaps vs frames;
(2) real Voodoo 1/2 PC run (FRAMELOG.TXT checklist); (3) static Glide (Carmageddon): grBufferSwap by signature.
Resume: as the section below; cost rig = `W=/tmp/gc1 LOG=cost EXTRA="glflip.exe GLIDE2X.OVL GLX.OVL DOS4GW.EXE" sh
dos/glide/gv1.sh 300 /tmp/none GL 'C:\GL\HDPMI32I.EXE -r -x > NUL' 'CD \GL' 'FRAMELOG /AUTO > C:\GL\LOAD.TXT'
[SET GLOVL=GLX.OVL / GLFLIP 600 50 0 / SET GLOVL= / GLFLIP 600 50 0] x4 'FRAMELOG /STATUS > C:\GL\ST.TXT'`.
Test downloads: OpenWatcom ow-snapshot (GLFLIP), andrewwutw/build-djgpp v3.4, npm node-unrar-js@2, apt unshield.

## 2026-11-18 DOS Glide: 3 real games log on 86Box Voodoo Graphics (HDPMI32i DLL-start fix); merged into main

Done: the 3dfx OVLs' Watcom DLL start code aborts under HDPMI32i even without FRAMELOG ("Run-time library initialization
failure: DLL load aborted": INT 21h 35E9h DOS/4GW loader hand-over missing, then ring-0 / fixed-selector path). glhook.c
gl_dllfix now patches it in the read buffers like grBufferSwap (signatures in the 512 bytes after the LE entry: 35E9h ->
thunk ES=EBX=PSP via AH=62h; FF00h DOS/4G check -> AL=1, GS=0); /STATUS "DLL start fixes 2". Same start code in OVL 2.31,
2.42 and the VRUSH one. Rig: dos/glide/gv1.sh (store-dos71, 86Box P3B-F PII-450, Cirrus + Voodoo Graphics 4 MB, KEYSEQ
with +Key = held key, SHOTS, LOG=fllog), dos/glide/csvbuck.py (fps per time bucket).
Results (serial-bench docs/GAMES.md, fixtures tests/fixtures/86box-v1-dosglide): Extreme Assault 1.1.1 3706 swaps = 3706
frames, 0 lost, mission ~29.5 fps; Archimedean Dynasty (`DOS4GW FIGHT3 solo`) 6552 = 6552, 0 lost, 95-148 fps; Redguard
(unshield data1.cab on the host instead of a Win98 install; no OVL in the demo -> AD's 2.42) gameplay 18.2 fps, 3829 frames,
exit not scripted; Carmageddon Splat Pack (static Glide) = negative case: 0 opens, 0 frames, clean quit.
State: on main (merge of wip/dosglide2-2026-11-18 + the DLL-start fix + this section, applied from priavte
pending/framelog-dosglide3.patch; real merge, main had the test7 Win98 fixes; dos/ = the tested branch tip, win98/ = main
54492f0). wip/dosglide-2026-11-18 + wip/dosglide2-2026-11-18 are merged: delete them on the branches page if still listed.
Demos: priavte store-dosglide-demos (+SHA256SUMS). No cost run yet for gl_dllfix (load-time only). No zip yet.
Next 3: (1) 86Box PII-450 cost check of the merged FRAMELOG.EXE (perf86.sh, Glide off = same as test7);
(2) test8 zip + FRAMELOG.TXT DOS Glide section (HDPMI32I + FRAMELOG before the game; no Voodoo in P5PE-VM/D810);
(3) static Glide (Carmageddon): grBufferSwap by byte signature in the EXE/BDD reads.
Resume: clone priavte store-dos71 + store-86rig + store-dosglide-demos + store-personal-1.0.0-beta.130-usb (HDPMI32I),
DJGPP v3.4, `W=/tmp/gv1 LOG=x sh dos/glide/gv1.sh 225 /tmp/dl/xa_inst XA 'C:\GL\HDPMI32I.EXE -r -x > NUL'
'C:\GL\FRAMELOG /AUTO > C:\GL\LOAD.TXT' 'CD \XA' 'CALL 3DFX.BAT' 'C:\GL\FRAMELOG /STATUS > C:\GL\ST.TXT'` (XA: install layout
= ROOT\* + DATA, DATA.SFX, DRIVERS, INSTALL beside it; keys as in GAMES.md).
Test downloads: archive.org xa_demo.exe, addemo.exe, REDGUARD-Demo.rar, splatdem.zip (SHA256 OK); npm node-unrar-js@2;
apt unshield; open-watcom-v2 bld/clib/startup (dllstart.asm, read-only reference).

## 2026-11-18 DOS Glide hook WORKS on 86Box Voodoo3 (GLIDE2X.OVL grBufferSwap patched at load; no release yet)

Findings (details + result codes: docs/GLIDE.md): DOS Glide 2 games load GLIDE2X.OVL = an LE DLL (Watcom
`system dos4g dll`) through 3dfx glide2x.lib's `call __loadme` thunks + Tenberry's DLL loader (plain DOS/4GW 1.97:
"error 2302: DLL modules not supported"); exports `_GRBUFFERSWAP@4` (upper case, stdcall). On a Voodoo3 a swap is a
command-FIFO packet: no port I/O, no CRTC write -> needs a code hook. Voodoo1-era games link Glide statically (not covered).
Hook (like the VBE PM thunk): RM INT 21h stub (dos/glide/glstub.asm) sees open of ...\GLIDE2X.OVL by a PM program
(caller CS = the DPMI host's, learnt at load, so COPY never gets patched bytes), gl_open_cb parses the LE (export ->
file offset, prologue >= 6 bytes, no fixups there), builds a 32-bit thunk (IN 3D4h with 'GL' 474Ch in EAX high ->
crtc_trap -> gl_flip) and gl_read_cb writes `FF 15 ptr` + NOPs into the read buffer before DOS/4GW copies it up.
Default on, /NOGLIDE; /STATUS line; header modes bit 5 (PROTOCOL.md). Voodoo3 Glide sets VESA 111h itself, so the M
record already says res=640x480 depth=16.
Test: GLFLIP.EXE (dos/glide/glflip.c + glload.c = small LE loader standing in for Tenberry's, OpenWatcom v2,
dos/glide/mk.sh) + dos/glide/gl86.sh (store-dos71 image, 86Box P3B-F PII-450 voodoo3_3k_agp, V3 OVL 2.61.00.0439 from
the store-w98qi-86box-v3 image C:\WINDOWS). fllog: 599/600 frames, 50.00 fps, 20.00-20.25 ms, 0 lost, modes=37.
Cost: 4x hooked vs 4x unhooked (GLX.OVL) alternating in one boot, 600 frames @ 50 fps with /AUTO: spare loops -0.07 %.
SBEMU (perf86.sh idle, SC-55 + FLIP 70 fps, 86Box PII-450): new = old main exe: SC55LOG 0.48x, SBDIAG 61 % CPU, 0.0 %
lost, longest IRQ 3.57 ms (old 3.64 / 3.57). Bug found + fixed on the way: the stub returned with retf 2 -> caller got IF=0
(BIOS ticks stopped: SC55LOG 0 s, SBEMU hung at start); now iret with the caller's flags + DOS's CF etc.
Gotcha: the OVL's Watcom start code ABORTS under HDPMI32i ("Run-time library initialization failure: DLL load
aborted" - INT 21h 35E9h hand-over that DOS/4GW answers itself); Glide 2.61 runs without it (GLFLIP skips it unless
GLINIT=1). A real game may hit this under HDPMI32i with or without FRAMELOG - first thing to check.
State: branch wip/dosglide2-2026-11-18 (fast-forward of main 6019295; old wip/dosglide-2026-11-18 = first prototype, delete). Not on main yet: the session could not push to main. No zip/tag taken.
Next 3: (1) a real DOS Glide game with the V3 OVL on 86Box (user: which game/demo do you own? e.g. Tomb Raider 3dfx,
Screamer Rally, Archimedean Dynasty, Redguard; or the 3dfx Glide 2.x SDK DOS test programs) -> /STATUS result 0 +
fllog; (2) test7 zip + FRAMELOG.TXT section + hardware checklist (P5PE-VM/D810 have no Voodoo: needs a Voodoo
PC); (3) statically linked Glide: grBufferSwap by byte signature in the EXE reads, same thunk.
Test downloads: OpenWatcom v2 snapshot (github open-watcom/open-watcom-v2 Current-build ow-snapshot.tar.xz ->
/tmp/dl/ow), dgVoodoo 1.x source (dege-diosg/dgVoodoo, OVL layout), sezero/glide (glimport.asm = __loadme thunks),
efliks/nostalgic-glide-demo (Voodoo1 SST1 OVL, checked with dos/glide/leinfo.py).

## 2026-11-18 serial-bench: flreport = Retro GPU Shootout (host only, framelog code untouched)

serial-bench d82a78e: flreport.py writes index (summary table per GPU), game-*.html per game across GPUs + CPUs
(avg / 1% / 0.1% low bars, frame-time overlay, percentile curves, ranked table, run notes), gpu-*.html, compare.html,
run-*.html (metadata, stats, histogram, CSV), shootout.csv/.json; 90s look. Real fixtures tests/fixtures/86box-v3: Q2
demo GL 640x480 39.0 / 800x600 38.0 fps, soft 320x240 73.4 / 640x480 43.3 fps over the whole log incl. load (Q2 soft
640x480 itself 39.9 fps); 86Box PII-450 Voodoo3, test6. Old DOS 86Box CSVs (Duke/ROTT/FLIP) still look-alikes only.
Wiki Host-Tools edit not posted (wiki push = proxy 403, Chrome offline): priavte wiki/pending/framelog-Host-Tools.md.

## 2026-11-18 test7: Win98 games on Voodoo Graphics / Voodoo2 / Voodoo3 (86Box) + 3 Win98 fixes

VMs (priavte): store-w98qi-86box-v1 (Voodoo Graphics 4 MB add-on + Cirrus 5446, 3dfx 3.01.00), -v2 (Voodoo2 12 MB
add-on + Cirrus, 3.02.02), -vb (Voodoo Banshee 1.04.00), -v3 unchanged in the store (local copy got the demos). All
have C:\EXPEND (Expendable demo, unshield of archive.org expendabledemo), C:\INCOMING (archive.org incoming_201401),
C:\UT (archive.org UTDemo338, Glide, FirstRun=338). Rig: win98/testrig/3dfx (README; go.sh CARD TAG PARTS, table.py;
results.tgz = the 11/18 runs).
Results (game / FRAMELOG over the same window, same run): Q2 MiniGL V1 42.7/42.72, V2 41.4/41.36, V3 52.0/52.04; ICD
V1 39.7/39.70, V2 40.6/40.64, V3 56.9/56.95; Q2 soft Cirrus 24.5/24.56; Incoming V2 41.82/41.82; UT V3 (Hardcore off)
72.51/72.51, V1/V2 Hardcore on: UT's TimeDemo = real fps x 1.1 (divides by TimeDilation), FRAMELOG = UT/1.1;
Expendable ~1% (whole-second timer). Off vs on in 86Box swings 5-15% per boot (first game after boot slow): no use
for cost. 3DMark99: V2 runs to the end (3265/3498), Banshee 4180 -> only 86Box's Voodoo3 crashes. Fails without
FRAMELOG too: Expendable on V1 hangs black after writing timedemo.txt; Incoming -primary on V3 = garbage + hang;
G2TEST (pass without FRAMELOG) hung twice on V2 (not looked into).
Fixes (win98/): FRAMELOG.EXE waits 3 s (was 0.5) for the run info (first DDraw game lost gpu); Glide 2 type 0 split by
fbiRev (V1 2, V2 261, V3 69634) -> "3dfx Voodoo2" / "3dfx Voodoo Banshee/3"; a Blt-claimed process that starts to Flip
rebuilds meta + FLSHM.meta_gen -> fresh M block (3DMark99 on V2: change@177 api=ddraw-flip gpu=Voodoo2). Zip = test6
DOS/HOST + new WIN98 exe/dll/FRAMEL98.TXT.
Next 3: (1) 3DMark99 per-test fps vs FRAMELOG (Result Browser takes no keys in 86Box: mouse, or real hardware); (2)
Forsaken + Incoming on V1/V2; (3) real-hardware test7 (HWTEST list in FRAMEL98.TXT).

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

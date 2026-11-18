# Game Coverage

What FRAMELOG can time, how, and how to repeat it. All runs so far are in emulators (86Box Pentium II 450 unless
noted). The full table with suite lines is kept in serial-bench
[docs/GAMES.md](https://github.com/deon81997/serial-bench/blob/main/docs/GAMES.md).

**Modes**: `CRTC` (DOS start-address trap, default), `VESA` (DOS 4F07h + VBE 2.0 PM hook, default), `3DA` (DOS
`/3DA` vsync wait), `DDRAW` / `D3D` / `GLIDE` / `OGL` (Win98 hooks), `CAPTURE` (VGA capture + `flcap.py`), `RM`
(real-mode games, planned).

## DOS

| Game | Video | Mode | Status | Benchmark | Notes |
|---|---|---|---|---|---|
| Duke Nukem 3D 1.3D | Build, ScreenMode 0, 320x200 page flip | CRTC | works | attract demo; DUKE3D.CFG `ScreenMode = 0`, SB at IRQ 7 | 2508 frames, 0 lost, 10-20 ms |
| Duke Nukem 3D 1.3D | ScreenMode 1, 640x480 VESA (VBE 2.0 PM) | VESA | works | attract demo; `ScreenMode = 1` | 596 frames, ~18 fps on 86Box Voodoo3; flips via the PM table, never 4F07h |
| Duke Nukem 3D 1.3D | ScreenMode 2 (buffered), 320x200 | CAPTURE | untested | attract demo | no start-address writes; needs a capture card |
| Rise of the Triad 1.3 | Mode X page flip, 320x200 | CRTC | partial | attract demo; SOUND.ROT from SNDSETUP (8-bit FX) | 1073 frames, ~30 fps, regular ~65 ms hitches not explained yet |
| Heretic 1.2 | VGA 13h copy, 320x200 | CAPTURE | untested | attract demo | copies to A000h, never flips |
| Doom | Mode Y page flip | CRTC | untested | `DOOM -timedemo demo3` | expected to work |
| FLIP (test tool) | Mode X 320x240 | CRTC | works | `FLIP 20 70` | 1839/1839 frames in 40 s with SBEMU + SC-55 |
| FLIP B | VESA 101h 640x480 | VESA | works | `FLIP 20 70 10 B` | cost -0.09% |
| FLIP V | Mode X, vsync wait | 3DA | works | `FLIP V`, 35 fps | exact |

## Windows 98

| Game | API | Mode | Status | Benchmark | Notes |
|---|---|---|---|---|---|
| Quake II 3.14 demo | OpenGL (3dfx ICD), 640x480 | OGL | works | see command line below | game 58.3 fps; FRAMELOG 742 presents (632 demo + load), within 1% |
| Quake II 3.14 demo | software renderer, 640x480 8-bit (DirectDraw BltFast) | DDRAW | works (test6) | same + `+set vid_ref soft +set sw_mode 3` | 743 presents; game 39.9 fps with and without FRAMELOG |
| Forsaken demo | Direct3D (DX5), 800x600 | D3D | works | PLAY DEMO, biodome.dmo, 100% | game 85.19 fps / 19162 frames; FRAMELOG 84.80 fps (0.5%) |
| 3DMark99 Max | Direct3D 6 | D3D | no (86Box) | Game 1 + 2 | crashes / hangs on 86Box's Voodoo3 at every setting, also without FRAMELOG: needs real hardware |
| DDTEST / DDDYN | DirectDraw flip, Blt, BltFast, Mode X | DDRAW | works | `DDTEST 600 30` | N-1 frames at 1000/FPS ms |
| D3DTEST | Direct3D 6 RGB / HAL | D3D | works | `D3DTEST hal 300 30` | |
| D3D8TEST / D3D9TEST | Direct3D 8 / 9 | D3D | works | `D3D8TEST 300 40` | D3D9 needs a DX7-class driver |
| GLTEST | OpenGL | OGL | works | `GLTEST 300 30` | |
| G2TEST | Glide 2 | GLIDE | works | `G2TEST 600 60` | real 3dfx glide2x: +155 frames of the 3dfx splash |

### Quake II timedemo that quits by itself

`exec` of a cfg does not work in the 3.14 demo, and `+set` is applied before the map, so use:

```
QUAKE2.EXE +set logfile 2 +set vid_fullscreen 1 +timedemo 1 +alias fin "killserver; quit" +alias go "demomap q2demo1.dm2; set nextserver fin" +go
```

Quake II prints `632 frames, N seconds: X fps` to `baseq2\qconsole.log`.

## Adding a game

Add a row to serial-bench `docs/GAMES.md`: game + version, platform, API / video mode, FRAMELOG mode, status
(`works`, `partial`, `no-frames`, `planned`, `untested`), how to get a repeatable run, the suite line
(`game=`, `res=`, `depth=`) and where / when it was tested.

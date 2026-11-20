# Game Coverage

Here's every game I've tried so far, whether FRAMELOG times it, and how to get a run you can repeat. All of these
were tested in emulators (86Box with a Pentium II 450 unless it says otherwise). Real hardware is next.

The full list with ready-made suite lines lives in serial-bench's
[GAMES.md](https://github.com/deon81997/serial-bench/blob/main/docs/GAMES.md).

## DOS games

| Game | How it draws | Works? | How to benchmark it |
|---|---|---|---|
| Duke Nukem 3D 1.3D | ScreenMode 0, 320x200 page flips | ✅ Yes: 2508 frames, none lost | Attract demo. In DUKE3D.CFG: `ScreenMode = 0`, Sound Blaster on IRQ 7 |
| Duke Nukem 3D 1.3D | ScreenMode 1, 640x480 VESA | ✅ Yes: about 18 fps on 86Box's Voodoo3 | Attract demo, `ScreenMode = 1` |
| Duke Nukem 3D 1.3D | ScreenMode 2, buffered | ❌ Not with a trap: never flips | Needs a capture card |
| Rise of the Triad 1.3 | Mode X page flips | ⚠️ Mostly: about 30 fps, with regular ~65 ms hitches I can't explain yet | Attract demo. Run SNDSETUP first (8-bit sound effects) |
| Heretic 1.2 | Copies to the screen | ❌ Not with a trap | Needs a capture card |
| Doom | Page flips | ❔ Should work, not tried yet | `DOOM -timedemo demo3` |

## DOS Glide (3dfx) games

Tested on 86Box's Voodoo Graphics (Voodoo 1) with HDPMI32i. See [[DOS FRAMELOG]] for how to start them.

| Game | Glide | Works? | How to benchmark it |
|---|---|---|---|
| Extreme Assault 1.1.1 demo | GLIDE2X.OVL | ✅ Yes: every swap logged, none lost, about 29.5 fps in a mission | `3DFX.BAT` |
| Archimedean Dynasty demo | GLIDE2X.OVL | ✅ Yes: every swap logged, none lost, 95-148 fps | `DOS4GW FIGHT3 solo` |
| Redguard demo | GLIDE2X.OVL (borrowed from Archimedean Dynasty) | ✅ Yes: 1275 of 1275 swaps, about 29 fps | `RGFX.EXE` |
| Carmageddon Splat Pack demo | Built in (test10) | ✅ Yes: 6887 of 6887 swaps, race 41-52 fps | `CARM3DFX.EXE` |
| Tomb Raider 3Dfx demo | Built in, Glide 2.03 (next build) | ✅ Yes: 2156 of 2156 swaps, about 30 fps | `TOMB.EXE` |
| Battle Arena Toshinden 2 3Dfx demo | Built in, Glide 2.03 (next build) | ✅ Yes: 1761 of 1761 swaps, fights about 51 fps | `TSD -fx` |
| Screamer 2 demo | None (software only) | ➖ Nothing to hook | — |

## Windows 98 games

Tested with a Voodoo3 3000 and the 3dfx 1.02.18 driver, and since test7 also with Voodoo Graphics, Voodoo2 and
Voodoo Banshee.

| Game | API | Works? | How to benchmark it |
|---|---|---|---|
| Quake II 3.14 demo | OpenGL | ✅ Yes: within 1% of the game's own number | Timedemo, see below |
| Quake II 3.14 demo | Software renderer, 640x480 | ✅ Yes (new in test6): 39.9 fps, same as the game | Timedemo + `+set vid_ref soft +set sw_mode 3` |
| Forsaken demo | Direct3D | ✅ Yes: within 0.5% | Play Demo, biodome.dmo |
| Quake II 3.14 demo | MiniGL / OpenGL on Voodoo 1, 2 and 3 (test7) | ✅ Yes: within 0.1 fps of the game everywhere | Timedemo |
| Incoming | Direct3D on Voodoo2 | ✅ Yes: 41.82 fps, same as the game | Its own benchmark |
| Unreal Tournament demo | Glide on Voodoo3 | ✅ Yes: 72.51 fps, same as the game | TimeDemo with Hardcore off (with Hardcore on, UT's own number is 1.1x too high) |
| Expendable demo | Direct3D | ✅ Yes: about 1% off (the game's timer only counts whole seconds) | Timedemo |
| 3DMark99 Max | Direct3D 6 | ⚠️ Runs to the end on Voodoo2 and Banshee in 86Box; crashes only on 86Box's Voodoo3 | Real hardware for per-test fps |

The test programs in `WIN98\TEST` (DirectDraw, Direct3D 6/8/9, OpenGL, Glide 2) all log exactly what they draw.

## A Quake II timedemo that quits by itself

The Quake II demo is picky: it won't run a config file, and it applies `+set` before the map loads. This command line
runs the timedemo once and exits cleanly, so it fits a batch of benchmarks:

```
QUAKE2.EXE +set logfile 2 +set vid_fullscreen 1 +timedemo 1 +alias fin "killserver; quit" +alias go "demomap q2demo1.dm2; set nextserver fin" +go
```

Quake II writes its own result (`632 frames, N seconds: X fps`) to `baseq2\qconsole.log`, so you can check
FRAMELOG's number against it.

## Tried a game that isn't here?

Please tell me! The useful details are: the game and its version, DOS or Windows, how it draws (VGA, Mode X, VESA,
Glide, DirectDraw, Direct3D, OpenGL), whether FRAMELOG caught the frames, and how you got a repeatable run.

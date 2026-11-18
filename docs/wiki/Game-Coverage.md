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

## Windows 98 games

Tested with a Voodoo3 3000 and the 3dfx 1.02.18 driver.

| Game | API | Works? | How to benchmark it |
|---|---|---|---|
| Quake II 3.14 demo | OpenGL | ✅ Yes: within 1% of the game's own number | Timedemo, see below |
| Quake II 3.14 demo | Software renderer, 640x480 | ✅ Yes (new in test6): 39.9 fps, same as the game | Timedemo + `+set vid_ref soft +set sw_mode 3` |
| Forsaken demo | Direct3D | ✅ Yes: within 0.5% | Play Demo, biodome.dmo |
| 3DMark99 Max | Direct3D 6 | ❔ Crashes in 86Box even without FRAMELOG | Needs real hardware |

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

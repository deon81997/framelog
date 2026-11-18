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

Tested on three 3dfx cards in 86Box: Voodoo Graphics (4 MB, driver 3.01.00) and Voodoo2 (12 MB, 3.02.02) as add-on
cards next to a Cirrus 5446, and a Voodoo3 3000 (1.02.18). "Within" = FRAMELOG's frames per second over the same
stretch the game times, against the game's own number, in the same run.

| Game | API | Voodoo Graphics | Voodoo2 | Voodoo3 | How to benchmark it |
|---|---|---|---|---|---|
| Quake II 3.14 demo | 3dfx MiniGL (`3dfxgl`) | ✅ 42.7 fps, within 0.1% | ✅ 41.4 fps, within 0.1% | ✅ 52.0 fps, within 0.1% | Timedemo below + `+set gl_driver 3dfxgl` |
| Quake II 3.14 demo | 3dfx OpenGL ICD | ✅ 39.7 fps, within 0.1% | ✅ 40.6 fps, within 0.1% | ✅ 56.9 fps, within 0.1% | `+set gl_driver 3dfxogl` (Voodoo Graphics), `3dfxvgl` (Voodoo2), `opengl32` (Voodoo3) |
| Quake II 3.14 demo | Software, 640x480 | (Cirrus) ✅ 24.5 fps, within 0.3% | (same) | ✅ 39.9 fps, same as the game | `+set vid_ref soft +set sw_mode 3` |
| Unreal Tournament demo 338 | Glide | ✅ within 0.1%* | ✅ within 0.1%* | ✅ 72.5 fps, exact | See below |
| Expendable demo | Direct3D | ✅ within 1.3% | ✅ within 0.9% | ✅ within 1.1% | `go.exe -timedemo` (press Enter on its box), result in `timedemo.txt` |
| Incoming demo | Direct3D | ❔ not run | ✅ 41.82 fps, exact | ❌ 86Box shows garbage and hangs, also without FRAMELOG | `gameindex.exe -secondary -nosound` (Voodoo1/2), result in `fps.txt` |
| Forsaken demo | Direct3D | ❔ not run | ❔ not run | ✅ within 0.5% | Play Demo, biodome.dmo |
| 3DMark99 Max | Direct3D 6 | ❔ not run | ✅ runs to the end (3498 3DMarks), all frames logged | ❌ crashes 86Box, also without FRAMELOG | Run with the defaults; also fine on a Voodoo Banshee (4180) |

\* Unreal Tournament's own timedemo number is 10% too high in Hardcore mode (the default): it divides the time by
the game speed 1.1. With `bHardCoreMode=False` in `UnrealTournament.ini` it matches FRAMELOG exactly; with Hardcore on,
divide its number by 1.1.

Notes:

- On the Voodoo Graphics, Expendable hangs with a black screen after writing its `timedemo.txt` (also without
  FRAMELOG), so run it last.
- Expendable and Incoming on a Voodoo Graphics / Voodoo2 need the 3dfx card picked: `DisplayDriver [1]` in
  Expendable's `config.txt`, `-secondary` for Incoming.
- 3DMark99 on an add-on card draws its title screens on the desktop card first. FRAMELOG now notices when the game
  starts page-flipping on the 3dfx card and updates the run info (new in test7).
- In 86Box the same game can run 5-15% faster or slower from one boot to the next, with or without FRAMELOG, so
  "with" vs "without" numbers from the emulator only show there's no big slowdown. The real cost check is on the
  [[Measurements]] page.

## Unreal Tournament demo: a timedemo by hand

The demo has no flyby, so time a fixed stretch: start `UnrealTournament.exe DM-TurbineDEMO.unr -nosound`, press Tab
and type `timedemo 1`, wait a minute, then Tab `timedemo 0`. The game writes
`N frames rendered in S seconds. X FPS average.` to `System\UnrealTournament.log`.

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

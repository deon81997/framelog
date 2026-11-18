# Can you trust the numbers?

A benchmark tool has two jobs. It has to **see every frame** the game draws, and it must **not slow the game down**
while it watches. Here's how FRAMELOG does on both so far.

These are all emulator results (86Box, Pentium II 450). Real hardware will be added once the
[[Hardware Test Checklist]] runs are in.

## Does it see every frame?

The simplest check: the game counts its own frames, so compare.

| Game | The game says | FRAMELOG says | Difference |
|---|---|---|---|
| Quake II, OpenGL timedemo | 58.3 fps | 56.7 fps over the whole run, including loading | about 1% |
| Quake II, software renderer | 39.9 fps | 39.88 fps over the same 632 demo frames | under 0.1% |
| Forsaken demo | 85.19 fps over 19162 frames | 84.80 fps over the same 19162 frames | 0.5% |
| The test programs | N frames, each 1000/FPS ms | N-1 frames, each exactly 1000/FPS ms, none lost | none |

Why "N-1"? When you press start on the host, the next frame is the starting line. The times are the gaps *between*
frames, so N frames on screen make N-1 frame times.

FRAMELOG usually logs a few more frames than a timedemo reports, because it also sees the loading screen and the
console. Comparing the matching stretch gives the near-perfect numbers above.

On DOS, Duke Nukem 3D, Rise of the Triad and the FLIP test program all log their page flips with no lost frames.

## Does it slow the game down?

To measure that, the test programs count how much spare CPU time they have between frames, once with FRAMELOG
logging and once without. The difference is FRAMELOG's cost. The emulator wobbles by about half a percent between
runs, so anything that small is just noise.

| What | Cost |
|---|---|
| DOS page flips at 70 fps | 0.35% |
| DOS VESA flips, run info | nothing measurable |
| Windows DirectDraw, Direct3D 6/8/9, OpenGL, Glide | all within the noise |

And measured in the games themselves:

| Game | Without FRAMELOG | With FRAMELOG |
|---|---|---|
| Quake II, software renderer | 39.9 fps | 39.9 and 39.8 fps (two runs) |
| Quake II, OpenGL | 56.1 fps | 56.7 fps |

With NUKED SB-EMU and the SC-55 emulation running alongside, their load doesn't change either.

## Is the cable fast enough?

Easily. Each frame is 9 bytes on the wire. At 115200 baud that's room for about 1280 frames per second, far beyond
anything a 90s PC will draw. Even a slow 9600-baud link would manage 106.

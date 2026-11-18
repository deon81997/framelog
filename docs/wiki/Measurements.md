# Measurements

Two questions: does FRAMELOG see the frames the game says it drew, and what does it cost the game?
All numbers below are from 86Box (Pentium II 450, 440BX) and are emulator results; real-hardware numbers will be
added after the [[Hardware Test Checklist]] runs.

## Accuracy: FRAMELOG vs the game's own fps

Windows 98, Voodoo3 3000 AGP, 3dfx driver 1.02.18, DirectX 9.0c:

| Game | Game says | FRAMELOG | Difference |
|---|---|---|---|
| Quake II 3.14 demo, OpenGL, timedemo | 58.3 fps (632 frames) | 742 presents (demo + load/console), 56.7 fps whole run | within 1% |
| Quake II 3.14 demo, software 640x480 | 39.9 fps (632 frames) | 743 presents; the 632-frame demo window = 39.88 fps | < 0.1% |
| Forsaken demo, biodome.dmo | 85.19 fps, 19162 frames | last 19162 frames 84.80 fps | 0.5% |
| Test programs (DDTEST, D3D8TEST, GLTEST, G2TEST ...) | N frames at 1000/FPS ms | N-1 frames at 1000/FPS ms, 0 lost | exact |

DOS: Duke Nukem 3D, ROTT, FLIP all log their page flips with 0 lost frames (see [[Game Coverage]]).

## Cost on the game PC

Measured with spare-CPU counters in the test programs (busy loops while waiting for the next frame), FRAMELOG
logging vs not running. Run-to-run noise is about +-0.5% (86Box drifts ~1% between runs).

| Platform / API | Cost |
|---|---|
| DOS CRTC logging, 70 fps | 0.35% |
| DOS VESA flips | -0.09% (none) |
| DOS run info | +0.07% (none) |
| Win98 DirectDraw flip / Blt | -0.5% / +0.5% (noise) |
| Win98 Direct3D 6 | +1.5% (noise) |
| Win98 OpenGL / Glide 2 | +0.06% / +0.35% (noise) |
| Win98 Direct3D 8 / 9 | -0.5% / -0.6..+2% (noise) |
| Quake II software (fps) | 39.9 without, 39.9 / 39.8 with |
| Quake II OpenGL (fps) | 56.1 without, 56.7 with |

With NUKED SB-EMU + SC-55 running, its load is unchanged by FRAMELOG.

## Serial bandwidth

A frame record is 9 bytes on the wire (90 bits): 115200 baud carries 1280 frames/s, 9600 baud carries 106.

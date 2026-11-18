# Known Issues and Roadmap

## Known issues

| Issue | Status |
|---|---|
| 3DMark99 Max garbage / error box / 86Box crash or guest hang on 86Box's Voodoo3, at 800x600 triple, 640x480 triple and 640x480 double | emulator bug (also without FRAMELOG); test on real hardware |
| DOS: real-mode games are not logged | `/RM` in progress: the V86 trap layer (JEMM + QPIEMU) loses word-sized OUT data, so half the flips are missed |
| DOS: copy-to-screen games without vsync waits (Heretic, Duke ScreenMode 2) give no frames | by design: use a capture card + `flcap.py` |
| ROTT 1.3: regular ~65 ms hitches in the log | not explained yet |
| Win98 Direct3D 9 on Voodoo3 1.02.18 drivers: CreateDevice fails | driver limit (DX6 class); D3D8 works |
| Win98: killing FRAMELOG (End Task) while a DirectDraw game runs | quit the game before starting FRAMELOG again |
| Everything is emulator-tested only | hardware runs pending ([[Hardware Test Checklist]]) |

Fixed in test6: Quake II's software renderer logged nothing (it presents with BltFast, which was not hooked).

## Roadmap

1. Real-hardware runs on the D810 and P5PE-VM (DOS + Win98, incl. 3DMark99).
2. `/RM`: real-mode DOS games.
3. DOS Glide games (3dfx DOS Glide, e.g. on a Voodoo card under DOS).
4. Capture workflow for copy-to-screen games (`flcap.py` with a real capture card).
5. More games in [[Game Coverage]] and GPU-vs-GPU reports with `flreport.py`.

# Known Issues and Roadmap

<img src="https://raw.githubusercontent.com/deon81997/framelog/main/docs/90s/construction.gif" width="200" align="right" alt="Under construction">

FRAMELOG is still a test build, so here's what doesn't work yet and what's coming.

<br clear="right">

## Known issues

**Nothing tested on real hardware yet.** Every result so far comes from 86Box and QEMU. That's the very next step;
see the [[Hardware Test Checklist]].

**3DMark99 won't run in 86Box.** On the emulated Voodoo3 it shows garbage, then either crashes the emulator or
freezes Windows. That happens at every resolution and buffer setting, with or without FRAMELOG, so it's an
emulator problem. It needs a real PC to test.

**Real-mode DOS games aren't logged.** FRAMELOG works with protected-mode games (DOS4GW, DJGPP, PMODE/W...). Real-mode
support (`/RM`) is in progress. The tricky part: the layer that traps a real-mode game's port writes currently
loses half of a 16-bit write, so half the page flips go missing.

**Some DOS games can't be trapped at all.** Heretic and Duke Nukem 3D in ScreenMode 2 copy each frame straight to the
screen without flipping or waiting for anything. Nothing on the game PC can see those frames, so they need a VGA
capture card and `flcap.py`.

**Rise of the Triad shows regular ~65 ms hitches.** They might be real or might be the emulator; not explained
yet.

**Direct3D 9 on a Voodoo3** fails with the 1.02.18 driver, because that driver is too old for D3D9. Direct3D 8
works.

**Killing FRAMELOG with End Task** while a DirectDraw game runs leaves things half-hooked. Quit the game before you
start FRAMELOG again.

Fixed in test6: Quake II's software renderer used to log nothing. It turned out to put frames on screen with
DirectDraw's `BltFast`, which FRAMELOG didn't watch yet.

## What's next

1. **Real hardware**: the D810 and the P5PE-VM, DOS and Windows 98, finally including 3DMark99.
2. **Real-mode DOS games** (`/RM`).
3. **DOS Glide games**: 3dfx games that talk to a Voodoo directly from DOS.
4. **A real capture card** for the games no trap can see.
5. **More games and more GPUs**, and proper side-by-side comparison pages with `flreport.py`.

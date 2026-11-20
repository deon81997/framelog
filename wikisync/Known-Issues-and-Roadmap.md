# Known Issues and Roadmap

<img src="https://raw.githubusercontent.com/deon81997/framelog/main/docs/90s/construction.gif" width="200" align="right" alt="Under construction">

FRAMELOG is still a test build, so here's what doesn't work yet and what's coming.

<br clear="right">

## Known issues

**Nothing tested on real hardware yet.** Every result so far comes from 86Box and QEMU. That's the very next step;
see the [[Hardware Test Checklist]].

**3DMark99 crashes on 86Box's Voodoo3.** It shows garbage, then crashes the emulator or freezes Windows, with or
without FRAMELOG. On the emulated Voodoo2 and Banshee it runs to the end, so it's the emulator's Voodoo3. Per-test
numbers still need a real PC.

**Real-mode DOS games aren't logged.** FRAMELOG works with protected-mode games (DOS4GW, DJGPP, PMODE/W...). Real-mode
support (`/RM`) is in progress. The tricky part: the layer that traps a real-mode game's port writes currently
loses half of a 16-bit write, so half the page flips go missing.

**Some DOS games can't be trapped at all.** Heretic and Duke Nukem 3D in ScreenMode 2 copy each frame straight to the
screen without flipping or waiting for anything. Nothing on the game PC can see those frames, so they need a VGA
capture card and `flcap.py`.

**Rise of the Triad shows regular ~65 ms hitches.** They might be real or might be the emulator; not explained
yet.

**DOS Glide is only tested on an emulated Voodoo 1.** Voodoo2 Glide files haven't been tried, and a few very old
Glide 2.1x builds aren't recognised yet. If `/STATUS` shows a "last error" of 21-24 on the Static Glide line,
please tell me the game.

**The very last frame of a DOS Glide run can be missing from the CSV.** It waits in FRAMELOG's buffer until the
next event that never comes. One frame out of thousands; a fix is planned.

**Some games hang in 86Box on certain Voodoo cards, with or without FRAMELOG:** Expendable on Voodoo 1, Incoming
in `-primary` mode on Voodoo3. Emulator problems, not FRAMELOG's.

**Direct3D 9 on a Voodoo3** fails with the 1.02.18 driver, because that driver is too old for D3D9. Direct3D 8
works.

**Killing FRAMELOG with End Task** while a DirectDraw game runs leaves things half-hooked. Quit the game before you
start FRAMELOG again.

Fixed in test6: Quake II's software renderer used to log nothing. It turned out to put frames on screen with
DirectDraw's `BltFast`, which FRAMELOG didn't watch yet.

## What's next

1. **Real hardware**: the D810 and the P5PE-VM, DOS and Windows 98, finally including 3DMark99.
2. **Real-mode DOS games** (`/RM`).
3. **A real Voodoo 1 / Voodoo2 PC** for the DOS Glide games (done in 86Box since test8 and test10).
4. **A real capture card** for the games no trap can see.
5. **More games and more GPUs**, and proper side-by-side comparison pages with `flreport.py`.

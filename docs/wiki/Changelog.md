# Changelog

Every build so far is a test build. They're on the [releases page](https://github.com/deon81997/framelog/releases),
and each zip has the DOS, Windows 98 and host parts together.

## test6 (November 17, 2026)

**Quake II's software renderer is logged now.** It turned out Quake II never flips pages in its software mode. It
copies each finished frame to the screen with DirectDraw's `BltFast`, which FRAMELOG wasn't watching. Now it is, and
Quake II scores the same 39.9 fps with and without FRAMELOG.

Also new: a `fast` mode for the DDTEST test program that draws the same way, and `/LOG` now shows how many Flip, Blt
and BltFast calls each program made. The DOS part didn't change.

## test5 (November 17, 2026)

**Direct3D 8 and 9.** FRAMELOG catches `Present` in DirectX 8 and 9 games. This is also the first build tested
against real games on an emulated Voodoo3: Quake II in OpenGL came within 1% of its own fps, and Forsaken within 0.5%.

## test4 (November 17, 2026)

**Windows 98!** The first Windows build, with DirectDraw, Direct3D 5-7, Glide 2 and 3, and OpenGL, plus a mode where
FRAMELOG launches the game itself. The DOS version learned to send run info (card, driver, resolution, CPU), so CSV
files label themselves. The HTML report tool joined the zip.

## test3

**VESA games.** Page flips through the VESA BIOS, including the fast VBE 2.0 protected-mode interface that Duke
Nukem 3D uses at 640x480. On by default; `/NOVESA` turns it off.

## test2

**Games that never flip.** The `/3DA` switch counts vertical-retrace waits instead, for games that copy to the
screen but wait for vsync first.

## test1

**The first package.** The DOS program that times page flips and sends them over serial, the FLIP test game, the host
logger and stats tool, and the first guide.

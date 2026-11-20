# Changelog

Every build so far is a test build. They're on the [releases page](https://github.com/deon81997/framelog/releases),
and each zip has the DOS, Windows 98 and host parts together.

## Coming in the next build (on main, after test10)

**More DOS Glide games: Tomb Raider 3Dfx and Battle Arena Toshinden 2 3Dfx.** These demos use the older Glide 2.03,
which FRAMELOG now finds as well. On 86Box's Voodoo Graphics every buffer swap was logged with none lost: Tomb
Raider ran at about 30 fps and Toshinden 2's fights at about 51 fps. The Redguard demo now quits cleanly in the test
rig, and its swaps match the logged frames exactly (1275 of 1275, about 29 fps).

## test10 (November 18, 2026)

**Games with Glide built in.** Some 3dfx games don't load `GLIDE2X.OVL`; Glide is linked straight into the game,
like Carmageddon. FRAMELOG now scans the game's own files once while it loads, finds Glide's buffer swap and hooks
it in memory (the files on disk aren't changed). Carmageddon logged every one of 6887 swaps, with the race at 41-52
fps. The scan takes about a third of a second at load and costs nothing while you play. `/STATUS` has a new "Static
Glide" line.

## test9 (November 18, 2026)

**A live status line in the host logger.** While it records, `fllog.py` now shows one line that updates every
second: frames so far, fps over the last second and since the start, the slowest recent frame and any lost frames.
It also tells you when the game PC is connected but hasn't drawn yet. `--no-live` turns it off. The CSV files don't
change.

Every release now also comes with a separate **FRAMELOG-HOST** zip with just the host tools, including
double-click `FLLOG.BAT` and `REPORT.BAT`.

## test8 (November 18, 2026)

**DOS Glide games.** FRAMELOG for DOS now logs 3dfx games that load Glide from `GLIDE2X.OVL`: every buffer swap is
a frame. Tested on 86Box's Voodoo Graphics: the Extreme Assault and Archimedean Dynasty demos logged every swap with
none lost, and the Redguard demo logged too. Cost: under 0.1% of the CPU.

These games stop with "DLL load aborted" under HDPMI32i, even without FRAMELOG. FRAMELOG fixes that while the game
loads, so they run with HDPMI32i (and NUKED SB-EMU) after all. `/NOGLIDE` turns the Glide hook off.

## test7 (November 18, 2026)

**Every 3dfx card in Windows 98.** Tested on 86Box's Voodoo Graphics, Voodoo2, Voodoo Banshee and Voodoo3: Quake II
(MiniGL and OpenGL), Incoming and Unreal Tournament all matched their own fps counters. Three Windows 98 fixes came
out of it: the first DirectDraw game after a reboot sometimes lost its GPU name, Voodoo cards are now told apart
correctly, and a program that switches from one way of drawing to another gets fresh run info.

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

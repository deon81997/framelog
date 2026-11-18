# Hardware Test Checklist

Everything so far was tested in emulators, and real PCs always find new ways to surprise you. This is the list I run
on my Dell Latitude D810 and ASUS P5PE-VM with
[v0.1-test6](https://github.com/deon81997/framelog/releases/tag/v0.1-test6). If you'd like to help, run any part of it
on your own machine and send me the results.

Name the CSV files `game-machine.csv` (like `duke-d810.csv`) so they don't get mixed up.

## DOS

- [ ] Load FRAMELOG and run `FRAMELOG /STATUS` before starting anything. Note what it prints.
- [ ] `FLIP 20 70` while logging. Does the CSV show about 70 fps?
- [ ] `FLIP 20 70 10 B`, the same with VESA flips. Still about 70 fps?
- [ ] Duke Nukem 3D attract demo with `ScreenMode = 0`, once without FRAMELOG and once with it.
- [ ] Duke Nukem 3D in VESA (`ScreenMode = 1`, 640x480) with FRAMELOG, then `FRAMELOG /STATUS` again.
- [ ] Doom: `DOOM -timedemo demo3` without and with FRAMELOG. The "realtics" number should match.
- [ ] Look at the top of a CSV: do the `gpu`, `driver`, `res`, `depth` and `cpu` lines match your card, CPU and the
      game's mode? (Duke ScreenMode 0 should say 320x200 X; ScreenMode 1 should say 640x480 VESA.)
- [ ] `FRAMELOG /STATUS` once more after all the runs.
- [ ] SBDIAG reports with and without FRAMELOG, if you use NUKED SB-EMU.

## Windows 98

- [ ] Start `FRAMELOG /LOG`, then run `DDTEST 600 30`, `DDTEST fast 300 30`, `D3DTEST hal 300 30` and
      `GLTEST 300 30`. Each should give N-1 frames of 33.3 ms, with the right card and driver in the CSV.
- [ ] The Quake II OpenGL timedemo (command line on [[Game Coverage]]). How does FRAMELOG's fps compare with Quake
      II's own?
- [ ] The same with the **software renderer** (`+set vid_ref soft +set sw_mode 3`). This one is new in test6.
- [ ] The Forsaken demo (Play Demo, biodome.dmo): its AVG fps against FRAMELOG's.
- [ ] 3DMark99 Max, Game 1 and Game 2. It doesn't run in the emulator, so this is its first real test. Registering it
      as Pro lets you pick tests and buffering: Register, name `Futuremark`, code `78TR2-8DBDU-BMXXS` (a free key
      Futuremark published).
- [ ] D3D8TEST and D3D9TEST on a DirectX 8/9 card (the P5PE-VM's GeForce 6800).
- [ ] Let FRAMELOG launch a game (`FRAMELOG game.exe`), then press Q while the game is still running. The game
      should carry on happily.

## What to send back

All the CSVs, `FRAMELOG.LOG` from Windows, the `/STATUS` output from DOS, any SBDIAG reports, and a few words about
anything odd: hangs, sound glitches, missing frames, numbers that look wrong. Odd results are the most useful ones.

# Hardware Test Checklist

What to run on the real machines (Dell Latitude D810, ASUS P5PE-VM) with
[v0.1-test6](https://github.com/deon81997/framelog/releases/tag/v0.1-test6), and what to send back.
Name CSVs `game-machine.csv`.

## DOS

- [ ] `FRAMELOG /STATUS` after loading, before any game: send the output.
- [ ] `FLIP 20 70` logged: ~70 fps in the CSV?
- [ ] `FLIP 20 70 10 B` (VESA flips) logged: ~70 fps?
- [ ] Duke Nukem 3D demo, `ScreenMode = 0`, without FRAMELOG, then with FRAMELOG logged.
- [ ] Duke Nukem 3D VESA (`ScreenMode = 1`, 640x480) with FRAMELOG logged, then `FRAMELOG /STATUS`.
- [ ] Doom: `DOOM -timedemo demo3` without and with FRAMELOG (realtics should match).
- [ ] Run info: does the CSV top (`# gpu= / driver= / res= / depth= / cpu=`) match the real card, CPU and mode?
      (Duke ScreenMode 0 = 320x200 X, ScreenMode 1 = 640x480 VESA.) Send the lines FRAMELOG prints when loading.
- [ ] `FRAMELOG /STATUS` again after the runs.
- [ ] SBDIAG reports with and without FRAMELOG.

## Windows 98

- [ ] `FRAMELOG /LOG`, then `DDTEST 600 30`, `DDTEST fast 300 30`, `D3DTEST hal 300 30`, `GLTEST 300 30`:
      each N-1 frames at 33.3 ms, gpu / driver lines right.
- [ ] Quake II OpenGL timedemo (command line on [[Game Coverage]]): FRAMELOG fps vs Quake II's number.
- [ ] Quake II **software renderer** (`+set vid_ref soft +set sw_mode 3`): same comparison (new in test6).
- [ ] Forsaken demo (biodome.dmo): AVG fps vs FRAMELOG.
- [ ] 3DMark99 Max Game 1 and Game 2 (register it as Pro: Register > name `Futuremark`, code
      `78TR2-8DBDU-BMXXS`, a free key Futuremark published, to pick tests and buffering): per-test fps vs FRAMELOG.
- [ ] D3D8TEST / D3D9TEST on the P5PE-VM's GeForce 6800.
- [ ] Launch mode: `FRAMELOG game.exe` for one game, then Q while the game still runs (the game must keep running).

## Send back

All CSVs, FRAMELOG.LOG (Win98), the `/STATUS` outputs (DOS), SBDIAG reports, and any problems: hangs, sound
glitches, missing frames.

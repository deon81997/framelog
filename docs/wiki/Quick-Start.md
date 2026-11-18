# Quick Start

## What you need

- The **game PC** (DOS and/or Windows 98) with a COM port.
- A **host PC** (Windows or Linux) with a COM port or a USB-serial adapter.
- A **null-modem** serial cable (TX/RX crossed). A straight cable does not work.
- `FRAMELOG-0.1-test6.zip` from the [release page](https://github.com/deon81997/framelog/releases/tag/v0.1-test6).

## 1. Host

1. Install Python 3, then `pip install pyserial matplotlib`.
2. Unzip the `HOST` folder and start the logger before (or while) the game runs:

   ```
   py fllog.py COM3 -o run.csv --label D810
   ```

   Linux: `python3 fllog.py /dev/ttyUSB0 -o run.csv --label D810`. Windows shortcut: `LOG.BAT COM3 run.csv D810`.
3. Press Ctrl+C when the run is done.
4. Stats and a graph: `py flstats.py run.csv --png run.png` (several CSVs = a comparison).

## 2a. DOS game PC

1. Copy `FRAMELOG\FRAMELOG.EXE` to e.g. `C:\FRAMELOG`.
2. In `AUTOEXEC.BAT`, **after** `HDPMI32I.EXE -r -x` (or after the SC55 line when NUKED SB-EMU is used):

   ```
   C:\FRAMELOG\FRAMELOG /COM:1
   ```
3. Reboot, start a game. Logging starts at the game's next page flip once the host sent its start byte.

Test without a game: `FLIP 20 70` flips the screen at 70 fps for 20 s; the CSV should show ~70 fps.

## 2b. Windows 98 game PC

1. Copy `WIN98\FRAMELOG.EXE` and `FLHOOK.DLL` into one folder.
2. Either run `FRAMELOG /COM:1` and then start games the normal way (each game that presents frames gets logged in
   turn), or launch one game through it:

   ```
   FRAMELOG /COM:1 C:\QUAKE2\QUAKE2.EXE +timedemo 1 +demomap q2demo1.dm2
   ```
3. Quit FRAMELOG with Q / Esc / Ctrl+C or `FRAMELOG /STOP` before shutting down (it puts the DirectDraw tables back).

Test without a game: `WIN98\TEST\DDTEST 600 30` should log 599 frames at 33.3 ms.

## 3. Reports

```
py flreport.py results/ -o report/
```

makes one HTML page per GPU, a side-by-side comparison page and an index (see [[Host Tools]]).

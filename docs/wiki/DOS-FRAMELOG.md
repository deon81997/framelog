# DOS FRAMELOG

`FRAMELOG\FRAMELOG.EXE` is a small resident program for the DOS game PC. It notices every page flip of a running
game and sends a timestamp per frame over the serial port. No overlay, no game patching, no files changed: load it
once and every game is logged.

## Install

```
HDPMI32I.EXE -r -x
C:\FRAMELOG\FRAMELOG /COM:1
```

FRAMELOG must come **after** HDPMI32i (or after the SC55 line with NUKED SB-EMU), because it needs HDPMI32i
resident. When loaded it prints its UART, baud rate and CPU MHz.

## Options

| Option | What |
|---|---|
| `/COM:n[,baud]` | COM1-COM4 (default COM1, 115200) |
| `/COM:hex[,baud]` | UART at a hex port address, e.g. `/COM:2F8` |
| `/AUTO` | start logging at once (no host `S` byte needed) |
| `/3DA[:ms]` | frames = vertical-retrace waits, for games that copy to the screen (costs CPU, see below) |
| `/NOVESA` | do not watch VESA page flips (INT 10h left alone) |
| `/NOMETA` | send no run info |
| `/STATUS` | counters of the loaded copy: flips, frames, lost, bytes sent, host bytes, starts, traps, VESA display-start calls, run-info blocks, mode sets |

## How frames are caught

| Mode | A frame is | When |
|---|---|---|
| CRTC (default) | a write to CRTC 0Ch/0Dh (start address) via 3D4h/3D5h, 8-bit or `OUT 3D4h,AX`; writes < 0.5 ms apart = one flip | page-flipping VGA / Mode X games |
| VESA (default on) | INT 10h AX=4F07h, or the VBE 2.0 protected-mode SetDisplayStart (4F0Ah): FRAMELOG hands the game a copy of the card's table whose SetDisplayStart entry notes the time and then runs the card's own code | SVGA games, e.g. Duke Nukem 3D ScreenMode 1 |
| `/3DA` | the first retrace start (3DAh bit 3) the game reads after being away from 3DAh for >= 6 ms (`/3DA:ms`): one vsync wait = one frame | copy-to-screen games that wait for vsync |

VESA support takes about 1.2 KB of DOS memory (16 bytes with `/NOVESA`). Use `/NOVESA` if a VESA game or VESA
driver (UNIVBE, SOLVBE) misbehaves with FRAMELOG loaded, and report it.

## Run info (metadata)

When logging starts FRAMELOG sends a short `M` record and `fllog.py` writes it into the CSV as `# key=value` lines,
filling what you did not type yourself:

- `gpu`: the VESA BIOS OEM string (+ vendor and product on VBE 2.0+), `VGA` without a VESA BIOS
- `driver`: VBE version and the card's OEM software revision
- `res`, `depth`, `mode`: the screen mode (VESA mode list, BIOS, or read from the VGA registers: mode 13h,
  Mode X `320x240`, text `03h` ...)
- `cpu`: CPUID brand string, or vendor / family / model / stepping, plus the measured MHz

A mode set while logging (INT 10h AH=00h or 4F02h) sends a fresh block before the next frame; the CSV gets a
`# change@N=res=640x480 ...` line before frame N. Values you give `fllog.py` (`--gpu`, `--res`, a suite file) win;
a different game-PC value is kept as `pc_res` etc.

## Limits

- Protected-mode games only (DOS4GW, DJGPP, PMODE/W ...). Real-mode games are not logged yet (`/RM` is planned).
- Start and stop act at the game's **next** page flip (the UART is read only from inside the trap).
- Do not share the COM port with SBEMU `/DBG` or serial MIDI.
- Games that copy to the screen without waiting for vsync (Heretic, Duke ScreenMode 2) give no frames in any
  mode: only a capture card + `flcap.py` sees them (see [[Host Tools]]).

## Cost

| What | Cost (86Box Pentium II 450) |
|---|---|
| CRTC logging, 70 fps | about 0.35% of the game's CPU |
| VESA flips (4F07h), 70 fps | none measurable (-0.09%) |
| Run info (M records) | none measurable (+0.07%, noise 0.6%) |
| `/3DA` | the game keeps its frame rate but its wait loops run slower (3 million trapped reads in 30 s): opt-in only |

SBEMU / SC-55 load is unchanged: no IRQ0, PIT or keyboard hooks.

## Test game FLIP.EXE

`FLIP SECONDS FPS [LONG] [M]` flips the screen at a fixed rate; every LONG-th frame (default 10) takes twice as
long. `M` also plays MIDI on MPU-401 330h. `B` flips with VESA 4F07h in 640x480 (mode 101h) instead.
Example: `FLIP 20 70`.

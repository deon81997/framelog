# Host Tools

The host side lives in [deon81997/serial-bench](https://github.com/deon81997/serial-bench) and ships in the zip's
`HOST` folder. Needs Python 3 + `pip install pyserial matplotlib` (`flcap.py` also `opencv-python numpy`).

## fllog.py: the logger

```
fllog.py PORT -o run.csv [--label NAME] [--idle S] [--seconds N]
```

Sends `S` (start), writes frames to a CSV, sends `X` (stop) on Ctrl+C. PORT = `COM3`, `/dev/ttyUSB0`, or a pyserial
URL (`socket://127.0.0.1:4555` for a QEMU serial port).

**Metadata**: `--gpu G --driver D --res WxH --depth BITS --cpu C --game NAME --notes TEXT [--meta file.ini]` are
stored as `# key=value` lines at the top of the CSV. The game PC's run info fills what you leave empty; where both
differ the game PC's value is kept as `pc_<key>`.

**Suites** (a batch of timedemos in one go):

```
fllog.py COM3 --suite suite.ini -o results/ [--report report/]
```

One CSV per run, named `game-gpu-res.csv`. A run ends at an end / new begin marker or a gap (`gap` seconds without
frames); fragments under `min_frames` are dropped; runs map to the INI's sections in order:

```ini
[suite]
name = d810-dos
gap = 5
min_frames = 30
gpu = ATI Mobility Radeon X300
cpu = Pentium M 2 GHz

[duke]
game = Duke3D 1.3D attract
res = 320x200
depth = 8

[rott]
game = ROTT 1.3 attract
res = 320x200
depth = 8
```

CSV body: `frame,t_ms,frametime_ms`.

## flstats.py: numbers and a graph

```
flstats.py a.csv [b.csv ...] [--png g.png] [--skip S] [--label-by gpu,res] [--group-by gpu] [--where key=value]
```

Per run: frames, seconds, average fps, average / min / max frame time, 1% and 0.1% lows (fps at the 99th / 99.9th
percentile frame time), lost frames. Several files = a comparison; `--group-by` pools runs per group.

## flreport.py: HTML report

```
flreport.py FOLDER [...] [-o report/] [--skip S] [--title T] [--where k=v] [--default k=v]
```

From a folder of CSVs (recursive): `gpu-<name>.html` per GPU (avg fps, 1% / 0.1% lows, frame-time graph and
percentile curve per game, with hover), `compare.html` (all GPUs side by side per game), `index.html`.
Grouping: GPU = `gpu`, game = `game` + `res` + `depth`. `--default gpu="S3 Trio64"` fills metadata older CSVs
lack. Self-contained pages, light and dark.

## flcap.py: capture-card analyzer

```
flcap.py capture.avi -o run.csv [--fps 70.086] [--crop X,Y,W,H] [--serial run.csv] [--tears t.csv] [--multi-tear 3]
```

Frame times from a VGA capture (Datapath etc.) of what actually reached the screen, row by row: tear lines,
repeats, runts, adaptive noise threshold. Same CSV format (`source=capture`), so flstats / flreport work on it.
`--serial` aligns it with the fllog CSV of the same run. This is the way to time copy-to-screen games no trap sees
(Heretic, Duke Nukem 3D ScreenMode 2).

## flproto.py

Protocol encoder / decoder (resyncs on line noise), including the `M` run-info record. See [[Serial Protocol]].

## Tests

`python3 -m unittest discover -s tests`. Preview data without hardware:
`python3 tests/synth.py /tmp/syn && python3 flreport.py /tmp/syn -o /tmp/report`.

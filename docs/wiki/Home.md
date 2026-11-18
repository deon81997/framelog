# FRAMELOG

**Frame-time logging for DOS and Windows 98 games, over a serial cable, to a second PC.**

FRAMELOG times every frame a retro game shows and sends the times over a null-modem cable to a host PC. The host
writes one CSV per run and turns the CSVs into stats, graphs and HTML reports: average fps, 1% / 0.1% lows,
frame-time plots, GPU-vs-GPU comparisons. Think FRAPS / PresentMon, but for a Pentium with a Voodoo, and with
no overlay, no patched game files and almost no cost on the game PC.

| Part | Runs on | What it does |
|---|---|---|
| `FRAMELOG\FRAMELOG.EXE` | DOS game PC | resident program (TSR, needs HDPMI32i): catches page flips (VGA CRTC start address, VESA 4F07h, VBE 2.0 PM SetDisplayStart) |
| `WIN98\FRAMELOG.EXE` + `FLHOOK.DLL` | Windows 98 game PC | hooks DirectDraw / Direct3D 5-9 / Glide 2-3 / OpenGL presents in memory |
| `HOST\fllog.py` + friends | host PC (Windows or Linux) | logger, stats, HTML reports, capture analyzer ([serial-bench](https://github.com/deon81997/serial-bench)) |

## Download

Latest test build: **[v0.1-test6](https://github.com/deon81997/framelog/releases/tag/v0.1-test6)** (pre-release,
`FRAMELOG-0.1-test6.zip`). One zip holds all three folders. See [[Changelog]].

## Pages

- [[Quick Start]]: cable, host, first log in five minutes
- [[DOS FRAMELOG]]: install, options, VESA, run info, which games log, cost
- [[Windows 98 FRAMELOG]]: what counts as a frame, launch mode, options, limits, test programs
- [[Host Tools]]: `fllog.py`, suites, `flstats.py`, `flreport.py`, `flcap.py`
- [[Game Coverage]]: tested games and how to benchmark them
- [[Measurements]]: accuracy against the games' own fps, and FRAMELOG's own cost
- [[Serial Protocol]]: the wire format (records, run info, what a frame is)
- [[Hardware Test Checklist]]: what to run on real machines and send back
- [[Known Issues and Roadmap]]
- [[Changelog]]

## Status (2026-11-17)

Everything so far is tested in emulators (86Box Pentium II 450 with Voodoo3 / Cirrus, QEMU). Real-hardware runs
on a Dell Latitude D810 and an ASUS P5PE-VM are the next step; see [[Hardware Test Checklist]].

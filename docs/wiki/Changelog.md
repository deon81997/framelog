# Changelog

Test builds are pre-releases on the [releases page](https://github.com/deon81997/framelog/releases); each zip holds
`FRAMELOG\` (DOS), `WIN98\` and `HOST\`.

## 0.1-test6 (2026-11-17)

- Win98: DirectDraw `BltFast` to the primary surface counts as a frame. Fixes Quake II's software renderer (it
  presents only with BltFast in linear modes): 743 presents, game 39.9 fps with and without FRAMELOG.
- `DDTEST fast` (full-screen BltFast to the front buffer, the Quake II pattern).
- `/LOG` counts Flip / Blt / BltFast per process.
- DOS part unchanged.

## 0.1-test5 (2026-11-17)

- Win98: Direct3D 8 and 9 (`Device::Present`, `SwapChain9::Present`); D3D8TEST / D3D9TEST.
- First real games on 86Box Voodoo3: Quake II OpenGL (within 1%), Forsaken (0.5%).

## 0.1-test4 (2026-11-17)

- Windows 98 version: FRAMELOG.EXE + FLHOOK.DLL (DirectDraw, Direct3D 5-7, Glide 2/3, OpenGL; launch mode).
- DOS: run info (`M` record: gpu, driver, res, depth, cpu, mode; `/NOMETA`).
- Host: `flmeta.py`, `flreport.py` in the zip.

## 0.1-test3

- DOS: VESA page flips (INT 10h 4F07h and the VBE 2.0 PM SetDisplayStart), on by default, `/NOVESA`.

## 0.1-test2

- DOS: `/3DA` vsync-wait mode for copy-to-screen games; `FLIP V`.

## 0.1-test1

- First hardware test package: DOS TSR (CRTC start-address trap over HDPMI32i), FLIP.EXE, host logger and stats,
  FRAMELOG.TXT guide, LOG.BAT.

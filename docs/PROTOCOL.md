# FRAMELOG serial protocol (version 1)

One protocol for the DOS TSR and the Win98 hook DLL. 8N1, default 115200 baud, no flow control.
Little-endian. All timing is in TSC ticks of the game PC; the header gives the TSC rate.

## Host -> game PC (single bytes)

| Byte | Meaning |
|---|---|
| `S` (53h) | start logging: game PC sends `H` header then `B` begin marker; ignored while logging (send `X` first to restart) |
| `X` (58h) | stop logging: game PC sends `E` end marker |

The DOS TSR reads the UART only from inside its CRTC trap, so a command takes effect at the next CRTC
write of a running game (a byte sent before the game starts waits in the UART FIFO). Unknown bytes are ignored.

## Game PC -> host (records)

Every record: `A5h`, type byte, fixed-size payload (by type), checksum = (type + payload bytes) mod 256.
Readers resync by scanning for `A5h` and dropping candidates whose checksum fails.

| Type | Payload | Size | Meaning |
|---|---|---|---|
| `H` | ver u8 (=1), source u8 (0 DOS, 1 Win98), tsc_hz u32, modes u8, reserved u8 | 8 | header, sent on start |
| `B` | tsc u64 | 8 | begin: TSC at the moment logging started |
| `F` | frame u16, delta u32 | 6 | frame: number (low 16 bits, first = 1), TSC ticks since the previous frame (or since `B`) |
| `G` | frame u16, delta u64 | 10 | same as `F` when the delta does not fit 32 bits |
| `D` | count u32 | 4 | frames lost (ring overflow) since the last record; their time is inside the next delta |
| `E` | frames u32, lost u32 | 8 | end: totals since `B` |
| `M` | seq u8, text 15 bytes | 16 | metadata text chunk (see below) |

`modes` bits: 0 CRTC start-address trap, 1 3DAh vsync poll, 2 VESA 4F07h, 3 real-mode (/RM), 4 Win98 swap hook,
5 DOS Glide grBufferSwap hook (docs/GLIDE.md).

## `M` metadata (run info)

Optional; old hosts skip it (unknown type = junk, the checksum still frames it). Sent after `H`, before `B`, and again
(a new block, seq 0) when the game PC's mode changes while logging, before the next frame record.

- Text = `key=value\n` lines, ASCII, split into 15-byte chunks with seq 0, 1, 2 ... (wraps at 255). The last chunk is
  NUL-padded; a text that ends exactly on a chunk edge gets one more, all-NUL chunk.
- seq 0 starts a new block; a gap in seq drops the block. Keys are lower-cased; lines without `=` are ignored.
- Keys: `gpu`, `driver`, `res` (`640x480`), `depth` (bits), `cpu`, `game`, `mode`, plus any others (kept as CSV comments).
- DOS TSR (FRAMELOG, `/NOMETA` = none): `gpu` = VBE 4F00h OEM string (+ vendor / product names on VBE 2.0+), else
  `VGA`; `driver` = `VBE x.y, OEM rev a.bb[, product rev]`; `res`/`depth`/`mode` = the current mode (VESA: the BIOS mode
  list + 4F01h read at load, tracked through INT 10h 4F02h; VGA: 40:49h, mode 13h geometry from CRTC 01h/07h/09h/12h,
  `mode=X` when chain-4 is off; text modes send `mode=text 03h` and no `res`); `cpu` = CPUID brand string, else
  `<vendor> family F model M stepping S`, then `, N MHz` (the TSC rate of `H`). A mode set (INT 10h AH=00h / 4F02h) while
  logging = a fresh block before the next frame.
- Win98 (FRAMELOG.EXE + FLHOOK.DLL, once per start, plus a fresh block when a game that was claimed by Blts to the
  primary starts to Flip - the flipping surface's DirectDraw object gives the new `gpu`/`driver`/`api`; FRAMELOG.EXE
  waits up to 3 s for the first block): `game` (exe name), `gpu` + `driver` (DirectDraw GetDeviceIdentifier of the
  game's own DirectDraw object; Glide 2 grGlideGetVersion + grSstQueryHardware type, type 0 split by fbiRev: < 100h
  Voodoo Graphics, < 10000h Voodoo2, else Banshee/Voodoo3; Glide 3 grGetString; OpenGL
  GL_RENDERER / GL_VENDOR + GL_VERSION), `res` + `depth` (DirectDraw: the primary surface at the first present; Glide 2:
  grSstScreenWidth/Height, 16; OpenGL: the window's client size + desktop depth), `api` (ddraw-flip, ddraw-blt, glide2,
  glide3, opengl, d3d8, d3d9; D3D8/9: gpu/driver = GetAdapterIdentifier, res/depth = the back buffer, `windowed=1`),
  `d3d=1` (a Direct3D DLL is loaded; `8` / `9` for D3D8/9), `gl_dll` (the OpenGL DLL, e.g. 3dfxgl.dll), `pid`, `cpu`
  (CPUID brand string, or the family/model name + TSC MHz), `os`, `hook` (FRAMELOG version).
- Cost: 19 bytes on the wire per chunk, once per start or mode set; nothing per frame.

Host rule (serial-bench fllog.py): what the user gives (options, `--meta`, suite INI) wins; game-PC values fill empty
keys; where both differ the game PC's value is kept as `pc_<key>`. A block after the first frame becomes one
`# change@N=key=value ...` CSV line (`key=` = gone) before frame N.

## What a frame is (Win98, header modes bit 4)

One successful present of the logging process, timed at the call (RDTSC on entry, like PresentMon's
MsBetweenPresents): `IDirectDrawSurface*::Flip` (DirectDraw and Direct3D 5-7 full screen), `Blt` or `BltFast` to the primary
surface with a source surface until the process first flips (windowed DirectDraw/Direct3D; Quake II's software
renderer presents with BltFast), Glide 2/3
`grBufferSwap`, OpenGL `wglSwapBuffers` (any DLL exporting it: opengl32, 3dfx MiniGL) or gdi32 `SwapBuffers`.
Calls that fail (`DDERR_WASSTILLDRAWING` ...) and calls nested inside another hooked present on the same thread do
not count. The first process that presents while FRAMELOG runs is logged until it exits (then `E`, and the next
`S` arms the next process). `S` takes effect at the next present: that present is the `B` moment (not a frame), so
N presents after `S` give N-1 `F` records.

## What a frame is (DOS)

A write to CRTC register 0Ch or 0Dh (start address high/low) through 3D4h/3D5h, 8-bit or `OUT 3D4h,AX`.
Writes closer than 0.5 ms to the previous start-address write belong to the same flip; the frame time is
the first write of the burst.

With `/3DA` (header modes bit 1): a frame is the first vertical-retrace start (3DAh bit 3, 0 -> 1) the game reads
after it was away from 3DAh for at least 6 ms (`/3DA:ms`), i.e. one vsync wait = one frame even if it spans
several retraces. CRTC writes then only pump the UART.

## Bandwidth

`F` = 9 bytes on the wire = 90 bits: 115200 baud carries 1280 frames/s; 9600 baud carries 106.

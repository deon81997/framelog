# Windows 98 FRAMELOG

`WIN98\FRAMELOG.EXE` + `FLHOOK.DLL` time every frame a Windows 98 game presents and send the times over serial to
the same host tools as the DOS version. Per frame the game pays one RDTSC and a push into a shared ring buffer;
FRAMELOG.EXE sends the ring out on the COM port. Nothing on disk changes: no proxy DLLs, no replaced files;
everything is patched in memory and put back when FRAMELOG quits. Windows 95/98/ME only.

## What counts as a frame

One per **successful** call:

| API | Present call |
|---|---|
| DirectDraw / Direct3D 5-7 | `IDirectDrawSurface::Flip` (full screen); `Blt` or `BltFast` to the primary surface (windowed; Quake II's software renderer in linear modes) |
| Direct3D 8 / 9 | `IDirect3DDevice8/9::Present`, `IDirect3DSwapChain9::Present` |
| Glide 2 / Glide 3 | `grBufferSwap` (glide2x.dll / glide3x.dll) |
| OpenGL | `wglSwapBuffers` in any DLL that has it (OPENGL32.DLL, 3dfx MiniGL such as 3DFXGL.DLL, or an OPENGL32.DLL in the game folder) and gdi32 `SwapBuffers` |

- Failed calls (`DDERR_WASSTILLDRAWING` ...) do not count.
- Presents nested inside another present do not count (a D3D8 Present that flips through DirectDraw;
  SwapBuffers -> wglSwapBuffers -> grBufferSwap): only the outermost call per thread.
- Glide swaps in a process with an OpenGL DLL (3dfx ICD / MiniGL) are left to the OpenGL swap.
- A game that both flips and blits to the primary is counted by its flips only.
- `S` from the host takes effect at the next present, which is the start mark: N presents give N-1 frames.

## How it hooks (for the curious)

- **DirectDraw**: on Win9x the surface vtables live in DDRAW.DLL's shared, writable data. FRAMELOG.EXE patches the
  Flip, Blt and BltFast slots of every surface interface (1, 2, 3, 4, 7) to small stubs in a named shared memory
  block (same address in every process) that dispatch per process. DDRAW rewrites its vtables when a process loads
  it, so a watchdog re-hooks them.
- **Direct3D 8/9**: inline hook on `Direct3DCreate8/9`, then the `CreateDevice` vtable slot, then `Present`.
- **Glide / OpenGL**: inline hooks on the exports, including DLLs loaded later through `LoadLibrary`.
- **Getting into the game**: window hooks load FLHOOK.DLL into every GUI process; in launch mode FRAMELOG also starts
  the game suspended and patches a jump at its entry point that loads FLHOOK.DLL first (no CreateRemoteThread on 9x).

## Use

```
FRAMELOG [/COM:n] [/BAUD:n]                          then start games the normal way
FRAMELOG [/COM:n] [/BAUD:n] C:\GAMES\GAME.EXE args   launch mode
```

- **Normal mode** logs the first program that presents a frame until it exits, then the next one: good for a batch
  of timedemos with `fllog.py --suite`.
- **Launch mode** logs only that game, also patches FLHOOK.DLL in at the game's entry point (catches games without a
  normal window; `/NOINJECT` = window hooks only) and quits when the game exits.
- Quit with **Q**, **Esc**, **Ctrl+C** or `FRAMELOG /STOP` from a batch file, always before shutting down.

| Option | What |
|---|---|
| `/COM:n` | COM port (default 1) |
| `/BAUD:n` | baud rate (default 115200) |
| `/LOG` | write FRAMELOG.LOG next to the exe: what got hooked, which process logs, per-process Flip / Blt / BltFast counts |
| `/NOINJECT` | launch mode without the entry-point patch |
| `/Q` | no status line |
| `/STOP` | stop a running FRAMELOG |

The status line shows LOG / ARM / idle, the process, frames and lost frames.

## Run info

Sent once per start as the `M` record: `game` (exe name); `gpu` / `driver` (DirectDraw device identifier of the
game's own DirectDraw, Direct3D 8/9 adapter identifier, Glide version + board, or the OpenGL renderer / vendor /
version); `res` / `depth` (primary surface or back buffer at the first present; Glide 2 screen size; OpenGL window
size + desktop depth); `api` (`ddraw-flip`, `ddraw-blt`, `glide2`, `glide3`, `opengl`, `d3d8`, `d3d9`); `cpu`, `os`,
`hook`. Values given to `fllog.py` win.

## Limits

- Windows 9x only.
- Direct3D 8/9: the device must be created after FLHOOK.DLL is in the game (start FRAMELOG first, or use launch
  mode). Direct3D 9 needs a DirectX 7 class driver (Voodoo3 1.02.18: D3D9 CreateDevice fails, D3D8 works).
- Glide 2 on 3dfx drivers: the 3dfx splash animation counts as frames; `SET FX_GLIDE_NO_SPLASH=1` hides it.
- Without launch mode, a program that opens no window and reads no messages is not caught.
- DOS games in a Win98 DOS box: use the DOS FRAMELOG in a DOS-only boot.
- One FRAMELOG at a time, one process logged at a time.
- If FRAMELOG is killed (End Task) while a DirectDraw game runs, quit the game before starting FRAMELOG again.
- 3DMark99 crashes or hangs on 86Box's emulated Voodoo3 with or without FRAMELOG; untested on real hardware.

## Test programs (`WIN98\TEST`)

N frames at FPS, a result line in `<name>.TXT`; `spin` = busy-wait and count spare CPU loops.

| Program | What |
|---|---|
| `DDTEST [blt\|fast] [modex] [spin] N FPS` | DirectDraw 640x480x8 page flips (`modex` = DDSCL_ALLOWMODEX), windowed Blt to the primary (`blt`), or full-screen BltFast to the front buffer like Quake II's software renderer (`fast`) |
| `DDDYN N FPS` | DDTEST loading DDRAW.DLL itself (no import) |
| `D3DTEST [hal] [spin] N FPS` | Direct3D 6, 640x480x16, RGB software device (`hal` = the card's HAL) |
| `GLTEST [wgl] [spin] N FPS` | OpenGL window 320x240, gdi32 SwapBuffers (`wgl` = opengl32 wglSwapBuffers) |
| `G2TEST [nowin] [spin] N FPS` | Glide 2 640x480 with your glide2x.dll (`nowin` needs launch mode) |
| `D3D8TEST` / `D3D9TEST [win] [sc] N FPS` | Direct3D 8 / 9 HAL, fullscreen 640x480 (`win` = windowed, `sc` = D3D9 swap-chain Present) |

The host should see N-1 frames at 1000/FPS ms each (median), 0 lost.

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

`modes` bits: 0 CRTC start-address trap, 1 3DAh vsync poll, 2 VESA 4F07h, 3 real-mode (/RM), 4 Win98 swap hook.

## What a frame is (DOS)

A write to CRTC register 0Ch or 0Dh (start address high/low) through 3D4h/3D5h, 8-bit or `OUT 3D4h,AX`.
Writes closer than 0.5 ms to the previous start-address write belong to the same flip; the frame time is
the first write of the burst.

With `/3DA` (header modes bit 1): a frame is the first vertical-retrace start (3DAh bit 3, 0 -> 1) the game reads
after it was away from 3DAh for at least 6 ms (`/3DA:ms`), i.e. one vsync wait = one frame even if it spans
several retraces. CRTC writes then only pump the UART.

## Bandwidth

`F` = 9 bytes on the wire = 90 bits: 115200 baud carries 1280 frames/s; 9600 baud carries 106.

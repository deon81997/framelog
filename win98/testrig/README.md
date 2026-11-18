# FRAMELOG Win98 test rigs

Need: `make -C win98 all tests`, priavte store-w98qi-qemu (QEMU) or store-w98qi-86box + store-86rig (86Box PII-450),
serial-bench (SB=). Win98 runs D:\GO.BAT at login (WIN.INI `run=`), the batch ends with a shutdown.

| Script | Emulator | What |
|---|---|---|
| q98.sh GO.BAT FILES... | QEMU acpi.img copy | boot, run GO.BAT, D:\ copied to /tmp/q98out; COM1 = TCP 127.0.0.1:4598 (~40 s per boot) |
| t1.sh (BAT= INI=) | QEMU | q98.sh + fllog --suite -> /tmp/t1/csv: T1 = all APIs, T3 = launch mode (+ windowless Glide), T5 = host X/S mid-game (by hand, see HANDOFF), T6 = quit FRAMELOG while 3 games run |
| r86.sh GO.BAT INI FILES... | 86Box P3B-F PII-450, Cirrus 5446 | same on 86Box, COM1 = socat pty, results /tmp/r86 (P1.BAT + p1.ini = cost: spare loops without/with FRAMELOG, ~4 min) |

test/glide2x.dll is a stand-in Glide 2 (test/fakeglide.c) for VMs without a 3dfx driver - never ship it next to G2TEST.

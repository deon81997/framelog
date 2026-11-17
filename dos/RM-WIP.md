# /RM work in progress (parked 2026-11-17)

State: FRAMELOG /RM installs QPI (JEMM + QPIEMU) V86 traps on 3D4h/3D5h (lib/qemm.c from SBEMU). QEMU e2e
`dos/testrig/e2e.sh ZIP rm 70` (rm.img, JEMMEX, JLOAD QPIEMU, RMFLIP.COM): 2282 RM trap calls for 760 flips, but only
385 flips counted. Cause (likely): an OUT 3D4h,AX from V86 arrives as ONE QPI callback (port 3D4h, AX = word, flag in
CX), and qemm.c's QEMM_TrapHandler only passes AL - the 3D5h half (data) is lost, so word-OUT frames never count.
Next: log CX/AX per RM callback (VESADBG-style dbg_out on the UART, read raw from the QEMU TCP serial), handle the word
flag in QEMM_TrapHandler (call 3D4h with AL, then 3D5h with AH), rerun e2e rm + rmsbemu, then GOT/Epic Pinball (real-mode
games in testgames) and perf86 JEMM cost.

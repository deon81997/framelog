# HANDOFF — FRAMELOG

## RESUME HERE (2026-11-17)

Repo set up; no code yet. Next: milestone 1a feasibility test.
- Home repo: deon81997/framelog (private). VMs: priavte store-dos71 (+ store-86box-irq),
  per PLAYBOOK - there is no sbemu-vms repo.
- Borrow source: NUKED-SBEMU-MUXED sc55-pack/ (hdpmipt.c/.h, qemm.h, sbemu/serial.*,
  sbemu/dbglink.c, main.c) - all present.

## Status

| Milestone | State |
|---|---|
| 1a feasibility (2 trap clients under HDPMI32i) | not started — needs VMs |
| 1b DOS TSR | not started |
| 1c Python host logger | not started |
| 1d overhead measurement | not started — needs VMs |

## Next steps

1. 1a: minimal resident TSR trapping 3D4h/3D5h alongside SBEMU; verify each
   handler runs with its own selectors/stack; SB + SC-55 still work.
   If two trap clients can't coexist: stop, fallback = build into SBEMU.

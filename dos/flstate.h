/* FRAMELOG resident state, read by FRAMELOG /STATUS and test tools.
 * Found through 0040:00F0 (segment of a 16-byte DOS block: magic, linear address of FLSTATE) + 'FL' at 0040:00F2. */
#ifndef FLSTATE_H
#define FLSTATE_H
#include <stdint.h>
#define FL_VER   "0.1"
#define FL_SIG   0x4C46
#define FL_MAGIC 0x474F4C46u   /* "FLOG" */
#define FL_RINGN 256           /* flip timestamps waiting for the UART (power of two) */
typedef struct {
    uint32_t magic;
    uint32_t base, baud, tsc_hz, merge_ticks;
    volatile uint32_t logging, starts;
    volatile uint32_t traps, flips, frames, lost, sent, txlost, rx;
    uint32_t mode3da, arm_ticks; volatile uint32_t traps3da;
    uint32_t rm_res[2];                         /* reserved (/RM, wip/rm-2026-11-17) */
    uint32_t vesa; volatile uint32_t vesa_calls;   /* /NOVESA clears vesa; display-start calls (INT 10h 4F07h + VBE PM thunk) */
    volatile uint32_t vbepm, vbepm_skip;            /* INT 10h 4F0Ah (PM interface) requests, tables not patched */
    uint32_t meta; volatile uint32_t metas, modesets;   /* /NOMETA clears meta; M blocks sent; INT 10h mode sets seen */
    uint32_t glide; volatile uint32_t glide_opens, glide_patched, glide_swaps, glide_err, glide_dllfix,
             glide_stscan, glide_static, glide_sterr, glide_stus;   /* DOS Glide hook (glide/glhook.c, glstatic.c) */
    uint32_t bank; volatile uint32_t bank_frames;   /* /BANK: frames = window-A bank wraps (INT 10h 4F05h, vesastub.asm) */
} FLSTATE;
#endif

/* shared stats block between FLTSR (writer, inside the trap) and FLTEST (reader) */
#ifndef FLSTATS_H
#define FLSTATS_H
#include <stdint.h>
#include <stddef.h>
#define FL_SIG  0x4C46  /* 'FL' at 0040:00F2, segment at 0040:00F0 */
#define FL_RING 64
#define FL_RAW  32
typedef struct {
    uint32_t magic, installed;
    uint32_t own_ds, own_ss;            /* TSR's selectors at install time */
    uint32_t calls, ins, wide, flips;   /* trap calls, IN calls, calls with value > FFh, start-address writes */
    uint32_t bad_ds, bad_ss;            /* calls where DS/SS were not ours */
    uint32_t last_ds, last_ss, last_esp, max_depth;
    uint32_t rawn;
    uint32_t raw[FL_RAW * 2];           /* port | 10000h out | 20000h val>FFh, value */
    uint32_t ring[FL_RING];             /* low 32 bits of RDTSC per flip */
} FLSTATS;
#endif

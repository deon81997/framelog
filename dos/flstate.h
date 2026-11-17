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
} FLSTATE;
#endif

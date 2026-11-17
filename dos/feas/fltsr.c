/* FLTSR - milestone 1a feasibility TSR for FRAMELOG.
 * Goes resident under HDPMI32i (same model as SBEMU) and traps CRTC 3D4h/3D5h with HDPMI's port-trap API,
 * while SBEMU may be resident with its own traps. Every access is passed through untrapped; the handler
 * only counts and records what it sees, so FLTEST.EXE can check:
 *   - the handler runs with OUR data selector and OUR stack (not the game's, not SBEMU's)
 *   - 8-bit and 16-bit OUT 3D4h forms, start-address (0Ch/0Dh) writes = flips
 *   - RDTSC timestamp per flip
 * Stats block: conventional memory, segment in the BIOS intra-app area 0040:00F0 (+ 'FL' signature at F2h).
 * GPL-2.0-or-later (uses SBEMU's hdpmipt.c / dpmi lib). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include "dpmi/dpmi.h"
#include "hdpmipt.h"
#include "flstats.h"

static FLSTATS *S;          /* in our own DS: the handler writes here; copied nowhere (see below) */
static uint32_t S_lin;      /* linear address of the DOS block (= what FLTEST reads) */
static uint32_t ownDS, ownSS;
static uint8_t crtc_index;  /* last index written to 3D4h */
static QEMM_IOPT iopt;
extern volatile uint32_t HDPMIPT_Depth;

static inline uint64_t rdtsc(void) { uint32_t lo, hi; asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }

/* our block lives in DOS memory; we reach it through _dos_ds (our own selector table, valid in every trap) */
#define W32(field, v) _farpokel(_dos_ds, S_lin + offsetof(FLSTATS, field), (v))
#define R32(field)    _farpeekl(_dos_ds, S_lin + offsetof(FLSTATS, field))

static void note_flip(void)
{
    uint64_t t = rdtsc();
    uint32_t n = R32(flips);
    _farpokel(_dos_ds, S_lin + offsetof(FLSTATS, ring) + (n & (FL_RING - 1)) * 4, (uint32_t)t);
    W32(flips, n + 1);
}

static uint32_t crtc_trap(uint32_t port, uint32_t val, uint32_t out)
{
    uint16_t ds, ss; uint32_t esp;
    asm volatile("mov %%ds,%0; mov %%ss,%1; mov %%esp,%2" : "=r"(ds), "=r"(ss), "=r"(esp));
    W32(calls, R32(calls) + 1);
    if (ds != ownDS) W32(bad_ds, R32(bad_ds) + 1);
    if (ss != ownSS) W32(bad_ss, R32(bad_ss) + 1);
    W32(last_ds, ds); W32(last_ss, ss); W32(last_esp, esp);
    if (HDPMIPT_Depth > R32(max_depth)) W32(max_depth, HDPMIPT_Depth);
    {   /* record the raw call */
        uint32_t k = R32(rawn);
        uint32_t a = S_lin + offsetof(FLSTATS, raw) + (k % FL_RAW) * 8;
        _farpokel(_dos_ds, a, port | (out ? 0x10000 : 0) | ((out & 0xFF) << 24));
        _farpokel(_dos_ds, a + 4, val);
        W32(rawn, k + 1);
    }
    if (!out) {
        W32(ins, R32(ins) + 1);
        return HDPMIPT_UntrappedIO_Read((uint16_t)port);
    }
    /* HDPMI hands OUT DX,AX to us as two byte calls (3D4h = AL, then 3D5h = AH); a byte call's EAX keeps
     * stale upper bits, so only AL is the value (measured in 1a: "raw out 3D4 340C" for OUTB 3D4h,0Ch) */
    /* ...and the 3D5h half of a word OUT is taken from what we RETURN (shifted right 8): return EAX unchanged */
    uint32_t eax = val;
    if (val > 0xFF) W32(wide, R32(wide) + 1);
    val &= 0xFF;
    HDPMIPT_UntrappedIO_Write((uint16_t)port, (uint8_t)val);
    if (port == 0x3D4) crtc_index = (uint8_t)val;
    else if (crtc_index == 0x0C || crtc_index == 0x0D) note_flip();
    return eax;
}

static QEMM_IODT iodt[2] = { {0x3D4, crtc_trap}, {0x3D5, crtc_trap} };

int main(void)
{
    if (_farpeekw(_dos_ds, 0x4F2) == FL_SIG) { printf("FLTSR: already resident (seg %04X)\n", _farpeekw(_dos_ds, 0x4F0)); return 1; }
    DPMI_Init();
    if (!HDPMIPT_Detect()) { printf("FLTSR: HDPMI32i not found\n"); return 2; }
    int sel; int seg = __dpmi_allocate_dos_memory((sizeof(FLSTATS) + 15) / 16, &sel);
    if (seg < 0) { printf("FLTSR: no DOS memory\n"); return 3; }
    S_lin = (uint32_t)seg << 4;
    for (unsigned i = 0; i < sizeof(FLSTATS); i += 4) _farpokel(_dos_ds, S_lin + i, 0);
    asm volatile("mov %%ds,%0; mov %%ss,%1" : "=r"(ownDS), "=r"(ownSS));
    /* the trap wrapper switches to its own stack, allocated from our heap with SS = our DS */
    W32(own_ds, ownDS); W32(own_ss, ownSS); W32(magic, 0x474F4C46);
    if (!HDPMIPT_Install_IOPortTrap(0x3D4, 0x3D5, iodt, 2, &iopt)) { printf("FLTSR: port trap failed\n"); return 4; }
    W32(installed, 1);
    _farpokew(_dos_ds, 0x4F0, seg); _farpokew(_dos_ds, 0x4F2, FL_SIG);
    printf("FLTSR: resident, traps 3D4h-3D5h, block %04X, DS %04X\n", seg, (unsigned)ownDS);
    fflush(stdout);
    (void)S;
    DPMI_TSR();
    printf("FLTSR: going resident failed\n");
    return 5;
}

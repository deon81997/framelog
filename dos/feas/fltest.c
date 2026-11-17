/* FLTEST - milestone 1a checker: drives CRTC 3D4h/3D5h like a game flipping pages and checks FLTSR's stats,
 * then checks SBEMU's Sound Blaster still answers (DSP reset + version) from the same program.
 *   FLTEST [N]   N flips per form (default 1000). Prints PASS/FAIL lines; exit 0 = all PASS.
 * GPL-2.0-or-later. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pc.h>
#include <go32.h>
#include <sys/farptr.h>
#include "flstats.h"

static uint32_t B;
static inline uint64_t rdtsc(void) { uint32_t lo, hi; asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }
static int fails;
#define R(f) _farpeekl(_dos_ds, B + offsetof(FLSTATS, f))
static void chk(int ok, const char *what) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", what); if (!ok) fails++; }

static int sb_check(unsigned base)
{   /* DSP reset -> AAh, then E1h = version */
    outportb(base + 6, 1); for (volatile int i = 0; i < 1000; i++) inportb(0x80); outportb(base + 6, 0);
    int t; for (t = 0; t < 100000 && !(inportb(base + 0xE) & 0x80); t++) ;
    if (inportb(base + 0xA) != 0xAA) return -1;
    for (t = 0; t < 100000 && (inportb(base + 0xC) & 0x80); t++) ;
    outportb(base + 0xC, 0xE1);
    for (t = 0; t < 100000 && !(inportb(base + 0xE) & 0x80); t++) ;
    int hi = inportb(base + 0xA);
    for (t = 0; t < 100000 && !(inportb(base + 0xE) & 0x80); t++) ;
    int lo = inportb(base + 0xA);
    return hi << 8 | lo;
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 1000;
    uint16_t myds; asm volatile("mov %%ds,%0" : "=r"(myds));
    if (_farpeekw(_dos_ds, 0x4F2) != FL_SIG) { printf("FLTEST: FLTSR not resident\nFAIL\n"); return 1; }
    B = (uint32_t)_farpeekw(_dos_ds, 0x4F0) << 4;
    printf("FLTEST: block %05lX, TSR DS %04lX SS %04lX, my DS %04X\n", (unsigned long)B, (unsigned long)R(own_ds), (unsigned long)R(own_ss), myds);
    chk(R(magic) == 0x474F4C46 && R(installed) == 1, "stats block valid");

    uint32_t c0 = R(calls), f0 = R(flips);
    /* passthrough, 8-bit form: write start address high, read it back */
    outportb(0x3D4, 0x0C); outportb(0x3D5, 0x12);
    outportb(0x3D4, 0x0C); int v1 = inportb(0x3D5);
    chk(v1 == 0x12, "8-bit write reaches the VGA (read back 12h)");
    chk(R(calls) - c0 == 4, "4 trap calls for 2 OUT index + OUT data + IN data");
    /* 16-bit form */
    uint32_t c1 = R(calls);
    outportw(0x3D4, 0x340C);
    outportb(0x3D4, 0x0C); int v2 = inportb(0x3D5);
    printf("        OUT 3D4h,AX -> %lu trap call(s)\n", (unsigned long)(R(calls) - c1 - 2));
    chk(v2 == 0x34, "16-bit OUT 3D4h,AX reaches the VGA (read back 34h)");
    outportw(0x3D4, 0x000C); outportw(0x3D4, 0x000D);
    chk(R(flips) - f0 == 4, "flip count after 2 byte + 2 word start-address writes = 4");

    /* raw records of the last few calls */
    uint32_t k = R(rawn);
    for (uint32_t i = (k > 8 ? k - 8 : 0); i < k; i++) {
        uint32_t a = B + offsetof(FLSTATS, raw) + (i % FL_RAW) * 8;
        uint32_t pv = _farpeekl(_dos_ds, a), v = _farpeekl(_dos_ds, a + 4);
        printf("        raw %s %03lX val %08lX ecx.lo %02lX\n", pv & 0x10000 ? "out" : "in ", (unsigned long)(pv & 0xFFFF), (unsigned long)v, (unsigned long)(pv >> 24));
    }

    /* flips, both forms */
    uint32_t f1 = R(flips);
    uint64_t u0 = rdtsc(); /* never uclock(): it reprograms the PIT, which SBEMU watches */
    for (int i = 0; i < n; i++) { outportb(0x3D4, 0x0C); outportb(0x3D5, i >> 8); outportb(0x3D4, 0x0D); outportb(0x3D5, i); }
    for (int i = 0; i < n; i++) { outportw(0x3D4, (i & 0xFF00) | 0x0C); outportw(0x3D4, (i << 8) | 0x0D); }
    uint64_t u1 = rdtsc();
    chk(R(flips) - f1 == (uint32_t)(4 * n), "every start-address write counted");
    printf("        %d trapped CRTC writes: %lu TSC ticks each\n", 6 * n, (unsigned long)((u1 - u0) / (6 * n)));
    uint32_t t1 = _farpeekl(_dos_ds, B + offsetof(FLSTATS, ring) + ((R(flips) - 1) & (FL_RING - 1)) * 4);
    uint32_t t0 = _farpeekl(_dos_ds, B + offsetof(FLSTATS, ring) + ((R(flips) - 2) & (FL_RING - 1)) * 4);
    chk(t1 != t0, "TSC timestamps advance");
    outportw(0x3D4, 0x000C); outportw(0x3D4, 0x000D);

    printf("        last DS %04lX SS %04lX ESP %08lX, max depth %lu, calls %lu\n", (unsigned long)R(last_ds), (unsigned long)R(last_ss), (unsigned long)R(last_esp), (unsigned long)R(max_depth), (unsigned long)R(calls));
    chk(R(bad_ds) == 0, "handler always ran with FLTSR's own DS");
    chk(R(bad_ss) == 0, "handler always ran on FLTSR's own stack");
    chk(R(own_ds) != myds, "FLTSR's DS differs from this program's DS (separate client)");

    int ver = sb_check(0x220);
    printf("        SB at 220h: DSP %s %d.%02d\n", ver < 0 ? "no answer" : "version", ver < 0 ? 0 : ver >> 8, ver < 0 ? 0 : ver & 0xFF);
    if (argc > 2) chk(ver >= 0x200, "SBEMU's DSP answers (reset AAh + version)");
    printf(fails ? "FAIL\n" : "PASS\n");
    return fails ? 1 : 0;
}

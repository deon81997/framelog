/* GLFLIP.EXE [N] [FPS] [interval] - DOS Glide 2 test (OpenWatcom, DOS/4GW): loads GLIDE2X.OVL from the current
 * directory with glload.c (stand-in for the Tenberry DLL loader that DOS Glide games link), grSstWinOpen 640x480,
 * N frames of grBufferClear + grBufferSwap(interval) paced to FPS (0 = as fast as possible), spare-CPU loops counted
 * while it waits (FRAMELOG cost = fewer loops). Prints the first bytes of grBufferSwap (FF 15 = FRAMELOG's patch)
 * and appends the result line to GLFLIP.TXT. GLOVL=file loads another file name (not hooked: A/B cost runs).
 * Build: dos/glide/mk.sh. */
#include <stdio.h>
#include <stdlib.h>
#include "glload.h"

typedef void (__stdcall *V0)(void);
typedef int (__stdcall *I1)(void *);
typedef void (__stdcall *V1)(int);
typedef void (__stdcall *V1p)(char *);
typedef void (__stdcall *V3)(unsigned long, unsigned long, unsigned long);
typedef int (__stdcall *I7)(unsigned long, int, int, int, int, int, int);

unsigned __int64 rdtsc(void);
#pragma aux rdtsc = ".586" "rdtsc" value [edx eax];

static unsigned __int64 tsc_hz(void)
{
    volatile unsigned long *ticks = (volatile unsigned long *)0x46C;
    unsigned long t0 = *ticks, t;
    unsigned __int64 a, b;
    while ((t = *ticks) == t0) ;
    a = rdtsc();
    while (*ticks - t < 5) ;
    b = rdtsc();
    return (b - a) * 1193182 / (5 * 65536);
}

static void dump(const char *when, unsigned char *p)
{
    printf("GLFLIP: %s grBufferSwap at %p: %02X %02X %02X %02X %02X %02X %02X %02X\n", when, p, p[0], p[1], p[2], p[3],
           p[4], p[5], p[6], p[7]);
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 600, fps = argc > 2 ? atoi(argv[2]) : 0, interval = argc > 3 ? atoi(argv[3]) : 1, i;
    unsigned long hw[64], spare = 0;
    unsigned __int64 hz, t0, next, step, t1;
    char ver[80] = "?", err[80];
    static GLDLL g;
    V0 init, shut; I1 qh; V1 sel, swap; V1p getver; V3 clr; I7 open;
    unsigned char *p;
    FILE *f;
    const char *ovl = getenv("GLOVL") ? getenv("GLOVL") : "GLIDE2X.OVL";   /* another name = FRAMELOG doesn't hook it */
    if (!gl_load(ovl, &g, err)) { printf("GLFLIP: load %s: %s\n", ovl, err); return 1; }
    init = (V0)gl_sym(&g, "_grGlideInit@0"); shut = (V0)gl_sym(&g, "_grGlideShutdown@0");
    qh = (I1)gl_sym(&g, "_grSstQueryHardware@4"); sel = (V1)gl_sym(&g, "_grSstSelect@4");
    open = (I7)gl_sym(&g, "_grSstWinOpen@28"); clr = (V3)gl_sym(&g, "_grBufferClear@12");
    swap = (V1)gl_sym(&g, "_grBufferSwap@4"); getver = (V1p)gl_sym(&g, "_grGlideGetVersion@4");
    if (!init || !shut || !qh || !sel || !open || !clr || !swap || !getver) { puts("GLFLIP: missing exports"); return 1; }
    p = (unsigned char *)swap;
    dump("loaded", p);
    if (getenv("GLINIT")) printf("GLFLIP: DLL start returned %d\n", gl_start(&g));   /* aborts under HDPMI32i, see glload.c */
    hz = tsc_hz();
    init();
    getver(ver);
    if (!qh(hw)) { puts("GLFLIP: no 3dfx hardware"); return 1; }
    sel(0);
    if (!open(0, 7 /* 640x480 */, 0 /* 60 Hz */, 0, 0, 2, 1)) { puts("GLFLIP: grSstWinOpen failed"); shut(); return 1; }
    step = fps ? hz / fps : 0;
    t0 = next = rdtsc();
    for (i = 0; i < n; i++) {
        clr(0x00FF0000UL + ((i * 4) & 255), 0, 0xFFFF);
        swap(interval);
        if (step) { next += step; while (rdtsc() < next) spare++; }
    }
    t1 = rdtsc();
    shut();
    dump("end", p);
    f = fopen("GLFLIP.TXT", "a");
    for (i = 0; i < 2; i++) {
        FILE *o = i ? f : stdout;
        if (!o) continue;
        fprintf(o, "GLFLIP %s %s frames %d fps*100 %lu spare %lu tscMHz %lu patch %02X%02X\n", ovl, ver, n,
                (unsigned long)((unsigned __int64)n * 100 * hz / (t1 - t0)), spare, (unsigned long)(hz / 1000000), p[0], p[1]);
    }
    if (f) fclose(f);
    return 0;
}

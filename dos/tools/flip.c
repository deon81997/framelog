/* FLIP.EXE - test "game" for FRAMELOG: flips the CRTC start address at a fixed rate, timed by RDTSC (never the PIT).
 *   FLIP SECONDS FPS [LONG]   every LONG-th frame (default 10) takes twice as long; alternates 8-bit and OUT 3D4h,AX.
 * Prints the frame count and the TSC rate it measured. GPL-2.0-or-later. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pc.h>
#include <go32.h>
#include <sys/farptr.h>
static inline uint64_t rdtsc(void) { uint32_t lo, hi; asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }
int main(int argc, char **argv)
{
    if (argc < 3) { puts("FLIP SECONDS FPS [LONG]"); return 1; }
    double secs = atof(argv[1]), fps = atof(argv[2]); int lng = argc > 3 ? atoi(argv[3]) : 10;
    uint32_t t0 = _farpeekl(_dos_ds, 0x46C), t;
    while ((t = _farpeekl(_dos_ds, 0x46C)) == t0) ;
    uint64_t a = rdtsc(); while (_farpeekl(_dos_ds, 0x46C) - t < 10) ; uint64_t b = rdtsc();
    double hz = (double)(b - a) * 1193182.0 / (10.0 * 65536.0);
    uint64_t per = (uint64_t)(hz / fps), next = rdtsc(), end = next + (uint64_t)(secs * hz);
    unsigned n = 0;
    while (next < end) {
        while (rdtsc() < next) ;
        unsigned sa = (n & 1) ? 0x0000 : 0x0050;    /* two text rows apart */
        if (n & 2) { outportw(0x3D4, (sa & 0xFF00) | 0x0C); outportw(0x3D4, ((sa & 0xFF) << 8) | 0x0D); }
        else { outportb(0x3D4, 0x0C); outportb(0x3D5, sa >> 8); outportb(0x3D4, 0x0D); outportb(0x3D5, sa & 0xFF); }
        n++;
        next += (lng && n % lng == 0) ? 2 * per : per;
    }
    outportw(0x3D4, 0x000C); outportw(0x3D4, 0x000D);
    printf("FLIP: %u frames, %.0f fps, every %d. frame long, TSC %.0f Hz\n", n, fps, lng, hz);
    return 0;
}

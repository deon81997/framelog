/* FLIP.EXE - test "game" for FRAMELOG: flips the CRTC start address at a fixed rate, timed by RDTSC (never the PIT).
 *   FLIP SECONDS FPS [LONG] [M]   every LONG-th frame (default 10, 0 = none) takes twice as long; alternates 8-bit and
 *   OUT 3D4h,AX. M = also play MIDI on the MPU-401 at 330h (UART mode), a 4-note chord change every 8th frame, like a
 *   game's music (keeps the SC-55 busy for overhead measurements). V = copy-to-screen game: no page flips, each
 *   frame "renders" for half a frame time (RDTSC spin) and then waits for round(70/FPS) vertical retraces on 3DAh
 *   (text mode = 70 Hz) like a vsync'd game; LONG frames wait twice as many. B = VESA game: VBE mode 101h (640x480x8)
 *   and flips with INT 10h AX=4F07h (BL 00h / 80h alternating) between lines 0 and 480 (needs a VBE BIOS). K = banked
 *   VESA game: mode 101h, no flips, each frame "copied" as 5 window-A banks (INT 10h 4F05h, 64 KB stores each; FRAMELOG /BANK).
 * Prints the frame count and the TSC rate it measured. GPL-2.0-or-later. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pc.h>
#include <string.h>
#include <go32.h>
#include <sys/farptr.h>
#include <dpmi.h>
static void mpu(int b) { for (int t = 0; t < 100000 && (inportb(0x331) & 0x40); t++) ; outportb(0x330, b); }
static inline uint64_t rdtsc(void) { uint32_t lo, hi; asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }
int main(int argc, char **argv)
{
    if (argc < 3) { puts("FLIP SECONDS FPS [LONG]"); return 1; }
    double secs = atof(argv[1]), fps = atof(argv[2]); int lng = argc > 3 ? atoi(argv[3]) : 10, midi = argc > 4 && strpbrk(argv[4], "Mm"), vs = argc > 4 && strpbrk(argv[4], "Vv"), vb = argc > 4 && strpbrk(argv[4], "Bb"), vk = argc > 4 && strpbrk(argv[4], "Kk");
    __dpmi_regs r;
    if (vk) vb = 2;
    if (vb) { memset(&r, 0, sizeof r); r.x.ax = 0x4F02; r.x.bx = 0x101; __dpmi_int(0x10, &r);
              if (r.x.ax != 0x004F) { printf("FLIP: VBE mode 101h failed (AX=%04X)\n", r.x.ax); return 2; } }
    int rpf = (int)(70.0 / fps + 0.5); if (rpf < 1) rpf = 1;
    static const int prog[4] = { 0, 48, 33, 73 }, base[4] = { 60, 64, 43, 72 }; int on[4] = { 0 };
    if (midi) { outportb(0x331, 0x3F); for (int c = 0; c < 4; c++) { mpu(0xC0 | c); mpu(prog[c]); } }
    uint32_t t0 = _farpeekl(_dos_ds, 0x46C), t;
    while ((t = _farpeekl(_dos_ds, 0x46C)) == t0) ;
    uint64_t a = rdtsc(); while (_farpeekl(_dos_ds, 0x46C) - t < 10) ; uint64_t b = rdtsc();
    double hz = (double)(b - a) * 1193182.0 / (10.0 * 65536.0);
    uint64_t per = (uint64_t)(hz / fps), next = rdtsc(), end = next + (uint64_t)(secs * hz);
    unsigned n = 0; uint64_t spins = 0;   /* spare CPU: loop passes while waiting for the next flip */
    while (next < end) {
        if (vs) {   /* wait for the end of a retrace, then for the next one to start, rpf (x2 for long) times */
            int k = (lng && (n + 1) % lng == 0) ? 2 * rpf : rpf;
            while (k--) { while (inportb(0x3DA) & 8) spins++; while (!(inportb(0x3DA) & 8)) spins++; }
            uint64_t w = rdtsc() + (uint64_t)(hz / fps / 2);   /* "render": half a frame away from 3DAh */
            while (rdtsc() < w) spins++;
            next = rdtsc();
        } else if (vb == 2) {
            while (rdtsc() < next) spins++;
            for (int k = 0; k < 5; k++) {
                memset(&r, 0, sizeof r); r.x.ax = 0x4F05; r.x.dx = k; __dpmi_int(0x10, &r);
                for (int o = 0; o < 65536; o += 4096) _farpokel(_dos_ds, 0xA0000 + o, n * 0x01010101u);
            }
        } else if (vb) {
            while (rdtsc() < next) spins++;
            memset(&r, 0, sizeof r); r.x.ax = 0x4F07; r.x.bx = (n & 2) ? 0x80 : 0x00; r.x.cx = 0; r.x.dx = (n & 1) ? 0 : 480;
            __dpmi_int(0x10, &r);
        } else {
        while (rdtsc() < next) spins++;
        unsigned sa = (n & 1) ? 0x0000 : 0x0050;    /* two text rows apart */
        if (n & 2) { outportw(0x3D4, (sa & 0xFF00) | 0x0C); outportw(0x3D4, ((sa & 0xFF) << 8) | 0x0D); }
        else { outportb(0x3D4, 0x0C); outportb(0x3D5, sa >> 8); outportb(0x3D4, 0x0D); outportb(0x3D5, sa & 0xFF); }
        }
        if (midi && n % 8 == 0) for (int c = 0; c < 4; c++) {
            if (on[c]) { mpu(0x80 | c); mpu(on[c]); mpu(0); }
            on[c] = base[c] + ((n / 8) * 5 + c * 3) % 12; mpu(0x90 | c); mpu(on[c]); mpu(90);
        }
        n++;
        if (!vs) next += (lng && n % lng == 0) ? 2 * per : per;
    }
    if (midi) for (int c = 0; c < 4; c++) if (on[c]) { mpu(0x80 | c); mpu(on[c]); mpu(0); }
    if (vb) { memset(&r, 0, sizeof r); r.x.ax = 0x0003; __dpmi_int(0x10, &r); }
    else { outportw(0x3D4, 0x000C); outportw(0x3D4, 0x000D); }
    printf("FLIP: %u frames, %.0f fps, every %d. frame long, TSC %.0f Hz, spare loops %llu\n", n, fps, lng, hz, (unsigned long long)spins);
    return 0;
}

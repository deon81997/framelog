/* metatest.c - host test of dos/flmeta.h (the TSR's 'M' record builder + encoder); run by metatest.sh, which compares
 * the bytes with serial-bench flproto.encode_meta. Output: name TAB text ('|' = newline) TAB records as hex. */
#include <stdio.h>
#include <string.h>
#include "../../flmeta.h"

static void out(const char *name, const char *t, int tl)
{
    uint8_t r[FLM_MAXREC];
    int n = flm_encode(t, tl, r, sizeof r);
    printf("%s\t", name);
    for (int i = 0; i < tl; i++) putchar(t[i] == '\n' ? '|' : t[i]);
    putchar('\t');
    for (int i = 0; i < n; i++) printf("%02x", r[i]);
    putchar('\n');
}

/* the same composition as framelog.c meta_init + meta_build */
static void ccase(const char *name, int vbe, uint16_t ver, const char *oem, const char *ven, const char *prod,
                  uint16_t orev, const char *prev, const char *brand, const char *vendor, int fam, int model, int step,
                  uint32_t mhz, int vmode, int bm, const FLM_MODE *vm, const uint8_t *crtc, int unch)
{
    char gpu[160], drv[96], cpu[96], t[FLM_TEXT], nm[16];
    flm_gpu(gpu, sizeof gpu, vbe, ver, oem, ven, prod);
    flm_driver(drv, sizeof drv, vbe, ver, orev, prev);
    flm_cpu(cpu, sizeof cpu, brand, vendor, fam, model, step, mhz);
    int tl = flm_add(t, 0, FLM_TEXT, "gpu", gpu);
    tl = flm_add(t, tl, FLM_TEXT, "driver", drv);
    tl = flm_add(t, tl, FLM_TEXT, "cpu", cpu);
    FLM_MODE m = {0, 0, 0, 0};
    int ok;
    if (vmode >= 0) { ok = vm != 0; if (vm) m = *vm; }
    else { ok = flm_vga_mode(bm, &m); if (bm == 0x13) flm_vga256(crtc[0], crtc[1], crtc[2], crtc[3], &m); }
    flm_name(nm, vmode, bm, unch, m.text);
    tl = flm_mode_text(t, tl, FLM_TEXT, ok ? &m : 0, nm);
    out(name, t, tl);
}

int main(void)
{
    char line[1024];
    while (fgets(line, sizeof line, stdin)) {          /* raw texts: "name TAB text" ('|' = newline) */
        char *tab = strchr(line, '\t'), t[FLM_TEXT];
        if (!tab) continue;
        *tab++ = 0;
        int tl = 0;
        for (; *tab && *tab != '\n' && tl < FLM_TEXT - 1; tab++) t[tl++] = *tab == '|' ? '\n' : *tab;
        out(line, t, tl);
    }
    static const uint8_t m13[4] = {0x4F, 0x1F, 0x41, 0x8F}, mx240[4] = {0x4F, 0x3E, 0x41, 0xDF}, mx360[4] = {0x59, 0x1F, 0x41, 0x8F};
    FLM_MODE v111 = {640, 480, 16, 0}, v101 = {640, 480, 8, 0};
    ccase("d810", 1, 0x300, "ATI MOBILITY RADEON X300", "ATI Technologies Inc.", "M22", 0x0100, "01.00",
          "      Intel(R) Pentium(R) M processor 2.00GHz", "GenuineIntel", 6, 13, 8, 1995, 0x111, 0, &v111, 0, 0);
    ccase("nvidia", 1, 0x300, "NVIDIA", "NVIDIA Corporation", "NV41 Board - p226h1  ", 0x0100, "Chip Rev   A2",
          "", "GenuineIntel", 15, 2, 9, 2400, 0x101, 0, &v101, 0, 0);
    ccase("pii_vga13", 0, 0, "", "", "", 0, "", "", "GenuineIntel", 6, 5, 2, 450, -1, 0x13, 0, m13, 0);
    ccase("modex240", 1, 0x102, "S3 Incorporated. Trio64", "", "", 0, "", "", "GenuineIntel", 6, 5, 2, 450, -1, 0x13, 0, mx240, 1);
    ccase("modex360", 1, 0x200, "3dfx Interactive, Inc.", "3dfx Interactive, Inc.", "Voodoo3", 0x0100, "", "", "AuthenticAMD", 6, 4, 2, 1000, -1, 0x13, 0, mx360, 1);
    ccase("text03", 1, 0x200, "Matrox", "Matrox Graphics Inc.", "G400", 0x0105, "Rev 1", "AMD Athlon(tm) Processor", "AuthenticAMD", 6, 4, 2, 1000, -1, 0x03, 0, 0, 0);
    ccase("vga12", 0, 0, "", "", "", 0, "", "", "", 0, 0, 0, 0, -1, 0x12, 0, 0, 0);
    ccase("vesa_unknown", 1, 0x200, "x\x01y\x7f  z", "", "", 0, "", "", "CyrixInstead", 5, 2, 0, 166, 0x1FF, 0, 0, 0, 0);
    return 0;
}

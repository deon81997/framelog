/* gstest.c - host test of glstatic.c (static Glide grBufferSwap signature + PE/LE checks).
 *   cc -O2 -I.. -o gstest gstest.c && ./gstest            synthetic cases (reloc / fixup / two hits / none)
 *   ./gstest FILE[=OFFSET|=none]...                        real files: found offset (or result) must match
 * Expected for the store-dosglide-demos files: see gstest.sh. */
#define GS_HOST
#include "../glstatic.c"
#include <stdio.h>
#include <stdlib.h>

struct mem { const uint8_t *d; uint32_t n; };
static int mrd(void *c, uint32_t off, void *dst, uint32_t n)
{
    struct mem *m = c;
    if (off > m->n || n > m->n - off) return 0;
    memcpy(dst, m->d + off, n); return 1;
}
static int fails;
#define CHECK(x, ...) do { if (!(x)) { printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); puts(""); fails++; } } while (0)

static void put32(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static void put16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }

/* the swap function as in Glide 2.11-2.42 (pro ... fence ... epi) */
static uint32_t putfn(uint8_t *d, uint32_t at)
{
    static const uint8_t f[] = { 0x53, 0x56, 0x57, 0x55, 0x8B, 0x5C, 0x24, 0x14,
        0x8B, 0x0D, 1, 2, 3, 4, 0x09, 0xCB,
        0x87, 0x05, 0x80, 0xEE, 0x50, 0x00, 0x89, 0x9E, 0x28, 0x01, 0x00, 0x00, 0x87, 0x05, 0x80, 0xEE, 0x50, 0x00,
        0xEB, 0x08, 0x09, 0xCB, 0x89, 0x9E, 0x28, 0x01, 0x00, 0x00, 0x5D, 0x5F, 0x5E, 0x5B, 0xC2, 0x04, 0x00 };
    memcpy(d + at, f, sizeof f); return at;
}

/* tiny PE: MZ, PE at 0x80, one code section raw 0x200..0x6200 at rva 0x1000, .reloc raw 0x6200 at rva 0x7000 */
static uint8_t *mkpe(uint32_t *n, uint32_t fnat, int relocat)
{
    uint8_t *d = calloc(1, *n = 0x6400);
    d[0] = 'M'; d[1] = 'Z'; put32(d + 0x3C, 0x80);
    uint8_t *pe = d + 0x80; memcpy(pe, "PE\0\0", 4); put16(pe + 6, 2); put16(pe + 20, 224);
    uint8_t *opt = pe + 24; put16(opt, 0x10B); put32(opt + 96 + 40, 0x7000); put32(opt + 96 + 44, relocat >= 0 ? 12 : 0);
    uint8_t *s = opt + 224; put32(s + 12, 0x1000); put32(s + 16, 0x6000); put32(s + 20, 0x200);
    s += 40; put32(s + 12, 0x7000); put32(s + 16, 0x200); put32(s + 20, 0x6200);
    putfn(d, 0x200 + fnat);
    if (relocat >= 0) { uint8_t *r = d + 0x6200; uint32_t rva = 0x1000 + relocat; put32(r, rva & ~0xFFF); put32(r + 4, 12);
                        put16(r + 8, 0x3000 | (rva & 0xFFF)); put16(r + 10, 0); }
    return d;
}

static void synth(void)
{
    uint32_t n, fn; uint8_t code[GS_PRO];
    struct mem m;
    uint8_t *d = mkpe(&n, 0x1234, 0x10);                     /* reloc far away */
    m.d = d; m.n = n;
    CHECK(gs_probe(mrd, &m, n, &fn, code) == 0 && fn == 0x200 + 0x1234 && !memcmp(code, gs_pro, GS_PRO), "pe ok %x", fn);
    free(d);
    d = mkpe(&n, 0x1234, 0x1234 + 4); m.d = d;               /* reloc inside the prologue */
    CHECK(gs_probe(mrd, &m, n, &fn, code) == GS_FIX, "pe reloc in prologue");
    free(d);
    d = mkpe(&n, 0x1234, 0x1234 - 3); m.d = d;               /* reloc spilling into byte 0 */
    CHECK(gs_probe(mrd, &m, n, &fn, code) == GS_FIX, "pe reloc spill");
    free(d);
    d = mkpe(&n, 0x1234, -1); m.d = d;                       /* no relocs */
    CHECK(gs_probe(mrd, &m, n, &fn, code) == 0, "pe no relocs");
    putfn(d, 0x200 + 0x3000);                                /* second swap function */
    CHECK(gs_probe(mrd, &m, n, &fn, code) == GS_MANY, "two hits");
    free(d);
    /* window edges: function straddling every block boundary */
    for (uint32_t at = GS_BLK - 0x40; at < GS_BLK + 0x20; at++) {
        d = mkpe(&n, at - 0x200 + 0x200, -1); m.d = d;
        int e = gs_probe(mrd, &m, n, &fn, code);
        CHECK(e == 0 && fn == 0x200 + at, "edge %x e=%d fn=%x", at, e, fn);
        free(d);
    }
    d = mkpe(&n, 0x1234, -1); d[0x200 + 0x1234 + 0x2C] = 0x90; m.d = d;   /* epilogue broken */
    CHECK(gs_probe(mrd, &m, n, &fn, code) == GS_NONE, "no epilogue");
    free(d);
    static uint8_t mzonly[0x400] = { 'M', 'Z' }; m.d = mzonly; m.n = sizeof mzonly;
    CHECK(gs_probe(mrd, &m, m.n, &fn, code) == GS_FMT, "real-mode exe");
}

int main(int argc, char **argv)
{
    synth();
    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq) *eq++ = 0;
        FILE *f = fopen(argv[i], "rb");
        if (!f) { printf("FAIL open %s\n", argv[i]); fails++; continue; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
        uint8_t *d = malloc(sz); if (fread(d, 1, sz, f) != (size_t)sz) sz = 0; fclose(f);
        struct mem m = { d, (uint32_t)sz };
        uint32_t fn = 0; uint8_t code[GS_PRO];
        int e = gs_probe(mrd, &m, m.n, &fn, code);
        char got[32];
        if (e) snprintf(got, sizeof got, "%s", e == GS_NONE ? "none" : e == GS_FMT ? "fmt" : e == GS_MANY ? "many" : e == GS_FIX ? "fix" : "io");
        else snprintf(got, sizeof got, "0x%x", fn);
        int ok = !eq || !strcmp(eq, got) || (!strcmp(eq, "none") && !strcmp(got, "fmt"));
        printf("%s %s %s%s%s\n", ok ? "ok  " : "FAIL", argv[i], got, eq ? " want " : "", eq ? eq : "");
        if (!ok) fails++;
        free(d);
    }
    puts(fails ? "FAIL" : "PASS");
    return !!fails;
}

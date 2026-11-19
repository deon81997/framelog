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
/* Glide 2.03h3 (Tomb Raider): 7-byte start, fence with 9 bytes between xchg and store, unfenced store too */
static const uint8_t f203h3[] = { 0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10,
    0x8B, 0x00, 0x25, 0x00, 0x00, 0x00, 0x70, 0xC1, 0xE8, 0x1C,
    0x87, 0x05, 0xD0, 0x79, 0x26, 0x00, 0x8B, 0x46, 0x14, 0x8B, 0x15, 0x00, 0x12, 0x22, 0x00,
    0x89, 0x90, 0x28, 0x01, 0x00, 0x00, 0x87, 0x05, 0xD0, 0x79, 0x26, 0x00, 0xEB, 0x06,
    0x89, 0x98, 0x28, 0x01, 0x00, 0x00, 0xE8, 1, 2, 3, 4, 0x5F, 0x5E, 0x5B, 0xC2, 0x04, 0x00 };
/* Glide 2.03h2 (Toshinden): start = mov ebx,[abs] (fixup), no fence -> patch the end (last 6 bytes) */
static const uint8_t f203h2[] = { 0x53, 0x56, 0x57, 0x8B, 0x1D, 0x94, 0x9D, 0x2A, 0x00,
    0x8B, 0x00, 0x25, 0x00, 0x00, 0x00, 0x70, 0xC1, 0xE8, 0x1C, 0x8B, 0x43, 0x04,
    0x89, 0x90, 0x28, 0x01, 0x00, 0x00, 0xE8, 1, 2, 3, 4, 0x5F, 0x5E, 0x5B, 0xC2, 0x04, 0x00 };

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
    uint32_t n; gs_hit g;
    struct mem m;
    uint8_t *d = mkpe(&n, 0x1234, 0x10);                     /* reloc far away */
    m.d = d; m.n = n;
    CHECK(gs_probe(mrd, &m, n, &g) == 0 && g.fn == 0x200 + 0x1234 && g.n == 8 && !g.epi && !memcmp(g.code, gs_p8, 8), "pe ok %x", g.fn);
    free(d);
    d = mkpe(&n, 0x1234, 0x1234 + 4); m.d = d;               /* reloc inside the prologue */
    CHECK(gs_probe(mrd, &m, n, &g) == GS_FIX, "pe reloc in prologue");
    free(d);
    d = mkpe(&n, 0x1234, 0x1234 - 3); m.d = d;               /* reloc spilling into byte 0 */
    CHECK(gs_probe(mrd, &m, n, &g) == GS_FIX, "pe reloc spill");
    free(d);
    d = mkpe(&n, 0x1234, -1); m.d = d;                       /* no relocs */
    CHECK(gs_probe(mrd, &m, n, &g) == 0, "pe no relocs");
    putfn(d, 0x200 + 0x3000);                                /* second swap function */
    CHECK(gs_probe(mrd, &m, n, &g) == GS_MANY, "two hits");
    free(d);
    /* window edges: function straddling every block boundary */
    for (uint32_t at = GS_BLK - 0x40; at < GS_BLK + 0x20; at++) {
        d = mkpe(&n, at - 0x200 + 0x200, -1); m.d = d;
        int e = gs_probe(mrd, &m, n, &g);
        CHECK(e == 0 && g.fn == 0x200 + at, "edge %x e=%d fn=%x", at, e, g.fn);
        free(d);
    }
    d = mkpe(&n, 0x1234, -1); d[0x200 + 0x1234 + 0x2C] = 0x90; m.d = d;   /* epilogue broken */
    CHECK(gs_probe(mrd, &m, n, &g) == GS_NONE, "no epilogue");
    free(d);
    d = mkpe(&n, 0x1234, -1); memset(d + 0x200 + 0x1234, 0, 0x40);          /* 2.03h3: 7-byte start */
    memcpy(d + 0x200 + 0x1234, f203h3, sizeof f203h3); m.d = d;
    CHECK(gs_probe(mrd, &m, n, &g) == 0 && g.fn == 0x200 + 0x1234 && g.n == 7 && !g.epi && !memcmp(g.code, gs_p7, 7), "2.03h3 %x", g.fn);
    d[0x200 + 0x1234 + 0x13] = 0x11;                                       /* fence addresses differ: unfenced store only */
    CHECK(gs_probe(mrd, &m, n, &g) == 0 && g.fn == 0x200 + 0x1234, "2.03h3 one store");
    memset(d + 0x200 + 0x1234 + 9, 0x90, 8);                               /* and no wait loop: nothing */
    CHECK(gs_probe(mrd, &m, n, &g) == GS_NONE, "unfenced without wait");
    free(d);
    d = mkpe(&n, 0x1234, -1); memset(d + 0x200 + 0x1234, 0, 0x40);          /* 2.03h2: end patch */
    memcpy(d + 0x200 + 0x1234, f203h2, sizeof f203h2); m.d = d;
    uint32_t e6 = 0x200 + 0x1234 + sizeof f203h2 - 6;
    CHECK(gs_probe(mrd, &m, n, &g) == 0 && g.fn == e6 && g.n == 6 && g.epi && !memcmp(g.code, gs_e6, 6), "2.03h2 %x", g.fn);
    free(d);
    d = mkpe(&n, 0x1234, -1); memset(d + 0x200 + 0x1234, 0, 0x40);
    memcpy(d + 0x200 + 0x1234, f203h2, sizeof f203h2); m.d = d;
    d[e6 - 1] = 0x5D;                                                       /* ebp popped but no 8-byte start: refuse */
    CHECK(gs_probe(mrd, &m, n, &g) == GS_NONE, "5D end without start");
    free(d);
    d = mkpe(&n, 0x1234, 0x1234 + sizeof f203h2 - 8); memset(d + 0x200 + 0x1234, 0, 0x40);   /* reloc before the end */
    memcpy(d + 0x200 + 0x1234, f203h2, sizeof f203h2); m.d = d;
    CHECK(gs_probe(mrd, &m, n, &g) == GS_FIX, "2.03h2 reloc into end");
    free(d);
    static uint8_t mzonly[0x400] = { 'M', 'Z' }; m.d = mzonly; m.n = sizeof mzonly;
    CHECK(gs_probe(mrd, &m, m.n, &g) == GS_FMT, "real-mode exe");
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
        gs_hit g = { 0 };
        int e = gs_probe(mrd, &m, m.n, &g);
        char got[32];
        if (e) snprintf(got, sizeof got, "%s", e == GS_NONE ? "none" : e == GS_FMT ? "fmt" : e == GS_MANY ? "many" : e == GS_FIX ? "fix" : "io");
        else snprintf(got, sizeof got, g.epi ? "end:0x%x" : "0x%x", g.fn);
        int ok = !eq || !strcmp(eq, got) || (!strcmp(eq, "none") && !strcmp(got, "fmt"));
        printf("%s %s %s%s%s\n", ok ? "ok  " : "FAIL", argv[i], got, eq ? " want " : "", eq ? eq : "");
        if (!ok) fails++;
        free(d);
    }
    puts(fails ? "FAIL" : "PASS");
    return !!fails;
}

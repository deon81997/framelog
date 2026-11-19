/* glstatic.c - FRAMELOG static Glide: grBufferSwap linked into the game's EXE (LE) or a driver file it loads (PE,
 * e.g. Carmageddon's BRender driver 3DFX_DOS.BDD = Glide 2.11), no GLIDE2X.OVL. #included by glhook.c; the
 * signature + format code above GS_HOST is plain C, host-tested by dos/glide/test/gstest.c. See docs/GLIDE.md.
 *
 * The stub (glstub.asm) also reports opens of *.EXE / *.BDD by a protected-mode program. gs_parse scans that file
 * with our own handle (load time only, 4 KB reads) for Glide 2.x SST-1's grBufferSwap, anchored on
 *   store  89 /r(disp32) 28 01 00 00   mov [hw+128h],r = the swapbufferCMD write (SST-1 register 128h), accepted if
 *     fenced: 87 05 A right after it and 87 05 A (same A) <= 16 bytes before (xchg [p6fence],eax; 2.03h3-2.42), or
 *     not:    25 00 00 00 70 C1 E8 1C (and 70000000h; shr 1Ch = the swaps-pending wait) <= 180h before (2.03h2)
 *   end    5F 5E 5B C2 04 00  (pop edi/esi/ebx; ret 4) <= 50h after the store; 5D before it = ebp pushed too
 *   start  the nearest 53 56 57 55 8B 5C 24 14 (ebp; 2.11-2.42) or 53 56 57 8B 7C 24 10 (2.03h3) <= 180h before
 * Games: GLIDE2X.OVL 2.31 (Extreme Assault), 2.42 (Archimedean Dynasty), 3DFX_DOS.BDD (Carmageddon), TOMB.EXE 2.03h3
 * (Tomb Raider 3Dfx demo: fence only on P6, 7 start bytes), TSD.EXE 2.03h2 (Toshinden 3Dfx demo: no fence; its start
 * reads a global = LE fixup, so the end is patched: JMP thunk that runs pop/pop/pop/ret 4 itself).
 * Exactly one function (all its stores agree), no PE base relocation / LE fixup in the patched bytes -> thunk + patch in the
 * read buffers as the OVL (gl_pt[0]). GLIDE2X.OVL itself never comes here (own path), so nothing counts twice. */
#include "glstatic.h"
#include <string.h>

#define GS_BACK 0x180
#define GS_EPI  0x50                  /* epilogue <= 50h bytes after the store */
#define GS_FWD  0x60
#define GS_WIN  4096                  /* one DOS read per window */
#define GS_BLK  (GS_WIN - GS_BACK - GS_FWD)
static const uint8_t gs_p8[8] = { 0x53, 0x56, 0x57, 0x55, 0x8B, 0x5C, 0x24, 0x14 },  /* 2.11-2.42 */
                     gs_p7[7] = { 0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10 },        /* 2.03 (Tomb Raider) */
                     gs_e6[6] = { 0x5F, 0x5E, 0x5B, 0xC2, 0x04, 0x00 },              /* pop edi/esi/ebx; ret 4 */
                     gs_poll[8] = { 0x25, 0x00, 0x00, 0x00, 0x70, 0xC1, 0xE8, 0x1C }; /* and 70000000h; shr eax,1Ch */
static uint8_t gs_w[GS_WIN];

static uint32_t gs32(const uint8_t *p) { return p[0] | p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t gs16(const uint8_t *p) { return p[0] | p[1] << 8; }

/* mov [r32+128h], r32 = the swapbufferCMD write */
static int gs_store(const uint8_t *p)
{
    return p[0] == 0x89 && (p[1] & 0xC0) == 0x80 && (p[1] & 7) != 4 && p[2] == 0x28 && p[3] == 0x01 && !p[4] && !p[5];
}

/* signature scan: 0 = one function found (*h), GS_NONE / GS_MANY / GS_IO */
static int gs_scan(gs_rd rd, void *c, uint32_t size, gs_hit *h)
{
    int hits = 0;
    for (uint32_t pos = 0; pos < size; pos += GS_BLK) {
        uint32_t ws = pos >= GS_BACK ? pos - GS_BACK : 0, we = pos + GS_BLK + GS_FWD;
        if (we > size) we = size;
        if (!rd(c, ws, gs_w, we - ws)) return GS_IO;
        uint32_t ie = pos + GS_BLK < we ? pos + GS_BLK : we;
        for (uint32_t i = pos; i + 12 <= ie; i++) {
            const uint8_t *p = gs_w + (i - ws);
            if (p[0] != 0x89 || !gs_store(p)) continue;
            uint32_t lo = i >= ws + GS_BACK ? i - GS_BACK : ws, j, k;
            int ok = 0;
            if (p[6] == 0x87 && p[7] == 0x05)                     /* P6 fence: xchg [A],eax <= 16 bytes before */
                for (k = 6; k <= 22 && i >= ws + k; k++)
                    if (p[-(int)k] == 0x87 && p[1 - (int)k] == 0x05 && !memcmp(p + 2 - k, p + 8, 4)) { ok = 1; break; }
            for (j = lo; !ok && j + sizeof gs_poll <= i; j++)      /* no fence: the swaps-pending wait before it */
                ok = !memcmp(gs_w + (j - ws), gs_poll, sizeof gs_poll);
            if (!ok) continue;
            for (k = i + 6, ok = 0; k < i + 6 + GS_EPI && k + sizeof gs_e6 <= we; k++)
                if (!memcmp(gs_w + (k - ws), gs_e6, sizeof gs_e6)) { ok = 1; break; }
            if (!ok) continue;
            int ebp = gs_w[k - 1 - ws] == 0x5D;
            gs_hit t = { k, sizeof gs_e6, 1 };                    /* no known prologue: patch the epilogue */
            for (j = i; j-- > lo;) {                              /* nearest start; must match the end */
                int s8 = !memcmp(gs_w + (j - ws), gs_p8, 8), s7 = !s8 && !memcmp(gs_w + (j - ws), gs_p7, 7);
                if (s8 || s7) { t.fn = j; t.n = s8 ? 8 : 7; t.epi = 0; ebp ^= s8; break; }
            }
            if (ebp) continue;
            if (hits++ && h->fn != t.fn) return GS_MANY;
            *h = t;
        }
    }
    return hits ? 0 : GS_NONE;
}

/* PE: no base relocation may touch [fn, fn+n) */
static int gs_pe(gs_rd rd, void *c, uint32_t pe, uint32_t fn, uint32_t n)
{
    uint8_t h[24], s[40], b[8], e[512];
    if (!rd(c, pe, h, 24)) return GS_IO;
    uint32_t ns = gs16(h + 6), opt = pe + 24, so = opt + gs16(h + 20), rva = 0, rr = 0, rs = 0, i;
    if (!rd(c, opt, s, 2) || gs16(s) != 0x10B || gs16(h + 20) < 96 + 6 * 8 || !rd(c, opt + 96 + 5 * 8, b, 8)) return GS_FMT;
    uint32_t rdir = gs32(b), rlen = gs32(b + 4), found = 0;
    for (i = 0; i < ns; i++) {                                /* fn -> rva; reloc dir rva -> file offset */
        if (!rd(c, so + 40 * i, s, 40)) return GS_IO;
        uint32_t va = gs32(s + 12), sz = gs32(s + 16), raw = gs32(s + 20);
        if (fn >= raw && fn - raw < sz) { rva = va + fn - raw; found = 1; }
        if (rdir >= va && rdir - va < sz) { rr = raw + rdir - va; rs = sz - (rdir - va); }
    }
    if (!found) return GS_FMT;
    if (!rlen) return 0;                                      /* no relocations at all */
    if (!rr || rlen > rs) return GS_FMT;
    for (uint32_t p = 0; p + 8 <= rlen;) {
        if (!rd(c, rr + p, b, 8)) return GS_IO;
        uint32_t pg = gs32(b), bs = gs32(b + 4);
        if (bs < 8 || p + bs > rlen) return GS_FMT;
        if (pg <= rva + n && pg + 0x1000 + 4 > rva)
            for (uint32_t q = 8; q < bs; q += sizeof e) {
                uint32_t k = bs - q < sizeof e ? bs - q : sizeof e;
                if (!rd(c, rr + p + q, e, k)) return GS_IO;
                for (uint32_t t = 0; t + 2 <= k; t += 2) {
                    uint16_t v = gs16(e + t);
                    uint32_t r = pg + (v & 0xFFF);
                    if ((v >> 12) && r + 8 > rva && r < rva + n) return GS_FIX;   /* up to 8-byte types */
                }
            }
        p += bs;
    }
    return 0;
}

/* LE (DOS/4GW EXE): pages stored in order, no fixup (by its source size) reaching into [po, po+pn) of the page */
static int gs_le(gs_rd rd, void *c, uint32_t o, uint32_t fn, uint32_t pn)
{
    uint8_t h[0xC4], m[4], f[8], r[512];
    if (!rd(c, o, h, sizeof h)) return GS_IO;
    if (h[1] != 'E') return GS_FMT;                           /* LX: other page map / offsets */
    uint32_t ps = gs32(h + 0x28), dp = gs32(h + 0x80), np = gs32(h + 0x14);
    if (!ps || fn < dp || (fn - dp) / ps >= np) return GS_FMT;
    uint32_t pg = (fn - dp) / ps + 1, po = (fn - dp) % ps;
    if (po + pn > ps) return GS_FIX;
    if (!rd(c, o + gs32(h + 0x48) + 4 * (pg - 1), m, 4)) return GS_IO;
    if (((uint32_t)m[0] << 16 | m[1] << 8 | m[2]) != pg && gs16(m + 1) != pg) return GS_FMT;   /* page map: in order */
    if (!rd(c, o + gs32(h + 0x68) + 4 * (pg - 1), f, 8)) return GS_IO;
    uint32_t a = gs32(f), z = gs32(f + 4), base = o + gs32(h + 0x6C);
    if (z < a || z - a > gs32(h + 0x30)) return GS_FMT;
    /* records of this page (read in 512-byte pieces, a record is <= 16 bytes + its list) */
    uint32_t n = z - a, p = 0, have = 0, at = 0;
    while (p < n) {
        if (p + 64 > at + have && at + have < n) {            /* refill so a whole record is in r[] */
            at = p; have = n - p < sizeof r ? n - p : sizeof r;
            if (!rd(c, base + a + at, r, have)) return GS_IO;
        }
        const uint8_t *q = r + (p - at);
        uint8_t st = q[0], fl = q[1], cnt = 1, list = st & 0x20;
        static const uint8_t gs_fsz[16] = { 1, 6, 2, 4, 6, 2, 6, 4, 4, 6, 6, 6, 6, 6, 6, 6 };   /* bytes per source type */
        int32_t sz = gs_fsz[st & 0xF];
        int32_t so = 0;
        uint32_t k = 2;
        if (list) cnt = q[k++]; else { so = (int16_t)gs16(q + k); k += 2; }
        switch (fl & 3) {
        case 0: k += fl & 0x40 ? 2 : 1; if ((st & 0xF) != 2) k += fl & 0x10 ? 4 : 2; break;
        case 1: k += fl & 0x40 ? 2 : 1; k += fl & 0x80 ? 1 : fl & 0x10 ? 4 : 2; if (fl & 4) k += fl & 0x20 ? 4 : 2; break;
        case 2: k += fl & 0x40 ? 2 : 1; k += fl & 0x10 ? 4 : 2; if (fl & 4) k += fl & 0x20 ? 4 : 2; break;
        case 3: k += fl & 0x40 ? 2 : 1; if (fl & 4) k += fl & 0x20 ? 4 : 2; break;
        }
        if (list && p + k + 2u * cnt > at + have) {           /* long list: reload from the record start */
            at = p; have = n - p < sizeof r ? n - p : sizeof r;
            if (k + 2u * cnt > have || !rd(c, base + a + at, r, have)) return GS_FMT;
            q = r;
        }
        for (int i = 0; i < cnt; i++) {
            if (list) { so = (int16_t)gs16(q + k); k += 2; }
            if (so + sz > (int32_t)po && so < (int32_t)(po + pn)) return GS_FIX;
        }
        p += k;
    }
    return 0;
}

int gs_probe(gs_rd rd, void *c, uint32_t size, gs_hit *h)
{
    uint8_t mz[0x40], sg[4];
    if (size < 0x40 || !rd(c, 0, mz, 0x40) || mz[0] != 'M' || mz[1] != 'Z') return GS_FMT;
    uint32_t o = gs32(mz + 0x3C);
    if (o > size - 4 || !rd(c, o, sg, 4)) return GS_FMT;
    int pe = sg[0] == 'P' && sg[1] == 'E' && !sg[2] && !sg[3], le = sg[0] == 'L' && (sg[1] == 'E' || sg[1] == 'X');
    if (!pe && !le) return GS_FMT;                            /* real-mode EXE, DOS4GW.EXE, ...: no 32-bit code */
    int e = gs_scan(rd, c, size, h);
    if (e) return e;
    if ((e = pe ? gs_pe(rd, c, o, h->fn, h->n) : gs_le(rd, c, o, h->fn, h->n))) return e;
    return rd(c, h->fn, h->code, h->n) ? 0 : GS_IO;
}

#ifndef GS_HOST
/* ---- DOS side (inside glhook.c: gl_read, gl_pt, St) ---- */
static uint16_t gs_seg;               /* own 64-byte DOS block for the static thunk (the OVL's stays separate) */
static int gs_rdh(void *c, uint32_t off, void *dst, uint32_t n) { return gl_read(*(int *)c, off, dst, n); }

/* open file handle h (stub busy): 0 = armed (gl_pt[0] = the patch), else a GS_ code */
static int gs_parse(int h)
{
    __dpmi_regs r;
    memset(&r, 0, sizeof r);
    r.x.ax = 0x4202; r.x.bx = h;                             /* size */
    if (!gl_dos(&r)) return GS_IO;
    uint32_t size = (uint32_t)r.x.dx << 16 | r.x.ax;
    gs_hit g;
    int e = gs_probe(gs_rdh, &h, size, &g);
    if (e) return e;
    if (!gs_seg) { int sel, s = __dpmi_allocate_dos_memory(4, &sel); if (s < 0) return GS_IO; gs_seg = (uint16_t)s; }
    /* start (as glhook.c): pop [ret]; push eax; push edx; mov eax,474C0000h; mov dx,3D4h; in al,dx; pop edx; pop eax;
     *   <n moved bytes>; jmp [ret] | ret dd | ptr dd -> thunk; patch = CALL [ptr] + NOPs
     * end: push eax; ...; pop eax; <pop edi/esi/ebx; ret 4> | ptr dd -> thunk; patch = JMP [ptr] (6 bytes) */
    static const uint8_t mid[14] = { 0x50, 0x52, 0xB8, 0x00, 0x00, 0x4C, 0x47, 0x66, 0xBA, 0xD4, 0x03, 0xEC, 0x5A, 0x58 };
    uint32_t L = (uint32_t)gs_seg << 4, ret, ptr, len;
    uint8_t t[64];
    if (g.epi) {
        memcpy(t, mid, sizeof mid); memcpy(t + 14, g.code, g.n);
        ptr = L + 14 + g.n; memcpy(t + 14 + g.n, &L, 4); len = 18 + g.n;
    } else {
        ret = L + 20 + g.n + 6; ptr = ret + 4;
        t[0] = 0x8F; t[1] = 0x05; memcpy(t + 2, &ret, 4);
        memcpy(t + 6, mid, sizeof mid);
        memcpy(t + 20, g.code, g.n);
        t[20 + g.n] = 0xFF; t[21 + g.n] = 0x25; memcpy(t + 22 + g.n, &ret, 4);
        memset(t + 26 + g.n, 0, 4); memcpy(t + 30 + g.n, &L, 4); len = 34 + g.n;
    }
    dosmemput(t, len, L);
    gl_pt[0].tgt = g.fn; gl_pt[0].b[0] = 0xFF; gl_pt[0].b[1] = g.epi ? 0x25 : 0x15; memcpy(gl_pt[0].b + 2, &ptr, 4);
    memset(gl_pt[0].b + 6, 0x90, g.n - 6);
    gl_pt[0].n = g.n; gl_pt[0].done = 0; gl_np = 1;
    return 0;
}
#endif

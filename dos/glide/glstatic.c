/* glstatic.c - FRAMELOG static Glide: grBufferSwap linked into the game's EXE (LE) or a driver file it loads (PE,
 * e.g. Carmageddon's BRender driver 3DFX_DOS.BDD = Glide 2.11), no GLIDE2X.OVL. #included by glhook.c; the
 * signature + format code above GS_HOST is plain C, host-tested by dos/glide/test/gstest.c. See docs/GLIDE.md.
 *
 * The stub (glstub.asm) also reports opens of *.EXE / *.BDD by a protected-mode program. gs_parse scans that file
 * with our own handle (load time only, 4 KB reads) for Glide 2.x SST-1's grBufferSwap:
 *   fence  87 05 xx xx xx xx  89 /r(disp32) 28 01 00 00  87 05   xchg [p6fence],eax / mov [hw+128h],r / xchg ...
 *          = the write of swapbufferCMD (SST-1 register 128h) behind the P6 write fence; once per Glide, never in game code
 *   start  53 56 57 55 8B 5C 24 14  (push ebx/esi/edi/ebp; mov ebx,[esp+14h] = swapInterval), the nearest one
 *          <= 180h bytes before the fence
 *   end    5D 5F 5E 5B C2 04 00     (pop x4; ret 4) <= 40h bytes after the fence
 * Same function in GLIDE2X.OVL 2.31 (Extreme Assault) and 2.42 (Archimedean Dynasty) and the 2.11 in 3DFX_DOS.BDD.
 * Exactly one hit, no PE base relocation / LE fixup in the 8 prologue bytes -> the same thunk + FF 15 patch in the
 * read buffers as the OVL (gl_pt[0]). GLIDE2X.OVL itself never comes here (own path), so nothing counts twice. */
#include "glstatic.h"
#include <string.h>

#define GS_BACK 0x180
#define GS_FWD  0x40
#define GS_WIN  4096                  /* one DOS read per window */
#define GS_BLK  (GS_WIN - GS_BACK - GS_FWD)
static const uint8_t gs_pro[GS_PRO] = { 0x53, 0x56, 0x57, 0x55, 0x8B, 0x5C, 0x24, 0x14 },
                     gs_epi[7] = { 0x5D, 0x5F, 0x5E, 0x5B, 0xC2, 0x04, 0x00 };
static uint8_t gs_w[GS_WIN];

static uint32_t gs32(const uint8_t *p) { return p[0] | p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t gs16(const uint8_t *p) { return p[0] | p[1] << 8; }

static int gs_fence(const uint8_t *p)
{
    return p[0] == 0x87 && p[1] == 0x05 && p[6] == 0x89 && (p[7] & 0xC0) == 0x80 && (p[7] & 7) != 4 &&
           p[8] == 0x28 && p[9] == 0x01 && p[10] == 0 && p[11] == 0 && p[12] == 0x87 && p[13] == 0x05;
}

/* signature scan: 0 = one function found (*fn), GS_NONE / GS_MANY / GS_IO */
static int gs_scan(gs_rd rd, void *c, uint32_t size, uint32_t *fn)
{
    int hits = 0;
    for (uint32_t pos = 0; pos < size; pos += GS_BLK) {
        uint32_t ws = pos >= GS_BACK ? pos - GS_BACK : 0, we = pos + GS_BLK + GS_FWD;
        if (we > size) we = size;
        if (!rd(c, ws, gs_w, we - ws)) return GS_IO;
        uint32_t ie = pos + GS_BLK < we ? pos + GS_BLK : we;
        for (uint32_t i = pos; i + 14 <= ie; i++) {
            const uint8_t *p = gs_w + (i - ws);
            if (p[0] != 0x87 || !gs_fence(p)) continue;
            int ok = 0;
            for (uint32_t k = i + 14; k + sizeof gs_epi <= we && k < i + GS_FWD; k++)
                if (!memcmp(gs_w + (k - ws), gs_epi, sizeof gs_epi)) { ok = 1; break; }
            if (!ok) continue;
            for (uint32_t j = i; j-- > ws && j + GS_BACK >= i;)
                if (!memcmp(gs_w + (j - ws), gs_pro, GS_PRO)) {
                    if (hits++ && *fn != j) return GS_MANY;
                    *fn = j;
                    break;
                }
        }
    }
    return hits ? 0 : GS_NONE;
}

/* PE: no base relocation may touch [fn, fn+GS_PRO) */
static int gs_pe(gs_rd rd, void *c, uint32_t pe, uint32_t fn)
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
        if (pg <= rva + GS_PRO && pg + 0x1000 + 4 > rva)
            for (uint32_t q = 8; q < bs; q += sizeof e) {
                uint32_t k = bs - q < sizeof e ? bs - q : sizeof e;
                if (!rd(c, rr + p + q, e, k)) return GS_IO;
                for (uint32_t t = 0; t + 2 <= k; t += 2) {
                    uint16_t v = gs16(e + t);
                    uint32_t r = pg + (v & 0xFFF);
                    if ((v >> 12) && r + 8 > rva && r < rva + GS_PRO) return GS_FIX;   /* up to 8-byte types */
                }
            }
        p += bs;
    }
    return 0;
}

/* LE (DOS/4GW EXE): pages stored in order, no fixup in [po-6, po+GS_PRO) of the page */
static int gs_le(gs_rd rd, void *c, uint32_t o, uint32_t fn)
{
    uint8_t h[0xC4], m[4], f[8], r[512];
    if (!rd(c, o, h, sizeof h)) return GS_IO;
    if (h[1] != 'E') return GS_FMT;                           /* LX: other page map / offsets */
    uint32_t ps = gs32(h + 0x28), dp = gs32(h + 0x80), np = gs32(h + 0x14);
    if (!ps || fn < dp || (fn - dp) / ps >= np) return GS_FMT;
    uint32_t pg = (fn - dp) / ps + 1, po = (fn - dp) % ps;
    if (po + GS_PRO > ps) return GS_FIX;
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
            if (so > (int32_t)po - 6 && so < (int32_t)(po + GS_PRO)) return GS_FIX;
        }
        p += k;
    }
    return 0;
}

int gs_probe(gs_rd rd, void *c, uint32_t size, uint32_t *fn, uint8_t *code)
{
    uint8_t mz[0x40], sg[4];
    if (size < 0x40 || !rd(c, 0, mz, 0x40) || mz[0] != 'M' || mz[1] != 'Z') return GS_FMT;
    uint32_t o = gs32(mz + 0x3C);
    if (o > size - 4 || !rd(c, o, sg, 4)) return GS_FMT;
    int pe = sg[0] == 'P' && sg[1] == 'E' && !sg[2] && !sg[3], le = sg[0] == 'L' && (sg[1] == 'E' || sg[1] == 'X');
    if (!pe && !le) return GS_FMT;                            /* real-mode EXE, DOS4GW.EXE, ...: no 32-bit code */
    int e = gs_scan(rd, c, size, fn);
    if (e) return e;
    if ((e = pe ? gs_pe(rd, c, o, *fn) : gs_le(rd, c, o, *fn))) return e;
    return rd(c, *fn, code, GS_PRO) ? 0 : GS_IO;
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
    uint32_t size = (uint32_t)r.x.dx << 16 | r.x.ax, fn;
    uint8_t code[GS_PRO];
    int e = gs_probe(gs_rdh, &h, size, &fn, code);
    if (e) return e;
    if (!gs_seg) { int sel, s = __dpmi_allocate_dos_memory(4, &sel); if (s < 0) return GS_IO; gs_seg = (uint16_t)s; }
    /* thunk (as glhook.c): pop [ret]; push eax; push edx; mov eax,474C0000h; mov dx,3D4h; in al,dx; pop edx; pop eax;
     * <8 prologue bytes>; jmp [ret] | ret dd | ptr dd -> thunk */
    uint32_t L = (uint32_t)gs_seg << 4, ret = L + 20 + GS_PRO + 6, ptr = ret + 4;
    static const uint8_t mid[14] = { 0x50, 0x52, 0xB8, 0x00, 0x00, 0x4C, 0x47, 0x66, 0xBA, 0xD4, 0x03, 0xEC, 0x5A, 0x58 };
    uint8_t t[64];
    t[0] = 0x8F; t[1] = 0x05; memcpy(t + 2, &ret, 4);
    memcpy(t + 6, mid, sizeof mid);
    memcpy(t + 20, code, GS_PRO);
    t[20 + GS_PRO] = 0xFF; t[21 + GS_PRO] = 0x25; memcpy(t + 22 + GS_PRO, &ret, 4);
    memset(t + 26 + GS_PRO, 0, 4); memcpy(t + 30 + GS_PRO, &L, 4);
    dosmemput(t, 34 + GS_PRO, L);
    gl_pt[0].tgt = fn; gl_pt[0].b[0] = 0xFF; gl_pt[0].b[1] = 0x15; memcpy(gl_pt[0].b + 2, &ptr, 4);
    memset(gl_pt[0].b + 6, 0x90, GS_PRO - 6);
    gl_pt[0].n = GS_PRO; gl_pt[0].done = 0; gl_np = 1;
    return 0;
}
#endif

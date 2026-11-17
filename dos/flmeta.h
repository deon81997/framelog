/* flmeta.h - FRAMELOG 'M' record (run metadata, docs/PROTOCOL.md): text builders + encoder.
 * Plain C, no DOS calls: framelog.c gathers the raw data (VBE 4F00h/4F01h, CPUID, VGA registers), these turn it into
 * 'key=value\n' text and M records. Also compiled on the host: dos/testrig/metatest.sh compares the bytes with
 * serial-bench flproto.encode_meta. Nothing here is called per frame. */
#ifndef FLMETA_H
#define FLMETA_H
#include <stdint.h>

#define FLM_TEXT  320                /* longest metadata text (bytes) */
#define FLM_CHUNK 15
#define FLM_RECSZ 19                 /* A5h 'M' seq text[15] sum */
#define FLM_MAXREC ((FLM_TEXT / FLM_CHUNK + 1) * FLM_RECSZ)

/* text -> M records (seq 0, 1, ...; last chunk NUL-padded, one all-NUL chunk more when the text ends on a chunk
 * edge). Returns the bytes written to out, 0 if they don't fit in max. */
static int flm_encode(const char *t, int len, uint8_t *out, int max)
{
    int n = len / FLM_CHUNK + 1;
    if (n * FLM_RECSZ > max) return 0;
    for (int i = 0; i < n; i++) {
        uint8_t *r = out + i * FLM_RECSZ, sum = (uint8_t)('M' + i);
        r[0] = 0xA5; r[1] = 'M'; r[2] = (uint8_t)i;
        for (int j = 0; j < FLM_CHUNK; j++) {
            int k = i * FLM_CHUNK + j;
            uint8_t c = k < len ? (uint8_t)t[k] : 0;
            r[3 + j] = c; sum += c;
        }
        r[3 + FLM_CHUNK] = sum;
    }
    return n * FLM_RECSZ;
}

/* append src (at most n bytes, stops at NUL): printable ASCII only, runs of blanks/control bytes -> one space,
 * no leading/trailing blanks. Returns the new length of d. */
static int flm_cat(char *d, int dl, int max, const char *src, int n)
{
    int sp = dl > 0 && d[dl - 1] != ' ' ? 0 : 1;   /* at the start: drop leading blanks */
    for (int i = 0; i < n && src[i] && dl < max - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c <= ' ' || c > '~') { if (!sp && dl) { d[dl++] = ' '; sp = 1; } continue; }
        d[dl++] = (char)c; sp = 0;
    }
    while (dl > 0 && d[dl - 1] == ' ') dl--;
    d[dl] = 0;
    return dl;
}

static int flm_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

static int flm_u(char *d, uint32_t v)       /* decimal, returns length */
{
    char b[12]; int n = 0, k = 0;
    do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) d[k++] = b[--n];
    d[k] = 0;
    return k;
}

static int flm_hex(char *d, uint32_t v, int digits)
{
    for (int i = 0; i < digits; i++) d[i] = "0123456789ABCDEF"[(v >> (4 * (digits - 1 - i))) & 15];
    d[digits] = 0;
    return digits;
}

static int flm_lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int flm_has(const char *hay, const char *needle)   /* case-insensitive substring */
{
    if (!*needle) return 1;
    for (; *hay; hay++) {
        int i = 0;
        while (needle[i] && hay[i] && flm_lower(hay[i]) == flm_lower(needle[i])) i++;
        if (!needle[i]) return 1;
    }
    return 0;
}

/* "key=value\n" (skipped when the value is empty or doesn't fit). Returns the new text length. */
static int flm_add(char *t, int tl, int max, const char *key, const char *val)
{
    int kl = flm_len(key), vl = flm_len(val);
    if (!vl || tl + kl + vl + 2 >= max) return tl;
    for (int i = 0; i < kl; i++) t[tl++] = key[i];
    t[tl++] = '=';
    for (int i = 0; i < vl; i++) t[tl++] = val[i];
    t[tl++] = '\n';
    t[tl] = 0;
    return tl;
}

/* gpu: VBE 4F00h OEM string, plus (VBE 2.0+) vendor / product names not already in it; no VBE = "VGA" */
static void flm_gpu(char *out, int max, int vbe, uint16_t ver, const char *oem, const char *vendor, const char *product)
{
    char tmp[80];
    int l = 0;
    out[0] = 0;
    if (vbe) l = flm_cat(out, 0, max, oem, 79);
    if (vbe && ver >= 0x200) {
        const char *x[2] = { vendor, product };
        for (int i = 0; i < 2; i++) {
            tmp[0] = 0; flm_cat(tmp, 0, sizeof tmp, x[i], 79);
            if (!tmp[0] || flm_has(out, tmp)) continue;
            if (l && flm_has(tmp, out)) l = 0;          /* "NVIDIA" + "NVIDIA Corporation" -> the longer one */
            if (l) out[l++] = ' ';
            out[l] = 0; l = flm_cat(out, l, max, tmp, 79);
        }
    }
    if (!l) { out[0] = 'V'; out[1] = 'G'; out[2] = 'A'; out[3] = 0; }
}

/* driver: "VBE 2.0, OEM rev 1.00[, product rev]" (VBE 2.0+ only has the revisions); no VBE = empty */
static void flm_driver(char *out, int max, int vbe, uint16_t ver, uint16_t oemrev, const char *prodrev)
{
    int l = 0;
    out[0] = 0;
    if (!vbe || max < 40) return;
    out[l++] = 'V'; out[l++] = 'B'; out[l++] = 'E'; out[l++] = ' ';
    l += flm_hex(out + l, ver >> 8, (ver >> 12) ? 2 : 1); out[l++] = '.';
    l += flm_hex(out + l, ver & 0xFF, (ver & 0xF0) ? 2 : 1);
    out[l] = 0;
    if (ver >= 0x200) {
        const char *s = ", OEM rev ";
        while (*s) out[l++] = *s++;
        l += flm_hex(out + l, oemrev >> 8, (oemrev >> 12) ? 2 : 1); out[l++] = '.';
        l += flm_hex(out + l, oemrev & 0xFF, 2);
        out[l] = 0;
        char t[64]; t[0] = 0;
        if (flm_cat(t, 0, sizeof t, prodrev, 63)) { out[l++] = ','; out[l++] = ' '; out[l] = 0; flm_cat(out, l, max, t, 63); }
    }
}

/* cpu: CPUID brand string, else "<vendor> family F model M stepping S"; then ", N MHz" (TSC rate) */
static void flm_cpu(char *out, int max, const char *brand, const char *vendor, int fam, int model, int step, uint32_t mhz)
{
    int l = flm_cat(out, 0, max, brand ? brand : "", 48);
    if (!l && (vendor[0] || fam)) {     /* no CPUID at all: MHz only */
        l = flm_cat(out, 0, max, vendor, 12);
        const char *f[3] = { " family ", " model ", " stepping " };
        int v[3] = { fam, model, step };
        for (int i = 0; i < 3 && l < max - 24; i++) {
            for (const char *s = (l || i) ? f[i] : f[i] + 1; *s; s++) out[l++] = *s;
            l += flm_u(out + l, (uint32_t)v[i]);
        }
        out[l] = 0;
    }
    if (mhz && l < max - 16) {
        if (l) { out[l++] = ','; out[l++] = ' '; }
        l += flm_u(out + l, mhz);
        out[l++] = ' '; out[l++] = 'M'; out[l++] = 'H'; out[l++] = 'z'; out[l] = 0;
    }
}

/* the mode part: res=WxH, depth=bits, mode=13h / X / 12h / VESA 101h / text 03h */
typedef struct { uint16_t w, h; uint8_t bpp, text; } FLM_MODE;

/* standard VGA BIOS modes (40:49) */
static int flm_vga_mode(int m, FLM_MODE *o)
{
    static const struct { uint8_t m; uint16_t w, h; uint8_t bpp; } t[] = {
        {0x04,320,200,2},{0x05,320,200,2},{0x06,640,200,1},{0x0D,320,200,4},{0x0E,640,200,4},{0x0F,640,350,1},
        {0x10,640,350,4},{0x11,640,480,1},{0x12,640,480,4},{0x13,320,200,8} };
    o->text = (m <= 3 || m == 7);
    o->w = o->h = 0; o->bpp = 0;
    if (o->text) return 1;
    for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++)
        if (t[i].m == m) { o->w = t[i].w; o->h = t[i].h; o->bpp = t[i].bpp; return 1; }
    return 0;
}

/* 256-colour VGA (mode 13h and its unchained 'Mode X' tweaks) from CRTC 01h (horizontal display end), 07h
 * (overflow), 09h (max scan line) and 12h (vertical display end): width = (01h+1)*4, height = scan lines shown /
 * (rows per line) / (2 when double-scanned). Mode 13h: 4Fh/1Fh/41h/8Fh = 320x200; Mode X 240: VDE DFh = 320x240. */
static void flm_vga256(uint8_t r01, uint8_t r07, uint8_t r09, uint8_t r12, FLM_MODE *o)
{
    uint32_t vde = (uint32_t)r12 | ((r07 & 0x02u) << 7) | ((r07 & 0x40u) << 3);
    uint32_t div = ((r09 & 0x1Fu) + 1) * ((r09 & 0x80) ? 2 : 1);
    o->w = (uint16_t)((r01 + 1u) * 4);
    o->h = (uint16_t)((vde + 1) / div);
    o->bpp = 8; o->text = 0;
}

/* mode name: "VESA 111h" (vmode >= 0), "X" (unchained 13h), "13h", "text 03h" */
static void flm_name(char *name, int vmode, int bm, int unchained, int text)
{
    int l = 0;
    if (vmode >= 0) { const char *v = "VESA "; while (*v) name[l++] = *v++; l += flm_hex(name + l, (uint32_t)vmode, 3); }
    else if (bm == 0x13 && unchained) name[l++] = 'X';
    else { if (text) { const char *v = "text "; while (*v) name[l++] = *v++; } l += flm_hex(name + l, (uint32_t)bm, 2); }
    if (name[l - 1] != 'X') name[l++] = 'h';
    name[l] = 0;
}

/* append res/depth/mode lines. modename: "13h", "X", "VESA 101h" ... (may be empty) */
static int flm_mode_text(char *t, int tl, int max, const FLM_MODE *m, const char *modename)
{
    char b[24];
    if (m && !m->text && m->w && m->h) {
        int l = flm_u(b, m->w); b[l++] = 'x'; flm_u(b + l, m->h);
        tl = flm_add(t, tl, max, "res", b);
        if (m->bpp) { flm_u(b, m->bpp); tl = flm_add(t, tl, max, "depth", b); }
    }
    return flm_add(t, tl, max, "mode", modename);
}
#endif

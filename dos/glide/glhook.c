/* glhook.c - FRAMELOG DOS Glide hook (#included by framelog.c; default on, /NOGLIDE = off). See docs/GLIDE.md.
 *
 * DOS Glide 2 games (3dfx glide2x.lib + the Tenberry DLL loader, or a static Glide lib - not covered) load
 * GLIDE2X.OVL, an LE DLL, at run time with plain DOS reads through the extender's real-mode transfer buffer.
 * A real-mode INT 21h stub (glstub.asm, own DOS block) sees the open; gl_open_cb parses the OVL from disk (own DOS
 * calls, stub passes them through): export _GRBUFFERSWAP@4 -> object/offset -> file offset, its first instructions
 * (>= 6 bytes, simple prologue ops only, no LE fixup inside) and builds a 32-bit thunk in the DOS block:
 *     pop dword [ret]; push eax; push edx; mov eax,474C0000h; mov dx,3D4h; in al,dx; pop edx; pop eax;
 *     <the original first instructions>; jmp dword [ret]
 * gl_read_cb then rewrites those bytes in every read buffer that covers them to  FF 15 <ptr>  (CALL DWORD [ptr],
 * ptr -> thunk) + NOPs, before the extender copies the page up and applies its fixups. Each grBufferSwap thus does
 * one IN 3D4h with 'GL' in EAX's upper half: crtc_trap -> gl_flip = a frame at the moment the game asks for the swap
 * (same idea as the VBE PM SetDisplayStart thunk). Needs a zero-based flat model (DOS/4GW: CS/DS base 0), which is
 * also what makes the low-memory thunk reachable with absolute addresses. */
#include "glide/glstub.h"
#define GL_BUF     4096               /* DOS read buffer for our own parsing */
#define GL_THUNK   320                /* thunk offset in the block (stub at 0) */
#define GL_DLL     384                /* DLL start-code thunks (2 x 16 bytes) */
#define GL_IO      448                /* DOS buffer offset (name copy + reads; thunk <= 64 bytes) */
_Static_assert(sizeof glstub_bin <= GL_THUNK, "glstub.asm grew into the thunk");
static DPMI_REG gl_open_reg, gl_read_reg;
static uint16_t gl_seg;
/* patches applied to the OVL bytes as the extender reads them: [0] = grBufferSwap, [1..] = DLL start-code fixes */
static struct { uint32_t tgt; uint8_t b[16], n; uint32_t done; } gl_pt[3];   /* file offset, bytes, count, written mask */
static int gl_np;
static uint8_t gl_ldr[65536];         /* LE loader section of the OVL being parsed */

static void gl_flip(void)
{
    St.glide_swaps++;
    uint64_t now = rdtsc();
    if (now - last_sa > St.merge_ticks) { last_sa = now; if (mode3da) pump(); else frame_event(now); }
    else last_sa = now;
}

static void gl_busy(int b) { _farpokeb(_dos_ds, ((uint32_t)gl_seg << 4) + GLSTUB_CB3 + 2, (uint8_t)b); }

/* our own DOS file calls (stub busy = passes them straight to DOS) */
static int gl_dos(__dpmi_regs *r) { r->x.ss = r->x.sp = 0; __dpmi_int(0x21, r); return !(r->x.flags & 1); }
static int gl_read(int h, uint32_t off, void *dst, uint32_t n)
{
    __dpmi_regs r;
    memset(&r, 0, sizeof r);
    r.x.ax = 0x4200; r.x.bx = h; r.x.cx = off >> 16; r.x.dx = off & 0xFFFF;
    if (!gl_dos(&r)) return 0;
    while (n) {
        uint32_t k = n > GL_BUF ? GL_BUF : n;
        memset(&r, 0, sizeof r);
        r.h.ah = 0x3F; r.x.bx = h; r.x.cx = k; r.x.ds = gl_seg + GL_IO / 16; r.x.dx = 0;
        if (!gl_dos(&r) || r.x.ax != k) return 0;
        dosmemget(((uint32_t)gl_seg << 4) + GL_IO, k, dst);
        dst = (uint8_t *)dst + k; n -= k;
    }
    return 1;
}
static uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t le16(const uint8_t *p) { return p[0] | p[1] << 8; }

/* length of one prologue instruction we can copy anywhere (no relative operands), 0 = not supported */
static int gl_insn(const uint8_t *p)
{
    uint8_t o = p[0];
    if (o >= 0x50 && o <= 0x5F) return 1;                     /* push/pop r32 */
    if (o == 0x6A) return 2;                                  /* push imm8 */
    if (o == 0x68 || (o >= 0xB8 && o <= 0xBF)) return 5;      /* push imm32, mov r32,imm32 */
    int imm;
    switch (o) {
    case 0x01: case 0x03: case 0x09: case 0x0B: case 0x21: case 0x23: case 0x29: case 0x2B: case 0x31: case 0x33:
    case 0x39: case 0x3B: case 0x85: case 0x89: case 0x8B: case 0x8D: imm = 0; break;
    case 0x83: imm = 1; break;
    case 0x81: case 0xC7: imm = 4; break;
    default: return 0;
    }
    uint8_t m = p[1], mod = m >> 6, rm = m & 7;
    int len = 2;
    if (mod != 3 && rm == 4) { len++; if (mod == 0 && (p[2] & 7) == 5) len += 4; }   /* SIB (+disp32 base 5) */
    if (mod == 1) len += 1; else if (mod == 2) len += 4; else if (mod == 0 && rm == 5) len += 4;
    return len + imm;
}

/* no LE fixup may touch [po-5, po+n) of page pg: 0 = ok, 9/10 = reason (fixup page table + records of that page) */
static int gl_fixok(int h, const uint8_t *hd, uint32_t o, uint32_t pg, uint32_t po, int n)
{
    uint32_t p, fixsz = le32(hd + 0x30), fpt[2], fr = o + le32(hd + 0x6C);
    if (!gl_read(h, o + le32(hd + 0x68) + 4 * (pg - 1), fpt, 8) || fpt[1] < fpt[0] || fpt[1] - fpt[0] > fixsz) return 9;
    uint32_t fn = fpt[1] - fpt[0];
    if (fn > sizeof gl_ldr || !gl_read(h, fr + fpt[0], gl_ldr, fn)) return 9;
    for (p = 0; p < fn;) {
        uint8_t st = gl_ldr[p], fl = gl_ldr[p + 1], cnt = 1, list = st & 0x20;
        int32_t so = 0;
        p += 2;
        if (list) cnt = gl_ldr[p++]; else { so = (int16_t)le16(gl_ldr + p); p += 2; }
        switch (fl & 3) {
        case 0: p += fl & 0x40 ? 2 : 1; if ((st & 0xF) != 2) p += fl & 0x10 ? 4 : 2; break;
        case 1: p += fl & 0x40 ? 2 : 1; p += fl & 0x80 ? 1 : fl & 0x10 ? 4 : 2; if (fl & 4) p += fl & 0x20 ? 4 : 2; break;
        case 2: p += fl & 0x40 ? 2 : 1; p += fl & 0x10 ? 4 : 2; if (fl & 4) p += fl & 0x20 ? 4 : 2; break;
        case 3: p += fl & 0x40 ? 2 : 1; if (fl & 4) p += fl & 0x20 ? 4 : 2; break;
        }
        for (int k = 0; k < cnt; k++) {
            if (list) { so = (int16_t)le16(gl_ldr + p); p += 2; }
            if (so > (int32_t)po - 6 && so < (int32_t)(po + n)) return 10;
        }
    }
    return 0;
}

/* DLL start code (Watcom 10.x __DLLstart_, 3dfx GLIDE2X.OVL 2.31-2.43) under HDPMI32i: it asks INT 21h 35E9h for
 * DOS/4GW's own loader hand-over (ES = PSP) and else insists on ring 0 / DOS/4GW's fixed selectors -> "Run-time library
 * initialization failure: DLL load aborted" (the game aborts the same way under HDPMI32i without FRAMELOG). Patched
 * in the read buffers like grBufferSwap, found by signature in the 512 bytes after the LE entry point:
 *   66 B8 E9 35 CD 21              mov ax,35E9h / int 21h  -> CALL [p1]: ES = EBX = PSP selector (INT 21h AH=62h)
 *   66 BA 78 00 66 B8 00 FF CD 21  DOS/4G check FF00h      -> CALL [p2] + NOPs: AL = 1, GS = 0 (Rational, zero base)
 * The rest of the start code then takes its DOS/4G path with the PSP/env selectors from ES (DPMI PSP layout). */
static void gl_dllfix(int h, const uint8_t *hd, uint32_t o)
{
    static const uint8_t s1[6] = { 0x66, 0xB8, 0xE9, 0x35, 0xCD, 0x21 },
                         s2[10] = { 0x66, 0xBA, 0x78, 0x00, 0x66, 0xB8, 0x00, 0xFF, 0xCD, 0x21 };
    static const uint8_t t1[12] = { 0x50, 0xB4, 0x62, 0xCD, 0x21, 0x8E, 0xC3, 0x58, 0xC3 },   /* push eax / mov ah,62h / int 21h / mov es,bx / pop eax / ret */
                         t2[12] = { 0x31, 0xC0, 0x8E, 0xE8, 0xB0, 0x01, 0xC3 };              /* xor eax,eax / mov gs,ax / mov al,1 / ret */
    uint8_t w[512], oe[24];
    uint32_t ps = le32(hd + 0x28), eobj = le32(hd + 0x18), eip = le32(hd + 0x1C);
    if (!eobj || eobj > le32(hd + 0x44) || !gl_read(h, o + le32(hd + 0x40) + 24 * (eobj - 1), oe, 24)) return;
    uint32_t pg0 = le32(oe + 12), ef = le32(hd + 0x80) + (pg0 - 1) * ps + eip;   /* entry's file offset (pages in order) */
    if (!gl_read(h, ef, w, sizeof w)) return;
    uint32_t L = ((uint32_t)gl_seg << 4) + GL_DLL;
    for (int k = 0; k < 2; k++) {
        const uint8_t *sg = k ? s2 : s1; int n = k ? sizeof s2 : sizeof s1, at = -1;
        for (int i = 0; i + n <= (int)sizeof w; i++) if (!memcmp(w + i, sg, n)) { at = i; break; }
        if (at < 0) return;                                        /* other start code: leave it alone */
        uint32_t off = eip + at, pg = pg0 + off / ps, po = off % ps;
        if (po + n > ps || gl_fixok(h, hd, o, pg, po, n)) return;
        uint8_t t[16];
        uint32_t ta = L + 16 * k, ptr = ta + 12;
        memcpy(t, k ? t2 : t1, 12); memcpy(t + 12, &ta, 4);
        dosmemput(t, 16, ta);
        typeof(gl_pt[0]) *q = &gl_pt[gl_np];
        q->tgt = ef + at; q->n = n; q->done = 0;
        q->b[0] = 0xFF; q->b[1] = 0x15; memcpy(q->b + 2, &ptr, 4); memset(q->b + 6, 0x90, n - 6);
        gl_np++;
    }
}

/* parse the OVL (handle h): 0 = armed, else a reason number (St.glide_err) */
static int gl_parse(int h)
{
    uint8_t hd[0xC4], code[24];
    uint32_t o;
    if (!gl_read(h, 0x3C, &o, 4) || !gl_read(h, o, hd, sizeof hd) || hd[0] != 'L' || hd[1] != 'E') return 1;
    uint32_t ldrsz = le32(hd + 0x38), ps = le32(hd + 0x28);
    if (ldrsz > sizeof gl_ldr || !gl_read(h, o + 0xC4, gl_ldr, ldrsz)) return 2;
    /* export by name (resident names, case-insensitive: 3dfx OVLs export upper case) */
    static const char nm[] = "_GRBUFFERSWAP@4";
    uint32_t ord = 0, p = le32(hd + 0x58) - 0xC4;
    while (p < ldrsz && gl_ldr[p]) {
        uint8_t n = gl_ldr[p];
        if (n == sizeof nm - 1 && !strncasecmp((char *)gl_ldr + p + 1, nm, n)) { ord = le16(gl_ldr + p + 1 + n); break; }
        p += 3 + n;
    }
    if (!ord) return 3;
    uint32_t obj = 0, off = 0, cur = 1;
    for (p = le32(hd + 0x5C) - 0xC4; p + 4 < ldrsz && gl_ldr[p] && !obj;) {
        uint8_t cnt = gl_ldr[p], typ = gl_ldr[p + 1];
        if (typ == 0) { cur += cnt; p += 2; continue; }
        if (typ != 3) return 4;
        for (int k = 0; k < cnt; k++, cur++)
            if (cur == ord) { obj = le16(gl_ldr + p + 2); off = le32(gl_ldr + p + 4 + 5 * k + 1); }
        p += 4 + 5 * cnt;
    }
    if (!obj || obj > le32(hd + 0x44)) return 5;
    uint32_t ot = le32(hd + 0x40) - 0xC4 + 24 * (obj - 1), pg = le32(gl_ldr + ot + 12) + off / ps, po = off % ps;
    if (po + sizeof code > ps) return 6;                      /* prologue across a page: not handled */
    gl_np = 0;
    uint32_t tgt = le32(hd + 0x80) + (pg - 1) * ps + po;
    if (!gl_read(h, tgt, code, sizeof code)) return 7;
    int n = 0, l;
    while (n < 6) { if (!(l = gl_insn(code + n))) return 8; n += l; }
    if (n > 16) return 8;
    if ((l = gl_fixok(h, hd, o, pg, po, n))) return l;
    /* thunk + patch */
    uint32_t L = ((uint32_t)gl_seg << 4) + GL_THUNK, ret = L + 20 + n + 6, ptr = ret + 4;
    uint8_t t[64];
    static const uint8_t mid[14] = { 0x50, 0x52, 0xB8, 0x00, 0x00, 0x4C, 0x47, 0x66, 0xBA, 0xD4, 0x03, 0xEC, 0x5A, 0x58 };
    t[0] = 0x8F; t[1] = 0x05; memcpy(t + 2, &ret, 4);
    memcpy(t + 6, mid, sizeof mid);
    memcpy(t + 20, code, n);
    t[20 + n] = 0xFF; t[21 + n] = 0x25; memcpy(t + 22 + n, &ret, 4);
    memset(t + 26 + n, 0, 4); memcpy(t + 30 + n, &L, 4);
    dosmemput(t, 34 + n, L);
    gl_pt[0].tgt = tgt; gl_pt[0].b[0] = 0xFF; gl_pt[0].b[1] = 0x15; memcpy(gl_pt[0].b + 2, &ptr, 4);
    memset(gl_pt[0].b + 6, 0x90, n - 6);
    gl_pt[0].n = n; gl_pt[0].done = 0; gl_np = 1;
    gl_dllfix(h, hd, o);
    return 0;
}

static void gl_open_cb(void)
{
    uint32_t sp = ((uint32_t)gl_open_reg.w.ss << 4) + gl_open_reg.w.sp;   /* [far return] flags 'FL' request */
    if (_farpeekw(_dos_ds, sp + 6) == 0x464C) sp += 8;
    else if (_farpeekw(_dos_ds, sp + 2) == 0x464C) sp += 4;              /* return address already popped */
    else return;
    uint16_t hostcs = _farpeekw(_dos_ds, ((uint32_t)gl_seg << 4) + GLSTUB_CB3 + 6);
    if (hostcs && _farpeekw(_dos_ds, sp + 4) != hostcs) return;          /* not from a protected-mode program */
    uint16_t req = _farpeekw(_dos_ds, sp), ofs = (req >> 8) == 0x3D ? gl_open_reg.w.dx : gl_open_reg.w.si;
    uint32_t a = ((uint32_t)gl_open_reg.w.ds << 4) + ofs;
    char name[128];
    int i, b = 0;
    for (i = 0; i < 127 && (name[i] = _farpeekb(_dos_ds, a + i)); i++)
        if (name[i] == '\\' || name[i] == '/' || name[i] == ':') b = i + 1;
    name[i] = 0;
    if (strcasecmp(name + b, "GLIDE2X.OVL")) return;
    St.glide_opens++;
    /* own handle: open the same name (same drive/dir/PATH context as the game) */
    __dpmi_regs r;
    memset(&r, 0, sizeof r);
    dosmemput(name, i + 1, ((uint32_t)gl_seg << 4) + GL_IO);
    gl_busy(1);
    r.x.ax = 0x3D00; r.x.ds = gl_seg + GL_IO / 16; r.x.dx = 0;
    int e = 11;
    if (gl_dos(&r)) {
        int h = r.x.ax;
        e = gl_parse(h);
        memset(&r, 0, sizeof r); r.h.ah = 0x3E; r.x.bx = h; gl_dos(&r);
    }
    gl_busy(0);
    St.glide_err = e;
    if (!e) _farpokew(_dos_ds, ((uint32_t)gl_seg << 4) + GLSTUB_CB3, gl_open_reg.w.ax);   /* arm the game's handle */
}

static void gl_read_cb(void)
{
    uint32_t n = gl_read_reg.w.ax, buf = ((uint32_t)gl_read_reg.w.ds << 4) + gl_read_reg.w.dx;
    __dpmi_regs r;
    memset(&r, 0, sizeof r);
    r.x.ax = 0x4201; r.x.bx = gl_read_reg.w.bx;                /* position after the read */
    gl_busy(1);
    int ok = gl_dos(&r);
    gl_busy(0);
    if (!ok) return;
    uint32_t end = ((uint32_t)r.x.dx << 16) | r.x.ax, start = end - n;
    for (int k = 0; k < gl_np; k++) {
        typeof(gl_pt[0]) *t = &gl_pt[k];
        for (int i = 0; i < t->n; i++)
            if (t->tgt + i >= start && t->tgt + i < end) { _farpokeb(_dos_ds, buf + t->tgt + i - start, t->b[i]); t->done |= 1u << i; }
        if (t->n && t->done == (1u << t->n) - 1) { if (k) St.glide_dllfix++; else St.glide_patched++; t->done = 0; }
    }
}

/* 0 = installed */
static int gl_install(void)
{
    int sel, seg = __dpmi_allocate_dos_memory((GL_IO + GL_BUF) / 16, &sel);
    if (seg < 0) return 1;
    gl_seg = (uint16_t)seg;
    uint32_t cb = DPMI_AllocateRMCB_RETF(gl_open_cb, &gl_open_reg), cb2 = DPMI_AllocateRMCB_RETF(gl_read_cb, &gl_read_reg);
    if (!cb || !cb2) return 2;
    uint8_t code[sizeof glstub_bin];
    memcpy(code, glstub_bin, sizeof code);
    uint32_t old = _farpeekl(_dos_ds, 0x21 * 4);
    memcpy(code + GLSTUB_CB, &cb, 4); memcpy(code + GLSTUB_OLD, &old, 4); memcpy(code + GLSTUB_CB2, &cb2, 4);
    dosmemput(code, sizeof code, (uint32_t)seg << 4);
    asm volatile("cli");
    _farpokel(_dos_ds, 0x21 * 4, (uint32_t)seg << 16);
    asm volatile("sti");
    __dpmi_regs r;                                             /* learn the DPMI host's real-mode CS */
    memset(&r, 0, sizeof r);
    r.x.ax = 0x464C; r.x.bx = 0x474C;
    __dpmi_int(0x21, &r);
    St.glide = 1;
    return 0;
}

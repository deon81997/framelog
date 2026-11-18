/* FRAMELOG.EXE - DOS frame-time logger TSR (DJGPP, resident under HDPMI32i, coexists with NUKED SB-EMU).
 *
 *   FRAMELOG [/COM:n|hex[,baud]] [/AUTO] [/3DA[:ms]] [/NOVESA] [/NOMETA] [/NOGLIDE]   load (default COM1, 115200); /AUTO = log from now, no host byte
 *       needed; /3DA = frames are the vertical retraces the game waits for (input status 3DAh polled), for games
 *       that copy to the screen instead of flipping pages. Opt-in: every 3DAh read is trapped (costly in vsync loops).
 *       VESA: INT 10h 4F07h (set display start, BL 00h/80h/02h/82h) = a page flip too (real-mode INT 10h vector,
 *       which DOS extenders reflect protected-mode INT 10h to); /NOVESA leaves INT 10h alone.
 *       Metadata: after the H header an 'M' block (gpu, driver, cpu from VBE 4F00h + CPUID at load; res/depth/mode of
 *       the current mode), and a fresh block when the game sets a mode (INT 10h AH=00h / 4F02h); /NOMETA = none.
 *       Glide: GLIDE2X.OVL's grBufferSwap = a frame (INT 21h stub patches it while the game loads the OVL,
 *       glide/glhook.c, docs/GLIDE.md); /NOGLIDE leaves INT 21h alone.
 *   FRAMELOG /STATUS                        print the resident copy's counters
 *
 * Traps CRTC 3D4h/3D5h (protected-mode programs, HDPMI port trap). A write to start-address register 0Ch/0Dh =
 * page flip: the trap stores one RDTSC in a ring. The same trap then pumps at most one UART FIFO load of records
 * (docs/PROTOCOL.md) and reads the host's S/X bytes. No IRQ0, no PIT, no INT 1Ch, no port 60h, no keyboard.
 * GPL-2.0-or-later (uses NUKED SB-EMU's hdpmipt.c, dpmi lib and dbg.h UART code). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <sys/nearptr.h>
#include "dpmi/dpmi.h"
#include "hdpmipt.h"
#include "uart.h"
#include "flstate.h"
#include "flmeta.h"

static FLSTATE St;                   /* everything the trap touches lives in our DS */
static UartPort U;
static QEMM_IOPT iopt;
static uint8_t crtc_index;
static uint64_t last_sa;             /* TSC of the last start-address write (burst merge) */
static int mode3da;                  /* /3DA: frames = vsync edges seen by the game, CRTC writes only pump */
static uint8_t last3da;              /* last 3DAh value the game read */
static uint64_t last3da_t;           /* TSC of the game's previous 3DAh read */
static int armed3da = 1;             /* a new wait started (the game was away from 3DAh for arm_ticks) */
static uint32_t arm_ms = 6;
static QEMM_IOPT iopt3da;
static uint64_t ring[FL_RINGN];      /* flip timestamps, written by the trap, drained by the pump */
static volatile uint32_t rhead, rtail;
static uint64_t prev_frame;          /* TSC of the previous frame sent (or of the begin marker) */
static uint32_t lost_pending;

/* metadata ('M' records): gpu/driver/cpu text built at load (foreground, VBE + CPUID); the mode part is added when a
 * block is sent: at start and once after each mode set (meta_dirty) - never per frame, never INT 10h from a trap */
static int nometa;
static char meta_pre[FLM_TEXT];      /* gpu=, driver=, cpu= lines */
static int meta_prelen;
static char meta_txt[FLM_TEXT];
static uint8_t meta_rec[FLM_MAXREC];
static volatile int meta_dirty;      /* the game set a mode: send a fresh block before the next frame */
static volatile int vmode = -1;      /* current VESA mode (100h-1FFh), -1 = a VGA BIOS mode (40:49) */
#define FL_VTABN 160
static struct { uint16_t mode, w, h; uint8_t bpp, text; } vtab[FL_VTABN];   /* VBE mode list + 4F01h, at load */
static int vtabn;

static inline uint64_t rdtsc(void) { uint32_t lo, hi; asm volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }

/* queue one record; the UART ring is 1 KB, records are <= 13 bytes: if it is full the record is dropped (counted) */
static void put_rec(uint8_t type, const void *p, int n)
{
    if (UART_RING - (U.head - U.tail) < (uint32_t)n + 3) { St.txlost++; return; }
    uint8_t sum = type; const uint8_t *b = p;
    U.ring[U.head++ & (UART_RING - 1)] = 0xA5;
    U.ring[U.head++ & (UART_RING - 1)] = type;
    for (int i = 0; i < n; i++) { U.ring[U.head++ & (UART_RING - 1)] = b[i]; sum += b[i]; }
    U.ring[U.head++ & (UART_RING - 1)] = sum;
}

/* VGA register read without our trap seeing it; the index register is put back */
static uint8_t vga_rd(uint16_t ip, uint8_t idx)
{
    uint8_t save = HDPMIPT_UntrappedIO_Read(ip);
    HDPMIPT_UntrappedIO_Write(ip, idx);
    uint8_t v = HDPMIPT_UntrappedIO_Read(ip + 1);
    HDPMIPT_UntrappedIO_Write(ip, save);
    return v;
}

/* prepared text + the current mode -> meta_rec; returns its size. VGA modes: 40:49, and for 13h the CRTC geometry
 * (Mode X 320x240 / 360 wide) + sequencer chain-4 (off = "X"); a few port reads, once per block. */
static int meta_build(void)
{
    FLM_MODE m = {0, 0, 0, 0};
    char name[16];
    int ok = 0, tl = meta_prelen;
    memcpy(meta_txt, meta_pre, meta_prelen + 1);
    int vm = vmode, bm = 0, unch = 0;
    if (vm >= 0) {
        int i = 0;
        while (i < vtabn && vtab[i].mode != vm) i++;
        if (i < vtabn) { m.w = vtab[i].w; m.h = vtab[i].h; m.bpp = vtab[i].bpp; m.text = vtab[i].text; ok = 1; }
    } else {
        bm = _farpeekb(_dos_ds, 0x449) & 0x7F;
        ok = flm_vga_mode(bm, &m);
        if (bm == 0x13) {
            flm_vga256(vga_rd(0x3D4, 0x01), vga_rd(0x3D4, 0x07), vga_rd(0x3D4, 0x09), vga_rd(0x3D4, 0x12), &m);
            unch = !(vga_rd(0x3C4, 0x04) & 0x08);
        }
    }
    flm_name(name, vm, bm, unch, m.text);
    tl = flm_mode_text(meta_txt, tl, FLM_TEXT, ok ? &m : 0, name);
    return flm_encode(meta_txt, tl, meta_rec, sizeof meta_rec);
}

static void put_meta(void)
{
    int n = meta_build();
    if (!n || UART_RING - (U.head - U.tail) < (uint32_t)n) { St.txlost++; return; }
    for (int i = 0; i < n; i++) U.ring[U.head++ & (UART_RING - 1)] = meta_rec[i];
    St.metas++;
}

static void start_log(uint64_t now)
{
    struct __attribute__((packed)) { uint8_t ver, src; uint32_t hz; uint8_t modes, res; } h = { 1, 0, St.tsc_hz, (mode3da ? 2 : 1) | (St.vesa ? 4 : 0) | (St.glide ? 0x20 : 0), 0 };
    put_rec('H', &h, 8);
    meta_dirty = 0;
    if (!nometa) put_meta();
    put_rec('B', &now, 8);
    prev_frame = now; St.frames = 0; St.lost = 0; lost_pending = 0;
    rtail = rhead;                   /* flips from before the start are not frames of this log */
    St.logging = 1; St.starts++;
}

static void stop_log(void)
{
    uint32_t e[2] = { St.frames, St.lost };
    put_rec('E', e, 8);
    St.logging = 0;
}

/* host bytes + ring -> records -> UART, at most one FIFO load (called from the trap only) */
static void pump(void)
{
    uint8_t lsr = inportb(U.base + 5);
    while (lsr & 0x01) {             /* host commands, all bytes in the RX FIFO, in order */
        uint8_t c = inportb(U.base);
        if (c == 'S') { if (!St.logging) start_log(rdtsc()); }   /* repeated S (host retries) = no restart */
        else if (c == 'X') { if (St.logging) stop_log(); }
        St.rx++;
        lsr = inportb(U.base + 5);
    }
    if (meta_dirty && St.logging && UART_RING - (U.head - U.tail) >= FLM_MAXREC) { meta_dirty = 0; put_meta(); }
    while (St.logging && !meta_dirty && rtail != rhead && UART_RING - (U.head - U.tail) >= 16) {   /* frames wait for a pending M */
        uint64_t t = ring[rtail++ & (FL_RINGN - 1)], d = t - prev_frame;
        prev_frame = t;
        if (lost_pending) { put_rec('D', &lost_pending, 4); lost_pending = 0; }
        uint16_t f = (uint16_t)++St.frames;
        if (d >> 32) { struct __attribute__((packed)) { uint16_t f; uint64_t d; } g = { f, d }; put_rec('G', &g, 10); }
        else { struct __attribute__((packed)) { uint16_t f; uint32_t d; } r = { f, (uint32_t)d }; put_rec('F', &r, 6); }
    }
    if ((lsr & 0x20) && U.head != U.tail) St.sent += uart_pump(&U);
}

#define FL_PMMAGIC 0x464Cu           /* EAX >> 16 of the VBE PM thunk's IN 3D4h (see vbepm_cb) */
static void vesa_flip(void);
static void gl_flip(void);
#define FL_GLMAGIC 0x474Cu           /* 'GL' = EAX >> 16 of the Glide grBufferSwap thunk's IN 3D4h (glide/glhook.c) */

static void frame_event(uint64_t now)
{
    if (St.logging) {
        if (rhead - rtail < FL_RINGN) ring[rhead++ & (FL_RINGN - 1)] = now;
        else { St.lost++; lost_pending++; }
    }
    St.flips++;
    pump();
}

/* /3DA: the game reads input status 1. One frame = the first 0 -> 1 change of bit 3 (vertical retrace) the game sees
 * after it came back to 3DAh from other work (no 3DAh read for arm_ms). So a game that waits 2 retraces per frame in
 * one polling loop (35 fps on a 70 Hz mode) counts once per frame, not once per retrace. arm_ms (/3DA:ms, default 6)
 * must be longer than what stops the game's loop mid-wait (SBEMU + SC-55 ISR up to 3.6 ms on a PII-450 + trap time:
 * 4 ms gave 10% double counts there, 6 ms none) and shorter than the game's own work between waits. */
static uint32_t vsync_trap(uint32_t port, uint32_t val, uint32_t out)
{
    St.traps3da++;
    if (out) { HDPMIPT_UntrappedIO_Write((uint16_t)port, (uint8_t)val); return val; }
    uint8_t v = HDPMIPT_UntrappedIO_Read((uint16_t)port);
    uint64_t now = rdtsc();
    if (now - last3da_t > St.arm_ticks) armed3da = 1;
    last3da_t = now;
    if ((v & 8) && !(last3da & 8)) {           /* retrace edge: a frame if armed; pump once per refresh anyway */
        if (armed3da) { armed3da = 0; frame_event(now); } else pump();
    }
    last3da = v;
    return (val & ~0xFFu) | v;
}

static uint32_t crtc_trap(uint32_t port, uint32_t val, uint32_t out)
{
    /* HDPMI: OUT DX,AX = two byte calls, the 3D5h half taken from our return >> 8 -> return EAX unchanged;
     * byte calls carry stale EAX upper bits -> AL only. See HANDOFF 1a. */
    St.traps++;
    if (!out) {
        uint8_t v = HDPMIPT_UntrappedIO_Read((uint16_t)port);
        if (port == 0x3D4 && (val >> 16) == FL_PMMAGIC && St.vesa) vesa_flip();   /* VBE PM SetDisplayStart thunk */
        else if (port == 0x3D4 && (val >> 16) == FL_GLMAGIC && St.glide) gl_flip();   /* Glide grBufferSwap thunk */
        return v;
    }
    uint8_t v = (uint8_t)val;
    HDPMIPT_UntrappedIO_Write((uint16_t)port, v);
    if (port == 0x3D4) { crtc_index = v; return val; }
    if (crtc_index != 0x0C && crtc_index != 0x0D) return val;
    uint64_t now = rdtsc();
    if (now - last_sa > St.merge_ticks) {        /* first write of a flip */
        last_sa = now;
        if (mode3da) pump(); else frame_event(now);   /* one LSR read per flip when idle */
    } else {
        last_sa = now;
        if (St.logging && U.head != U.tail) pump();
    }
    return val;
}

/* VESA: a real-mode stub on the INT 10h vector (tools/vesastub.asm -> vesastub.h, at seg+1:0 of our DOS block):
 *  - AX=4F07h, BL 00h/02h/80h/82h (set / schedule display start) -> vesa_cb through a DPMI real-mode callback = a flip;
 *  - AX=4F0Ah BL=0 (VBE 2.0 protected-mode interface, Build/Duke 3D use it to flip): the BIOS runs, then vbepm_cb copies
 *    the table into our block (seg+5:0) and points its SetDisplayStart entry at a 19-byte 32-bit thunk that does
 *    IN AL,3D4h with 'FL' in EAX's upper half (crtc_trap sees it: a flip, no side effect) and jumps to the original code.
 *    The game copies or calls our copy instead of the BIOS one; the code is position-independent (VBE 2.0 rule).
 *  - AH=00h / AX=4F02h (set mode): the BIOS runs, then mode_cb notes the new mode (metadata; /NOMETA = cb3 0, skipped).
 * Same burst merge as the CRTC trap (last_sa): a VBE BIOS that writes CRTC 0Ch/0Dh while we also trap them = one flip. */
#include "vesastub.h"
#define VBEPM_SEG  8                 /* table copy at seg+8:0 (stub at seg+1:0, <= 112 bytes) */
#define VBEPM_MAX  1024              /* longest BIOS table we copy (+ 19-byte thunk) */
static const uint8_t vbepm_thunk[19] = { 0x50, 0x52, 0xB8,0x00,0x00,0x4C,0x46, 0x66,0xBA,0xD4,0x03, 0xEC, 0x5A, 0x58, 0xE9 };
static DPMI_REG vesa_reg, vbepm_reg, mode_reg;
static uint16_t blkseg;
static uint8_t tbl[VBEPM_MAX + sizeof vbepm_thunk];
#ifdef VESADBG   /* diagnostic build (make VESADBG=1): INT 10h 4F07h/4F0Ah calls -> one text line each on the UART, polled */
static void dbg_out(const char *t) { for (; *t; t++) { while (!(inportb(U.base + 5) & 0x20)) ; outportb(U.base, *t); } }
#endif
static void vesa_flip(void)
{
    St.vesa_calls++;
    uint64_t now = rdtsc();
    if (now - last_sa > St.merge_ticks) { last_sa = now; if (mode3da) pump(); else frame_event(now); }
    else last_sa = now;
}
static void vesa_cb(void)
{
#ifdef VESADBG
    { char b[64]; sprintf(b, "V AX=%04X BX=%04X CX=%04X DX=%04X\r\n", vesa_reg.w.ax, vesa_reg.w.bx, vesa_reg.w.cx, vesa_reg.w.dx); dbg_out(b); }
#endif
    vesa_flip();
}
static void vbepm_cb(void)
{
    uint16_t n = vbepm_reg.w.cx;
    St.vbepm++;
#ifdef VESADBG
    { char b[64]; sprintf(b, "P AX=%04X ES:DI=%04X:%04X CX=%04X\r\n", vbepm_reg.w.ax, vbepm_reg.w.es, vbepm_reg.w.di, n); dbg_out(b); }
#endif
    if (vbepm_reg.w.ax != 0x004F || n < 8 || n > VBEPM_MAX) { St.vbepm_skip++; return; }
    dosmemget(((uint32_t)vbepm_reg.w.es << 4) + vbepm_reg.w.di, n, tbl);
    uint16_t orig = tbl[2] | tbl[3] << 8;
    if (orig >= n) { St.vbepm_skip++; return; }
    memcpy(tbl + n, vbepm_thunk, sizeof vbepm_thunk);
    int32_t rel = (int32_t)orig - (int32_t)(n + sizeof vbepm_thunk);
    memcpy(tbl + n + 15, &rel, 4);
    tbl[2] = (uint8_t)n; tbl[3] = (uint8_t)(n >> 8);
    dosmemput(tbl, n + sizeof vbepm_thunk, (uint32_t)(blkseg + VBEPM_SEG) << 4);
    vbepm_reg.w.es = blkseg + VBEPM_SEG; vbepm_reg.w.di = 0; vbepm_reg.w.cx = n + sizeof vbepm_thunk;
}

static void mode_cb(void)
{   /* runs inside the game's INT 10h call, after the BIOS: no INT 10h of our own, just note the mode */
    uint32_t sp = ((uint32_t)mode_reg.w.ss << 4) + mode_reg.w.sp;   /* stub stack: [far return] 'FL' AX BX */
    if (_farpeekw(_dos_ds, sp + 4) == 0x464C) sp += 6;
    else if (_farpeekw(_dos_ds, sp) == 0x464C) sp += 2;               /* return address already popped */
    else { vmode = -1; St.modesets++; meta_dirty = 1; return; }       /* unknown stack: 40:49 */
    uint16_t ax = _farpeekw(_dos_ds, sp), bx = _farpeekw(_dos_ds, sp + 2);
    if (ax == 0x4F02) {
        if (mode_reg.w.ax != 0x004F) return;     /* failed: mode unchanged */
        vmode = (bx & 0x1FF) >= 0x100 ? (bx & 0x1FF) : -1;
    } else vmode = -1;
    St.modesets++;
    meta_dirty = 1;
}

#include "glide/glhook.c"

static QEMM_IODT iodt[2] = { {0x3D4, crtc_trap}, {0x3D5, crtc_trap} };
static QEMM_IODT iodt3da[1] = { {0x3DA, vsync_trap} };

/* TSC rate from the BIOS tick count (0040:006C, 65536/1193182 s per tick): reads RAM only, never the PIT */
static uint32_t tsc_calibrate(void)
{
    uint32_t t0 = _farpeekl(_dos_ds, 0x46C), t;
    while ((t = _farpeekl(_dos_ds, 0x46C)) == t0) ;
    uint64_t a = rdtsc();
    while (_farpeekl(_dos_ds, 0x46C) - t < 10) ;
    uint64_t b = rdtsc();
    return (uint32_t)((b - a) * 1193182ull / (10ull * 65536ull));
}

/* metadata at load (foreground, before the INT 10h stub): VBE 4F00h strings + mode list (4F01h each), 4F03h current
 * mode, CPUID. Builds meta_pre (gpu/driver/cpu lines). */
static void rmstr(const uint8_t *p, char *d, int max)
{
    uint32_t fp; memcpy(&fp, p, 4);
    d[0] = 0;
    if (!fp) return;
    dosmemget(((fp >> 16) << 4) + (fp & 0xFFFF), max - 1, d);
    d[max - 1] = 0;
}
static void cpuid(uint32_t leaf, uint32_t r[4])
{
    asm volatile("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3]) : "a"(leaf), "c"(0));
}
static int has_cpuid(void)
{
    uint32_t a, b;
    asm volatile("pushfl; pushfl; popl %0; movl %0, %1; xorl $0x200000, %0; pushl %0; popfl; pushfl; popl %0; popfl"
                 : "=&r"(a), "=&r"(b));
    return ((a ^ b) & 0x200000) != 0;
}
static void meta_init(void)
{
    static uint8_t info[512];
    static uint16_t list[FL_VTABN];
    char oem[80], ven[80], prod[80], prev[64], gpu[160], drv[96], cpu[96];
    uint16_t ver = 0, orev = 0;
    int vbe = 0;
    __dpmi_regs r;
    dosmemput("VBE2", 4, __tb);
    memset(&r, 0, sizeof r); r.x.ax = 0x4F00; r.x.es = __tb >> 4; r.x.di = __tb & 15;
    __dpmi_int(0x10, &r);
    oem[0] = ven[0] = prod[0] = prev[0] = 0;
    if (r.x.ax == 0x004F) {
        dosmemget(__tb, sizeof info, info);
        if (!memcmp(info, "VESA", 4)) {
            vbe = 1;
            ver = info[4] | info[5] << 8;
            rmstr(info + 6, oem, sizeof oem);
            if (ver >= 0x200) { orev = info[0x14] | info[0x15] << 8; rmstr(info + 0x16, ven, sizeof ven); rmstr(info + 0x1A, prod, sizeof prod); rmstr(info + 0x1E, prev, sizeof prev); }
            uint32_t fp; memcpy(&fp, info + 0x0E, 4);     /* mode list: copy it before 4F01h can overwrite it */
            int n = 0;
            if (fp) {
                dosmemget(((fp >> 16) << 4) + (fp & 0xFFFF), sizeof list, list);
                while (n < FL_VTABN && list[n] != 0xFFFF) n++;
            }
            for (int i = 0; i < n; i++) {
                uint8_t mi[0x20];
                memset(&r, 0, sizeof r); r.x.ax = 0x4F01; r.x.cx = list[i]; r.x.es = (__tb + 512) >> 4; r.x.di = (__tb + 512) & 15;
                __dpmi_int(0x10, &r);
                if (r.x.ax != 0x004F) continue;
                dosmemget(__tb + 512, sizeof mi, mi);
                uint16_t attr = mi[0] | mi[1] << 8;
                if (!(attr & 1) || (ver < 0x102 && !(attr & 2))) continue;   /* not supported / no resolution info */
                vtab[vtabn].mode = list[i] & 0x1FF;
                vtab[vtabn].w = mi[0x12] | mi[0x13] << 8; vtab[vtabn].h = mi[0x14] | mi[0x15] << 8;
                vtab[vtabn].bpp = mi[0x19]; vtab[vtabn].text = !(attr & 0x10);
                vtabn++;
            }
            memset(&r, 0, sizeof r); r.x.ax = 0x4F03;
            __dpmi_int(0x10, &r);
            if (r.x.ax == 0x004F && (r.x.bx & 0x1FF) >= 0x100) vmode = r.x.bx & 0x1FF;
        }
    }
    flm_gpu(gpu, sizeof gpu, vbe, ver, oem, ven, prod);
    flm_driver(drv, sizeof drv, vbe, ver, orev, prev);
    char brand[49] = "", vendor[13] = "";
    int fam = 0, model = 0, step = 0;
    if (has_cpuid()) {
        uint32_t c[4];
        cpuid(0, c); memcpy(vendor, &c[1], 4); memcpy(vendor + 4, &c[3], 4); memcpy(vendor + 8, &c[2], 4);
        if (c[0] >= 1) {
            cpuid(1, c);
            step = c[0] & 15; model = (c[0] >> 4) & 15; fam = (c[0] >> 8) & 15;
            if (fam == 15) fam += (c[0] >> 20) & 0xFF;
            if (fam == 6 || fam >= 15) model += ((c[0] >> 16) & 15) << 4;
        }
        cpuid(0x80000000u, c);
        if ((c[0] & 0xFFFF0000u) == 0x80000000u && c[0] >= 0x80000004u)
            for (uint32_t i = 0; i < 3; i++) { cpuid(0x80000002u + i, c); memcpy(brand + 16 * i, c, 16); }
    }
    flm_cpu(cpu, sizeof cpu, brand, vendor, fam, model, step, (St.tsc_hz + 500000) / 1000000);
    meta_pre[0] = 0;
    meta_prelen = flm_add(meta_pre, 0, FLM_TEXT, "gpu", gpu);
    meta_prelen = flm_add(meta_pre, meta_prelen, FLM_TEXT, "driver", drv);
    meta_prelen = flm_add(meta_pre, meta_prelen, FLM_TEXT, "cpu", cpu);
}

static int find_resident(FLSTATE *out)
{   /* the DOS block holds our state's linear address; read it through a descriptor of our own */
    if (_farpeekw(_dos_ds, 0x4F2) != FL_SIG) return 0;
    uint32_t blk = (uint32_t)_farpeekw(_dos_ds, 0x4F0) << 4;
    if (_farpeekl(_dos_ds, blk) != FL_MAGIC) return 0;
    uint32_t lin = _farpeekl(_dos_ds, blk + 4);
    int sel = __dpmi_allocate_ldt_descriptors(1);
    if (sel < 0) return 0;
    __dpmi_set_segment_base_address(sel, lin); __dpmi_set_segment_limit(sel, sizeof(FLSTATE) - 1);
    movedata(sel, 0, _my_ds(), (unsigned)out, sizeof(FLSTATE));
    __dpmi_free_ldt_descriptor(sel);
    return 1;
}

static void usage(void)
{
    puts("FRAMELOG [/COM:n|hex[,baud]] [/AUTO] [/3DA] [/NOVESA] [/NOMETA] [/NOGLIDE] - load the frame-time logger (default COM1,115200)\n"
         "  /3DA[:ms]: frames = vertical retraces the game waits for (copy-to-screen games), costs CPU;\n"
         "     ms = how long the game must be away from the retrace port for a new frame (default 6)\n"
         "  /NOVESA: don't hook INT 10h (VESA 4F07h display-start calls are page flips by default)\n"
         "  /NOMETA: send no metadata (gpu, driver, cpu, resolution: 'M' records after the header)\n"
         "  /NOGLIDE: don't hook INT 21h (DOS Glide: GLIDE2X.OVL's grBufferSwap = a frame by default)\n"
         "FRAMELOG /STATUS                       - counters of the loaded copy\n"
         "Start/stop logging from the host PC (serial-bench: fllog.py). Needs HDPMI32i.");
}

int main(int argc, char **argv)
{
    const char *com = "COM1"; int autostart = 0, status = 0, novesa = 0, noglide = 0;
    for (int i = 1; i < argc; i++) {
        if (!strncasecmp(argv[i], "/COM:", 5)) com = argv[i] + 5;
        else if (!strcasecmp(argv[i], "/AUTO")) autostart = 1;
        else if (!strcasecmp(argv[i], "/STATUS")) status = 1;
        else if (!strcasecmp(argv[i], "/NOVESA")) novesa = 1;
        else if (!strcasecmp(argv[i], "/NOMETA")) nometa = 1;
        else if (!strcasecmp(argv[i], "/NOGLIDE")) noglide = 1;
        else if (!strncasecmp(argv[i], "/3DA", 4) && (argv[i][4] == 0 || argv[i][4] == ':')) {
            mode3da = 1; if (argv[i][4]) arm_ms = (uint32_t)atoi(argv[i] + 5);
            if (arm_ms < 1 || arm_ms > 100) { usage(); return 1; }
        }
        else { usage(); return 1; }
    }
    FLSTATE r;
    if (find_resident(&r)) {
        if (!status) { printf("FRAMELOG is already loaded (COM %lX). FRAMELOG /STATUS shows its counters.\n", (unsigned long)r.base); return 1; }
        printf("FRAMELOG %s: UART %lX %lu baud, TSC %lu Hz, %s, %s\n", FL_VER, (unsigned long)r.base, (unsigned long)r.baud,
               (unsigned long)r.tsc_hz, r.mode3da ? "/3DA vsync mode" : "page-flip mode", r.logging ? "LOGGING" : "idle (waiting for the host's S)");
        printf("  flips seen %lu, frames logged %lu, lost %lu, bytes sent %lu, record drops %lu, host bytes %lu, starts %lu, trap calls %lu\n",
               (unsigned long)r.flips, (unsigned long)r.frames, (unsigned long)r.lost, (unsigned long)r.sent,
               (unsigned long)r.txlost, (unsigned long)r.rx, (unsigned long)r.starts, (unsigned long)r.traps);
        if (r.mode3da) printf("  3DAh reads trapped %lu\n", (unsigned long)r.traps3da);
        printf("  VESA hook %s: display-start calls %lu (INT 10h 4F07h + VBE 2.0 PM interface), PM interface requests %lu (%lu not patched)\n",
               r.vesa ? "on" : "off (/NOVESA)", (unsigned long)r.vesa_calls, (unsigned long)r.vbepm, (unsigned long)r.vbepm_skip);
        if (r.meta) printf("  metadata on: blocks sent %lu, mode sets seen %lu (INT 10h AH=00h / 4F02h)\n", (unsigned long)r.metas, (unsigned long)r.modesets);
        else puts("  metadata off (/NOMETA)");
        if (r.glide) printf("  Glide hook on: GLIDE2X.OVL opens %lu, patched loads %lu, grBufferSwap calls %lu, last result %lu%s, DLL start fixes %lu\n",
                            (unsigned long)r.glide_opens, (unsigned long)r.glide_patched, (unsigned long)r.glide_swaps,
                            (unsigned long)r.glide_err, r.glide_err ? " (not hooked, docs/GLIDE.md)" : "", (unsigned long)r.glide_dllfix);
        if (r.glide) printf("  Static Glide: EXE/BDD files scanned %lu, grBufferSwap patched in %lu, last error %lu, scan time %lu ms\n",
                            (unsigned long)r.glide_stscan, (unsigned long)r.glide_static, (unsigned long)r.glide_sterr,
                            (unsigned long)(r.glide_stus / 1000));
        else puts("  Glide hook off (/NOGLIDE)");
        return 0;
    }
    if (status) { puts("FRAMELOG is not loaded."); return 1; }

    int c, lvl; uint16_t base; uint32_t baud;
    if (uart_parse(com, &c, &base, &baud, &lvl) < 0) { usage(); return 1; }
    if (c) base = _farpeekw(_dos_ds, 0x400 + (c - 1) * 2);
    if (!base) { printf("FRAMELOG: COM%d has no port address in the BIOS data area\n", c); return 2; }
    DPMI_Init();
    if (!HDPMIPT_Detect()) { puts("FRAMELOG: needs HDPMI32i (HDPMI32I.EXE -r -x); not loaded."); return 2; }
    if (uart_init(&U, base, baud, 1) < 0) { printf("FRAMELOG: no UART at %X\n", base); return 2; }

    memset(&St, 0, sizeof St);
    St.magic = FL_MAGIC; St.base = base; St.baud = U.baud;
    St.tsc_hz = tsc_calibrate();
    St.merge_ticks = St.tsc_hz / 2000;           /* 0.5 ms */
    St.arm_ticks = St.tsc_hz / 1000 * arm_ms;
    last_sa = 0;
    St.meta = !nometa;
    if (!nometa) meta_init();
    if (novesa) vmode = -1;          /* no INT 10h hook: mode from 40:49 only (standard VGA modes) */

    int sel, seg = __dpmi_allocate_dos_memory(novesa ? 1 : VBEPM_SEG + (VBEPM_MAX + sizeof vbepm_thunk + 15) / 16, &sel);
    /* 16 bytes id, INT 10h stub at seg+1:0 (<= 112 bytes), VBE PM table copy at seg+8:0 */
    blkseg = (uint16_t)seg;
    if (seg < 0) { puts("FRAMELOG: no DOS memory"); return 3; }
    uint32_t blk = (uint32_t)seg << 4;
    unsigned long dsbase; __dpmi_get_segment_base_address(_my_ds(), &dsbase);
    if (!HDPMIPT_Install_IOPortTrap(0x3D4, 0x3D5, iodt, 2, &iopt)) { puts("FRAMELOG: port trap failed"); return 4; }
    if (mode3da && !HDPMIPT_Install_IOPortTrap(0x3DA, 0x3DA, iodt3da, 1, &iopt3da)) { puts("FRAMELOG: 3DAh trap failed"); return 4; }
    St.mode3da = mode3da;
    if (!novesa) {
        uint32_t cb = DPMI_AllocateRMCB_RETF(vesa_cb, &vesa_reg), cb2 = DPMI_AllocateRMCB_RETF(vbepm_cb, &vbepm_reg);
        uint32_t cb3 = nometa ? 0 : DPMI_AllocateRMCB_RETF(mode_cb, &mode_reg);
        if (!cb || !cb2 || (!nometa && !cb3)) { puts("FRAMELOG: no real-mode callback for the VESA hook (try /NOVESA)"); return 4; }
        uint8_t code[sizeof vesastub_bin];
        memcpy(code, vesastub_bin, sizeof code);
        uint32_t old = _farpeekl(_dos_ds, 0x10 * 4);
        memcpy(code + VESASTUB_CB, &cb, 4); memcpy(code + VESASTUB_OLD, &old, 4); memcpy(code + VESASTUB_CB2, &cb2, 4); memcpy(code + VESASTUB_CB3, &cb3, 4);
        movedata(_my_ds(), (unsigned)code, _dos_ds, blk + 16, sizeof code);
        asm volatile("cli");
        _farpokel(_dos_ds, 0x10 * 4, (uint32_t)(seg + 1) << 16);
        asm volatile("sti");
        St.vesa = 1;
    }
    if (!noglide && gl_install()) { puts("FRAMELOG: Glide hook failed (try /NOGLIDE)"); return 4; }
    if (autostart) start_log(rdtsc());
    _farpokel(_dos_ds, blk, FL_MAGIC); _farpokel(_dos_ds, blk + 4, dsbase + (uint32_t)&St);
    _farpokew(_dos_ds, 0x4F0, seg); _farpokew(_dos_ds, 0x4F2, FL_SIG);
    printf("FRAMELOG %s loaded: UART %X %lu baud%s, CPU %lu MHz, %s (protected-mode games)%s%s%s%s\n",
           FL_VER, base, (unsigned long)U.baud, U.fifo == 16 ? "" : " (no FIFO)", (unsigned long)(St.tsc_hz / 1000000),
           mode3da ? "frames = vsync waits (3DAh trapped, /3DA)" : "frames = page flips (CRTC 3D4h-3D5h)", St.vesa ? ", VESA 4F07h" : "", St.glide ? ", Glide" : "", nometa ? ", no metadata" : "", autostart ? ", logging now" : "");
    if (!nometa) { fputs(meta_pre, stdout); printf("  VBE modes known: %d, current mode: %s\n", vtabn, vmode >= 0 ? "VESA" : "VGA"); }
    fflush(stdout);
    DPMI_TSR();
    puts("FRAMELOG: going resident failed");
    return 5;
}

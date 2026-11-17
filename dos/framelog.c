/* FRAMELOG.EXE - DOS frame-time logger TSR (DJGPP, resident under HDPMI32i, coexists with NUKED SB-EMU).
 *
 *   FRAMELOG [/COM:n|hex[,baud]] [/AUTO]   load (default COM1, 115200); /AUTO = log from now, no host byte needed
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

static FLSTATE St;                   /* everything the trap touches lives in our DS */
static UartPort U;
static QEMM_IOPT iopt;
static uint8_t crtc_index;
static uint64_t last_sa;             /* TSC of the last start-address write (burst merge) */
static uint64_t ring[FL_RINGN];      /* flip timestamps, written by the trap, drained by the pump */
static volatile uint32_t rhead, rtail;
static uint64_t prev_frame;          /* TSC of the previous frame sent (or of the begin marker) */
static uint32_t lost_pending;

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

static void start_log(uint64_t now)
{
    struct __attribute__((packed)) { uint8_t ver, src; uint32_t hz; uint8_t modes, res; } h = { 1, 0, St.tsc_hz, 1, 0 };
    put_rec('H', &h, 8);
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
    while (St.logging && rtail != rhead && UART_RING - (U.head - U.tail) >= 16) {
        uint64_t t = ring[rtail++ & (FL_RINGN - 1)], d = t - prev_frame;
        prev_frame = t;
        if (lost_pending) { put_rec('D', &lost_pending, 4); lost_pending = 0; }
        uint16_t f = (uint16_t)++St.frames;
        if (d >> 32) { struct __attribute__((packed)) { uint16_t f; uint64_t d; } g = { f, d }; put_rec('G', &g, 10); }
        else { struct __attribute__((packed)) { uint16_t f; uint32_t d; } r = { f, (uint32_t)d }; put_rec('F', &r, 6); }
    }
    if ((lsr & 0x20) && U.head != U.tail) St.sent += uart_pump(&U);
}

static uint32_t crtc_trap(uint32_t port, uint32_t val, uint32_t out)
{
    /* HDPMI: OUT DX,AX = two byte calls, the 3D5h half taken from our return >> 8 -> return EAX unchanged;
     * byte calls carry stale EAX upper bits -> AL only. See HANDOFF 1a. */
    St.traps++;
    if (!out) return HDPMIPT_UntrappedIO_Read((uint16_t)port);
    uint8_t v = (uint8_t)val;
    HDPMIPT_UntrappedIO_Write((uint16_t)port, v);
    if (port == 0x3D4) { crtc_index = v; return val; }
    if (crtc_index != 0x0C && crtc_index != 0x0D) return val;
    uint64_t now = rdtsc();
    if (now - last_sa > St.merge_ticks) {        /* first write of a flip */
        if (St.logging) {
            if (rhead - rtail < FL_RINGN) ring[rhead++ & (FL_RINGN - 1)] = now;
            else { St.lost++; lost_pending++; }
        }
        St.flips++;
        last_sa = now;
        pump();                                   /* one LSR read per flip when idle */
    } else {
        last_sa = now;
        if (St.logging && U.head != U.tail) pump();
    }
    return val;
}

static QEMM_IODT iodt[2] = { {0x3D4, crtc_trap}, {0x3D5, crtc_trap} };

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
    puts("FRAMELOG [/COM:n|hex[,baud]] [/AUTO]  - load the frame-time logger (default COM1,115200)\n"
         "FRAMELOG /STATUS                       - counters of the loaded copy\n"
         "Start/stop logging from the host PC (serial-bench: framelog_log.py). Needs HDPMI32i.");
}

int main(int argc, char **argv)
{
    const char *com = "COM1"; int autostart = 0, status = 0;
    for (int i = 1; i < argc; i++) {
        if (!strncasecmp(argv[i], "/COM:", 5)) com = argv[i] + 5;
        else if (!strcasecmp(argv[i], "/AUTO")) autostart = 1;
        else if (!strcasecmp(argv[i], "/STATUS")) status = 1;
        else { usage(); return 1; }
    }
    FLSTATE r;
    if (find_resident(&r)) {
        if (!status) { printf("FRAMELOG is already loaded (COM %lX). FRAMELOG /STATUS shows its counters.\n", (unsigned long)r.base); return 1; }
        printf("FRAMELOG %s: UART %lX %lu baud, TSC %lu Hz, %s\n", FL_VER, (unsigned long)r.base, (unsigned long)r.baud,
               (unsigned long)r.tsc_hz, r.logging ? "LOGGING" : "idle (waiting for the host's S)");
        printf("  flips seen %lu, frames logged %lu, lost %lu, bytes sent %lu, record drops %lu, host bytes %lu, starts %lu, trap calls %lu\n",
               (unsigned long)r.flips, (unsigned long)r.frames, (unsigned long)r.lost, (unsigned long)r.sent,
               (unsigned long)r.txlost, (unsigned long)r.rx, (unsigned long)r.starts, (unsigned long)r.traps);
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
    last_sa = 0;

    int sel, seg = __dpmi_allocate_dos_memory(1, &sel);
    if (seg < 0) { puts("FRAMELOG: no DOS memory"); return 3; }
    uint32_t blk = (uint32_t)seg << 4;
    unsigned long dsbase; __dpmi_get_segment_base_address(_my_ds(), &dsbase);
    if (!HDPMIPT_Install_IOPortTrap(0x3D4, 0x3D5, iodt, 2, &iopt)) { puts("FRAMELOG: port trap failed"); return 4; }
    if (autostart) start_log(rdtsc());
    _farpokel(_dos_ds, blk, FL_MAGIC); _farpokel(_dos_ds, blk + 4, dsbase + (uint32_t)&St);
    _farpokew(_dos_ds, 0x4F0, seg); _farpokew(_dos_ds, 0x4F2, FL_SIG);
    printf("FRAMELOG %s loaded: UART %X %lu baud%s, CPU %lu MHz, traps CRTC 3D4h-3D5h (protected-mode games)%s\n",
           FL_VER, base, (unsigned long)U.baud, U.fifo == 16 ? "" : " (no FIFO)", (unsigned long)(St.tsc_hz / 1000000),
           autostart ? ", logging now" : "");
    fflush(stdout);
    DPMI_TSR();
    puts("FRAMELOG: going resident failed");
    return 5;
}

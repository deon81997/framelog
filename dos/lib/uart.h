/* uart.h - polled 16550 UART for FRAMELOG, from NUKED SB-EMU tools/dbg.h (GPL-2.0-or-later, same author):
 * no UART interrupt, no DOS/BIOS calls; uart_pump moves at most one FIFO load per call (trap-safe). */
#ifndef FL_UART_H
#define FL_UART_H
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pc.h>
#define UART_RING     1024u          /* power of two */
#define UART_DEFBAUD  115200ul
typedef struct
{
    uint16_t base;
    uint8_t  level, fifo;
    uint32_t baud;
    volatile uint32_t head, tail;
    uint32_t lost, sent;
    uint8_t ring[UART_RING];
} UartPort;
/* "COM1,115200,2" -> base/baud/level; 0 ok, -1 bad. base from the BIOS data area for COM1..4 (bda = BDA word or 0) */
static inline int uart_parse(const char* s, int* com, uint16_t* base, uint32_t* baud, int* level)
{
    unsigned long v; char* e;
    *com = 0; *base = 0; *baud = UART_DEFBAUD; *level = 1;
    if (!s || !*s) return -1;
    if (!strncasecmp(s, "COM", 3)) { s += 3; if (*s < '1' || *s > '4') return -1; *com = *s++ - '0'; }
    else
    {
        v = strtoul(s, &e, 16); if (e == s) return -1;
        if (v >= 1 && v <= 4 && e == s + 1) *com = (int)v; else if (v >= 0x100 && v <= 0xFFF8) *base = (uint16_t)v; else return -1;
        s = e;
    }
    while (*s == ',' || *s == ':')
    {
        v = strtoul(++s, &e, 10); if (e == s) return -1; s = e;
        if (v >= 1 && v <= 3) *level = (int)v; else if (v >= 300 && v <= 115200) *baud = v; else return -1;
    }
    return *s ? -1 : 0;
}

/* program the UART: 8N1, divisor, FIFO on, DTR+RTS (OUT2 off: no UART interrupts). 0 ok, -1 no UART there */
static inline int uart_init(UartPort* d, uint16_t base, uint32_t baud, int level)
{
    uint16_t div = (uint16_t)(115200ul / (baud ? baud : UART_DEFBAUD)); uint8_t iir;
    if (!div) div = 1;
    outportb(base + 7, 0x5A); if (inportb(base + 7) != 0x5A) { outportb(base + 3, 0x03); if (inportb(base + 3) != 0x03) return -1; }
    outportb(base + 1, 0);                       /* no UART interrupts */
    outportb(base + 3, 0x80); outportb(base + 0, div & 0xFF); outportb(base + 1, div >> 8);
    outportb(base + 3, 0x03);
    outportb(base + 2, 0x07);                    /* FIFO on + clear */
    iir = inportb(base + 2);
    outportb(base + 4, 0x03);
    d->fifo = ((iir & 0xC0) == 0xC0) ? 16 : 1;
    d->base = base; d->baud = 115200ul / div; d->level = (uint8_t)level;
    d->head = d->tail = 0; d->lost = d->sent = 0;
    return 0;
}

/* move bytes from the ring to the UART while it takes them without waiting: at most one FIFO load (16 bytes) */
static inline int uart_pump(UartPort* d)
{
    int n = 0;
    if (!d->base || d->head == d->tail) return 0;
    if (!(inportb(d->base + 5) & 0x20)) return 0;
    while (n < d->fifo && d->tail != d->head) { outportb(d->base, d->ring[d->tail & (UART_RING - 1)]); d->tail++; n++; }
    d->sent += (uint32_t)n;
    return n;
}

#endif

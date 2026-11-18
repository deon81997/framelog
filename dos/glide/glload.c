/* glload.c - minimal loader for an LE DLL (GLIDE2X.OVL) in a DOS/4GW program, a stand-in for the Tenberry "__loadme"
 * DLL loader that DOS Glide games link from 3dfx's glide2x.lib (plain DOS/4GW 1.97 has no DLL support: error 2302).
 * Like that loader it reads the file with plain DOS reads (open/lseek/read -> DOS/4GW -> real-mode INT 21h), so
 * FRAMELOG's INT 21h read hook sees the same traffic. Objects are malloc'd, internal fixups applied (off32, rel32,
 * sel16, ptr16:32), then the DLL's start code is called once (Watcom startup, returns non-zero = ok).
 * Not a general loader: no imports, no 16-bit objects. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include "glload.h"

static unsigned long u32(const unsigned char *p) { return p[0] | p[1] << 8 | (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24; }
static unsigned u16(const unsigned char *p) { return p[0] | p[1] << 8; }

static int rd(int fd, long off, void *dst, unsigned n)
{
    return lseek(fd, off, SEEK_SET) == off && read(fd, dst, n) == (int)n;
}

static unsigned short ds_sel(void) { unsigned short s; __asm { mov s, ds } return s; }
static unsigned short cs_sel(void) { unsigned short s; __asm { mov s, cs } return s; }

int gl_load(const char *path, GLDLL *m, char *err)
{
    unsigned char h[0xC4], *ldr = 0, *fix = 0;
    unsigned long o, ldrsz, fixsz, nobj, i, pg;
    int fd = open(path, O_RDONLY | O_BINARY);
    memset(m, 0, sizeof *m);
    if (fd < 0) { sprintf(err, "can't open %s", path); return 0; }
    if (!rd(fd, 0x3C, &o, 4) || !rd(fd, o, h, sizeof h) || h[0] != 'L' || h[1] != 'E') { strcpy(err, "not LE"); goto bad; }
    m->pagesize = u32(h + 0x28); nobj = u32(h + 0x44); ldrsz = u32(h + 0x38); fixsz = u32(h + 0x30);
    if (u32(h + 0x74)) { strcpy(err, "has imports"); goto bad; }
    if (nobj > GL_MAXOBJ) { strcpy(err, "too many objects"); goto bad; }
    /* loader + fixup sections follow the header: object table, page map, names, entries, fixup page table + records */
    ldr = malloc(ldrsz + fixsz + 16);
    if (!ldr || !rd(fd, o + 0xC4, ldr, ldrsz + fixsz)) { strcpy(err, "read loader section"); goto bad; }
    m->nobj = nobj;
    for (i = 0; i < nobj; i++) {
        const unsigned char *e = ldr + u32(h + 0x40) - 0xC4 + 24 * i;
        unsigned long vs = u32(e), npg = u32(e + 16), first = u32(e + 12), k, raw, n;
        m->size[i] = vs; m->flags[i] = u32(e + 8);
        raw = (unsigned long)malloc(vs + 4096 + 16);
        if (!raw) { strcpy(err, "no memory"); goto bad; }
        m->base[i] = (unsigned char *)((raw + 4095) & ~4095UL);
        memset(m->base[i], 0, vs);
        for (k = 0; k < npg; k++) {
            pg = first + k;   /* 1-based logical page = physical page in files from wlink */
            n = pg == u32(h + 0x14) ? u32(h + 0x2C) : m->pagesize;
            if (k * m->pagesize + n > vs) n = vs - k * m->pagesize;
            if (!rd(fd, u32(h + 0x80) + (pg - 1) * m->pagesize, m->base[i] + k * m->pagesize, n)) { strcpy(err, "read page"); goto bad; }
        }
        m->pg0[i] = first; m->npg[i] = npg;
    }
    /* fixups */
    fix = ldr + u32(h + 0x68) - 0xC4;   /* fixup page table (npages + 1 entries) */
    for (i = 0; i < nobj; i++) {
        unsigned long k;
        for (k = 0; k < m->npg[i]; k++) {
            unsigned long p = u32(fix + 4 * (m->pg0[i] + k - 1)), end = u32(fix + 4 * (m->pg0[i] + k));
            const unsigned char *r = ldr + u32(h + 0x6C) - 0xC4 + p, *re = ldr + u32(h + 0x6C) - 0xC4 + end;
            unsigned char *page = m->base[i] + k * m->pagesize;
            while (r < re) {
                unsigned st = r[0], fl = r[1], cnt = 1, tobj, j;
                const unsigned char *srcs;
                unsigned long toff = 0, tgt;
                r += 2;
                if (st & 0x20) { cnt = *r++; srcs = 0; } else { srcs = r; r += 2; }
                if ((fl & 3) != 0) { sprintf(err, "fixup target type %u", fl & 3); goto bad; }
                tobj = fl & 0x40 ? u16(r) : *r; r += fl & 0x40 ? 2 : 1;
                if ((st & 0xF) != 2) { toff = fl & 0x10 ? u32(r) : u16(r); r += fl & 0x10 ? 4 : 2; }
                if (st & 0x20) { srcs = r; r += 2 * cnt; }
                if (!tobj || tobj > nobj) { strcpy(err, "fixup object"); goto bad; }
                tgt = (unsigned long)m->base[tobj - 1] + toff;
                for (j = 0; j < cnt; j++) {
                    short so = (short)u16(srcs + 2 * j);
                    unsigned char *s = page + so;
                    unsigned short sel = (m->flags[tobj - 1] & 4) ? cs_sel() : ds_sel();
                    switch (st & 0xF) {
                    case 7: *(unsigned long *)s = tgt; break;
                    case 8: *(unsigned long *)s = tgt - ((unsigned long)s + 4); break;
                    case 2: *(unsigned short *)s = sel; break;
                    case 6: *(unsigned long *)s = tgt; *(unsigned short *)(s + 4) = sel; break;
                    case 5: *(unsigned short *)s = (unsigned short)tgt; break;
                    default: sprintf(err, "fixup source type %u", st & 0xF); goto bad;
                    }
                }
            }
        }
    }
    m->ldr = ldr; m->h_names = u32(h + 0x58); m->h_entry = u32(h + 0x5C);
    m->eip = (unsigned long)m->base[u32(h + 0x18) - 1] + u32(h + 0x1C);
    close(fd);
    return 1;
bad:
    free(ldr);
    close(fd);
    return 0;
}

void *gl_sym(GLDLL *m, const char *name)
{
    const unsigned char *p = m->ldr + m->h_names - 0xC4, *e;
    unsigned ord = 0, cur = 1, n = strlen(name);
    for (; *p; p += 3 + *p)
        if (*p == n && !strnicmp((const char *)p + 1, name, n)) { ord = u16(p + 1 + n); break; }
    if (!ord) return 0;
    for (e = m->ldr + m->h_entry - 0xC4; e[0];) {
        unsigned cnt = e[0], typ = e[1], k;
        if (typ == 0) { cur += cnt; e += 2; continue; }
        if (typ != 3) return 0;
        for (k = 0; k < cnt; k++, cur++)
            if (cur == ord) return m->base[u16(e + 2) - 1] + u32(e + 4 + 5 * k + 1);
        e += 4 + 5 * cnt;
    }
    return 0;
}

static unsigned long st_esp, st_eip, st_ret;
int gl_start(GLDLL *m)
{   /* The OVL's Watcom start code. Under DOS/4GW alone (ring 0) it returns 1; under a DPMI host (HDPMI32i) it exits
     * with "Run-time library initialization failure: DLL load aborted": it asks INT 21h AX=35E9h for the loader's
     * hand-over (ES = PSP, BX = writable selector) and DOS/4GW answers that itself (016F:0D49, set 25E9h ignored).
     * Glide 2.61 runs fine without it, so GLFLIP only calls it with GLINIT set. Registers + ESP kept around the call. */
    st_eip = m->eip;
    __asm {
        pushad
        push ds
        push es
        mov st_esp, esp
        call dword ptr st_eip
        mov st_ret, eax
        mov esp, st_esp
        pop es
        pop ds
        popad
    }
    return (int)st_ret;
}

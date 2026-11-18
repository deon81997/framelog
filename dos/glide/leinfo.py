#!/usr/bin/env python3
"""leinfo.py OVL [NAME]: LE (DOS/4GW DLL, e.g. GLIDE2X.OVL) exports, objects, and where NAME's entry lives
(object, offset, file offset, first bytes, fixups that touch them). Host-side reference for dos/glide/glovl.c."""
import struct, sys

def le(path):
    d = open(path, 'rb').read()
    o = struct.unpack_from('<I', d, 0x3c)[0]
    assert d[o:o+2] == b'LE', 'not LE'
    u = lambda off: struct.unpack_from('<I', d, o + off)[0]
    h = dict(o=o, pagesize=u(0x28), lastpage=u(0x2c), objtab=u(0x40), nobj=u(0x44), pgmap=u(0x48),
             resnames=u(0x58), entry=u(0x5c), fixpg=u(0x68), fixrec=u(0x6c), datapages=u(0x80),
             nonres=u(0x88), nonreslen=u(0x8c), npages=u(0x14))
    objs = []
    for i in range(h['nobj']):
        vs, base, fl, pgi, npg, _ = struct.unpack_from('<6I', d, o + h['objtab'] + 24 * i)
        objs.append(dict(vsize=vs, base=base, flags=fl, pgidx=pgi, npg=npg))
    return d, h, objs

def names(d, h):
    out = {}
    def tab(p, end=None):
        while True:
            n = d[p]
            if n == 0 or (end and p >= end): break
            nm = d[p+1:p+1+n].decode('latin1'); ordn = struct.unpack_from('<H', d, p+1+n)[0]
            out[nm] = ordn; p += 3 + n
    tab(h['o'] + h['resnames'])
    if h['nonres']: tab(h['nonres'], h['nonres'] + h['nonreslen'])
    return out

def entry(d, h, ordn):
    p = h['o'] + h['entry']; cur = 1
    while True:
        cnt, typ = d[p], d[p+1]
        if cnt == 0: return None
        if typ == 0: cur += cnt; p += 2; continue
        obj = struct.unpack_from('<H', d, p+2)[0]; q = p + 4
        sz = {1: 3, 2: 5, 3: 5, 4: 7}[typ]
        for k in range(cnt):
            if cur == ordn:
                assert typ == 3, 'only 32-bit entries'
                return obj, struct.unpack_from('<I', d, q+1)[0]
            cur += 1; q += sz
        p = q

def fileoff(h, objs, obj, off):
    ob = objs[obj-1]; pg = ob['pgidx'] + off // h['pagesize']   # 1-based logical page
    # LE page map entry: 4 bytes, page number = (b0<<16)|(b1<<8)|b2 (1-based), b3 = flags
    e = h['o'] + h['pgmap'] + 4 * (pg - 1)
    return pg, h['datapages'] + (pg - 1) * h['pagesize'] + off % h['pagesize']

def fixups(d, h, pg):
    """source offsets (within page) of fixup records for 1-based page pg"""
    a = struct.unpack_from('<I', d, h['o'] + h['fixpg'] + 4*(pg-1))[0]
    b = struct.unpack_from('<I', d, h['o'] + h['fixpg'] + 4*pg)[0]
    p = h['o'] + h['fixrec'] + a; end = h['o'] + h['fixrec'] + b; srcs = []
    while p < end:
        st, fl = d[p], d[p+1]; p += 2
        if st & 0x20: n = d[p]; p += 1; lst = list(struct.unpack_from('<%dh' % n, d, p)); p += 2*n
        else: lst = [struct.unpack_from('<h', d, p)[0]]; p += 2
        t = fl & 3
        if t == 0:
            p += 2 if fl & 0x40 else 1                     # object
            if (st & 0xf) != 2: p += 4 if fl & 0x10 else 2  # target offset (not for selector fixups)
        elif t in (1, 2):
            p += 2 if fl & 0x40 else 1; p += 2 if (t == 2 or fl & 0x80) else 1 if t == 1 else 0
            if t == 1: pass
            else: pass
            if t == 2: p += 0
            if fl & 4: p += 4 if fl & 0x20 else 2
        else: p += 2 if fl & 0x40 else 1
        if st & 0x20: p += 0
        if t != 0 and (fl & 4) == 0 and t != 3: pass
        srcs += [(s, st & 0xf) for s in lst]
    return srcs

if __name__ == '__main__':
    d, h, objs = le(sys.argv[1]); nm = names(d, h)
    want = sys.argv[2] if len(sys.argv) > 2 else '_GRBUFFERSWAP@4'
    print('objects', [(hex(o['base']), hex(o['vsize']), hex(o['flags'])) for o in objs], 'pagesize', h['pagesize'], 'exports', len(nm))
    obj, off = entry(d, h, nm[want])
    pg, fo = fileoff(h, objs, obj, off)
    print('%s ord %d obj %d off %#x page %d fileoff %#x' % (want, nm[want], obj, off, pg, fo))
    print('bytes', d[fo:fo+24].hex(' '))
    po = off % h['pagesize']
    print('fixups near', [s for s in fixups(d, h, pg) if po - 4 < s[0] < po + 16])

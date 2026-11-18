#!/usr/bin/env python3
"""mkpak.py OUT.PAK name=file ... - tiny Quake II pak (the Q2 3.14 demo would not exec a loose baseq2/bench.cfg)."""
import sys, struct
ents, data = [], b''
for a in sys.argv[2:]:
    n, f = a.split('=', 1); b = open(f, 'rb').read(); ents.append((n, 12 + len(data), len(b))); data += b
d = b''.join(struct.pack('<56sii', n.encode(), o, l) for n, o, l in ents)
open(sys.argv[1], 'wb').write(struct.pack('<4sii', b'PACK', 12 + len(data), len(d)) + data + d)

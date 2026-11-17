#!/usr/bin/env python3
"""chkimp98.py EXE/DLL... - check every import against Windows 98 SE's system DLL exports (w98exports.txt,
made from the store-w98qi VM's C:\\WINDOWS\\SYSTEM with --make DIR). Exit 1 if anything is missing.
Imports from DLLs not in the list (our own, glide2x...) are skipped."""
import sys, os, pefile
here = os.path.dirname(os.path.abspath(__file__))
db = os.path.join(here, 'w98exports.txt')
if len(sys.argv) > 2 and sys.argv[1] == '--make':
    with open(db, 'w') as o:
        for f in sorted(os.listdir(sys.argv[2])):
            pe = pefile.PE(os.path.join(sys.argv[2], f))
            names = sorted(e.name.decode() for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name)
            o.write('[%s]\n%s\n' % (f.lower(), '\n'.join(names)))
    sys.exit(0)
ex, cur = {}, None
for l in open(db):
    l = l.strip()
    if l.startswith('['): cur = ex.setdefault(l[1:-1], set())
    elif l: cur.add(l)
bad = 0
for f in sys.argv[1:]:
    pe = pefile.PE(f)
    for d in getattr(pe, 'DIRECTORY_ENTRY_IMPORT', []):
        dll = d.dll.decode().lower()
        if dll not in ex: continue
        for i in d.imports:
            if i.name and i.name.decode() not in ex[dll]:
                print('%s: %s!%s not in Windows 98' % (f, dll, i.name.decode())); bad += 1
print('chkimp98: %d file(s), %d missing' % (len(sys.argv) - 1, bad))
sys.exit(1 if bad else 0)

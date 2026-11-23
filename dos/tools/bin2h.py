#!/usr/bin/env python3
"""bin2h.py BIN NAME ASM - C array of a nasm binary + NAME offsets of the asm's cb/old/cb2/cb3/nbank/bankon labels (from nasm -l)."""
import sys, subprocess, re, os, tempfile
b, name, asm = sys.argv[1:4]
data = open(b, 'rb').read()
lst = tempfile.mktemp()
subprocess.run(['nasm', '-f', 'bin', '-o', os.devnull, '-l', lst, asm], check=True)
offs = {}
for l in open(lst):
    m = re.match(r'\s*\d+\s+([0-9A-F]{8})\s+\S+\s+(\w+):', l)
    if m: offs[m.group(2)] = int(m.group(1), 16)
os.remove(lst)
print(f"/* generated from {os.path.basename(asm)} by tools/bin2h.py - do not edit */")
for k in ('cb', 'old', 'cb2', 'cb3', 'nbank', 'bankon'):
    print(f"#define {name.upper().replace('_BIN','')}_{k.upper()} 0x{offs[k]:02X}")
print(f"static const unsigned char {name}[{len(data)}] = {{")
for i in range(0, len(data), 16):
    print("    " + ", ".join(f"0x{x:02X}" for x in data[i:i+16]) + ",")
print("};")

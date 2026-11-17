"""metatest output (name TAB text TAB hex) -> bytes == serial-bench flproto.encode_meta(dict of the text), and
MetaText decodes them back; composition cases must give the expected text."""
import sys
sys.path.insert(0, sys.argv[1])
import flproto

EXPECT = {
    'd810': 'gpu=ATI MOBILITY RADEON X300 ATI Technologies Inc. M22|driver=VBE 3.0, OEM rev 1.00, 01.00|'
            'cpu=Intel(R) Pentium(R) M processor 2.00GHz, 1995 MHz|res=640x480|depth=16|mode=VESA 111h|',
    'nvidia': 'gpu=NVIDIA Corporation NV41 Board - p226h1|driver=VBE 3.0, OEM rev 1.00, Chip Rev A2|'
              'cpu=GenuineIntel family 15 model 2 stepping 9, 2400 MHz|res=640x480|depth=8|mode=VESA 101h|',
    'pii_vga13': 'gpu=VGA|cpu=GenuineIntel family 6 model 5 stepping 2, 450 MHz|res=320x200|depth=8|mode=13h|',
    'modex240': 'gpu=S3 Incorporated. Trio64|driver=VBE 1.2|cpu=GenuineIntel family 6 model 5 stepping 2, 450 MHz|'
                'res=320x240|depth=8|mode=X|',
    'modex360': 'gpu=3dfx Interactive, Inc. Voodoo3|driver=VBE 2.0, OEM rev 1.00|'
                'cpu=AuthenticAMD family 6 model 4 stepping 2, 1000 MHz|res=360x200|depth=8|mode=X|',
    'text03': 'gpu=Matrox Graphics Inc. G400|driver=VBE 2.0, OEM rev 1.05, Rev 1|cpu=AMD Athlon(tm) Processor, 1000 MHz|'
              'mode=text 03h|',
    'vga12': 'gpu=VGA|res=640x480|depth=4|mode=12h|',
    'vesa_unknown': 'gpu=x y z|driver=VBE 2.0, OEM rev 0.00|cpu=CyrixInstead family 5 model 2 stepping 0, 166 MHz|'
                    'mode=VESA 1FFh|',
}
fails = n = 0
seen = set()
for line in sys.stdin:
    name, text, hx = line.rstrip('\n').split('\t')
    n += 1
    seen.add(name)
    got = bytes.fromhex(hx)
    t = text.replace('|', '\n')
    d = dict(l.split('=', 1) for l in t.splitlines())
    want = flproto.encode_meta(d)
    ok = got == want
    if name in EXPECT and text != EXPECT[name]:
        ok = False
        print(f'  {name}: text {text!r}\n  want      {EXPECT[name]!r}')
    if name == 'long':
        ok = got == flproto.encode_meta({'gpu': 'G' * 200, 'cpu': 'y'})   # fits FLM_TEXT (320)
    mt, meta = flproto.MetaText(), None
    for kind, r in flproto.Decoder().feed(got):
        if kind == 'M':
            m = mt.add(r)
            meta = m if m is not None else meta
    if meta != {k.lower(): v.strip() for k, v in d.items()}:
        ok = False
        print(f'  {name}: decoded {meta}')
    print(f"{'PASS' if ok else 'FAIL'} {name}: {len(got) // 19} records{'' if ok else ' got ' + got.hex() + ' want ' + want.hex()}")
    fails += not ok
missing = set(EXPECT) - seen
if missing:
    print('FAIL missing cases', missing)
    fails += 1
print(f"metatest: {n - fails}/{n} PASS" if not fails else f"metatest: {fails} FAIL")
sys.exit(1 if fails else 0)

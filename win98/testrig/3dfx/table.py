#!/usr/bin/env python3
"""table.py RESDIR - one line per game: own fps without FRAMELOG, own fps with, FRAMELOG fps over the same window
(Q2: last 632 frames; Expendable/Incoming: their own frame count ending at the last frame before the result
screen = the longest run of frames), plus CSV gpu/api. Reads go.sh output (Q2*.LOG, EXP*.TXT, INC*.TXT, UT*.LOG, csv/)."""
import sys, os, re, glob
R = sys.argv[1]
def rd(p):
    try: return open(os.path.join(R, p), errors='replace').read()
    except OSError: return ''
def own(part, s):
    if part.startswith('q2'):
        m = re.findall(r'(\d+) frames, ([\d.]+) seconds: ([\d.]+) fps', rd(f'Q2{part[2:]}{s}.LOG')); return (float(m[-1][2]), int(m[-1][0])) if m else None
    if part == 'exp':
        t = rd(f'EXP{s}.TXT'); a = re.search(r'Average FPS\s*-\s*([\d.]+)', t); n = re.search(r'Gameframes\s*-\s*(\d+)', t)
        return (float(a.group(1)), int(n.group(1))) if a else None
    if part == 'inc':
        m = re.match(r'\s*([\d.]+)', rd(f'INC{s}.TXT')); return (float(m.group(1)), 5000) if m else None
    if part == 'ut':
        m = re.findall(r'(\d+) frames rendered in ([\d.]+) seconds\. ([\d.]+) FPS', rd(f'UT{s}.LOG')); return (float(m[-1][2]), int(m[-1][0])) if m else None
def csvs():
    out = []
    for f in sorted(glob.glob(os.path.join(R, 'csv', '*.csv'))):
        meta, t = {}, []
        for l in open(f, errors='replace'):
            if l.startswith('# ') and '=' in l: k, v = l[2:].strip().split('=', 1); meta.setdefault(k, v)
            elif l[:1].isdigit(): t.append(float(l.split(',')[1]))
        out.append((meta, t))
    return out
def win(t, n, mode):
    if len(t) <= n: return None
    if mode == 'last': e = len(t) - 1
    else:  # end of the steadiest n-frame stretch: the window whose span is closest to n / own fps is chosen by caller
        e = len(t) - 1
    return n * 1000 / (t[e] - t[e - n])
def best(t, n, fps):  # window of n frames whose fps is nearest the game's own number (games show a result screen after)
    if len(t) <= n: return None
    b = min(range(n, len(t)), key=lambda e: abs(n * 1000 / (t[e] - t[e - n]) - fps))
    return n * 1000 / (t[b] - t[b - n]), len(t) - 1 - b
parts = [p for p in ('q2mgl', 'q2icd', 'q2soft', 'exp', 'inc', 'ut') if own(p, 0) or own(p, 1)]
cs = csvs()
for i, p in enumerate(parts):
    o0, o1 = own(p, 0), own(p, 1)
    meta, t = cs[i] if i < len(cs) else ({}, [])
    fl = ''
    if o1 and t:
        if p.startswith('q2'): fl = f"{win(t, o1[1], 'last'):.2f} (last {o1[1]})"
        else:
            tgt = o1[0] / float(os.environ.get('UTDIL', '1.1')) if p == 'ut' else o1[0]  # UT Hardcore: TimeDemo divides real time by TimeDilation 1.1
            r = best(t, o1[1], tgt); fl = f"{r[0]:.2f} ({o1[1]} frames, ends {r[1]} before the last)" if r else 'n/a'
    print(f"{p:7s} off {o0[0] if o0 else '-':>8} on {o1[0] if o1 else '-':>8} FRAMELOG {fl:40s} csv {len(t)} frames gpu={meta.get('gpu','-')} api={meta.get('api','-')} game={meta.get('pc_game','-')}")

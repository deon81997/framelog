#!/usr/bin/env python3
"""csvbuck.py CSV [BUCKET_S] [FROM_S TO_S]: fllog CSV -> frames + fps + median/max frame time per time bucket (and window)."""
import sys, statistics as st
rows = [l.split(',') for l in open(sys.argv[1]) if l[:1].isdigit()]
t = [float(r[1]) / 1000 for r in rows]; ft = [float(r[2]) for r in rows]
b = float(sys.argv[2]) if len(sys.argv) > 2 else 10
k = {}
for a, f in zip(t, ft): k.setdefault(int(a // b), []).append(f)
for i in sorted(k): v = k[i]; print(f"{i*b:5.0f}s {len(v):5d} fr {len(v)/b:6.1f} fps  med {st.median(v):6.2f} max {max(v):7.1f} ms")
if len(sys.argv) > 4:
    lo, hi = float(sys.argv[3]), float(sys.argv[4]); v = [f for a, f in zip(t, ft) if lo <= a < hi]
    print(f"window {lo}-{hi}s: {len(v)} frames, {len(v)/(hi-lo):.2f} fps, avg {sum(v)/len(v):.2f} ms, min {min(v):.2f} max {max(v):.2f}")
print("total", len(ft), "frames")

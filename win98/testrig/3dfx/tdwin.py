#!/usr/bin/env python3
"""tdwin.py CSV N [skip] - FRAMELOG fps over a game's own timedemo window: N frames ending `skip` frames before the
end (Q2: skip 0, the game quits right after its timedemo). Prints frames total, window fps (N / span of N intervals)
and whole-run fps."""
import sys
t = [float(l.split(',')[1]) for l in open(sys.argv[1]) if l[:1].isdigit()]
n = int(sys.argv[2]); s = int(sys.argv[3]) if len(sys.argv) > 3 else 0
e = len(t) - 1 - s
print(f"frames {len(t)} window {n}: {n * 1000 / (t[e] - t[e - n]):.2f} fps  whole {(len(t) - 1) * 1000 / (t[-1] - t[0]):.2f} fps")

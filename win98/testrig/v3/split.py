#!/usr/bin/env python3
"""split.py fl.csv [--gap MS] [--min N] - split one fllog CSV (a whole VM boot) into runs at a frame gap > MS
(default 1000) or a restart (frame 1 / comment block), print per run: frames, seconds, avg fps (frames-1 / span),
median / p99 frame time, and the run's game/api/res comment keys."""
import sys, statistics

def runs(path, gap, minf):
    out, cur, meta, pending = [], [], {}, {}
    def close():
        if len(cur) >= minf:
            out.append((dict(meta), list(cur)))
        cur.clear()
    for line in open(path, errors='replace'):
        line = line.strip()
        if not line:
            continue
        if line.startswith('#'):
            kv = line[1:].strip()
            if '=' in kv and not kv.startswith('change@'):
                k, v = kv.split('=', 1)
                pending[k.strip()] = v.strip()
            elif kv.startswith('change@'):
                for p in kv.split(' ')[1:] if ' ' in kv else []:
                    if '=' in p:
                        k, v = p.split('=', 1); pending[k] = v
            continue
        if line.startswith('frame'):
            continue
        try:
            f, t, ft = line.split(',')[:3]
            f, t, ft = int(f), float(t), float(ft)
        except ValueError:
            continue
        if cur and (f == 1 or ft > gap or ft < 0):
            close()
        if not cur:
            meta.update(pending); pending.clear()
            cur.append((t, None))      # origin frame: its time is the gap before it, not a frame time
            continue
        cur.append((t, ft))
    close()
    return out

def main():
    a = sys.argv[1:]
    gap = float(a[a.index('--gap') + 1]) if '--gap' in a else 1000.0
    minf = int(a[a.index('--min') + 1]) if '--min' in a else 30
    for i, (m, r) in enumerate(runs(a[0], gap, minf), 1):
        fts = [ft for _, ft in r if ft is not None]
        span = (r[-1][0] - r[0][0]) / 1000.0
        fps = (len(r) - 1) / span if span > 0 else 0
        s = sorted(fts)
        p99 = s[min(len(s) - 1, int(len(s) * 0.99))]
        print(f"run {i}: {len(r)} frames {span:.2f} s avg {fps:.2f} fps median {statistics.median(fts):.2f} ms "
              f"p99 {p99:.2f} ms max {s[-1]:.1f} | {m.get('game','?')} {m.get('api','?')} {m.get('res','?')}x{m.get('depth','?')}")

if __name__ == '__main__':
    main()

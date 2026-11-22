import csv, sys, statistics as st
def load(p): return {r['file'].split('/')[-1]: r for r in csv.DictReader(open(p)) if r.get('file')}
a, b = load(sys.argv[1]), load(sys.argv[2]); ok = lambda s: s.strip().lower() == 'ok'
both = [k for k in a if k in b and ok(a[k]['status']) and ok(b[k]['status'])]
same = [k for k in both if a[k]['fnv1a'] == b[k]['fnv1a']]
print(f"files={len(a)} both-ok={len(both)} pixel-identical={len(same)}")
print("hash differs:", [k for k in both if k not in same])
print("only libavif ok:", [k for k in a if k in b and ok(a[k]['status']) and not ok(b[k]['status'])])
print("only crabby ok:", [k for k in a if k in b and ok(b[k]['status']) and not ok(a[k]['status'])])
r = [float(b[k]['median_ms']) / float(a[k]['median_ms']) for k in same if float(a[k]['median_ms']) > 0]
print(f"crabby/libavif decode-time ratio (identical-output files): median {st.median(r):.2f} min {min(r):.2f} max {max(r):.2f}")

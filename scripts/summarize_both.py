import csv, sys, statistics as st
for p in sys.argv[1:]:
    R = [r for r in csv.DictReader(open(p)) if r.get('file')]
    ok = [r for r in R if r['l_status'].lower() == 'ok' and r['c_status'].lower() == 'ok']
    ratios = [float(r['ratio']) for r in ok if float(r['l_median_ms']) > 0]
    big = [float(r['ratio']) for r in ok if float(r['l_median_ms']) >= 20]
    diff = [r for r in ok if r['identical'] != '1']
    print(f"\n== {p.split('/')[-1]}: {len(R)} files, both-ok {len(ok)}")
    print(f"ratio crabby/libavif: median {st.median(ratios):.3f}  (files >=20ms: median {st.median(big) if big else float('nan'):.3f}, n={len(big)})  min {min(ratios):.2f} max {max(ratios):.2f}")
    print(f"total time ratio: {sum(float(r['c_median_ms']) for r in ok)/sum(float(r['l_median_ms']) for r in ok):.3f}")
    print(f"pixel-different files: {len(diff)}; worst max_diff = {max([int(r['max_diff']) for r in diff], default=0)} /255; worst % bytes differing = {max([float(r['pct_bytes_diff']) for r in diff], default=0):.2f}%")
    for r in diff[:8]: print("   ", r['file'].split('/')[-1], 'depth', r['depth'], 'max_diff', r['max_diff'], f"{float(r['pct_bytes_diff']):.2f}% bytes")
    only = [r['file'].split('/')[-1] for r in R if (r['l_status'].lower() == 'ok') != (r['c_status'].lower() == 'ok')]
    print("one library fails:", only)

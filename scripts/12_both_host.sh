#!/usr/bin/env bash
# Same interleaved harness on the Mac (arm64 host). Needs 10_host.sh to have run once.
source "$(dirname "$0")/common.sh"; P="$OUT/host"; B="$ROOT/bench"
c++ -O2 -std=c++17 -I"$B" -I"$P/prefix_libavif/include" -c "$B/backend_libavif.cc" -o "$P/bl.o"
c++ -O2 -std=c++17 -I"$B" -I"$TP/CrabbyAvif/include" -c "$B/backend_crabby.cc" -o "$P/bc.o"
c++ -O2 -std=c++17 -I"$B" -c "$B/bench_both.cc" -o "$P/bb.o"
c++ -o "$P/bench_both" "$P/bb.o" "$P/bl.o" "$P/bc.o" "$P/prefix_libavif/lib/libavif.a" "$P/crabby/release/libcrabby_avif.a" "$P/prefix/lib/libdav1d.a" -lm
"$P/bench_both" "${ROUNDS:-9}" "$TP"/photos/*.avif > "$OUT/both_host_photos.csv"
"$P/bench_both" "${ROUNDS:-9}" "$TP"/libavif/tests/data/*.avif > "$OUT/both_host_testdata.csv"
python3 "$ROOT/scripts/summarize_both.py" "$OUT/both_host_photos.csv" "$OUT/both_host_testdata.csv"

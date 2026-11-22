#!/usr/bin/env bash
# Build both decoders for the HOST (macOS arm64) on the same dav1d, then run the corpus.
source "$(dirname "$0")/common.sh"; P="$OUT/host"; mkdir -p "$P"
meson setup "$P/dav1d" "$TP/dav1d" --prefix "$P/prefix" --libdir lib --default-library=static \
  -Dbuildtype=release -Denable_tools=false -Denable_tests=false >/dev/null 2>&1 || true
ninja -C "$P/dav1d" install >/dev/null
cmake -S "$TP/libavif" -B "$P/libavif" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DAVIF_CODEC_DAV1D=SYSTEM -DAVIF_LIBYUV=OFF -DAVIF_LIBSHARPYUV=OFF -DAVIF_BUILD_APPS=OFF -DAVIF_BUILD_TESTS=OFF \
  -DCMAKE_PREFIX_PATH="$P/prefix" -DCMAKE_INSTALL_PREFIX="$P/prefix_libavif" >/dev/null
ninja -C "$P/libavif" install >/dev/null
( cd "$TP/CrabbyAvif" && PKG_CONFIG_PATH="$P/prefix/lib/pkgconfig" CARGO_TARGET_DIR="$P/crabby" \
  cargo build --release --no-default-features --features capi,dav1d )
DAV="$P/prefix/lib/libdav1d.a"
c++ -O2 -o "$P/bench_libavif" "$ROOT/bench/decode_bench.cc" -I"$P/prefix_libavif/include" "$P/prefix_libavif/lib/libavif.a" "$DAV" -lm
c++ -O2 -DBACKEND_CRABBY -o "$P/bench_crabby" "$ROOT/bench/decode_bench.cc" -I"$TP/CrabbyAvif/include" "$P/crabby/release/libcrabby_avif.a" "$DAV" -lm
D="$TP/libavif/tests/data"
"$P/bench_libavif" 5 "$D"/*.avif > "$OUT/host_libavif.csv"
"$P/bench_crabby"  5 "$D"/*.avif > "$OUT/host_crabby.csv"
python3 "$ROOT/scripts/compare.py" "$OUT/host_libavif.csv" "$OUT/host_crabby.csv"

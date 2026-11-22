#!/usr/bin/env bash
# Interleaved head-to-head on the phone: ONE binary, both decoders, alternating per file. Needs 20_android.sh to have run once (builds the libs).
source "$(dirname "$0")/common.sh"
: "${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME}"; API=29; P="$OUT/android"; SER="${ANDROID_SERIAL:-device}"; ROUNDS="${ROUNDS:-9}"
BIN="$(echo "$ANDROID_NDK_HOME"/toolchains/llvm/prebuilt/*)/bin"; CXX="$BIN/aarch64-linux-android$API-clang++"; B="$ROOT/bench"
$CXX -O2 -std=c++17 -I"$B" -I"$P/prefix_libavif/include" -c "$B/backend_libavif.cc" -o "$P/bl.o"
$CXX -O2 -std=c++17 -I"$B" -I"$TP/CrabbyAvif/include" -c "$B/backend_crabby.cc" -o "$P/bc.o"
$CXX -O2 -std=c++17 -I"$B" -c "$B/bench_both.cc" -o "$P/bb.o"
$CXX -o "$P/bench_both" "$P/bb.o" "$P/bl.o" "$P/bc.o" "$P/prefix_libavif/lib/libavif.a" \
  "$P/crabby/aarch64-linux-android/release/libcrabby_avif.a" "$P/prefix/lib/libdav1d.a" -static-libstdc++
PIN=""; for m in "${TASKSET_MASK:-f0}" e0 c0 80; do if adb shell "taskset $m true" >/dev/null 2>&1; then PIN="taskset $m"; break; fi; done
echo "CPU pinning: ${PIN:-NONE}"; R=/data/local/tmp/avif
adb shell "mkdir -p $R/photos $R/data"; adb push "$P/bench_both" "$R/" >/dev/null; adb push "$TP/photos/." "$R/photos/" >/dev/null
adb shell "cd $R && chmod +x bench_both && $PIN ./bench_both $ROUNDS photos/*.avif" > "$OUT/both_${SER}_photos.csv"
adb shell "cd $R && $PIN ./bench_both $ROUNDS data/*.avif" > "$OUT/both_${SER}_testdata.csv"
python3 "$ROOT/scripts/summarize_both.py" "$OUT/both_${SER}_photos.csv" "$OUT/both_${SER}_testdata.csv"

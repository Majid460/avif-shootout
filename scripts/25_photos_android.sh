#!/usr/bin/env bash
# Run both already-built harnesses on the REAL-PHOTO corpus on the connected phone.
# Run 20_android.sh first (it builds the binaries). Alternates libavif/crabby over ROUNDS rounds to cancel thermal drift.
source "$(dirname "$0")/common.sh"; P="$OUT/android"; SER="${ANDROID_SERIAL:-device}"; ROUNDS="${ROUNDS:-3}"
PIN=""; for m in "${TASKSET_MASK:-f0}" e0 c0 80; do if adb shell "taskset $m true" >/dev/null 2>&1; then PIN="taskset $m"; break; fi; done
echo "CPU pinning: ${PIN:-NONE}"; R=/data/local/tmp/avif
adb shell "mkdir -p $R/photos"; adb push "$TP/photos/." "$R/photos/" >/dev/null
for i in $(seq 1 "$ROUNDS"); do
  adb shell "cd $R && $PIN ./bench_libavif 5 photos/*.avif" > "$OUT/photos_${SER}_r${i}_libavif.csv"
  adb shell "cd $R && $PIN ./bench_crabby 5 photos/*.avif"  > "$OUT/photos_${SER}_r${i}_crabby.csv"
  echo "-- round $i"; python3 "$ROOT/scripts/compare.py" "$OUT/photos_${SER}_r${i}_libavif.csv" "$OUT/photos_${SER}_r${i}_crabby.csv"
done

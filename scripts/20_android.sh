#!/usr/bin/env bash
# Cross-build both decoders for Android arm64 (API 29), push the two harness binaries + corpus, run on the connected device.
# Needs: ANDROID_NDK_HOME, meson, cmake, ninja, rustup target aarch64-linux-android, adb device.
source "$(dirname "$0")/common.sh"
: "${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME to your NDK dir}"
API=29; ABI=arm64-v8a; P="$OUT/android"; mkdir -p "$P"
TC="$(echo "$ANDROID_NDK_HOME"/toolchains/llvm/prebuilt/*)"; BIN="$TC/bin"
cat > "$P/cross.txt" <<X
[binaries]
c = '$BIN/aarch64-linux-android$API-clang'
cpp = '$BIN/aarch64-linux-android$API-clang++'
ar = '$BIN/llvm-ar'
strip = '$BIN/llvm-strip'
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
X
meson setup "$P/dav1d" "$TP/dav1d" --cross-file "$P/cross.txt" --prefix "$P/prefix" --libdir lib \
  --default-library=static -Dbuildtype=release -Denable_tools=false -Denable_tests=false -Dc_args=-fPIC >/dev/null 2>&1 || true
ninja -C "$P/dav1d" install >/dev/null
CM="-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake -DANDROID_ABI=$ABI -DANDROID_PLATFORM=android-$API"
cmake -S "$TP/libavif" -B "$P/libavif" -G Ninja $CM -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DAVIF_CODEC_DAV1D=SYSTEM -DAVIF_LIBYUV=OFF -DAVIF_LIBSHARPYUV=OFF -DAVIF_BUILD_APPS=OFF -DAVIF_BUILD_TESTS=OFF \
  -DCMAKE_PREFIX_PATH="$P/prefix" -DCMAKE_FIND_ROOT_PATH="$P/prefix" -DCMAKE_INSTALL_PREFIX="$P/prefix_libavif" >/dev/null
ninja -C "$P/libavif" install >/dev/null
( cd "$TP/CrabbyAvif" && export BINDGEN_EXTRA_CLANG_ARGS="--sysroot=$TC/sysroot --target=aarch64-linux-android$API" && export PKG_CONFIG_ALLOW_CROSS=1 PKG_CONFIG_PATH="$P/prefix/lib/pkgconfig" PKG_CONFIG_SYSROOT_DIR=/ \
  CARGO_TARGET_DIR="$P/crabby" CC_aarch64_linux_android="$BIN/aarch64-linux-android$API-clang" \
  CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER="$BIN/aarch64-linux-android$API-clang"
  cargo build --release --target aarch64-linux-android --no-default-features --features capi,dav1d )
DAV="$P/prefix/lib/libdav1d.a"; CXX="$BIN/aarch64-linux-android$API-clang++"
$CXX -O2 -o "$P/bench_libavif" "$ROOT/bench/decode_bench.cc" -I"$P/prefix_libavif/include" "$P/prefix_libavif/lib/libavif.a" "$DAV" -static-libstdc++
$CXX -O2 -DBACKEND_CRABBY -o "$P/bench_crabby" "$ROOT/bench/decode_bench.cc" -I"$TP/CrabbyAvif/include" \
  "$P/crabby/aarch64-linux-android/release/libcrabby_avif.a" "$DAV" -static-libstdc++
"$BIN/llvm-strip" -o "$P/bench_libavif.stripped" "$P/bench_libavif"; "$BIN/llvm-strip" -o "$P/bench_crabby.stripped" "$P/bench_crabby"
echo "== stripped binary size (decoder + dav1d + harness) =="; ls -l "$P"/*.stripped | awk '{print $5, $9}'
# With several devices attached, set ANDROID_SERIAL=<serial from `adb devices`> (adb reads it natively).
SER="${ANDROID_SERIAL:-device}"; TS="${TASKSET_MASK:-80}"   # 0x80 = CPU7 (OnePlus 8 prime core); override per device
adb shell getprop ro.product.model | tr -d '\r' | tee "$OUT/android_$SER.model"; adb shell "cat /proc/cpuinfo | grep -c processor" 
# Pick the first CPU mask the device accepts; fall back to no pinning (and say so).
PIN=""; for m in "$TS" f0 e0 c0 80; do if adb shell "taskset $m true" >/dev/null 2>&1; then PIN="taskset $m"; break; fi; done
echo "CPU pinning: ${PIN:-NONE (device refused taskset; timings less stable)}"; echo "${PIN:-none}" > "$OUT/android_${SER}.pin"
R=/data/local/tmp/avif; adb shell "mkdir -p $R/data"
adb push "$P/bench_libavif" "$P/bench_crabby" "$R/" >/dev/null; adb push "$TP/libavif/tests/data/." "$R/data/" >/dev/null
adb shell "cd $R && chmod +x bench_* && $PIN ./bench_libavif 15 data/*.avif" > "$OUT/android_${SER}_libavif.csv"
adb shell "cd $R && $PIN ./bench_crabby 15 data/*.avif" > "$OUT/android_${SER}_crabby.csv"
python3 "$ROOT/scripts/compare.py" "$OUT/android_${SER}_libavif.csv" "$OUT/android_${SER}_crabby.csv"

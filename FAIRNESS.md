# Fairness rules
1. Same dav1d (1.5.3, static, same build flags) under both libraries; only parse/glue/YUV->RGB differ.
2. libyuv and sharpyuv OFF on both sides, so YUV->RGB is each library's own code.
3. One harness source (bench/decode_bench.cc); single-threaded (`maxThreads = 1`).
4. Output hashed (FNV-1a over RGBA8) - speed only counts where pixels are identical.
5. Median of N runs after a warm-up decode. Report where C wins, and every file one library rejects.
6. Emulator numbers are NOT phone numbers; only a physical device goes in the article's speed charts.

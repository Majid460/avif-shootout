// Both decoders in ONE process, INTERLEAVED per file: L C L C ... so CPU-clock/thermal drift hits both equally.
// Also diffs the first frame's RGBA8 between the two libraries.
// Usage: bench_both <rounds> <file.avif>...   CSV: file,w,h,depth,l_status,c_status,frames,identical,max_diff,pct_bytes_diff,l_median_ms,c_median_ms,ratio
#include "decode_api.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static double now_ms() { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
static bool slurp(const char *p, std::vector<uint8_t> &b) { FILE *f = fopen(p, "rb"); if (!f) return false; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); b.resize(n); bool ok = fread(b.data(), 1, n, f) == (size_t)n; fclose(f); return ok; }
static double med(std::vector<double> v) { std::sort(v.begin(), v.end()); return v[v.size() / 2]; }
int main(int argc, char **argv) {
  int rounds = argc > 1 ? atoi(argv[1]) : 5; if (rounds < 1) rounds = 1;
  puts("file,w,h,depth,l_status,c_status,frames,identical,max_diff,pct_bytes_diff,l_median_ms,c_median_ms,ratio");
  for (int i = 2; i < argc; i++) {
    std::vector<uint8_t> data; if (!slurp(argv[i], data)) continue;
    DecodeOut l = decode_libavif(data.data(), data.size(), true), c = decode_crabby(data.data(), data.size(), true);  // warm-up + outcome
    int maxd = 0; double pct = 0;
    if (!l.first_rgba.empty() && l.first_rgba.size() == c.first_rgba.size()) {
      size_t nd = 0; for (size_t k = 0; k < l.first_rgba.size(); k++) { int d = abs((int)l.first_rgba[k] - (int)c.first_rgba[k]); if (d) nd++; if (d > maxd) maxd = d; }
      pct = 100.0 * nd / l.first_rgba.size();
    } else if (l.first_rgba.size() != c.first_rgba.size()) maxd = -1;
    std::vector<double> tl, tc;
    for (int k = 0; k < rounds; k++) {
      double t0, t1, t2;
      if (k & 1) {  // alternate who goes first to cancel order bias
        t0 = now_ms(); (void)decode_crabby(data.data(), data.size(), false); t1 = now_ms(); (void)decode_libavif(data.data(), data.size(), false); t2 = now_ms();
        tc.push_back(t1 - t0); tl.push_back(t2 - t1);
      } else {
        t0 = now_ms(); (void)decode_libavif(data.data(), data.size(), false); t1 = now_ms(); (void)decode_crabby(data.data(), data.size(), false); t2 = now_ms();
        tl.push_back(t1 - t0); tc.push_back(t2 - t1);
      }
    }
    double ml = med(tl), mc = med(tc);
    printf("%s,%u,%u,%u,%s,%s,%d,%d,%d,%.3f,%.3f,%.3f,%.3f\n", argv[i], l.w, l.h, l.depth, l.status.c_str(), c.status.c_str(), l.frames,
           (int)(l.hash == c.hash), maxd, pct, ml, mc, mc / ml);
    fflush(stdout);
  }
  return 0;
}

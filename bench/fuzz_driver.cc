// Mutation fuzzer, one backend per binary. Same seed => identical mutants for libavif and CrabbyAvif.
// Each mutant is decoded in a forked child (crash/hang isolation). Classes: ok, reject, crash:<SIG>, timeout, asan/ubsan, rust-panic.
// Usage: fuzz_driver <seed> <iterations> <outdir> <seed_file.avif>...
#include "decode_api.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <map>
#include <random>
#include <string>
#include <vector>
#ifdef BACKEND_CRABBY
#define DECODE decode_crabby
#else
#define DECODE decode_libavif
#endif
static std::vector<uint8_t> slurp(const char *p) { std::vector<uint8_t> b; FILE *f = fopen(p, "rb"); if (!f) return b; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); b.resize(n); if (fread(b.data(), 1, n, f) != (size_t)n) b.clear(); fclose(f); return b; }
static void mutate(std::vector<uint8_t> &d, std::mt19937_64 &r) {
  if (d.empty()) return;
  auto pos = [&]() { size_t lim = (r() % 10 < 7) ? std::min<size_t>(d.size(), 2048) : d.size(); return (size_t)(r() % lim); };
  int n = 1 + r() % 6;
  while (n--) {
    switch (r() % 8) {
      case 0: d[pos()] ^= 1u << (r() % 8); break;                                  // bit flip
      case 1: d[pos()] = (uint8_t)r(); break;                                     // random byte
      case 2: d[pos()] = (r() & 1) ? 0x00 : 0xFF; break;                          // 0 / FF
      case 3: { size_t p = pos(); for (int i = 0; i < 4 && p + i < d.size(); i++) d[p + i] = 0xFF; break; }   // huge 32-bit field
      case 4: { size_t p = pos(); for (int i = 0; i < 4 && p + i < d.size(); i++) d[p + i] = 0; break; }      // zeroed field
      case 5: d.resize(1 + r() % d.size()); break;                                // truncate
      case 6: { size_t p = pos(), l = 1 + r() % 64; if (p + l <= d.size()) d.insert(d.begin() + p, d.begin() + p, d.begin() + p + l); break; }  // duplicate chunk
      case 7: { size_t a = pos(), b = pos(); std::swap(d[a], d[b]); break; }      // swap
    }
  }
}
int main(int argc, char **argv) {
  if (argc < 5) { fprintf(stderr, "usage\n"); return 2; }
  uint64_t seed = strtoull(argv[1], 0, 10); long iters = atol(argv[2]); std::string out = argv[3];
  std::vector<std::vector<uint8_t>> seeds; std::vector<std::string> names;
  for (int i = 4; i < argc; i++) { auto b = slurp(argv[i]); if (!b.empty()) { seeds.push_back(b); names.push_back(argv[i]); } }
  std::mt19937_64 rng(seed); std::map<std::string, long> tally; int saved = 0;
  setenv("ASAN_OPTIONS", "abort_on_error=1:detect_leaks=0:allocator_may_return_null=1:max_allocation_size_mb=2048", 1);
  setenv("UBSAN_OPTIONS", "halt_on_error=1:abort_on_error=1:print_stacktrace=0", 1);
  for (long it = 0; it < iters; it++) {
    size_t si = rng() % seeds.size(); std::vector<uint8_t> m = seeds[si]; mutate(m, rng);
    char errp[] = "/tmp/fzerrXXXXXX"; int efd = mkstemp(errp);
    pid_t pid = fork();
    if (pid == 0) { dup2(efd, 2); alarm(10); DecodeOut o = DECODE(m.data(), m.size(), false); _exit(o.status == "OK" || o.status == "Ok" ? 0 : 3); }
    int st = 0; waitpid(pid, &st, 0); lseek(efd, 0, SEEK_SET); char buf[8192]; ssize_t k = read(efd, buf, sizeof buf - 1); buf[k > 0 ? k : 0] = 0; close(efd); unlink(errp);
    std::string cls;
    if (WIFEXITED(st)) cls = WEXITSTATUS(st) == 0 ? "ok" : WEXITSTATUS(st) == 3 ? "reject" : "exit" + std::to_string(WEXITSTATUS(st));
    else { int s = WTERMSIG(st); if (s == SIGALRM) cls = "timeout"; else if (strstr(buf, "panicked at")) cls = "rust-panic"; else if (strstr(buf, "AddressSanitizer")) cls = "asan"; else if (strstr(buf, "runtime error")) cls = "ubsan"; else cls = std::string("crash:") + strsignal(s); }
    tally[cls]++;
    if (cls != "ok" && cls != "reject" && saved < 40) {
      char fn[512]; snprintf(fn, sizeof fn, "%s/crash_%s_%ld.avif", out.c_str(), cls.c_str(), it); FILE *f = fopen(fn, "wb"); if (f) { fwrite(m.data(), 1, m.size(), f); fclose(f); }
      snprintf(fn, sizeof fn, "%s/crash_%s_%ld.txt", out.c_str(), cls.c_str(), it); f = fopen(fn, "w"); if (f) { fprintf(f, "seed=%s\n%.2000s\n", names[si].c_str(), buf); fclose(f); } saved++;
    }
  }
  printf("{\"seed\":%llu,\"iterations\":%ld", (unsigned long long)seed, iters);
  for (auto &kv : tally) printf(",\"%s\":%ld", kv.first.c_str(), kv.second);
  printf("}\n"); return 0;
}

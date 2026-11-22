// One harness, two libraries. Compile the SAME file against libavif (C) or CrabbyAvif's libavif-compatible C API (Rust).
// Usage: decode_bench <iterations> <file.avif> [more files...]
// Output (CSV): file,status,frames,width,height,depth,fnv1a,median_ms,min_ms
#ifdef BACKEND_CRABBY
#include <avif/avif.h>
#include <avif/libavif_compat.h>
using namespace crabbyavif;
#else
extern "C" {
#include <avif/avif.h>
}
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static int cmp_double(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return (x > y) - (x < y);
}

static uint8_t *read_file(const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buf = (uint8_t *)malloc(n > 0 ? (size_t)n : 1);
  if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return NULL; }
  fclose(f);
  *size = (size_t)n;
  return buf;
}

typedef struct { avifResult result; int frames; uint32_t w, h, depth; uint64_t hash; } Outcome;

// Full decode: parse -> every frame -> YUV->RGB. Hash covers all RGB bytes of all frames.
static Outcome decode_once(const uint8_t *data, size_t size) {
  Outcome o = {AVIF_RESULT_OK, 0, 0, 0, 0, 1469598103934665603ULL};
  avifDecoder *dec = avifDecoderCreate();
  if (!dec) { o.result = AVIF_RESULT_OUT_OF_MEMORY; return o; }
  dec->maxThreads = 1;  // single-threaded on both sides for a fair comparison
  avifResult r = avifDecoderSetIOMemory(dec, data, size);
  if (r == AVIF_RESULT_OK) r = avifDecoderParse(dec);
  while (r == AVIF_RESULT_OK && (r = avifDecoderNextImage(dec)) == AVIF_RESULT_OK) {
    avifRGBImage rgb;
    memset(&rgb, 0, sizeof(rgb));
    avifRGBImageSetDefaults(&rgb, dec->image);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.depth = 8;
    if (avifRGBImageAllocatePixels(&rgb) != AVIF_RESULT_OK) { r = AVIF_RESULT_OUT_OF_MEMORY; break; }
    r = avifImageYUVToRGB(dec->image, &rgb);
    if (r == AVIF_RESULT_OK) {
      for (uint32_t y = 0; y < rgb.height; y++) {
        const uint8_t *row = rgb.pixels + (size_t)y * rgb.rowBytes;
        for (size_t x = 0; x < (size_t)rgb.width * 4; x++) { o.hash ^= row[x]; o.hash *= 1099511628211ULL; }
      }
      o.w = rgb.width; o.h = rgb.height; o.depth = dec->image->depth;
      o.frames++;
    }
    avifRGBImageFreePixels(&rgb);
  }
  if (r == AVIF_RESULT_NO_IMAGES_REMAINING) r = AVIF_RESULT_OK;  // normal end of sequence
  o.result = r;
  avifDecoderDestroy(dec);
  return o;
}

int main(int argc, char **argv) {
  if (argc < 3) { fprintf(stderr, "usage: %s <iterations> <file.avif>...\n", argv[0]); return 2; }
  int iters = atoi(argv[1]);
  if (iters < 1) iters = 1;
  puts("file,status,frames,width,height,depth,fnv1a,median_ms,min_ms");
  for (int i = 2; i < argc; i++) {
    size_t size = 0;
    uint8_t *data = read_file(argv[i], &size);
    if (!data) { printf("%s,READ_ERROR,0,0,0,0,0,0,0\n", argv[i]); continue; }
    Outcome first = decode_once(data, size);  // warm-up + canonical outcome
    double *t = (double *)malloc(sizeof(double) * iters);
    for (int k = 0; k < iters; k++) { double a = now_ms(); (void)decode_once(data, size); t[k] = now_ms() - a; }
    qsort(t, iters, sizeof(double), cmp_double);
    printf("%s,%s,%d,%u,%u,%u,%016llx,%.3f,%.3f\n", argv[i], avifResultToString(first.result), first.frames, first.w,
           first.h, first.depth, (unsigned long long)first.hash, t[iters / 2], t[0]);
    free(t); free(data);
  }
  return 0;
}

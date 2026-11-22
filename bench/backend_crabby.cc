#include "decode_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avif/avif.h>
#include <avif/libavif_compat.h>
using namespace crabbyavif;

// Full decode: parse -> every frame -> YUV->RGB. Hash covers all RGB bytes of all frames.
DecodeOut decode_crabby(const uint8_t *data, size_t size, bool keep_first) {
  DecodeOut o; o.hash = 1469598103934665603ULL; avifResult res = AVIF_RESULT_OK;
  avifDecoder *dec = avifDecoderCreate();
  if (!dec) { o.status = "OOM"; return o; }
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
      if (keep_first && o.frames == 0) { o.first_rgba.resize((size_t)rgb.width * rgb.height * 4); for (uint32_t y = 0; y < rgb.height; y++) memcpy(&o.first_rgba[(size_t)y * rgb.width * 4], rgb.pixels + (size_t)y * rgb.rowBytes, (size_t)rgb.width * 4); }
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
  o.status = avifResultToString(r);
  avifDecoderDestroy(dec);
  return o;
}


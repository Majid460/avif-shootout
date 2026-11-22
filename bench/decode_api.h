#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
struct DecodeOut { std::string status; int frames = 0; uint32_t w = 0, h = 0, depth = 0; uint64_t hash = 0; std::vector<uint8_t> first_rgba; };
// keep_first: copy the first frame's RGBA8 into out.first_rgba (for pixel diffing). Single-threaded, full decode to RGBA8.
DecodeOut decode_libavif(const uint8_t *data, size_t size, bool keep_first);
DecodeOut decode_crabby(const uint8_t *data, size_t size, bool keep_first);

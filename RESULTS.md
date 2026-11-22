# Results (every number says where it was measured)
Pins: dav1d 1.5.3, libavif v1.4.2, CrabbyAvif 1e40c7de. libyuv/sharpyuv OFF, single-threaded.

## Speed, real photos (43 third-party AVIFs, 0.5-18 MP) - OnePlus 8 (IN2015), arm64, interleaved harness, 5 rounds
- crabby/libavif median ratio 0.995, total-time ratio 0.984 -> a tie. Faster on 17 files, slower on 16, within 3% on 10.
- Old per-pass method gave 0.27x-3.4x extremes: a CPU-clock artifact. Interleaving shrank it to 0.50x-1.18x.
## Speed, libavif test files (54, tiny) - OnePlus 8
- Median 1.07 (libavif faster), but the corpus decodes in ~170 ms total: not representative.
- weld_sato_12B_8B_q0.avif (12-bit colour + 8-bit alpha): Crabby 12.8x slower on phone, 4.2x on x86.
- sofa_grid1x5_420.avif: Crabby 6.1x slower on phone, 0.96x on x86 (ARM-specific, cause unknown).
## Speed, other platforms (small corpus, non-interleaved): Mac arm64 0.90, emulator arm64 0.88, x86 container 1.04. Real photos on x86 container (interleaved): 0.95.
## Correctness
- Real photos on phone: 12/43 differ, max 1/255, ~0.00% of bytes (rounding). x86: 0/43 differ.
- Test files: 47/54 decode in both; extended_pixi.avif decodes only in libavif; 2 grid files rejected by both.
## Size (stripped harness: decoder + dav1d + harness)
- Android arm64: libavif 885,128 B; Crabby 2,338,496 B (2.6x).
- x86 container: libavif 2.22 MB; Crabby 3.73 MB; Crabby tuned (opt-level s/z, panic=abort, LTO) 3.50-3.52 MB (-6%).
## Build time (x86 container, 2 cores, dav1d excluded as shared)
- libavif ~1 s; CrabbyAvif ~40 s (clean, default release, capi+dav1d features).
## Fuzzing (x86 container; 100,000 identical mutants per library, 47 small seeds, shared ASan+UBSan dav1d)
- libavif (ASan+UBSan C): 0 crashes, 0 sanitizer reports, 0 hangs; 17,147 accepted / 82,853 rejected.
- CrabbyAvif (release Rust, no ASan on the Rust side): 0 crashes, 0 panics, 0 hangs; 16,338 accepted / 83,662 rejected.
- Weak evidence either way: small seeds, simple mutator, no coverage guidance. Does NOT prove memory safety.

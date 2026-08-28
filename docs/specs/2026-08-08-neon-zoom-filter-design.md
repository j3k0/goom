# NEON zoom_filter — Design

Date: 2026-08-08
Status: approved by user (chat, 2026-08-08)

## Goal

Add an ARM NEON (armv8) optimized version of the Goom zoom filter, matching
the role of `zoom_filter_xmmx` (x86) and `ppc_zoom_G4` (AltiVec). Benefits the
SDL macOS arm64 build immediately and the Android build (which today falls
through to pure C `zoom_filter_c`).

## Background

`c_zoom` (`Goom/filters.c:403`) per output pixel:

1. Interpolate warp coordinate from interleaved x,y int buffers:
   `px = Sx + ((Dx-Sx)*buffratio >> 16)` (signed, low-32 product).
2. Clip: unsigned `px >= ax || py >= ay` ⇒ `pos = coeffs = 0` (negatives
   compare huge).
3. `pos = (px>>4) + prevX*(py>>4)`; `coeffs = precalCoef[px&15][py&15]`
   (4 packed u8 bilinear weights).
4. Per channel: sum of 4 neighbours × coeffs accumulated in `unsigned short`
   (wraps mod 2^16; unreachable in practice since max sum is 255*255=65025),
   then `if (sum > 5) sum -= 5; sum >>= 8`, stored as low byte. Alpha byte of
   dest is never written by the C version.

The existing asm versions deliberately diverge from C (xmmx.c STRICT_COMPAT is
off: per-axis clip, no -5 bias, saturating packuswb, blended alpha). So a
small tolerance has precedent.

## Deliverables

### 1. `Goom/zoom_filter_neon.c` (new)

- Whole file guarded by `#if defined(__ARM_NEON) || defined(__ARM_NEON__)`
  (same pattern as `mmx.c`/`HAVE_MMX`); no-op elsewhere.
- Same signature as `zoom_filter_xmmx`.
- 4 output pixels per iteration:
  - `vld2q_s32` deinterleaves (x,y) vectors from brutS/brutD.
  - Vector coordinate interpolation (`vmulq_s32` low-32 + arithmetic shift —
    exact), combined per-pixel clip mask (`vcgeq_u32`, both axes — stricter
    than MMX per-axis), vector pos.
  - Scalar gather (NEON has no gather): 4 × two 8-byte loads
    (`pos`, `pos+prevX`) + 4 precalCoef ints via lane loads.
  - Vector blend: `vmull_u8`/`vmlal_u8` into u16 lanes, `-5` bias via
    compare+mask, `>>8`, saturating narrow (`vqmovn_u16`), 16-byte store
    (alpha blended, like MMX).
- Scalar tail for `bufsize % 4` (odd widths, e.g. 409) copying C semantics.

### 2. Wiring

- `Goom/goom_fx.h`: guarded declaration of `zoom_filter_neon`.
- `Goom/plugin_info.c` `setOptimizedMethods()`: under `#ifdef __ARM_NEON`,
  `p->methods.zoom_filter = zoom_filter_neon;`. C stays the fallback.
- `sdl/Makefile`: add `zoom_filter_neon.c` to `SRC_ENGINE`.
- Android build (`goom-android/`): add source to CMake list.

### 3. `tests/zoom_filter/` (new, standalone, no framework)

- `zoom_filter_test.c` links `filters.o` and calls exported `zoom_filter_c`
  (non-static; `filters.c` untouched) + `zoom_filter_neon.o`. Small Makefile.
- Conformance (default): fixed-seed random warp buffers + adversarial cases
  (negative coords, coords >= ax/ay, buffratio in {0,1,32768,65535,65536},
  odd widths incl. 409). Pass: >=99% pixels byte-exact on RGB, max per-channel
  delta <= 2, alpha ignored. Reports exact %, max delta, worst frame.
- Stress/bench (`--bench [frames]`): evolving warp buffers, N frames at
  512x288 and 720x360, checksum accumulation vs dead-code elimination,
  ms/frame + speedup C vs NEON.

### 4. Verification

- Test binary green on this Mac (arm64).
- `make goom` then `make goom-run ARGS="--no-mic --exit-after 5"` smoke test.

## Known accepted divergences vs C

- Saturating narrow vs truncation: can differ only when a channel sum exceeds
  255 after `>>8` (rare; covered by <=2 delta / 99% tolerance).
- Alpha: C leaves dest alpha untouched; NEON blends it. Visually irrelevant;
  excluded from comparison.

/*
 * zoom_filter_neon.c
 *
 * ARM NEON (armv8) version of the goom zoom filter, same role as
 * zoom_filter_xmmx (x86 MMX) and ppc_zoom_G4 (AltiVec).
 *
 * Reference semantics: c_zoom() in filters.c. Divergences (same spirit as
 * the MMX STRICT_COMPAT trade-offs, covered by the test tolerance):
 *  - saturating narrow (vqmovn) instead of low-byte truncation,
 *  - alpha channel is blended (MMX-style) instead of left untouched.
 */

#if defined(__aarch64__)

#include <arm_neon.h>
#include <string.h>

#include "goom_graphic.h"

#define BUFFPOINTNB 16

/* faire : a % sqrtperte <=> a & pertemask */
#define PERTEMASK 0xf
/* faire : a / sqrtperte <=> a >> PERTEDEC */
#define PERTEDEC 4

void zoom_filter_neon (int prevX, int prevY,
                       Pixel *__restrict expix1, Pixel *__restrict expix2,
                       int *__restrict lbruS, int *__restrict lbruD, int buffratio,
                       int precalCoef[16][16])
{
    const unsigned int ax = (prevX - 1) << PERTEDEC;
    const unsigned int ay = (prevY - 1) << PERTEDEC;
    const int bufsize = prevX * prevY;

    int loop = 0;

    const int32x4_t vratio = vdupq_n_s32 (buffratio);
    const uint32x4_t vax = vdupq_n_u32 (ax);
    const uint32x4_t vay = vdupq_n_u32 (ay);
    const uint32x4_t vprevX = vdupq_n_u32 ((unsigned int) prevX);
    const uint32x4_t vmask15 = vdupq_n_u32 (PERTEMASK);
    const uint32x4_t vzero256 = vdupq_n_u32 (256);

    const uint16x8_t vfive = vdupq_n_u16 (5);

    /* precalCoef viewed flat: idx = ((px&15)<<4)|(py&15) indexes it as
     * precalCoef[px&15][py&15] (row stride 16, 4 bytes each). */
    const int *precal = (const int *) precalCoef;

    /* Pre-expand the coefficient table into the exact byte layout the blend
     * needs, [c1,c1,c1,c1, c2,c2,c2,c2, c3,c3,c3,c3, c4,c4,c4,c4] per
     * coefficient (what the old vdup+vqtbl1q produced). spreadtab[idx+256]
     * is all-zero so a clipped pixel (clip&256 folded into the index) loads a
     * zero spread directly — no scalar masking, no dup, no tbl. This is the
     * one-per-frame setup; measured ~0.4% of frame cost. */
    uint8_t spreadtab[512][16];
    {
        unsigned i, j;
        memset (spreadtab, 0, sizeof spreadtab);
        for (i = 0; i < 256; ++i) {
            uint32_t c = (uint32_t) precal[i];
            uint8_t c1 = (uint8_t) c, c2 = (uint8_t) (c >> 8),
                    c3 = (uint8_t) (c >> 16), c4 = (uint8_t) (c >> 24);
            for (j = 0; j < 4; ++j) spreadtab[i][j] = c1;
            for (j = 0; j < 4; ++j) spreadtab[i][4 + j] = c2;
            for (j = 0; j < 4; ++j) spreadtab[i][8 + j] = c3;
            for (j = 0; j < 4; ++j) spreadtab[i][12 + j] = c4;
        }
    }

    expix1[0].val = expix1[prevX-1].val = expix1[prevX*prevY-1].val = expix1[prevX*prevY-prevX].val = 0;

    /* 8 pixels/iteration: doubles in-flight gather MLP (16 expix1 gathers +
     * 8 coefficient loads overlap ~14-cycle L2 latency) and amortizes the
     * vector prologue over twice the work. Two independent 4px halves let
     * the compiler overlap one half's source gathers with the other's math. */
    for (; loop + 8 <= bufsize; loop += 8) {
        int32x4x2_t s0, d0, s1, d1;
        int32x4_t px0, py0, px1, py1;
        uint32x4_t clip0, clip1, vpos0, vpos1, vidx0, vidx1, vtotal0, vtotal1;
        int pos[8], vt[8];
        int b;

        s0 = vld2q_s32 (lbruS + 2 * loop);
        s1 = vld2q_s32 (lbruS + 2 * loop + 8);
        d0 = vld2q_s32 (lbruD + 2 * loop);
        d1 = vld2q_s32 (lbruD + 2 * loop + 8);

        px0 = vaddq_s32 (s0.val[0], vshrq_n_s32 (vmulq_s32 (vsubq_s32 (d0.val[0], s0.val[0]), vratio), BUFFPOINTNB));
        py0 = vaddq_s32 (s0.val[1], vshrq_n_s32 (vmulq_s32 (vsubq_s32 (d0.val[1], s0.val[1]), vratio), BUFFPOINTNB));
        px1 = vaddq_s32 (s1.val[0], vshrq_n_s32 (vmulq_s32 (vsubq_s32 (d1.val[0], s1.val[0]), vratio), BUFFPOINTNB));
        py1 = vaddq_s32 (s1.val[1], vshrq_n_s32 (vmulq_s32 (vsubq_s32 (d1.val[1], s1.val[1]), vratio), BUFFPOINTNB));

        clip0 = vorrq_u32 (vcgeq_u32 ((uint32x4_t) px0, vax), vcgeq_u32 ((uint32x4_t) py0, vay));
        clip1 = vorrq_u32 (vcgeq_u32 ((uint32x4_t) px1, vax), vcgeq_u32 ((uint32x4_t) py1, vay));

        vpos0 = vandq_u32 (vaddq_u32 (vshrq_n_u32 ((uint32x4_t) px0, PERTEDEC),
                                      vmulq_u32 (vshrq_n_u32 ((uint32x4_t) py0, PERTEDEC), vprevX)),
                           vmvnq_u32 (clip0));
        vpos1 = vandq_u32 (vaddq_u32 (vshrq_n_u32 ((uint32x4_t) px1, PERTEDEC),
                                      vmulq_u32 (vshrq_n_u32 ((uint32x4_t) py1, PERTEDEC), vprevX)),
                           vmvnq_u32 (clip1));
        vidx0 = vaddq_u32 (vshlq_n_u32 (vandq_u32 ((uint32x4_t) px0, vmask15), 4),
                           vandq_u32 ((uint32x4_t) py0, vmask15));
        vidx1 = vaddq_u32 (vshlq_n_u32 (vandq_u32 ((uint32x4_t) px1, vmask15), 4),
                           vandq_u32 ((uint32x4_t) py1, vmask15));
        /* total index = flat idx + (clip&256): a clipped lane folds +256 to
         * select the all-zero spreadtab half, so no separate coeff masking. */
        vtotal0 = vaddq_u32 (vidx0, vandq_u32 (clip0, vzero256));
        vtotal1 = vaddq_u32 (vidx1, vandq_u32 (clip1, vzero256));

        /* extract positions + spreadtab indices (unrolled constants). */
#define ZG2(n)                                                                                                     \
        {                                                                                                          \
            pos[n] = (int) vgetq_lane_u32 (n < 4 ? vpos0 : vpos1, n & 3);                                         \
            vt[n]  = (int) vgetq_lane_u32 (n < 4 ? vtotal0 : vtotal1, n & 3);                                     \
        }
        ZG2(0) ZG2(1) ZG2(2) ZG2(3) ZG2(4) ZG2(5) ZG2(6) ZG2(7)
#undef ZG2

        /* blend: two independent 4px groups, one 16-byte store each. */
        {
            uint8x16_t vP[4], vC[4];
            uint16x8_t a[4], acc_lo, acc_hi;
            int base, half;
            for (half = 0; half < 2; ++half) {
                base = half * 4;
                for (b = 0; b < 4; ++b) {
                    vP[b] = vcombine_u8 (vld1_u8 ((const uint8_t *) (expix1 + pos[base + b])),
                                         vld1_u8 ((const uint8_t *) (expix1 + pos[base + b] + prevX)));
                    vC[b] = vld1q_u8 ((const uint8_t *) spreadtab[vt[base + b]]);
                }
                for (b = 0; b < 4; ++b) {
                    a[b] = vmull_u8 (vget_low_u8 (vP[b]), vget_low_u8 (vC[b]));
                    a[b] = vmlal_high_u8 (a[b], vP[b], vC[b]);
                }
                acc_lo = vcombine_u16 (vadd_u16 (vget_low_u16 (a[0]), vget_high_u16 (a[0])),
                                       vadd_u16 (vget_low_u16 (a[1]), vget_high_u16 (a[1])));
                acc_hi = vcombine_u16 (vadd_u16 (vget_low_u16 (a[2]), vget_high_u16 (a[2])),
                                       vadd_u16 (vget_low_u16 (a[3]), vget_high_u16 (a[3])));
                acc_lo = vshrq_n_u16 (vsubq_u16 (acc_lo, vandq_u16 (vcgtq_u16 (acc_lo, vfive), vfive)), 8);
                acc_hi = vshrq_n_u16 (vsubq_u16 (acc_hi, vandq_u16 (vcgtq_u16 (acc_hi, vfive), vfive)), 8);
                vst1q_u8 ((uint8_t *) (expix2 + loop + base),
                          vqmovn_high_u16 (vqmovn_u16 (acc_lo), acc_hi));
            }
        }
    }

    for (; loop + 4 <= bufsize; loop += 4) {
        int32x4x2_t s, d;
        int32x4_t px, py;
        uint32x4_t clip, vpos, vidx, vtotal;
        int pos[4], vt[4];
        int k;

        /* coordinate interpolation: px = Sx + ((Dx-Sx)*ratio >> 16) */
        s = vld2q_s32 (lbruS + 2 * loop);
        d = vld2q_s32 (lbruD + 2 * loop);

        px = vaddq_s32 (s.val[0], vshrq_n_s32 (vmulq_s32 (vsubq_s32 (d.val[0], s.val[0]), vratio), BUFFPOINTNB));
        py = vaddq_s32 (s.val[1], vshrq_n_s32 (vmulq_s32 (vsubq_s32 (d.val[1], s.val[1]), vratio), BUFFPOINTNB));

        /* clip: unsigned compare, so negatives count as out of range;
         * either axis out => whole pixel clipped (stricter than MMX).
         * clip lanes are 0 (keep) or 0xFFFFFFFF (discard). */
        clip = vorrq_u32 (vcgeq_u32 ((uint32x4_t) px, vax),
                          vcgeq_u32 ((uint32x4_t) py, vay));

        /* pos = (px>>4 + prevX*(py>>4)) & ~clip. Masking the sum once is
         * bit-identical to masking px/py then shifting: for unclipped
         * (non-negative) lanes the shift+mask commute; clipped lanes give
         * pos 0 either way. */
        vpos = vandq_u32 (vaddq_u32 (vshrq_n_u32 ((uint32x4_t) px, PERTEDEC),
                                     vmulq_u32 (vshrq_n_u32 ((uint32x4_t) py, PERTEDEC), vprevX)),
                          vmvnq_u32 (clip));

        /* flat precalCoef index, always in [0,255] (indices masked with 15);
         * total = flat + (clip&256) selects the all-zero spreadtab half for
         * clipped lanes, so no separate coeff masking is needed. */
        vidx = vaddq_u32 (vshlq_n_u32 (vandq_u32 ((uint32x4_t) px, vmask15), 4),
                          vandq_u32 ((uint32x4_t) py, vmask15));
        vtotal = vaddq_u32 (vidx, vandq_u32 (clip, vzero256));

        /* extract positions + spreadtab indices (unrolled constants). */
        pos[0] = (int) vgetq_lane_u32 (vpos, 0);
        pos[1] = (int) vgetq_lane_u32 (vpos, 1);
        pos[2] = (int) vgetq_lane_u32 (vpos, 2);
        pos[3] = (int) vgetq_lane_u32 (vpos, 3);
        vt[0]  = (int) vgetq_lane_u32 (vtotal, 0);
        vt[1]  = (int) vgetq_lane_u32 (vtotal, 1);
        vt[2]  = (int) vgetq_lane_u32 (vtotal, 2);
        vt[3]  = (int) vgetq_lane_u32 (vtotal, 3);

        /* blend: one accumulator per output pixel.
         * vP[k] = [A B C D] (pos, pos+1, pos+prevX, pos+prevX+1 channels)
         * vC[k] = [c1 x4, c2 x4, c3 x4, c4 x4]
         * a[k] lanes 0..3 = A*c1 + C*c3, lanes 4..7 = B*c2 + D*c4
         * => horizontal pair add gives the 4 output channels.
         * u16 lanes wrap mod 2^16 like C's unsigned short. */
        {
            uint8x16_t vP[4], vC[4];
            uint16x8_t a[4];
            uint16x8_t acc_lo, acc_hi;

            /* gather all 4 source neighborhoods first so the L1/L2 gather
             * latency overlaps the coefficient load and multiply. */
            for (k = 0; k < 4; ++k) {
                vP[k] = vcombine_u8 (vld1_u8 ((const uint8_t *) (expix1 + pos[k])),
                                     vld1_u8 ((const uint8_t *) (expix1 + pos[k] + prevX)));
                vC[k] = vld1q_u8 ((const uint8_t *) spreadtab[vt[k]]);
            }
            for (k = 0; k < 4; ++k) {
                a[k] = vmull_u8 (vget_low_u8 (vP[k]), vget_low_u8 (vC[k]));
                a[k] = vmlal_high_u8 (a[k], vP[k], vC[k]);
            }

            /* pairwise channel sums: pixels 0,1 in acc_lo; 2,3 in acc_hi. */
            acc_lo = vcombine_u16 (vadd_u16 (vget_low_u16 (a[0]), vget_high_u16 (a[0])),
                                   vadd_u16 (vget_low_u16 (a[1]), vget_high_u16 (a[1])));
            acc_hi = vcombine_u16 (vadd_u16 (vget_low_u16 (a[2]), vget_high_u16 (a[2])),
                                   vadd_u16 (vget_low_u16 (a[3]), vget_high_u16 (a[3])));

            /* if (sum > 5) sum -= 5;  then sum >>= 8 */
            acc_lo = vshrq_n_u16 (vsubq_u16 (acc_lo, vandq_u16 (vcgtq_u16 (acc_lo, vfive), vfive)), 8);
            acc_hi = vshrq_n_u16 (vsubq_u16 (acc_hi, vandq_u16 (vcgtq_u16 (acc_hi, vfive), vfive)), 8);

            /* saturating narrow (vs C's truncation: tolerated divergence);
             * single 16-byte store covers all 4 pixels. */
            vst1q_u8 ((uint8_t *) (expix2 + loop),
                      vqmovn_high_u16 (vqmovn_u16 (acc_lo), acc_hi));
        }
    }

    /* scalar tail, C semantics (odd buffer sizes) */
    for (; loop < bufsize; ++loop) {
        int c1, c2, c3, c4, px, py;
        int pos, coeffs;
        unsigned char *src;
        unsigned int acc[4];
        int myPos = 2 * loop;

        px = lbruS[myPos] + (((lbruD[myPos] - lbruS[myPos]) * buffratio) >> BUFFPOINTNB);
        py = lbruS[myPos+1] + (((lbruD[myPos+1] - lbruS[myPos+1]) * buffratio) >> BUFFPOINTNB);

        if (((unsigned int) py >= ay) || ((unsigned int) px >= ax)) {
            pos = coeffs = 0;
        } else {
            pos = ((px >> PERTEDEC) + prevX * (py >> PERTEDEC));
            coeffs = precalCoef[px & PERTEMASK][py & PERTEMASK];
        }

        c1 = coeffs & 0xff;
        c2 = (coeffs >> 8) & 0xff;
        c3 = (coeffs >> 16) & 0xff;
        c4 = (coeffs >> 24) & 0xff;

        src = (unsigned char *) (expix1 + pos);
        {
            const unsigned char *src2 = (const unsigned char *) (expix1 + pos + prevX);
            int ch;
            for (ch = 0; ch < 4; ++ch) {
                acc[ch] = src[ch] * c1 + src[4+ch] * c2 + src2[ch] * c3 + src2[4+ch] * c4;
                if (acc[ch] > 5)
                    acc[ch] -= 5;
                acc[ch] >>= 8;
                ((unsigned char *) (expix2 + loop))[ch] = (unsigned char) acc[ch];
            }
        }
    }
}

#endif /* __aarch64__ */

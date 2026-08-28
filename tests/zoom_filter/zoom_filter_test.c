/*
 * zoom_filter_test.c — conformance + stress test for zoom_filter_neon
 * vs the reference zoom_filter_c (filters.c).
 *
 * Modes:
 *   (no args)        conformance: fixed-seed random + adversarial cases
 *   --bench [frames] stress/benchmark at real resolutions
 *
 * Conformance pass criteria (see design spec):
 *   >= 99% of pixels byte-exact on RGB, max per-channel delta <= 2.
 *   Alpha (byte 0) is ignored: C leaves dest alpha untouched, NEON blends it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_fx.h"

extern void zoom_filter_neon (int prevX, int prevY,
                              Pixel *expix1, Pixel *expix2,
                              int *lbruS, int *lbruD, int buffratio,
                              int precalCoef[16][16]);

#define BUFFPOINTNB 16
#define PERTEDEC 4

/* ------------------------------------------------------------------ */
/* xorshift32, deterministic across runs/platforms                     */

static uint32_t rng_state = 0x12345678u;

static uint32_t xrand (void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

/* ------------------------------------------------------------------ */
/* replicate of static generatePrecalCoef() from filters.c            */

static void generatePrecalCoef (int precalCoef[16][16])
{
    int coefh, coefv;

    for (coefh = 0; coefh < 16; coefh++) {
        for (coefv = 0; coefv < 16; coefv++) {
            int i;
            int diffcoeffh = 16 - coefh;
            int diffcoeffv = 16 - coefv;

            if (!(coefh || coefv)) {
                i = 255;
            }
            else {
                int i1 = diffcoeffh * diffcoeffv;
                int i2 = coefh * diffcoeffv;
                int i3 = diffcoeffh * coefv;
                int i4 = coefh * coefv;

                if (i1) i1--;
                if (i2) i2--;
                if (i3) i3--;
                if (i4) i4--;

                i = (i1) | (i2 << 8) | (i3 << 16) | (i4 << 24);
            }
            precalCoef[coefh][coefv] = i;
        }
    }
}

/* ------------------------------------------------------------------ */
/* warp field generators                                               */

/* smooth random field around the identity grid, 4.12 fixed point.
 * amplitude = max abs displacement in 1/16th pixels */
static void gen_smooth (int W, int H, int *buf, int amplitude)
{
    int x, y, i = 0;
    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            int dx = amplitude ? (int) (xrand () % (2 * amplitude + 1)) - amplitude : 0;
            int dy = amplitude ? (int) (xrand () % (2 * amplitude + 1)) - amplitude : 0;
            buf[i++] = (x << PERTEDEC) + dx;
            buf[i++] = (y << PERTEDEC) + dy;
        }
    }
}

/* adversarial: negatives, way out of range, overflow bait */
static void gen_adversarial (int W, int H, int *buf)
{
    static const int nasty[] = {
        -2147483647, -100000, -1000, -16, -1, 0, 1, 15, 16, 255, 100000, 2147483646
    };
    int i, n = W * H * 2;
    for (i = 0; i < n; ++i) {
        uint32_t r = xrand ();
        if (r % 3 == 0)
            buf[i] = nasty[(r >> 8) % (sizeof (nasty) / sizeof (nasty[0]))];
        else
            buf[i] = (int) (r % (unsigned) ((W + H) << PERTEDEC)) - (H << (PERTEDEC - 1));
    }
}

/* smooth, continuous random-walk field approximating the real goom warp
 * (zoomVector is smooth/continuous, so adjacent output pixels map to
 * adjacent source pixels: pos[k+1] == pos[k]+1 most of the time).
 * step in 1/16-pixels of the walk between adjacent pixels. */
static void gen_smooth_walk (int W, int H, int *buf, int step)
{
    int x, y, i = 0;
    int dx = 0, dy = 0;
    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            dx += (int) (xrand () % (2 * step + 1)) - step;
            dy += (int) (xrand () % (2 * step + 1)) - step;
            if (dx < -48) dx = -48;
            if (dx > 48) dx = 48;
            if (dy < -48) dy = -48;
            if (dy > 48) dy = 48;
            buf[i++] = (x << PERTEDEC) + dx;
            buf[i++] = (y << PERTEDEC) + dy;
        }
    }
}

static void fill_pixels (int W, int H, Pixel *pix)
{
    int i, n = W * H;
    for (i = 0; i < n; ++i)
        pix[i].val = xrand ();
}

/* ------------------------------------------------------------------ */
/* comparison                                                          */

typedef struct {
    long pixels;
    long exact;      /* pixels with RGB delta 0 */
    int  maxdelta;
} CompareStats;

static void compare (int W, int H, const Pixel *a, const Pixel *b, CompareStats *st)
{
    int i, ch;
    long n = (long) W * H;
    st->pixels = n;
    st->exact = 0;
    st->maxdelta = 0;
    for (i = 0; i < n; ++i) {
        int pixdelta = 0;
        for (ch = 0; ch < 4; ++ch) {
            if (ch == ALPHA)
                continue;   /* C leaves dest alpha untouched, NEON blends it */
            {
            int d = (int) a[i].cop[ch] - (int) b[i].cop[ch];
            if (d < 0) d = -d;
            if (d > pixdelta) pixdelta = d;
            }
        }
        if (pixdelta == 0)
            st->exact++;
        if (pixdelta > st->maxdelta)
            st->maxdelta = pixdelta;
    }
}

/* ------------------------------------------------------------------ */

typedef struct {
    int W, H;
} Size;

static int run_case (int W, int H, int adversarial, int buffratio, int amplitude,
                     int precalCoef[16][16], CompareStats *out)
{
    int bufsize2 = W * H * 2;
    Pixel *src = malloc (W * H * sizeof (Pixel));
    Pixel *src_copy = malloc (W * H * sizeof (Pixel));
    Pixel *dest_c = malloc (W * H * sizeof (Pixel));
    Pixel *dest_n = malloc (W * H * sizeof (Pixel));
    int *brutS = malloc (bufsize2 * sizeof (int));
    int *brutD = malloc (bufsize2 * sizeof (int));

    fill_pixels (W, H, src);
    fill_pixels (W, H, dest_c);
    memcpy (dest_n, dest_c, W * H * sizeof (Pixel));

    if (adversarial) {
        gen_adversarial (W, H, brutS);
        gen_adversarial (W, H, brutD);
    } else {
        gen_smooth (W, H, brutS, amplitude);
        gen_smooth (W, H, brutD, amplitude * 2);
    }

    memcpy (src_copy, src, W * H * sizeof (Pixel));
    zoom_filter_c (W, H, src, dest_c, brutS, brutD, buffratio, precalCoef);
    zoom_filter_neon (W, H, src_copy, dest_n, brutS, brutD, buffratio, precalCoef);

    compare (W, H, dest_c, dest_n, out);

    free (src); free (src_copy); free (dest_c); free (dest_n);
    free (brutS); free (brutD);
    return 0;
}

static int conformance (void)
{
    static const Size sizes[] = {
        {512, 288}, {720, 360}, {409, 256}, {333, 201}, {64, 48}, {5, 3}
    };
    static const int ratios[] = {0, 1, 32768, 65535, 65536, 131072};
    int precalCoef[16][16];

    long total_pixels = 0, total_exact = 0;
    int global_maxdelta = 0;
    int failures = 0;
    unsigned s, r, seed;

    generatePrecalCoef (precalCoef);

    for (s = 0; s < sizeof (sizes) / sizeof (sizes[0]); ++s) {
        int W = sizes[s].W, H = sizes[s].H;
        for (r = 0; r < sizeof (ratios) / sizeof (ratios[0]); ++r) {
            for (seed = 0; seed < 3; ++seed) {
                int adv;
                for (adv = 0; adv < 2; ++adv) {
                    CompareStats st;
                    rng_state = 0x9e3779b9u ^ (seed * 0x85ebca6bu) ^ (unsigned) (W << 16) ^ (unsigned) H;
                    run_case (W, H, adv, ratios[r], 48, precalCoef, &st);

                    total_pixels += st.pixels;
                    total_exact += st.exact;
                    if (st.maxdelta > global_maxdelta)
                        global_maxdelta = st.maxdelta;

                    if (st.maxdelta > 2 || st.exact * 100 < st.pixels * 99) {
                        printf ("FAIL %4dx%-4d ratio=%-6d seed=%u %-11s exact=%ld/%ld maxdelta=%d\n",
                                W, H, ratios[r], seed, adv ? "adversarial" : "smooth",
                                st.exact, st.pixels, st.maxdelta);
                        failures++;
                    }
                }
            }
        }
    }

    printf ("conformance: %ld/%ld pixels exact (%.3f%%), max RGB delta = %d\n",
            total_exact, total_pixels,
            100.0 * (double) total_exact / (double) total_pixels,
            global_maxdelta);

    if (failures || total_exact * 100 < total_pixels * 99 || global_maxdelta > 2) {
        printf ("CONFORMANCE: FAIL (%d case failures)\n", failures);
        return 1;
    }
    printf ("CONFORMANCE: PASS\n");
    return 0;
}

/* ------------------------------------------------------------------ */

static double now_ms (void)
{
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static int g_smooth_walk = 0;   /* use smooth/continuous field in bench */

static int bench (int frames)
{
    static const Size sizes[] = { {512, 288}, {720, 360} };
    int precalCoef[16][16];
    unsigned s;
    int rc = 0;

    generatePrecalCoef (precalCoef);

    for (s = 0; s < sizeof (sizes) / sizeof (sizes[0]); ++s) {
        int W = sizes[s].W, H = sizes[s].H;
        int bufsize2 = W * H * 2;
        Pixel *src = malloc (W * H * sizeof (Pixel));
        Pixel *dest = malloc (W * H * sizeof (Pixel));
        int *brutS = malloc (bufsize2 * sizeof (int));
        int *brutD = malloc (bufsize2 * sizeof (int));
        uint64_t checksum = 0;
        double ms_c, ms_n;
        int f, i;

        rng_state = 0xdeadbeefu;
        fill_pixels (W, H, src);
        if (g_smooth_walk) gen_smooth_walk (W, H, brutD, 1);
        else gen_smooth (W, H, brutD, 48);

        /* C version: evolve fields untimed, time only the kernel */
        ms_c = 0;
        for (f = 0; f < frames; ++f) {
            double ts, te;
            memcpy (brutS, brutD, bufsize2 * sizeof (int));
            if (g_smooth_walk) gen_smooth_walk (W, H, brutD, 1);
            else gen_smooth (W, H, brutD, 48);
            ts = now_ms ();
            zoom_filter_c (W, H, src, dest, brutS, brutD,
                           (f * 65536) / frames, precalCoef);
            te = now_ms ();
            ms_c += te - ts;
        }
        for (i = 0; i < W * H; ++i) checksum += dest[i].val;

        /* NEON version */
        ms_n = 0;
        for (f = 0; f < frames; ++f) {
            double ts, te;
            memcpy (brutS, brutD, bufsize2 * sizeof (int));
            if (g_smooth_walk) gen_smooth_walk (W, H, brutD, 1);
            else gen_smooth (W, H, brutD, 48);
            ts = now_ms ();
            zoom_filter_neon (W, H, src, dest, brutS, brutD,
                              (f * 65536) / frames, precalCoef);
            te = now_ms ();
            ms_n += te - ts;
        }
        for (i = 0; i < W * H; ++i) checksum += dest[i].val;

        printf ("%4dx%-4d  C: %7.2f ms (%5.2f ms/frame)   NEON: %7.2f ms (%5.2f ms/frame)   speedup: %4.2fx   checksum=%llx\n",
                W, H, ms_c, ms_c / frames, ms_n, ms_n / frames, ms_c / ms_n,
                (unsigned long long) checksum);

        free (src); free (dest); free (brutS); free (brutD);
    }
    return rc;
}

int main (int argc, char **argv)
{
    if (argc > 1 && strcmp (argv[1], "--bench") == 0) {
        int frames = argc > 2 ? atoi (argv[2]) : 300;
        if (frames <= 0) frames = 300;
        if (argc > 3 && strcmp (argv[3], "--smooth") == 0)
            g_smooth_walk = 1;
        return bench (frames);
    }
    return conformance ();
}

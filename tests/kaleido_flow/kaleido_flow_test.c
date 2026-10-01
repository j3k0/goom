/*
 * kaleido_flow_test.c — invariants of the perceived-flow field and of the
 * gauge-aware transitions of the kaleidoscope addon. Builds against the
 * real Goom/filters.c (like tests/zoom_filter). No framework.
 *
 * Checked properties (public API only):
 *
 *  1. radial identity (unfolded): the perceived flow of a radial zoom is the
 *     radial zoom: u(p) = coef * (p - m).
 *  2. radial identity (folded): idem with the fold on. The RAW transform
 *     vector is not this (it carries the fold's own displacement, O(screen),
 *     flipped across the seams), so this fails if the sector reprojection in
 *     zoomFilterGetVelocity is lost or wrong.
 *  3. equivariance (folded, non-radial mode): u(g.p) = g.u(p) for every
 *     dihedral image g of p. Exercises the sector element tracked by the
 *     fold: getting it wrong (identity/transpose) breaks the rotation
 *     images by O(|u|).
 *  4. morph symmetry (fold ON, then re-sectorisation): the rendered frame is
 *     exactly mirror-symmetric at every instant of the interpolation (the
 *     sampling map is constant on fold orbits). Without the gauge-aware bake
 *     the interpolation blends two gauges and breaks the symmetry.
 * 5. bounded flow (fold OFF): through the un-folding morph the flow stays
 *    bounded like the base mode. The fold displacement leaks O(screen) into
 *    the field if the tables are not re-projected into the pixel's sector.
 * 6. wipe (radius): a fold-ON transition with Fold Transition = Wipe Radius
 *    blooms the mandala from the center out: no click at the swap (the swap
 *    frame renders the old map), the grown core is exactly mirror-symmetric
 *    at every instant, the flow outside the wobbly frontier is frozen, the
 *    flow inside is equivariant under the NEW fold, completion (c = 1)
 *    leaves the whole frame exactly symmetric and settled, and the
 *    per-pixel wobble jitter (Wipe Wobble = 10% here) is orbit-invariant:
 *    the beta_hat flow classification agrees across every orbit8 pair.
 * 7. wipe (seams): idem with the seam frontier (distance to the fold's N
 *    mirror axes at angle step PI/N).
 * 8. re-slice (fold geometry change, Fold Transition = Linear): the bake
 *    re-gauges the merged state into the target fold and the morph
 *    interpolates chamber-locally, so the frame stays exactly orbit8-
 *    symmetric at every morph step; the flow query's sweep term (-g.ds) is
 *    present while the maps move, exactly linear in the ramp step dr and
 *    bounded.
 * 9. wipe settle at a real canvas (864x542): the per-pixel built tables
 *    wobble +/- 1/16 px between mirror mates at canvases where the fold
 *    does not land exactly on the grid (float noise in the build; absent
 *    at 64x64); the LINEAR path masks it at the last lerp step (residual
 *    ~0.3%), but the wipe's spatial selection rendered it raw (~3.3% of
 *    pixels ghosted along the seams, persistent after the transition).
 *    The folded-point convergence of wipeComplete must leave the settled
 *    frame exactly symmetric at any canvas, radius and seams alike.
 * 10. fold OFF with a concurrent middle jump: the goom "changement de
 *     milieu" can move middleX in the same mode-change event that stops
 *     the fold; the un-projection must use the apex captured in the gauge
 *     (the one the mandala was shown with), the flow stays bounded.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_fx.h"
#include "goom_plugin_info.h"

extern void zoom_filter_c (int sizeX, int sizeY, Pixel *src, Pixel *dest,
                           int *brutS, int *brutD, int buffratio,
                           int precalCoef[16][16], const ZoomBlend *blend);

/* wipe timing: the contract's progress rule is c = min(1, k / wipeFrames)
 * with k counting kernel calls from the swap call (swap call = k = 0) */
#define WIPE_FRAMES 20
/* no-click / outside-flow identity: the bake quantizes the merged map to
 * 1/16 px and S/D are frozen during the wipe */
#define TOL_FROZEN 0.1f

/* the canvas is square and even: the (N=4, angle=0) fold's 8 isometries map
 * pixels to pixels, so symmetry can be compared byte-exactly */
/* canvas size, runtime: test 9 re-runs the wipe settle at a real engine
 * canvas (864x542) where the fold does not land exactly on the pixel
 * grid; everything else here stays at the square 64x64. */
static int W = 64;
static int H = 64;

/* vitesse 200 -> coefVitesse = (1 + 72/128) / 50 = 0.03125 */
#define VITESSE 200
#define COEF 0.03125f

/* table quantization (1/16 px) + the min-displacement nudge (2/16 px on
 * small components, by design) + float noise */
#define TOL_FLOW 0.25f
/* + the min-displacement nudge can shift a component by 1/16 px */
#define TOL_EQUIV 0.25f
/* fold transitions: boost params must be 0 (checked below) */
#define SYM_TOL 0.01
#define FLOW_BOUND 3.0f
/* wobble spread 0.10: j = 1 + 0.10 * (2*luma/255 - 1) in [0.9, 1.1] */
#define WIPE_WOBBLE 10
/* sweep term: |u(si) - u(0)| <= 2.0 px bound, linearity in dr TOL 0.05 */
#define TOL_SWEEP_BOUND 2.0f
#define TOL_SWEEP_LINEAR 0.05f

static int failures = 0;

static void check (int ok, const char *what) {
    if (!ok) {
        printf ("FAIL: %s\n", what);
        failures++;
    }
}

/* ------------------------------------------------------------------ */
/* xorshift32, deterministic across runs/platforms                     */

static uint32_t rng_state = 0x2468ace0u;

static uint32_t xrand (void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

/* ------------------------------------------------------------------ */
/* harness context: drives the real zoomFilterFastRGB                  */

typedef struct {
    PluginInfo info;
    Pixel *a, *b;               /* ping-pong buffers */
    ZoomFilterData zf;          /* template, copied per config change */
} Ctx;

static void ctx_init (Ctx *c) {
    memset (c, 0, sizeof (*c));
    c->info.screen.width = W;
    c->info.screen.height = H;
    c->info.screen.size = W * H;
    c->info.gRandom = goom_random_init (1);
    c->info.methods.zoom_filter = zoom_filter_c;
    c->a = (Pixel *)calloc (W * H, sizeof (Pixel));
    c->b = (Pixel *)calloc (W * H, sizeof (Pixel));

    c->info.zoomFilter_fx = zoomFilterVisualFXWrapper_create ();
    c->info.zoomFilter_fx.init (&c->info.zoomFilter_fx, &c->info);

    /* legacy morph path for the shared ctx: Fold Transition = Linear and
     * the wipe params quieted; params 16..19 = Fold Transition /
     * Wipe Frames / Wipe Ring / Wipe Wobble */
    IVAL (*c->info.zoomFilter_fx.params->params[16]) = 0;
    IVAL (*c->info.zoomFilter_fx.params->params[17]) = 0;
    IVAL (*c->info.zoomFilter_fx.params->params[18]) = 0;
    IVAL (*c->info.zoomFilter_fx.params->params[19]) = 0;

    c->zf.vitesse = VITESSE;
    c->zf.middleX = W / 2;
    c->zf.middleY = H / 2;
    c->zf.mode = NORMAL_MODE;
}

static void fill_random (Pixel *p, int n) {
    int i;
    for (i = 0; i < n; i++)
        p[i].val = xrand ();
}

/* one frame; zf (or NULL) is the config change for this frame only.
 * si/sm drive the interpolation exactly (12.5% per step at 8192). */
static void frame (Ctx *c, ZoomFilterData *zf, int si, float sm, int inject) {
    if (inject)
        fill_random (c->a, W * H);
    zoomFilterFastRGB (&c->info, c->a, c->b, zf, W, H, &si, &sm);
    {
        Pixel *t = c->a;
        c->a = c->b;
        c->b = t;
    }
}

static void frames (Ctx *c, ZoomFilterData *zf, int n, int si, float sm, int inject) {
    int i;
    for (i = 0; i < n; i++)
        frame (c, (i == 0) ? zf : NULL, si, sm, inject);
}

/* the flow query reads the per-pixel blend the kernel applies this frame:
 * plain LINEAR outside a wipe (unchanged), the wipe's per-pixel beta
 * (radius / seams) during one */
static void query (Ctx *c, float x, float y, float *vx, float *vy) {
    zoomFilterGetVelocity (&c->info, x, y, vx, vy);
}

/* ------------------------------------------------------------------ */
/* helpers                                                             */

/* RGB only: the kernels write channels.r/g/b and leave channels.a (byte 3
 * of val in the COLOR_RGBA layout) untouched in the destination */
static int rgb_eq (Pixel a, Pixel b) {
    return ((a.val ^ b.val) & 0x00ffffffu) == 0;
}

/* the 8 isometries of the (N=4, angle=0) fold: (dx,dy) |-> (sdx, sdy) and
 * their transposes. Applied to centered integer coordinates. */
static void orbit8 (int dx, int dy, int ox[8], int oy[8]) {
    ox[0] =  dx; oy[0] =  dy;
    ox[1] = -dx; oy[1] =  dy;
    ox[2] =  dx; oy[2] = -dy;
    ox[3] = -dx; oy[3] = -dy;
    ox[4] =  dy; oy[4] =  dx;
    ox[5] = -dy; oy[5] =  dx;
    ox[6] =  dy; oy[6] = -dx;
    ox[7] = -dy; oy[7] = -dx;
}

/* fraction of orbit pairs whose RGB differs (0 = exactly symmetric) */
static double sym_ratio (const Pixel *img) {
    long bad = 0, tot = 0;
    int x, y, k, ox[8], oy[8];

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            orbit8 (x - W / 2, y - H / 2, ox, oy);
            for (k = 1; k < 8; k++) {
                int qx = ox[k] + W / 2, qy = oy[k] + H / 2;
                if (qx < 0 || qy < 0 || qx >= W || qy >= H)
                    continue;
                tot++;
                if (!rgb_eq (img[y * W + x], img[qy * W + qx]))
                    bad++;
            }
        }
    }
    return tot ? (double)bad / (double)tot : 0.0;
}

/* distance in px from (dx, dy) to the 4 seam lines (0, 45, 90, 135 deg) */
static double seam_dist (int dx, int dy) {
    double d = fabs ((double)dy);                       /* 0 deg line */
    double d2 = fabs ((double)dx);                      /* 90 deg line */
    double d3 = fabs ((double)(dx - dy)) * 0.70710678;            /* 45 deg line */
    double d4 = fabs ((double)(dx + dy)) * 0.70710678;            /* 135 deg line */
    if (d2 < d) d = d2;
    if (d3 < d) d = d3;
    if (d4 < d) d = d4;
    return d;
}

static float vlen (float x, float y) {
    return sqrtf (x * x + y * y);
}

/* ------------------------------------------------------------------ */
/* 6-7: wipe transitions (Fold Transition = Wipe Radius / Wipe Seams)  */

/* probe grid: even offsets from -28 to 28 (4 px clear of the borders,
 * outside the min-displacement / source-clamp zones); orbit8 maps grid
 * points to grid points */
#define WIPE_N (29 * 29)

static int wipe_gdx (int i) { return -28 + 2 * (i % 29); }
static int wipe_gdy (int i) { return -28 + 2 * (i / 29); }

static void snap_flow (Ctx *c, float u[][2]) {
    int i;
    for (i = 0; i < WIPE_N; i++) {
        float vx, vy;
        query (c, (float) (wipe_gdx (i) + W / 2), (float) (wipe_gdy (i) + H / 2), &vx, &vy);
        u[i][0] = vx;
        u[i][1] = vy;
    }
}

/* the seam distance exactly as the kernel computes it for the (N=4,
 * angle=0) fold: min over m of |p . rot(m*pi/4) . n1| with n1 = (0,1) */
static float wipe_seam_dist (int dx, int dy) {
    float s = 1e30f;
    int m;
    for (m = 0; m < 4; m++) {
        float th = (float) m * (float) M_PI / 4.0f;
        float ux = 0.0f * cosf (th) - 1.0f * sinf (th);
        float uy = 0.0f * sinf (th) + 1.0f * cosf (th);
        float d = fabsf ((float) dx * ux + (float) dy * uy);
        if (d < s) s = d;
    }
    return s;
}

/* 1 = p is in the grown CORE at progress c, hard wipe (ring = 0), for the
 * 10% wobble spread: the per-pixel frontier is W(p) = c * j * halfDiag with
 * j in [0.9, 1.1] (unknown per pixel but bounded), so every pixel with
 * metric <= c * 0.9 * halfDiag is inside the frontier and every pixel with
 * metric >= c * 1.1 * halfDiag is outside it, whatever the luma field.
 * Radius keeps its integer-exact d2 <= r*r form; the seam metric is
 * evaluated in float here and in the kernel and can disagree by an ulp AT
 * the frontier, so the seams core keeps a 0.05 px margin. */
static int wipe_grown (int dx, int dy, int mode, float c, float halfDiag) {
    float r = (c * 0.9f) * halfDiag;
    if (mode == 1)
        return (float) (dx * dx + dy * dy) <= r * r;
    return wipe_seam_dist (dx, dy) <= r - 0.05f;
}

/* fraction of orbit pairs with differing RGB among the pairs inside the
 * grown CORE (0.0 = exactly symmetric there) */
static double sym_ratio_grown (const Pixel *img, int mode, float c, float halfDiag) {
    long bad = 0, tot = 0;
    int x, y, k, ox[8], oy[8];

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int dx = x - W / 2, dy = y - H / 2;
            if (!wipe_grown (dx, dy, mode, c, halfDiag))
                continue;
            orbit8 (dx, dy, ox, oy);
            for (k = 1; k < 8; k++) {
                int qx = ox[k] + W / 2, qy = oy[k] + H / 2;
                if (qx < 0 || qy < 0 || qx >= W || qy >= H)
                    continue;
                tot++;
                if (!rgb_eq (img[y * W + x], img[qy * W + qx]))
                    bad++;
            }
        }
    }
    return tot ? (double) bad / (double) tot : 0.0;
}

/* one wipe transition (mode 1 = radius, 2 = seams): fold-ON injection on a
 * steady unfolded state, then the no-click / grown-core-symmetry / frozen-
 * outside / equivariant-inside / completion / wobble-invariance assertions */
static void test_wipe (int mode, const char *what) {
    static float u_prev[WIPE_N][2], u_k0[WIPE_N][2];
    static float u_mid[WIPE_N][2], u_end[WIPE_N][2];
    Ctx c;
    /* the kernel's halfDiagonal: 0.5 * sqrt(prevX^2 + prevY^2) */
    const float halfDiag = 0.5f * sqrtf ((float) (W * W + H * H));
    int i, k;

    ctx_init (&c);

    /* planar add-ons give the wobble classification of (f) its signal: the
     * folded and the unfolded flow then differ well above the residue noise
     * away from the fold axes (the planes are evaluated at the folded
     * point). No hypercos: its sin() terms are irrational and would move the
     * table landings off the 0.05-unit grid that keeps the byte-exact orbit
     * constancy the exact symmetry checks rely on. */
    c.zf.hPlaneEffect = 20;
    c.zf.vPlaneEffect = -14;

    /* steady unfolded state; si frozen at 0 for the whole test so the
     * buffratio ramp never moves the maps under our feet (and the flow
     * sweep term stays identically zero) */
    frames (&c, &c.zf, 32, 0, 1.0f, 1);

    /* one config injection turning the fold ON (N=4, angle=0) with the
     * 10% default wobble spread (Wipe Wobble = 10) and a hard frontier
     * (Wipe Ring = 0) */
    c.zf.kaleidoEffect = 1;
    c.zf.foldCount = 4;
    c.zf.foldAngle = 0.0f;
    IVAL (*c.info.zoomFilter_fx.params->params[16]) = mode;         /* fold transition */
    IVAL (*c.info.zoomFilter_fx.params->params[17]) = WIPE_FRAMES;  /* wipe frames */
    IVAL (*c.info.zoomFilter_fx.params->params[18]) = 0;            /* ring: hard step */
    IVAL (*c.info.zoomFilter_fx.params->params[19]) = WIPE_WOBBLE;  /* wobble: 10% */
    frame (&c, &c.zf, 0, 1.0f, 1);                  /* config commit */
    for (i = 0; i < 63; i++)                        /* stride-1 table build */
        frame (&c, NULL, 0, 1.0f, 1);
    snap_flow (&c, u_prev);                         /* k = -1: pre-swap */

    /* (a) no click: the swap frame flows exactly like the one before it
     * (the bake quantizes the merged map to 1/16 px). At c = 0 the global
     * progress override makes every pixel read the old map; only the exact
     * center (radius mode) is skipped, where the frontier is degenerate. */
    frame (&c, NULL, 0, 1.0f, 1);                   /* bake + swap: k = 0 */
    {
        float worst = 0.0f;
        snap_flow (&c, u_k0);
        for (i = 0; i < WIPE_N; i++) {
            float e;
            if (wipe_grown (wipe_gdx (i), wipe_gdy (i), mode, 0.0f, halfDiag))
                continue;
            e = vlen (u_k0[i][0] - u_prev[i][0], u_k0[i][1] - u_prev[i][1]);
            if (e > worst) worst = e;
        }
        printf ("  %-28s no-click |u_k0 - u_k-1| = %.4f px\n", what, worst);
        check (worst <= TOL_FROZEN, "wipe: no click at the swap");
    }

    /* one kernel call per k; call k renders at c = min(1, k / wipeFrames) */
    for (k = 1; k <= WIPE_FRAMES + 1; k++) {
        frame (&c, NULL, 0, 1.0f, 1);
        if (k == 5 || k == 10 || k == 15) {
            float cf = (float) k / (float) WIPE_FRAMES;
            double sym = sym_ratio_grown (c.a, mode, cf, halfDiag);
            float outside = 0.0f, inside = 0.0f;
            printf ("  %-28s c=%.2f grown-core sym_ratio = %.4f\n", what, cf, sym);
            /* (b) the grown CORE (inside the smallest possible wobbly
             * frontier c*0.9*halfDiag) is exactly mirror-symmetric */
            check (sym == 0.0, "wipe: grown core exactly symmetric");

            /* (c) outside the LARGEST possible frontier
             * (c*1.1*halfDiag + 0.05) the flow is the frozen k=0 flow
             * (S and D do not move during the wipe). */
            for (i = 0; i < WIPE_N; i++) {
                int dx = wipe_gdx (i), dy = wipe_gdy (i);
                float vx, vy, e;
                if (mode == 1) {
                    if (sqrtf ((float) (dx * dx + dy * dy))
                            < (cf * 1.1f) * halfDiag + 0.05f)
                        continue;
                }
                else {
                    if (wipe_seam_dist (dx, dy) < (cf * 1.1f) * halfDiag + 0.05f)
                        continue;
                }
                query (&c, (float) (dx + W / 2), (float) (dy + H / 2), &vx, &vy);
                e = vlen (vx - u_k0[i][0], vy - u_k0[i][1]);
                if (e > outside) outside = e;
            }
            check (outside <= TOL_FROZEN, "wipe: flow frozen outside the frontier");

            /* (d) inside the grown CORE the flow is equivariant under the
             * NEW fold (the orbit8 group of N=4 / angle=0); the seam flip
             * zone (mates within 2 px of a seam) is excluded */
            for (i = 0; i < WIPE_N; i++) {
                int dx = wipe_gdx (i), dy = wipe_gdy (i), g, ox[8], oy[8];
                float ux, uy;
                if (!wipe_grown (dx, dy, mode, cf, halfDiag))
                    continue;
                if (seam_dist (dx, dy) < 2.0)       /* seam flip zone */
                    continue;
                query (&c, (float) (dx + W / 2), (float) (dy + H / 2), &ux, &uy);
                orbit8 (dx, dy, ox, oy);
                for (g = 1; g < 8; g++) {
                    float gux, guy, qx, qy, e;
                    if (seam_dist (ox[g], oy[g]) < 2.0)
                        continue;
                    if (g < 4) {
                        gux = (g & 1) ? -ux : ux;
                        guy = (g & 2) ? -uy : uy;
                    }
                    else {
                        gux = (g & 1) ? -uy : uy;
                        guy = (g & 2) ? -ux : ux;
                    }
                    query (&c, (float) (ox[g] + W / 2), (float) (oy[g] + H / 2), &qx, &qy);
                    e = vlen (qx - gux, qy - guy);
                    if (e > inside) inside = e;
                }
            }
            check (inside <= TOL_EQUIV, "wipe: inside flow equivariant");
        }
        if (k == 10)
            snap_flow (&c, u_mid);          /* mid-wipe flow at c = 0.5 */
        if (k == WIPE_FRAMES)
            snap_flow (&c, u_end);          /* settled folded flow */
        /* (e) completion: the whole frame is exactly symmetric at the last
         * wipe frame AND on the first settled frame after it */
        if (k == WIPE_FRAMES || k == WIPE_FRAMES + 1) {
            double sym = sym_ratio (c.a);
            printf ("  %-28s k=%d full sym_ratio = %.4f\n", what, k, sym);
            check (sym == 0.0, "wipe: completed frame exactly symmetric");
        }
    }

    /* (f) wobble-invariance pin at k = 10 (c = 0.5): the per-pixel jitter
     * j = 1 + wobble*(2*luma/255 - 1) comes from the orbit-invariant
     * target-map source read (the new table is byte-exact orbit-constant),
     * so the grown set is orbit-invariant and the frontier never cuts an
     * orbit. The tables are frozen during the wipe and the hard frontier
     * samples exactly one of them per pixel, so u_mid(p) equals the k=0
     * flow u_k0(p) outside (exact) or the settled folded flow u_end(p)
     * inside (up to the 1/16 px beta_fix = 65535 residue): the closer
     * endpoint classifies the pixel (beta_hat = 1 = reads the NEW map).
     * The classification must agree across every orbit8 pair and match the
     * wobbly reach bounds. Pixels whose two endpoint flows coincide within
     * the residue noise (chamber pixels sample the same source in both
     * gauges, and their mates) carry no evidence and are skipped: 0.2 px
     * is above twice the largest residue. */
    {
        const float cf = 0.5f;              /* k = 10 of WIPE_FRAMES = 20 */
        const float reachIn = cf * 1.1f * halfDiag + 0.25f;
        const float reachOut = cf * 0.9f * halfDiag - 0.25f;
        int pair_bad = 0, reach_bad = 0, classified = 0;

        for (i = 0; i < WIPE_N; i++) {
            int dx = wipe_gdx (i), dy = wipe_gdy (i), g, ox[8], oy[8];
            float d0 = vlen (u_mid[i][0] - u_k0[i][0], u_mid[i][1] - u_k0[i][1]);
            float d1 = vlen (u_mid[i][0] - u_end[i][0], u_mid[i][1] - u_end[i][1]);
            float d01 = vlen (u_k0[i][0] - u_end[i][0], u_k0[i][1] - u_end[i][1]);
            float metric = (mode == 1)
                ? sqrtf ((float) (dx * dx + dy * dy))
                : wipe_seam_dist (dx, dy);
            int bh;

            if (d01 <= 0.2f)
                continue;               /* endpoints coincide: no evidence */
            classified++;
            bh = (d1 <= d0);            /* 1 = reads the settled (new) map */

            /* reach bounds: the classification agrees with the wobbly
             * frontier's certain reach */
            if (bh ? (metric > reachIn) : (metric < reachOut))
                reach_bad++;

            /* orbit8 invariance of the classification */
            orbit8 (dx, dy, ox, oy);
            for (g = 1; g < 8; g++) {
                int j = ((oy[g] + 28) / 2) * 29 + ((ox[g] + 28) / 2);
                float d0j, d1j, d01j;
                if (j < 0 || j >= WIPE_N)
                    continue;
                d01j = vlen (u_k0[j][0] - u_end[j][0], u_k0[j][1] - u_end[j][1]);
                if (d01j <= 0.2f)
                    continue;
                d0j = vlen (u_mid[j][0] - u_k0[j][0], u_mid[j][1] - u_k0[j][1]);
                d1j = vlen (u_mid[j][0] - u_end[j][0], u_mid[j][1] - u_end[j][1]);
                if ((d1j <= d0j) != bh)
                    pair_bad++;
            }
        }
        printf ("  %-28s beta_hat classified %d probes (endpoint flows > 0.2 px apart)\n",
                what, classified);
        check (pair_bad == 0, "wipe: wobble frontier orbit-invariant");
        check (reach_bad == 0, "wipe: wobble frontier reach bounds");
    }
}

/* ------------------------------------------------------------------ */
/* 1-3: settled-state flow invariants                                  */

static void test_radial_identity (Ctx *c, const char *what) {
    int dx, dy;
    double worst = 0.0;

    for (dy = -24; dy <= 24; dy++) {
        for (dx = -24; dx <= 24; dx++) {
            float ex, ey, ux, uy, e;
            if (dx * dx + dy * dy < 36 || dx * dx + dy * dy > 576)
                continue;          /* min-displacement / source clamp */
            if (abs (dx) < 2 || abs (dy) < 2)
                continue;          /* min-displacement nudge */
            ex = COEF * (float)dx;
            ey = COEF * (float)dy;
            query (c, (float)(dx + W / 2), (float)(dy + H / 2), &ux, &uy);
            e = vlen (ux - ex, uy - ey);
            if (e > worst) worst = e;
        }
    }
    printf ("  %-28s worst |u - coef*(p-m)| = %.4f px\n", what, worst);
    check (worst <= TOL_FLOW, what);
}

static void test_equivariance (Ctx *c) {
    int dx, dy, k, ox[8], oy[8];
    double worst = 0.0;

    for (dy = -24; dy <= 24; dy++) {
        for (dx = -24; dx <= 24; dx++) {
            float ux, uy;
            if (dx * dx + dy * dy < 36 || dx * dx + dy * dy > 576)
                continue;          /* min-displacement / source clamp */
            if (abs (dx) < 2 || abs (dy) < 2)
                continue;          /* min-displacement nudge */
            if (seam_dist (dx, dy) < 2.0)                /* seam flip zone */
                continue;
            orbit8 (dx, dy, ox, oy);
            query (c, (float)(dx + W / 2), (float)(dy + H / 2), &ux, &uy);
            for (k = 1; k < 8; k++) {
                /* image k of the velocity under the same signed permutation */
                float gux, guy, qx, qy, e;
                if (k < 4) {
                    gux = (k & 1) ? -ux : ux;
                    guy = (k & 2) ? -uy : uy;
                }
                else {
                    gux = (k & 1) ? -uy : uy;
                    guy = (k & 2) ? -ux : ux;
                }
                if (seam_dist (ox[k], oy[k]) < 2.0)
                    continue;
                query (c, (float)(ox[k] + W / 2), (float)(oy[k] + H / 2), &qx, &qy);
                e = vlen (qx - gux, qy - guy);
                if (e > worst) worst = e;
            }
        }
    }
    printf ("  %-28s worst |u(g.p) - g.u(p)| = %.4f px\n", "equivariance", worst);
    check (worst <= TOL_EQUIV, "equivariance u(g.p) == g.u(p)");
}

/* ------------------------------------------------------------------ */
/* 4-5: transition invariants                                          */

/* the flow must never carry the fold's own displacement: bounded like the
 * base mode everywhere, at every instant of a transition */
static void check_flow_bound (Ctx *c, const char *what) {
    int dx, dy;
    float worst = 0.0f;

    for (dy = -H / 2 + 4; dy <= H / 2 - 4; dy += 2) {
        for (dx = -W / 2 + 4; dx <= W / 2 - 4; dx += 2) {
            float ux, uy, m;
            int d2 = dx * dx + dy * dy;
            /* clamp-safe probe range, same convention as the flow tests:
             * the source clamp bites when the folded source lands outside
             * the buffer (beyond radius 24 px at chamber-edge angles) and
             * the min-displacement nudge dominates below radius 6. The
             * clamp displacement is documented kernel behavior, not
             * flow. */
            if (d2 < 6 * 6 || d2 > 24 * 24)
                continue;
            query (c, (float)(dx + W / 2), (float)(dy + H / 2), &ux, &uy);
            m = vlen (ux, uy);
            if (m > worst) worst = m;
        }
    }
    check (worst <= FLOW_BOUND, what);
    if (worst > FLOW_BOUND)
        printf ("  %-28s worst |u| = %.3f px (bound %.3f)\n", what, worst, FLOW_BOUND);
}

/* ------------------------------------------------------------------ */
/* 8: re-slice interpolation + the flow sweep term                     */

/* Re-slice to a fold whose group contains orbit8: N=4 -> N=8, angle 0 on
 * both sides (the D4 axes at m*pi/4 are among the D8 axes at m*pi/8). The
 * Linear path re-gauges the merged state into the N=8 chamber at the bake
 * and then interpolates chamber-locally, so every blend of the two
 * orbit8-constant tables renders exactly orbit8-symmetric, at every step
 * (a re-slice must interpolate, not wipe). The flow sweep term is measured
 * at one morph state: three fresh contexts advance identically to the same
 * buffratio with different last-frame ramp steps dr (si, 2*si, 0), so the
 * base flow is identical and the query differences are exactly the sweep,
 * which must be present, linear in dr and bounded. */
static void test_reslice_sweep (void) {
    static float ua[WIPE_N][2], ub[WIPE_N][2], uz[WIPE_N][2];
    Ctx a, b, z;
    int i, k, moved = 0;
    float worst_lin = 0.0f, worst_sweep = 0.0f;

    /* three identical contexts: steady N=4/angle=0 state, then one re-slice
     * to N=8/angle=0 (Fold Transition = Linear) and the stride-1 table
     * build; the bake + swap happens on the first morph frame */
    {
        Ctx *ctxs[3];
        ctxs[0] = &a; ctxs[1] = &b; ctxs[2] = &z;
        for (i = 0; i < 3; i++) {
            Ctx *c = ctxs[i];
            ctx_init (c);
            c->zf.kaleidoEffect = 1;
            c->zf.foldCount = 4;
            c->zf.foldAngle = 0.0f;
            frames (c, &c->zf, 128, 0x7fff, 1.0f, 1);   /* build + click + settle */
            c->zf.foldCount = 8;
            frame (c, &c->zf, 0, 1.0f, 1);              /* re-slice config */
            for (k = 0; k < 63; k++)                    /* stride-1 table build */
                frame (c, NULL, 0, 1.0f, 1);
        }
    }

    /* context a: the morph, one ramp step per frame; every step must stay
     * exactly orbit8-symmetric and the flow bounded. 8191 then 8192 land
     * exactly on the buffratio clamp (65535) with the last step = 8192, so
     * this context is also the dr = 8192 probe state. */
    printf ("8. re-slice interpolation + flow sweep\n");
    for (i = 0; i < 6; i++) {
        frame (&a, NULL, 8192, 1.0f, 1);             /* buffratio 49152 */
        check (sym_ratio (a.a) == 0.0, "reslice: step exactly orbit8-symmetric");
        check_flow_bound (&a, "flow bound (re-slice morph)");
    }
    frame (&a, NULL, 8191, 1.0f, 1);                 /* 57343 */
    check (sym_ratio (a.a) == 0.0, "reslice: step exactly orbit8-symmetric");
    frame (&a, NULL, 8192, 1.0f, 1);                 /* 65535 exactly */
    check (sym_ratio (a.a) == 0.0, "reslice: step exactly orbit8-symmetric");
    check_flow_bound (&a, "flow bound (re-slice morph)");
    snap_flow (&a, ua);                             /* last ramp step dr = 8192 */

    /* context b: same buffratio clamp, last ramp step 16384 */
    for (i = 0; i < 5; i++)
        frame (&b, NULL, 8192, 1.0f, 1);             /* 40960 */
    frame (&b, NULL, 8191, 1.0f, 1);                 /* 49151 */
    frame (&b, NULL, 16384, 1.0f, 1);                /* 65535, dr = 16384 */
    snap_flow (&b, ub);

    /* context z: same buffratio clamp, ramp frozen on the last frame: the
     * step is absorbed by the clamp, so dr = 0 */
    for (i = 0; i < 6; i++)
        frame (&z, NULL, 8192, 1.0f, 1);             /* 49152 */
    frame (&z, NULL, 8191, 1.0f, 1);                 /* 57343 */
    frame (&z, NULL, 8192, 1.0f, 1);                 /* 65535 */
    frame (&z, NULL, 8192, 1.0f, 1);                 /* clamped: dr = 0 */
    snap_flow (&z, uz);

    /* (ii) on the ring where the re-slice moved the map: the sweep is
     * present, exactly linear in dr (the dr = 16384 difference must be
     * twice the dr = 8192 one) and bounded. The three queries share the
     * tables, the buffratio and the committed gauge, so ua - uz and
     * ub - uz are the sweep term alone: -g . (brutD - brutS) * dr/(65536*16)
     * with dr = this frame's ramp step (prevRatio holds the pre-ramp
     * ratio). */
    for (i = 0; i < WIPE_N; i++) {
        float dx = (float) wipe_gdx (i), dy = (float) wipe_gdy (i);
        float rr = sqrtf (dx * dx + dy * dy);
        float d1x, d1y, d2x, d2y, dl;
        if (rr < 8.0f || rr > 24.0f)
            continue;
        d1x = ua[i][0] - uz[i][0];
        d1y = ua[i][1] - uz[i][1];
        d2x = ub[i][0] - uz[i][0];
        d2y = ub[i][1] - uz[i][1];
        dl = vlen (d1x, d1y);
        if (dl > 0.0f)
            moved++;
        if (dl > worst_sweep)
            worst_sweep = dl;
        if (fabsf (d2x - 2.0f * d1x) > worst_lin)
            worst_lin = fabsf (d2x - 2.0f * d1x);
        if (fabsf (d2y - 2.0f * d1y) > worst_lin)
            worst_lin = fabsf (d2y - 2.0f * d1y);
    }
    printf ("  %-28s sweep: %d probes moved, max %.4f px, linearity err %.4f px\n",
            "re-slice", moved, worst_sweep, worst_lin);
    check (moved > 0, "reslice: flow sweep present where the morph moved");
    check (worst_sweep <= TOL_SWEEP_BOUND, "reslice: sweep bounded");
    check (worst_lin <= TOL_SWEEP_LINEAR, "reslice: sweep linear in dr");
}

/* ------------------------------------------------------------------ */
/* 9: wipe settle at a real canvas (W/H set by the caller; see main)   */

static void test_wipe_settle_real (int mode, const char *what) {
    Ctx c;
    ctx_init (&c);
    c.zf.kaleidoEffect = 1;
    c.zf.foldCount = 4;
    c.zf.foldAngle = 0.0f;
    c.zf.hPlaneEffect = 20;
    c.zf.vPlaneEffect = -14;
    IVAL (*c.info.zoomFilter_fx.params->params[16]) = mode;   /* fold transition */
    /* frames 17..19 stay at the engine defaults (20 / hard / 10% wobble) */
    /* build (~resy/(resy/64) = 68 frames at 542) + wipe (20) + settle */
    frames (&c, &c.zf, 160, 0, 1.0f, 1);
    {
        double sym = sym_ratio (c.a);
        printf ("  %-28s settled sym_ratio = %.6f\n", what, sym);
        check (sym == 0.0, "wipe: settled exactly symmetric at a real canvas");
    }
}

int main (void) {
    Ctx c;
    int i, click = -1;

    ctx_init (&c);
    check (IVAL (*c.info.zoomFilter_fx.params->params[16]) == 0
           && IVAL (*c.info.zoomFilter_fx.params->params[17]) == 0
           && IVAL (*c.info.zoomFilter_fx.params->params[18]) == 0
           && IVAL (*c.info.zoomFilter_fx.params->params[19]) == 0,
           "legacy param defaults (Linear fold transition, wipe params quieted)");

    /* ---- 1. radial identity, no fold ------------------------------ */
    frames (&c, &c.zf, 32, 0x7fff, 1.0f, 1);             /* settle */
    printf ("1. flow radial identity\n");
    test_radial_identity (&c, "unfolded");

    /* ---- 2. radial identity, folded ------------------------------- */
    c.zf.kaleidoEffect = 1;
    c.zf.foldCount = 6;
    c.zf.foldAngle = 1.3f;
    frames (&c, &c.zf, 128, 0x7fff, 1.0f, 1);            /* build + morph */
    printf ("2. flow radial identity (folded)\n");
    test_radial_identity (&c, "folded N=6");

    /* ---- 3. equivariance, non-radial mode ------------------------- */
    c.zf.mode = AMULETTE_MODE;
    c.zf.hypercosEffect = 1;
    c.zf.hPlaneEffect = 20;
    c.zf.vPlaneEffect = -14;
    c.zf.foldCount = 4;          /* orbit8 below is this fold's group */
    c.zf.foldAngle = 0.0f;
    frames (&c, &c.zf, 80, 0x7fff, 1.0f, 1);             /* no gauge change */
    printf ("3. equivariance of the folded flow\n");
    test_equivariance (&c);

    /* ---- 4. morph symmetry (fold ON) ------------------------------ */
    /* fresh state: square N=4 / angle=0 fold so orbit mates are pixels.
     * Fold Transition = Linear: this test pins the legacy click + lerp
     * morph path. */
    {
        Ctx d;
        ctx_init (&d);
        d.zf.kaleidoEffect = 0;
        IVAL (*d.info.zoomFilter_fx.params->params[16]) = 0;    /* fold transition: linear */
        IVAL (*d.info.zoomFilter_fx.params->params[17]) = 2;    /* wipe frames (unused) */
        IVAL (*d.info.zoomFilter_fx.params->params[18]) = 0;    /* wipe ring (unused) */
        IVAL (*d.info.zoomFilter_fx.params->params[19]) = 0;    /* wipe wobble (unused) */
        frames (&d, &d.zf, 32, 0x7fff, 1.0f, 1);
        d.zf.kaleidoEffect = 1;
        d.zf.foldCount = 4;
        d.zf.foldAngle = 0.0f;
        printf ("4. mirror symmetry through the fold morph\n");
        for (i = 0; i < 72; i++) {                       /* build runs ~65 */
            frame (&d, (i == 0) ? &d.zf : NULL, 0, 1.0f, 1);
            check_flow_bound (&d, "flow bound (fold ON)");
            if (click < 0) {
                if (sym_ratio (d.a) <= SYM_TOL) {
                    click = i;
                    check (i >= 60 && i <= 70, "click lands at the table swap");
                }
            } else {
                check (sym_ratio (d.a) <= SYM_TOL, "symmetric after the click");
            }
        }
        check (click >= 0, "fold click observed");
        printf ("  %-28s click at frame %d (of ~65)\n", "fold ON", click);
        /* stepped interpolation: 12.5% per frame, symmetric throughout */
        for (i = 0; i < 8; i++) {
            frame (&d, NULL, 8192, 1.0f, 1);
            check (sym_ratio (d.a) <= SYM_TOL, "symmetric mid-morph");
            check_flow_bound (&d, "flow bound (fold ON morph)");
        }

        /* ---- 5. bounded flow through the un-folding ---------------- */
        d.zf.kaleidoEffect = 0;
        d.zf.foldCount = 0;
        d.zf.foldAngle = 0.0f;
        printf ("5. bounded flow through the un-folding morph\n");
        for (i = 0; i < 24; i++) {
            frame (&d, (i == 0) ? &d.zf : NULL, 0, 1.0f, 1);
            check_flow_bound (&d, "flow bound (fold OFF)");
        }
        for (i = 0; i < 8; i++) {
            frame (&d, NULL, 8192, 1.0f, 1);
            check_flow_bound (&d, "flow bound (fold OFF morph)");
        }
    }

    /* ---- 6-7. wipe transitions (radius, seams) --------------------- */
    test_wipe (1, "wipe radius");
    test_wipe (2, "wipe seams");

    /* ---- 8. re-slice interpolation + flow sweep -------------------- */
    test_reslice_sweep ();

    /* ---- 9. wipe settle at a real canvas (864x542) ------------------ */
    {
        int savedW = W, savedH = H;
        W = 864; H = 542;
        printf ("9. wipe settle at a real canvas\n");
        test_wipe_settle_real (1, "wipe radius 864x542");
        test_wipe_settle_real (2, "wipe seams 864x542");
        W = savedW; H = savedH;
    }

    /* ---- 10. fold OFF with a concurrent middle jump ------------------ */
    /* the goom "changement de milieu" fires in the SAME mode-change event
     * that stops the fold (and the recenter pin only guards while the
     * fold is on): the un-projection must fold around the apex the
     * mandala was SHOWN with - the one captured in the gauge - not
     * around the new middle. Without the capture the sources tear by
     * O(screen) at the swap and the perceived flow explodes. */
    {
        Ctx e;
        ctx_init (&e);
        e.zf.kaleidoEffect = 1;
        e.zf.foldCount = 4;
        e.zf.foldAngle = 0.0f;
        frames (&e, &e.zf, 100, 0, 1.0f, 1);   /* build + settle */
        printf ("10. fold OFF with a concurrent middle jump\n");
        e.zf.kaleidoEffect = 0;
        e.zf.foldCount = 0;
        e.zf.middleX = W - 1;                  /* the goom-event jump */
        for (i = 0; i < 24; i++) {
            frame (&e, (i == 0) ? &e.zf : NULL, 0, 1.0f, 1);
            check_flow_bound (&e, "flow bound (fold OFF + middle jump)");
        }
        for (i = 0; i < 8; i++) {
            frame (&e, NULL, 8192, 1.0f, 1);
            check_flow_bound (&e, "flow bound (fold-off morph)");
        }
    }

    if (failures == 0) {
        printf ("KALEIDO FLOW: PASS\n");
        return 0;
    }
    printf ("KALEIDO FLOW: %d FAILURES\n", failures);
    return 1;
}

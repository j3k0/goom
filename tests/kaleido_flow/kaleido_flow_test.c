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
 *  5. bounded flow (fold OFF): through the un-folding morph the flow stays
 *     bounded like the base mode. The fold displacement leaks O(screen) into
 *     the field if the tables are not re-projected into the pixel's sector.
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
                           int precalCoef[16][16]);

/* the canvas is square and even: the (N=4, angle=0) fold's 8 isometries map
 * pixels to pixels, so symmetry can be compared byte-exactly */
#define W 64
#define H 64

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

    /* transition boost off: the harness steps buffratio itself */
    IVAL (*c->info.zoomFilter_fx.params->params[16]) = 0;
    IVAL (*c->info.zoomFilter_fx.params->params[17]) = 0;
    IVAL (*c->info.zoomFilter_fx.params->params[18]) = 0;

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
            query (c, (float)(dx + W / 2), (float)(dy + H / 2), &ux, &uy);
            m = vlen (ux, uy);
            if (m > worst) worst = m;
        }
    }
    check (worst <= FLOW_BOUND, what);
    if (worst > FLOW_BOUND)
        printf ("  %-28s worst |u| = %.3f px (bound %.3f)\n", what, worst, FLOW_BOUND);
}

int main (void) {
    Ctx c;
    int i, click = -1;

    ctx_init (&c);
    check (IVAL (*c.info.zoomFilter_fx.params->params[16]) == 0
           && IVAL (*c.info.zoomFilter_fx.params->params[17]) == 0
           && IVAL (*c.info.zoomFilter_fx.params->params[18]) == 0,
           "transition boost disabled for the test");

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
    /* fresh state: square N=4 / angle=0 fold so orbit mates are pixels */
    {
        Ctx d;
        ctx_init (&d);
        d.zf.kaleidoEffect = 0;
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

    if (failures == 0) {
        printf ("KALEIDO FLOW: PASS\n");
        return 0;
    }
    printf ("KALEIDO FLOW: %d FAILURES\n", failures);
    return 1;
}

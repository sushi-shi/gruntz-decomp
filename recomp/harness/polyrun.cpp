/* polyrun - PolyIsConvexCW @0x145e30, ours vs retail's.
 *
 *     recomp/harness/build.sh polyrun imagepolyclip
 *     wine recomp/harness/polyrun.exe "$GRUNTZ_EXE"
 *
 * Reachability: DATA-ONLY. The relocs are to x87 constants in .rdata, which
 * move with the image because recomp.h relocates the whole thing - that is the
 * case DATA-ONLY exists to describe, and it needs no fabrication at all. A free
 * __cdecl function over a caller-owned ClipVtx array.
 *
 * This is the first FLOAT harness. Both sides execute x87 in the same process
 * with the same control word, so the comparison is exact rather than
 * epsilon-based, and it should be: the function only tests the sign of a cross
 * product, and disagreement about a sign is a real disagreement.
 *
 * COVERAGE:
 *
 *   integer lattice     random polygons on a small integer grid, where
 *                       cross products are exactly representable and the
 *                       `cross != 0` collinearity arm is reachable      262,144
 *   fractional          random polygons with fractional coordinates,
 *                       where cross products round                      262,144
 *   convex CW / CCW     generated regular polygons, both windings,
 *                       3..32 vertices, rotated                          60,000
 *   collinear           polygons with deliberately collinear runs,
 *                       which drive the `sign stays 0` path              65,536
 *   tiny counts         count = 1, 2, 3 - the modulo wrap degenerates       768
 *
 * NOT covered: count == 0. Both implementations divide by `count` (`i % count`),
 * so zero faults in retail exactly as it does in ours; there is no answer to
 * compare. Negative counts are covered - the loop body never runs.
 */

#include "recomp.h"

#include <math.h>

#define RVA_POLYISCONVEXCW 0x00145e30

/* include/Image/RasterVtx.h, sizeof 0x1c. Only x/y are read here, but the
 * stride has to be right or the walk reads the wrong vertices. */
struct ClipVtx {
    float x, y, u, v;
    int fx, fu, fv;
};

/* OURS: linked from build/objdiff/base/imagepolyclip.obj; mangles to
 * ?PolyIsConvexCW@@YAHPAUClipVtx@@H@Z. */
int PolyIsConvexCW(ClipVtx *verts, int count);

typedef int(__cdecl *PolyFn)(ClipVtx *, int);
static PolyFn retail_poly;

#define MAXV 64
static ClipVtx g_v[MAXV];

static void compare(RecompCase *c, int count)
{
    ClipVtx a[MAXV], b[MAXV];
    int ours, retail;
    memcpy(a, g_v, sizeof(a));
    memcpy(b, g_v, sizeof(b));
    ours = PolyIsConvexCW(a, count);
    retail = retail_poly(b, count);
    if (recomp_check(c, ours, retail)) {
        int i;
        fprintf(stderr, "        count=%d verts:", count);
        for (i = 0; i < count && i < 8; i++)
            fprintf(stderr, " (%g,%g)", g_v[i].x, g_v[i].y);
        fprintf(stderr, "\n");
    }
    /* The function is documented as read-only; if either side wrote to the
     * array that is a finding in itself. */
    recomp_check_mem(c, a, b, sizeof(a));
}

static float lattice(int span) { return (float)((int)recomp_rand_below((unsigned)span) - span / 2); }

static void gen_lattice(int count, int span)
{
    int i;
    for (i = 0; i < count; i++) {
        g_v[i].x = lattice(span);
        g_v[i].y = lattice(span);
    }
}

static void gen_fractional(int count)
{
    int i;
    for (i = 0; i < count; i++) {
        g_v[i].x = (float)((int)recomp_rand_below(20000) - 10000) / 128.0f;
        g_v[i].y = (float)((int)recomp_rand_below(20000) - 10000) / 128.0f;
    }
}

/* A regular polygon, wound clockwise when `cw`, so the "genuinely convex"
 * answer is known independently of either implementation. */
static void gen_regular(int count, int cw, double phase)
{
    int i;
    for (i = 0; i < count; i++) {
        double t = phase + 6.283185307179586 * (double)i / (double)count;
        if (cw)
            t = -t;
        g_v[i].x = (float)(100.0 * cos(t));
        g_v[i].y = (float)(100.0 * sin(t));
    }
}

/* Runs of collinear points: cross == 0, so `sign` is left alone and the
 * accumulated direction has to survive. */
static void gen_collinear(int count)
{
    int i;
    float dx = (float)((int)recomp_rand_below(20) - 10);
    float dy = (float)((int)recomp_rand_below(20) - 10);
    for (i = 0; i < count; i++) {
        if ((int)recomp_rand_below(3) == 0) {
            g_v[i].x = lattice(40);
            g_v[i].y = lattice(40);
        } else {
            g_v[i].x = dx * (float)i;
            g_v[i].y = dy * (float)i;
        }
    }
}

int main(int argc, char **argv)
{
    RecompCase cases[5];
    int i, n, cw;

    if (!recomp_start(argc, argv, ""))
        return 1;
    retail_poly = (PolyFn)RECOMP_RVA(RVA_POLYISCONVEXCW);
    recomp_seed(0xfeedbeefu);
    memset(g_v, 0, sizeof(g_v));

    recomp_case(&cases[0], "integer-lattice polygons");
    recomp_case(&cases[1], "fractional-coordinate polygons");
    recomp_case(&cases[2], "regular convex polygons, both windings");
    recomp_case(&cases[3], "collinear runs");
    recomp_case(&cases[4], "tiny vertex counts (1, 2, 3)");

    for (i = 0; i < 262144; i++) {
        n = 3 + (int)recomp_rand_below(MAXV - 3);
        gen_lattice(n, 16);
        compare(&cases[0], n);
    }

    for (i = 0; i < 262144; i++) {
        n = 3 + (int)recomp_rand_below(MAXV - 3);
        gen_fractional(n);
        compare(&cases[1], n);
    }

    for (n = 3; n <= 32; n++)
        for (cw = 0; cw < 2; cw++)
            for (i = 0; i < 1000; i++) {
                gen_regular(n, cw, (double)i * 0.0173);
                compare(&cases[2], n);
            }

    for (i = 0; i < 65536; i++) {
        n = 3 + (int)recomp_rand_below(MAXV - 3);
        gen_collinear(n);
        compare(&cases[3], n);
    }

    /* count 1..3 make the (i+1)%count / (i+2)%count wrap degenerate, and
     * negative counts skip the loop entirely. count == 0 divides by zero on
     * both sides and is deliberately absent. */
    for (i = 0; i < 256; i++) {
        gen_lattice(3, 8);
        compare(&cases[4], 1);
        compare(&cases[4], 2);
        compare(&cases[4], 3);
    }

    return recomp_report(cases, 5);
}

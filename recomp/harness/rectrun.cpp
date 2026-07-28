/* rectrun - CGrunt::RectSegProbe @0x62b70, ours vs retail's.
 *
 *     recomp/harness/build.sh rectrun gruntentrancearrival
 *     wine recomp/harness/rectrun.exe "$GRUNTZ_EXE"
 *
 * Reachability: ISLAND, 517 bytes - the largest thing harnessed here. It is a
 * __thiscall member of CGrunt, which reads as "needs a whole grunt", and it
 * needs nothing: the prologue is
 *
 *     sub esp,0x10 / mov edx,[esp+0x18] / mov eax,[esp+0x1c] / ...
 *     mov ecx,[edx+0x4]
 *
 * - `ecx` is overwritten with `p->top` before `this` is ever read, and never
 * read again. The audit's "3 fields" are RECT/POINT members, not CGrunt ones.
 * This is exactly the under-count noted in recomp/README.md.
 *
 * Does the directed segment e1->e2 cross into the box `p`? Four edge tests,
 * each straddle-then-interpolate in float. The interpolation divides by
 * (e2y - e1y) or (e2x - e1x), which the straddle test guards against being
 * zero - so axis-aligned segments are a boundary worth aiming at deliberately,
 * and they are, below.
 *
 * COVERAGE:
 *
 *   random             random rects and segments over a +-64 grid,
 *                      dense enough that hits and misses are both common  524,288
 *   axis-aligned       horizontal and vertical segments, where the
 *                      straddle test is what stops a divide by zero       131,072
 *   on-edge            endpoints placed exactly on a rect edge or
 *                      corner, the float-compare boundary                 131,072
 *   degenerate rects   empty (left==right), inverted (left>right),
 *                      zero-area, and single-point rects                   65,536
 *   large magnitudes   coordinates near +-2^20, where int->float
 *                      conversion in the interpolation starts to round     65,536
 *
 * The space is 8 independent ints, so this is a sweep. The grid is kept small
 * on purpose: over +-64 a random segment actually crosses the box often, where
 * over +-2^31 essentially every case would be a trivial miss and the sweep
 * would prove nothing.
 */

#include "recomp.h"

#define RVA_RECTSEGPROBE 0x00062b70

/* OURS: linked from build/objdiff/base/gruntentrancearrival.obj; mangles to
 * ?RectSegProbe@CGrunt@@QAEHPAUtagRECT@@PAUtagPOINT@@1@Z. */
class CGrunt {
public:
    int RectSegProbe(RECT *p, POINT *e1, POINT *e2);
};

static int retail_probe(RECT *p, POINT *e1, POINT *e2)
{
    int dummy = 0; /* never dereferenced - see the header comment */
    return recomp_thiscall3(RECOMP_RVA(RVA_RECTSEGPROBE), &dummy, (int)p, (int)e1,
                            (int)e2);
}

static long g_hits = 0;
static RecompCase g_mutation; /* neither side should touch the caller's structs */

static void compare(RecompCase *c, RECT *r, POINT *a, POINT *b)
{
    RECT r1 = *r, r2 = *r;
    POINT a1 = *a, a2 = *a, b1 = *b, b2 = *b;
    int ours = ((CGrunt *)&r1)->RectSegProbe(&r1, &a1, &b1);
    int retail = retail_probe(&r2, &a2, &b2);
    if (ours)
        g_hits++;
    if (recomp_check(c, ours, retail))
        fprintf(stderr, "        rect(%ld,%ld,%ld,%ld) seg(%ld,%ld)-(%ld,%ld)\n", r->left,
                r->top, r->right, r->bottom, a->x, a->y, b->x, b->y);
    recomp_check_mem(&g_mutation, &r1, &r2, sizeof(RECT));
    recomp_check_mem(&g_mutation, &a1, &a2, sizeof(POINT));
    recomp_check_mem(&g_mutation, &b1, &b2, sizeof(POINT));
}

static long grid(int span)
{
    return (long)((int)recomp_rand_below((unsigned)(2 * span + 1)) - span);
}

int main(int argc, char **argv)
{
    RecompCase cases[6];
    RECT r;
    POINT a, b;
    int i;

    if (!recomp_start(argc, argv, ""))
        return 1;
    recomp_seed(0x5f3759dfu);

    recomp_case(&cases[0], "random rects x random segments");
    recomp_case(&cases[1], "axis-aligned segments (the divide-by-zero guard)");
    recomp_case(&cases[2], "endpoints exactly on an edge or corner");
    recomp_case(&cases[3], "degenerate / inverted rects");
    recomp_case(&cases[4], "large coordinates (int->float rounding)");
    recomp_case(&g_mutation, "caller structs left unmodified");

    for (i = 0; i < 131072; i++) {
        r.left = grid(64);
        r.top = grid(64);
        r.right = r.left + grid(64) + 64;
        r.bottom = r.top + grid(64) + 64;
        a.x = grid(96);
        a.y = grid(96);
        b.x = grid(96);
        b.y = grid(96);
        compare(&cases[0], &r, &a, &b);
    }

    for (i = 0; i < 32768; i++) {
        r.left = grid(32);
        r.top = grid(32);
        r.right = r.left + grid(32) + 32;
        r.bottom = r.top + grid(32) + 32;
        a.x = grid(64);
        a.y = grid(64);
        if (i & 1) { /* horizontal */
            b.x = grid(64);
            b.y = a.y;
        } else { /* vertical */
            b.x = a.x;
            b.y = grid(64);
        }
        compare(&cases[1], &r, &a, &b);
    }

    for (i = 0; i < 32768; i++) {
        r.left = grid(32);
        r.top = grid(32);
        r.right = r.left + 1 + grid(16) + 16;
        r.bottom = r.top + 1 + grid(16) + 16;
        /* place one endpoint exactly on a corner or edge midpoint */
        switch (recomp_rand_below(6)) {
        case 0: a.x = r.left;  a.y = r.top;    break;
        case 1: a.x = r.right; a.y = r.top;    break;
        case 2: a.x = r.left;  a.y = r.bottom; break;
        case 3: a.x = r.right; a.y = r.bottom; break;
        case 4: a.x = (r.left + r.right) / 2; a.y = r.top;  break;
        default: a.x = r.left; a.y = (r.top + r.bottom) / 2; break;
        }
        b.x = grid(64);
        b.y = grid(64);
        compare(&cases[2], &r, &a, &b);
    }

    for (i = 0; i < 16384; i++) {
        long x = grid(32), y = grid(32);
        switch (recomp_rand_below(4)) {
        case 0: r.left = x; r.right = x; r.top = y; r.bottom = y + 8; break; /* zero width */
        case 1: r.left = x; r.right = x + 8; r.top = y; r.bottom = y; break; /* zero height */
        case 2: r.left = x; r.right = x; r.top = y; r.bottom = y; break;     /* a point */
        default: r.left = x + 16; r.right = x; r.top = y + 16; r.bottom = y; break; /* inverted */
        }
        a.x = grid(48);
        a.y = grid(48);
        b.x = grid(48);
        b.y = grid(48);
        compare(&cases[3], &r, &a, &b);
    }

    for (i = 0; i < 16384; i++) {
        r.left = grid(1 << 20);
        r.top = grid(1 << 20);
        r.right = r.left + grid(1 << 20) + (1 << 20);
        r.bottom = r.top + grid(1 << 20) + (1 << 20);
        a.x = grid(1 << 21);
        a.y = grid(1 << 21);
        b.x = grid(1 << 21);
        b.y = grid(1 << 21);
        compare(&cases[4], &r, &a, &b);
    }

    fprintf(stderr, "(segments reported as crossing: %ld - a sweep of pure misses "
                    "would prove nothing)\n", g_hits);
    cases[5] = g_mutation;
    return recomp_report(cases, 6);
}

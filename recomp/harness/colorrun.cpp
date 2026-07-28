/* colorrun - CShadeTableCache::FindNearestColor @0x14fbf0, ours vs retail's.
 *
 *     recomp/harness/build.sh colorrun shadetablecache
 *     wine recomp/harness/colorrun.exe "$GRUNTZ_EXE"
 *
 * Reachability: ISLAND (no relocs, no calls). A static member, so plain
 * __cdecl - no `this` at all. Every input is a scalar or a caller-owned
 * 256-entry PALETTEENTRY array. Nothing to fabricate.
 *
 * Retail (0x14fbf0) masks all three query channels with `and reg,0xff`
 * (0x14fc09 / 0x14fc16 / 0x14fc25) and keeps the LOWER index on a distance tie
 * (`cmp ecx,ebp / jge` - strictly-less wins). Both are behaviours a plausible
 * reimplementation could get wrong without moving the byte-match needle, so
 * both are swept for explicitly below rather than left to random luck.
 *
 * COVERAGE - what is actually checked, no more:
 *
 *   exhaustive-query   1 palette x all 2^24 (r,g,b) triples          16,777,216
 *   random             256 palettes x 4,096 random queries            1,048,576
 *   tie-break          palettes built with deliberate equidistant
 *                      entries, queries aimed between them              65,536
 *   out-of-range       queries with negative / >255 channels, which
 *                      only the `and 0xff` makes well-defined           65,536
 *   degenerate         all-identical, all-zero, all-0xff palettes        3,072
 *
 * The (palette x query) space is far too large to exhaust - 2^6144 palettes -
 * so this is a sweep, not a proof. What it does exhaust is the query space for
 * a fixed palette, which is where the tie-break and masking live.
 */

#include "recomp.h"

#define RVA_FINDNEARESTCOLOR 0x0014fbf0

/* OURS: linked from build/objdiff/base/shadetablecache.obj. Declaring the
 * class with just this member reproduces the mangled name exactly
 * (?FindNearestColor@CShadeTableCache@@SAHPAUtagPALETTEENTRY@@HHH@Z), so the
 * linker binds our real compiled body - not a copy of its source. */
class CShadeTableCache {
public:
    static int FindNearestColor(PALETTEENTRY *pal, int r, int g, int b);
};

typedef int(__cdecl *FindNearestColorFn)(PALETTEENTRY *, int, int, int);

static FindNearestColorFn retail_find;

static void fill_random_palette(PALETTEENTRY *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        unsigned int v = recomp_rand();
        pal[i].peRed = (unsigned char)(v & 0xff);
        pal[i].peGreen = (unsigned char)((v >> 8) & 0xff);
        pal[i].peBlue = (unsigned char)((v >> 16) & 0xff);
        pal[i].peFlags = (unsigned char)((v >> 24) & 0xff); /* must be ignored */
    }
}

/* A palette where many entries sit at the SAME distance from the probe points,
 * so the lower-index-wins rule is what decides the answer. */
static void fill_tie_palette(PALETTEENTRY *pal)
{
    int i;
    for (i = 0; i < 256; i++) {
        /* Four colours, each repeated 64 times: every query is equidistant
         * from 64 identical entries and the tie-break picks among them. */
        static const unsigned char base[4][3] = {
            { 0, 0, 0 }, { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }
        };
        pal[i].peRed = base[i & 3][0];
        pal[i].peGreen = base[i & 3][1];
        pal[i].peBlue = base[i & 3][2];
        pal[i].peFlags = (unsigned char)i;
    }
}

static void fill_const_palette(PALETTEENTRY *pal, unsigned char v)
{
    int i;
    for (i = 0; i < 256; i++) {
        pal[i].peRed = v;
        pal[i].peGreen = v;
        pal[i].peBlue = v;
        pal[i].peFlags = 0;
    }
}

static int compare(RecompCase *c, PALETTEENTRY *pal, int r, int g, int b)
{
    int ours = CShadeTableCache::FindNearestColor(pal, r, g, b);
    int retail = retail_find(pal, r, g, b);
    if (recomp_check(c, ours, retail)) {
        fprintf(stderr, "        query (r=%d g=%d b=%d)\n", r, g, b);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    static PALETTEENTRY pal[256];
    RecompCase cases[5];
    int i, j, r, g, b;

    if (!recomp_start(argc, argv, ""))
        return 1;
    retail_find = (FindNearestColorFn)RECOMP_RVA(RVA_FINDNEARESTCOLOR);
    recomp_seed(0x9e3779b9u);

    recomp_case(&cases[0], "exhaustive query sweep, 1 random palette");
    recomp_case(&cases[1], "random palettes x random queries");
    recomp_case(&cases[2], "tie-break (equidistant entries)");
    recomp_case(&cases[3], "out-of-range channels (tests the and 0xff)");
    recomp_case(&cases[4], "degenerate palettes");

    /* 1. every (r,g,b) against one palette. */
    fill_random_palette(pal);
    for (r = 0; r < 256; r++) {
        for (g = 0; g < 256; g++)
            for (b = 0; b < 256; b++)
                compare(&cases[0], pal, r, g, b);
        if ((r & 31) == 0)
            fprintf(stderr, "  ... exhaustive sweep r=%d\n", r);
    }

    /* 2. broad random. */
    for (i = 0; i < 256; i++) {
        fill_random_palette(pal);
        for (j = 0; j < 4096; j++)
            compare(&cases[1], pal, (int)recomp_rand_below(256),
                    (int)recomp_rand_below(256), (int)recomp_rand_below(256));
    }

    /* 3. ties. */
    fill_tie_palette(pal);
    for (i = 0; i < 65536; i++)
        compare(&cases[2], pal, (int)recomp_rand_below(256),
                (int)recomp_rand_below(256), (int)recomp_rand_below(256));

    /* 4. channels outside 0..255. Only the `and 0xff` gives these a defined
     * answer; a reimplementation that skipped the mask would diverge here and
     * nowhere else. */
    fill_random_palette(pal);
    for (i = 0; i < 65536; i++) {
        int hi = (int)(recomp_rand() & 0xffffff);
        int neg = -(int)(recomp_rand() & 0xffff);
        compare(&cases[3], pal, hi, neg, (int)recomp_rand());
    }

    /* 5. degenerate palettes: nothing to choose between. */
    for (i = 0; i < 3; i++) {
        static const unsigned char v[3] = { 0x00, 0x7f, 0xff };
        fill_const_palette(pal, v[i]);
        for (j = 0; j < 1024; j++)
            compare(&cases[4], pal, (int)recomp_rand_below(256),
                    (int)recomp_rand_below(256), (int)recomp_rand_below(256));
    }

    return recomp_report(cases, 5);
}

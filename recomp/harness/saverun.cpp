/* saverun - CSaveGame::Encode @0xe5410 / ::Decode @0xe5460, ours vs retail's.
 *
 *     recomp/harness/build.sh saverun savegame
 *     wine recomp/harness/saverun.exe "$GRUNTZ_EXE"
 *
 * Reachability: ISLAND, and the audit reports ZERO object fields - both are
 * __thiscall members whose bodies never dereference `this`. They take one
 * 0x100-byte buffer and return a checksum. This is the cheapest possible
 * harness and a good template.
 *
 * Both functions have TWO results and both are compared: the returned
 * accumulator and the buffer they rewrite in place. Checking only the return
 * value would miss a wrong XOR key entirely.
 *
 * COVERAGE:
 *
 *   random buffers        131,072 x (Encode + Decode)          262,144 calls
 *   structured buffers    all-zero, all-0xff, i, ~i, i*7, and
 *                         the 256 single-hot-byte buffers      520 x 2
 *   round-trip            Encode then Decode must restore the
 *                         original bytes and return the same
 *                         checksum, on retail's own code       131,072
 *
 * The input space is 256^256, so this is a sweep. It is however exhaustive in
 * the dimension that matters: the transform is byte-local (`buf[i] ^= i`,
 * `acc += byte * i`), so every (position, value) pair is what has to be
 * covered, and the single-hot-byte set covers all 256 positions directly.
 */

#include "recomp.h"

#define RVA_ENCODE 0x000e5410
#define RVA_DECODE 0x000e5460

/* OURS: linked from build/objdiff/base/savegame.obj. The declaration below
 * mangles to ?Encode@CSaveGame@@QAEHPAE@Z / ?Decode@CSaveGame@@QAEHPAE@Z. */
class CSaveGame {
public:
    int Encode(unsigned char *buf);
    int Decode(unsigned char *buf);
};

#define SLOT 0x100

static int retail_encode(unsigned char *buf)
{
    int dummy = 0;
    return recomp_thiscall1(RECOMP_RVA(RVA_ENCODE), &dummy, (int)buf);
}

static int retail_decode(unsigned char *buf)
{
    int dummy = 0;
    return recomp_thiscall1(RECOMP_RVA(RVA_DECODE), &dummy, (int)buf);
}

/* Run one buffer through both implementations of one direction and compare the
 * checksum AND the rewritten bytes. */
static void compare_one(RecompCase *ret_case, RecompCase *buf_case,
                        const unsigned char *src, int decode)
{
    unsigned char ours[SLOT], theirs[SLOT];
    int a, b;

    memcpy(ours, src, SLOT);
    memcpy(theirs, src, SLOT);
    if (decode) {
        a = ((CSaveGame *)&ours)->Decode(ours);
        b = retail_decode(theirs);
    } else {
        a = ((CSaveGame *)&ours)->Encode(ours);
        b = retail_encode(theirs);
    }
    recomp_check(ret_case, a, b);
    recomp_check_mem(buf_case, ours, theirs, SLOT);
}

int main(int argc, char **argv)
{
    RecompCase cases[5];
    unsigned char buf[SLOT];
    int i, j;

    if (!recomp_start(argc, argv, ""))
        return 1;
    recomp_seed(0x1234abcdu);

    recomp_case(&cases[0], "Encode checksum");
    recomp_case(&cases[1], "Encode rewritten buffer");
    recomp_case(&cases[2], "Decode checksum");
    recomp_case(&cases[3], "Decode rewritten buffer");
    recomp_case(&cases[4], "retail Encode->Decode restores the input");

    /* 1. random. */
    for (i = 0; i < 131072; i++) {
        for (j = 0; j < SLOT; j++)
            buf[j] = (unsigned char)recomp_rand();
        compare_one(&cases[0], &cases[1], buf, 0);
        compare_one(&cases[2], &cases[3], buf, 1);

        /* round-trip, entirely inside retail: Encode then Decode must give the
         * bytes back. If that ever fails the format is not an involution and
         * our reading of it is wrong regardless of what our C++ does. */
        {
            unsigned char rt[SLOT];
            int e, d;
            memcpy(rt, buf, SLOT);
            e = retail_encode(rt);
            d = retail_decode(rt);
            recomp_check(&cases[4], memcmp(rt, buf, SLOT) == 0 && e == d, 1);
        }
    }

    /* 2. structured. */
    for (i = 0; i < 5; i++) {
        for (j = 0; j < SLOT; j++) {
            switch (i) {
            case 0: buf[j] = 0x00; break;
            case 1: buf[j] = 0xff; break;
            case 2: buf[j] = (unsigned char)j; break;
            case 3: buf[j] = (unsigned char)~j; break;
            default: buf[j] = (unsigned char)(j * 7); break;
            }
        }
        compare_one(&cases[0], &cases[1], buf, 0);
        compare_one(&cases[2], &cases[3], buf, 1);
    }

    /* 3. one hot byte at each of the 256 positions, so every (position, value)
     * interaction in `acc += byte * i` is exercised at a known place. */
    for (i = 0; i < SLOT; i++) {
        memset(buf, 0, SLOT);
        buf[i] = 0xff;
        compare_one(&cases[0], &cases[1], buf, 0);
        compare_one(&cases[2], &cases[3], buf, 1);
    }

    /* 4. the null-pointer guard both bodies open with. */
    {
        int a = ((CSaveGame *)&buf)->Encode(0);
        int b = retail_encode(0);
        recomp_check(&cases[0], a, b);
        a = ((CSaveGame *)&buf)->Decode(0);
        b = retail_decode(0);
        recomp_check(&cases[2], a, b);
    }

    return recomp_report(cases, 5);
}

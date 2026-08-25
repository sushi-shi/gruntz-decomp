/* pidrun - the THIRD implementation: retail's own machine code, re-linked.
 *
 * Between the C++ reconstruction under src/ and the clean-room Rust decoder in
 * tools/gruntz-codec sits an obvious gap: both are our work, so if they agree
 * they may simply be wrong together. This harness closes it by running the
 * ACTUAL BYTES out of retail GRUNTZ.EXE and comparing against those.
 *
 * How it works
 * ------------
 * The PE mapping, relocation and __thiscall bridging all live in recomp.h,
 * which every harness here shares. The one function this harness invokes,
 *
 *     ?DecodeByteRun1Plane@CDDSurface@@QAEHPAX0HH@Z   RVA 0x145270, VA 0x00545270
 *
 * is entirely self-contained: `gruntz sema disasm 0x145270 --target` reports
 * "Relocations: none", and the disassembly contains no CALL at all - it is a
 * pure loop over the token stream. It also never dereferences `this` (it
 * stores ecx to [ebp-0x24] and never reads it back), so any pointer will do.
 * Relocating the image is therefore a no-op *for this function*; it is done
 * anyway so other harnesses can point at DATA-ONLY code whose constant tables
 * move with it.
 *
 * Functions that DO call the CRT or DirectDraw (CDDSurface::DecodeRun8 locks a
 * surface, CRezImage::DecodePidData builds a DIB section) are deliberately out
 * of scope - reaching them means standing up the statically-linked CRT and the
 * import table, which buys nothing for the RLE grammar.
 *
 * Calling convention: __thiscall, four stack args, callee cleans 0x10
 * (`ret 0x10`). MSVC 5.0 cannot spell __thiscall on a function pointer, so the
 * call is written in inline asm - which is also more honest about what is
 * happening.
 *
 * Protocol (little-endian throughout; the driver is
 * `gruntz-oracle recomp`, which writes the jobs and reads the results):
 *
 *   jobs file:    u32 magic 'PJOB'  u32 count
 *                 count x { u32 width, u32 height, u32 stream_len,
 *                           u8 stream[stream_len] }      (no padding)
 *   results file: u32 magic 'PRES'  u32 count
 *                 count x { u32 rc, u32 pixel_len,
 *                           u8 pixels[pixel_len] }       (no padding)
 *
 * Build + run:  recomp/harness/build.sh pidrun
 *               wine recomp/harness/pidrun.exe "$GRUNTZ_EXE" jobs.bin out.bin
 */

#include "recomp.h"

#define RVA_DECODE_BYTE_RUN_1_PLANE 0x00145270

#define JOB_MAGIC 0x424f4a50 /* 'PJOB' little-endian */
#define RES_MAGIC 0x53455250 /* 'PRES' little-endian */

/* CDDSurface::DecodeByteRun1Plane(void* dst, void* src, int width, int height).
 * __thiscall with four stack args; `this` is never dereferenced, so any
 * pointer will do. */
static int call_decode_byte_run_1_plane(void *dst, void *src, int width, int height)
{
    int this_dummy = 0;
    return recomp_thiscall4(RECOMP_RVA(RVA_DECODE_BYTE_RUN_1_PLANE), &this_dummy, (int)dst,
                            (int)src, width, height);
}

static unsigned int rd32(FILE *fp)
{
    unsigned char b[4];
    if (fread(b, 1, 4, fp) != 4)
        return 0xffffffffu;
    return (unsigned int)b[0] | ((unsigned int)b[1] << 8) |
           ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24);
}

static void wr32(FILE *fp, unsigned int v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xff);
    b[1] = (unsigned char)((v >> 8) & 0xff);
    b[2] = (unsigned char)((v >> 16) & 0xff);
    b[3] = (unsigned char)((v >> 24) & 0xff);
    fwrite(b, 1, 4, fp);
}

int main(int argc, char **argv)
{
    FILE *jobs, *out;
    unsigned int magic, count, i;
    unsigned char *stream = NULL;
    unsigned char *pixels = NULL;
    unsigned int stream_cap = 0, pixel_cap = 0;

    if (argc != 4) {
        fprintf(stderr, "usage: pidrun <GRUNTZ.EXE> <jobs.bin> <results.bin>\n");
        return 2;
    }
    if (!recomp_map(argv[1]))
        return 1;

    jobs = fopen(argv[2], "rb");
    if (!jobs) {
        fprintf(stderr, "pidrun: cannot open %s\n", argv[2]);
        return 1;
    }
    magic = rd32(jobs);
    count = rd32(jobs);
    if (magic != JOB_MAGIC) {
        fprintf(stderr, "pidrun: bad job magic %08x\n", magic);
        return 1;
    }
    out = fopen(argv[3], "wb");
    if (!out) {
        fprintf(stderr, "pidrun: cannot create %s\n", argv[3]);
        return 1;
    }
    wr32(out, RES_MAGIC);
    wr32(out, count);

    for (i = 0; i < count; i++) {
        unsigned int w = rd32(jobs);
        unsigned int h = rd32(jobs);
        unsigned int n = rd32(jobs);
        unsigned int need = w * h;
        int rc;

        if (n > stream_cap) {
            free(stream);
            stream_cap = n + 1024;
            stream = (unsigned char *)malloc(stream_cap);
        }
        if (need > pixel_cap) {
            free(pixels);
            pixel_cap = need + 1024;
            pixels = (unsigned char *)malloc(pixel_cap);
        }
        if (!stream || !pixels) {
            fprintf(stderr, "pidrun: out of memory on job %u\n", i);
            return 1;
        }
        if (n && fread(stream, 1, n, jobs) != n) {
            fprintf(stderr, "pidrun: short job stream at %u\n", i);
            return 1;
        }
        /* Retail writes exactly width*height bytes on success; pre-fill with a
         * marker so a short write shows up as a difference rather than as
         * whatever the allocator left behind. */
        memset(pixels, 0xcd, need);
        rc = call_decode_byte_run_1_plane(pixels, stream, (int)w, (int)h);
        wr32(out, (unsigned int)rc);
        wr32(out, need);
        fwrite(pixels, 1, need, out);
    }
    fclose(jobs);
    fclose(out);
    fprintf(stderr, "pidrun: %u job(s) done\n", count);
    return 0;
}

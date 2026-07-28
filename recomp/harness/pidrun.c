/* pidrun - the THIRD implementation: retail's own machine code, re-linked.
 *
 * Between the C++ reconstruction under src/ and the clean-room Rust decoder in
 * tools/gruntz-codec sits an obvious gap: both are our work, so if they agree
 * they may simply be wrong together. This harness closes it by running the
 * ACTUAL BYTES out of retail GRUNTZ.EXE and comparing against those.
 *
 * How it works
 * ------------
 * GRUNTZ.EXE is a PE with a preferred base of 0x00400000. This program is
 * linked at 0x10000000 (see build.sh) so that base is free, VirtualAllocs the
 * image, copies each section to its RVA, applies the .reloc fixups if it did
 * not land on the preferred base, and then simply CALLS into it. (Wine does
 * not always hand out 0x00400000 - it keeps parts of the low address space
 * reserved - so the relocation path is the normal one, not a fallback.)
 *
 * The one function we invoke,
 *
 *     ?RunDecode1@CDDSurface@@QAEHPAX0HH@Z   RVA 0x145270, VA 0x00545270
 *
 * is entirely self-contained: `gruntz sema disasm 0x145270 --target` reports
 * "Relocations: none", and the disassembly contains no CALL at all - it is a
 * pure loop over the token stream. It also never dereferences `this` (it
 * stores ecx to [ebp-0x24] and never reads it back), so any pointer will do.
 * Relocating the image is therefore a no-op *for this function*; it is done
 * anyway so the harness can be pointed at other code later.
 *
 * The sprite RLE decoder in retail having no dependencies is why this is ~200
 * lines instead of a full PE loader. Functions that DO call the CRT or
 * DirectDraw (CDDSurface::DecodeRun8 locks a surface, CRezImage::DecodePidData
 * builds a DIB section) are deliberately out of scope - reaching them means
 * initialising the statically-linked CRT and the import table, which buys
 * nothing for the RLE grammar.
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
 * Build + run:  see tools/recomp/build.sh
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_BASE 0x00400000
#define RVA_RUNDECODE1 0x00145270

#define JOB_MAGIC 0x424f4a50  /* 'PJOB' little-endian */
#define RES_MAGIC 0x53455250  /* 'PRES' little-endian */

/* The address the image actually landed at; 0 until map_image succeeds. */
static unsigned char *g_base = 0;

/* Apply .reloc fixups for a delta from the preferred base. */
static void apply_relocs(unsigned char *base, IMAGE_NT_HEADERS *nt, long delta)
{
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_BASE_RELOCATION *blk;
    unsigned char *end;

    if (delta == 0)
        return;
    dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (dir->VirtualAddress == 0 || dir->Size == 0) {
        fprintf(stderr, "pidrun: image has no .reloc but needs relocating\n");
        return;
    }
    blk = (IMAGE_BASE_RELOCATION *)(base + dir->VirtualAddress);
    end = base + dir->VirtualAddress + dir->Size;
    while ((unsigned char *)blk < end && blk->SizeOfBlock >= sizeof(*blk)) {
        unsigned short *ent = (unsigned short *)(blk + 1);
        unsigned int count =
            (blk->SizeOfBlock - sizeof(*blk)) / sizeof(unsigned short);
        unsigned int i;
        for (i = 0; i < count; i++) {
            unsigned int type = ent[i] >> 12;
            unsigned int off = ent[i] & 0x0fff;
            if (type == IMAGE_REL_BASED_HIGHLOW)
                *(long *)(base + blk->VirtualAddress + off) += delta;
            /* type 0 (ABSOLUTE) is padding; VC5 emits nothing else here. */
        }
        blk = (IMAGE_BASE_RELOCATION *)((unsigned char *)blk + blk->SizeOfBlock);
    }
}

/* Map GRUNTZ.EXE's sections, relocating if we did not get the preferred base.
 * Returns 0 on failure. */
static int map_image(const char *path)
{
    FILE *fp;
    long size;
    unsigned char *file;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    void *base;
    unsigned int i;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "pidrun: cannot open %s\n", path);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    file = (unsigned char *)malloc((size_t)size);
    if (!file || fread(file, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "pidrun: short read on %s\n", path);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    dos = (IMAGE_DOS_HEADER *)file;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        fprintf(stderr, "pidrun: not an MZ image\n");
        return 0;
    }
    nt = (IMAGE_NT_HEADERS *)(file + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        fprintf(stderr, "pidrun: not a PE image\n");
        return 0;
    }
    if (nt->OptionalHeader.ImageBase != IMAGE_BASE) {
        fprintf(stderr, "pidrun: image base is %08lx, expected %08x\n",
                (unsigned long)nt->OptionalHeader.ImageBase, IMAGE_BASE);
        return 0;
    }

    base = VirtualAlloc((void *)IMAGE_BASE, nt->OptionalHeader.SizeOfImage,
                        MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (base == NULL)
        base = VirtualAlloc(NULL, nt->OptionalHeader.SizeOfImage,
                            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (base == NULL) {
        fprintf(stderr, "pidrun: VirtualAlloc of %lu bytes failed\n",
                (unsigned long)nt->OptionalHeader.SizeOfImage);
        return 0;
    }
    memcpy(base, file, nt->OptionalHeader.SizeOfHeaders);
    sec = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (sec[i].SizeOfRawData == 0)
            continue;
        memcpy((unsigned char *)base + sec[i].VirtualAddress,
               file + sec[i].PointerToRawData, sec[i].SizeOfRawData);
    }
    free(file);

    g_base = (unsigned char *)base;
    /* nt still points into the freed file image - re-derive it from the map. */
    dos = (IMAGE_DOS_HEADER *)g_base;
    nt = (IMAGE_NT_HEADERS *)(g_base + dos->e_lfanew);
    apply_relocs(g_base, nt, (long)((unsigned char *)base - (unsigned char *)IMAGE_BASE));
    if (base != (void *)IMAGE_BASE)
        fprintf(stderr, "pidrun: image at %p (relocated from %08x)\n", base,
                IMAGE_BASE);
    return 1;
}

/* CDDSurface::RunDecode1(void* dst, void* src, int width, int height).
 * __thiscall: `this` in ecx, four stack args pushed right-to-left, callee
 * cleans (`ret 0x10`) - hence no stack fixup after the call. */
static int call_rundecode1(void *dst, void *src, int width, int height)
{
    void *fn = (void *)(g_base + RVA_RUNDECODE1);
    int result = 0;
    int this_dummy = 0; /* never dereferenced by the callee */
    void *thisp = &this_dummy;

    __asm {
        push height
        push width
        push src
        push dst
        mov  ecx, thisp
        call fn
        mov  result, eax
    }
    return result;
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
    if (!map_image(argv[1]))
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
        rc = call_rundecode1(pixels, stream, (int)w, (int)h);
        wr32(out, (unsigned int)rc);
        wr32(out, need);
        fwrite(pixels, 1, need, out);
    }
    fclose(jobs);
    fclose(out);
    fprintf(stderr, "pidrun: %u job(s) done\n", count);
    return 0;
}

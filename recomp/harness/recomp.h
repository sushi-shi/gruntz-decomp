/* recomp.h - the shared core every harness in this directory is built on.
 *
 * A harness answers one question: *does retail's own executed machine code
 * agree with the C++ we ship?* It does that by holding both in one 32-bit
 * process:
 *
 *   RETAIL  - GRUNTZ.EXE mapped at (or relocated from) its preferred base, and
 *             CALLed directly. See recomp_map().
 *   OURS    - our compiled object from build/objdiff/base/<unit>.obj, linked
 *             into the harness. Declare the function with a matching signature
 *             and the C++ mangling binds it; you are then calling the EXACT
 *             bytes the decompilation produces, not a transcription of them.
 *
 * That second half matters. Re-typing the body into the harness would test our
 * reading of our own source; linking the object tests the artefact.
 *
 * Header-only on purpose, so a harness is one translation unit and build.sh
 * stays a one-liner per target.
 *
 * ---------------------------------------------------------------------------
 * Adding a harness
 * ---------------------------------------------------------------------------
 *   1. Confirm the function is reachable:
 *          python -m gruntz.audit.recomp_islands --max-fields 12
 *      It must be ISLAND / SELF-CALL / DATA-ONLY *and* touch few enough object
 *      fields that you can fabricate them. Read recomp/README.md first.
 *   2. Copy an existing harness. Declare the retail entry point with
 *      RECOMP_RVA, declare OURS with a signature that mangles identically, and
 *      write an input generator.
 *   3. Build and run:
 *          recomp/harness/build.sh <name> [unit ...]
 *          wine recomp/harness/<name>.exe "$GRUNTZ_EXE"
 *
 * ---------------------------------------------------------------------------
 * Calling conventions
 * ---------------------------------------------------------------------------
 * __cdecl and __stdcall need nothing: cast the address to a function pointer of
 * the right type and call it. Only __thiscall needs help, because MSVC 5.0
 * cannot spell it on a function pointer - hence recomp_thiscall*(), which put
 * `this` in ecx and let the callee clean the stack (`ret N`).
 */

#ifndef RECOMP_H
#define RECOMP_H

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECOMP_IMAGE_BASE 0x00400000

/* Where the retail image actually landed. */
static unsigned char *g_recomp_base = 0;

/* Retail's code for an RVA, once recomp_map() has run. */
#define RECOMP_RVA(rva) ((void *)(g_recomp_base + (rva)))

/* ------------------------------------------------------------------ mapping */

static void recomp_apply_relocs(unsigned char *base, IMAGE_NT_HEADERS *nt, long delta)
{
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_BASE_RELOCATION *blk;
    unsigned char *end;

    if (delta == 0)
        return;
    dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (dir->VirtualAddress == 0 || dir->Size == 0) {
        fprintf(stderr, "recomp: image has no .reloc but needs relocating\n");
        return;
    }
    blk = (IMAGE_BASE_RELOCATION *)(base + dir->VirtualAddress);
    end = base + dir->VirtualAddress + dir->Size;
    while ((unsigned char *)blk < end && blk->SizeOfBlock >= sizeof(*blk)) {
        unsigned short *ent = (unsigned short *)(blk + 1);
        unsigned int count = (blk->SizeOfBlock - sizeof(*blk)) / sizeof(unsigned short);
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

/* Map GRUNTZ.EXE, relocating if we did not get the preferred base. Returns 0
 * on failure. Wine keeps parts of the low address space reserved, so the
 * relocation path is the normal one rather than a fallback - which is also why
 * DATA-ONLY functions work: their constant tables move with the code. */
static int recomp_map(const char *path)
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
        fprintf(stderr, "recomp: cannot open %s\n", path);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    file = (unsigned char *)malloc((size_t)size);
    if (!file || fread(file, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "recomp: short read on %s\n", path);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    dos = (IMAGE_DOS_HEADER *)file;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        fprintf(stderr, "recomp: not an MZ image\n");
        return 0;
    }
    nt = (IMAGE_NT_HEADERS *)(file + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        fprintf(stderr, "recomp: not a PE image\n");
        return 0;
    }
    if (nt->OptionalHeader.ImageBase != RECOMP_IMAGE_BASE) {
        fprintf(stderr, "recomp: image base is %08lx, expected %08x\n",
                (unsigned long)nt->OptionalHeader.ImageBase, RECOMP_IMAGE_BASE);
        return 0;
    }

    base = VirtualAlloc((void *)RECOMP_IMAGE_BASE, nt->OptionalHeader.SizeOfImage,
                        MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (base == NULL)
        base = VirtualAlloc(NULL, nt->OptionalHeader.SizeOfImage,
                            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (base == NULL) {
        fprintf(stderr, "recomp: VirtualAlloc of %lu bytes failed\n",
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

    g_recomp_base = (unsigned char *)base;
    /* nt pointed into the freed file image - re-derive it from the map. */
    dos = (IMAGE_DOS_HEADER *)g_recomp_base;
    nt = (IMAGE_NT_HEADERS *)(g_recomp_base + dos->e_lfanew);
    recomp_apply_relocs(g_recomp_base, nt,
                        (long)(g_recomp_base - (unsigned char *)RECOMP_IMAGE_BASE));
    if (base != (void *)RECOMP_IMAGE_BASE)
        fprintf(stderr, "recomp: image at %p (relocated from %08x)\n", base,
                RECOMP_IMAGE_BASE);
    return 1;
}

/* -------------------------------------------------------- __thiscall bridges
 *
 * `this` in ecx, args pushed right-to-left, callee cleans (`ret 4*N`) - so
 * there is deliberately no stack fixup after the call.
 */

static int recomp_thiscall0(void *fn, void *self)
{
    int result;
    __asm {
        mov  ecx, self
        call fn
        mov  result, eax
    }
    return result;
}

static int recomp_thiscall1(void *fn, void *self, int a)
{
    int result;
    __asm {
        push a
        mov  ecx, self
        call fn
        mov  result, eax
    }
    return result;
}

static int recomp_thiscall2(void *fn, void *self, int a, int b)
{
    int result;
    __asm {
        push b
        push a
        mov  ecx, self
        call fn
        mov  result, eax
    }
    return result;
}

static int recomp_thiscall3(void *fn, void *self, int a, int b, int c)
{
    int result;
    __asm {
        push c
        push b
        push a
        mov  ecx, self
        call fn
        mov  result, eax
    }
    return result;
}

static int recomp_thiscall4(void *fn, void *self, int a, int b, int c, int d)
{
    int result;
    __asm {
        push d
        push c
        push b
        push a
        mov  ecx, self
        call fn
        mov  result, eax
    }
    return result;
}

/* ----------------------------------------------------------------- input RNG
 *
 * xorshift32, seeded explicitly. Deterministic on purpose: a harness that
 * disagrees must be re-runnable with the same inputs, and "it failed once on
 * some random palette" is not a bug report.
 */

static unsigned int g_recomp_rng = 0x2545f491u;

static void recomp_seed(unsigned int s) { g_recomp_rng = s ? s : 1u; }

static unsigned int recomp_rand(void)
{
    unsigned int x = g_recomp_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_recomp_rng = x;
    return x;
}

/* Uniform in [0, n). */
static unsigned int recomp_rand_below(unsigned int n)
{
    return n ? recomp_rand() % n : 0;
}

/* ------------------------------------------------------------------ verdicts */

typedef struct {
    const char *name;
    unsigned long checked;
    unsigned long disagreed;
    unsigned int shown;
    unsigned int show_max;
} RecompCase;

static void recomp_case(RecompCase *c, const char *name)
{
    c->name = name;
    c->checked = 0;
    c->disagreed = 0;
    c->shown = 0;
    c->show_max = 10;
}

/* Record one comparison. Returns 1 when it disagreed, so the caller can print
 * whatever detail is meaningful for that function. */
static int recomp_check(RecompCase *c, int ours, int retail)
{
    c->checked++;
    if (ours == retail)
        return 0;
    c->disagreed++;
    if (c->shown < c->show_max) {
        c->shown++;
        fprintf(stderr, "  [%s] #%lu  ours=%d retail=%d\n", c->name, c->checked, ours,
                retail);
    }
    return 1;
}

/* Same, for buffer results. */
static int recomp_check_mem(RecompCase *c, const void *ours, const void *retail, size_t n)
{
    c->checked++;
    if (memcmp(ours, retail, n) == 0)
        return 0;
    c->disagreed++;
    if (c->shown < c->show_max) {
        size_t i;
        c->shown++;
        for (i = 0; i < n; i++) {
            if (((const unsigned char *)ours)[i] != ((const unsigned char *)retail)[i]) {
                fprintf(stderr, "  [%s] #%lu  first byte diff at %u: ours=%02x retail=%02x\n",
                        c->name, c->checked, (unsigned int)i,
                        ((const unsigned char *)ours)[i],
                        ((const unsigned char *)retail)[i]);
                break;
            }
        }
    }
    return 1;
}

static int recomp_report(RecompCase *cases, int n)
{
    int i, bad = 0;
    printf("\n%-46s %12s %12s\n", "CASE", "CHECKED", "DISAGREED");
    printf("--------------------------------------------------------------------------\n");
    for (i = 0; i < n; i++) {
        printf("%-46s %12lu %12lu%s\n", cases[i].name, cases[i].checked,
               cases[i].disagreed, cases[i].disagreed ? "   <-- DISAGREE" : "");
        if (cases[i].disagreed)
            bad = 1;
        if (cases[i].checked == 0) {
            printf("%-46s   (nothing checked - the generator produced no inputs)\n", "");
            bad = 1;
        }
    }
    printf("\n%s\n", bad ? "VERDICT: retail and our C++ DISAGREE - see above."
                         : "VERDICT: retail's executed code and our compiled C++ agree "
                           "on every input.");
    return bad;
}

/* Boilerplate every harness main() starts with. */
static int recomp_start(int argc, char **argv, const char *usage)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <GRUNTZ.EXE>%s\n", argv[0], usage ? usage : "");
        return 0;
    }
    return recomp_map(argv[1]);
}

#endif /* RECOMP_H */

/* replay.cpp - the replay half: restore a captured process state, run one
 * function on it, and diff every writable byte against the recorded answer.
 *
 * ---------------------------------------------------------------------------
 * Why restoring at the ORIGINAL addresses is the whole trick
 * ---------------------------------------------------------------------------
 * Every pointer in a snapshot is valid by construction if the bytes go back
 * where they came from. There is no closure to compute, no pointer fix-up, and
 * none of the false differences that plague synthesized state: allocator
 * bookkeeping is identical because the allocator's own free lists were
 * snapshotted with everything else, padding is identical because neither side
 * writes it, and a field the function does not touch keeps the snapshot value
 * on both sides. That is what makes a FULL memory diff affordable - and a full
 * diff is stronger than any hand-written output check, because it catches
 * effects nobody thought to look for.
 *
 * ---------------------------------------------------------------------------
 * Three comparisons, and what each one proves
 * ---------------------------------------------------------------------------
 *   [1] RETAIL re-run vs the RECORDED exit.  A SELF-TEST OF THE HARNESS: the
 *       same code on the same state must reproduce the same answer. If this
 *       fails, nothing else the harness says counts, and the bug is here.
 *   [2] OURS vs the RECORDED exit.  The oracle verdict on the input the game
 *       actually produced. Ground truth.
 *   [3] OURS vs the RETAIL re-run.  The only verdict available once the entry
 *       state has been MUTATED, because a mutated input has no recorded
 *       answer. Speculative by construction - report it separately.
 *
 * ---------------------------------------------------------------------------
 * How the harness survives overwriting its own address space
 * ---------------------------------------------------------------------------
 * A recorded region can land on memory this process is already using - wine
 * puts a fresh 32-bit process's heap and main thread stack at much the same low
 * addresses the game had them. So:
 *
 *   * the whole restore/call/collect sequence runs on a PRIVATE scratch stack
 *     in a high VirtualAlloc region, so the main thread's stack can be
 *     overwritten underneath it;
 *   * before restoring, the current bytes of every clobbered region are saved
 *     into the arena and put back afterwards, so the CRT, the heap and the
 *     original stack are intact again by the time main() resumes. Nothing
 *     prints, allocates or touches a FILE* between the first restore and the
 *     final undo;
 *   * three ranges are non-negotiable and abort the run if a recorded region
 *     touches them: this image, the arena, and the scratch stack. You cannot
 *     both overwrite the code you are executing and survive.
 *
 * The stack exclusion rule is stated in snapshot.h and implemented in
 * excluded() - below entry_esp, plus the four bytes at entry_esp that hold the
 * return address the replay redirects. Nothing else is excluded, ever.
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snapshot.h"
#include "targets.h"

#define ARENA_BASE 0x60000000
#define ARENA_SIZE 0x14000000 /* 320 MB: two snapshots + two post-states + saves */
#define SCRATCH_SIZE 0x100000
#define STACK_HEADROOM 0x20000 /* extra committed pages under the restored stack */
#define MAX_SAVE 16384
#define MAX_SET 64
#define MAX_SHOW 32
#define MAX_TRACE 4096
#define STEP_LIMIT 20000

/* ------------------------------------------------------------------- arena */

static BYTE *g_arena;
static DWORD g_arena_used;
static BYTE *g_scratch;
static DWORD g_scratch_esp;
static int g_fatal;

static void *arena(DWORD n)
{
    void *p;
    n = (n + 15) & ~15u;
    if (g_arena_used + n > ARENA_SIZE) {
        fprintf(stderr, "replay: arena exhausted (%lu + %lu > %d)\n", g_arena_used, n,
                (int)ARENA_SIZE);
        exit(2);
    }
    p = g_arena + g_arena_used;
    g_arena_used += n;
    return p;
}

/* ------------------------------------------------------------- stage trace
 *
 * Between the first restore and undo_prepare the process heap holds the GAME's
 * bytes, so the CRT cannot be used - no printf, no fopen. This writes progress
 * markers straight through a pre-opened kernel handle using a static buffer, so
 * a fault anywhere in the sequence still leaves a record of how far it got. */

static HANDLE g_stage_h = INVALID_HANDLE_VALUE;
static char g_stage_buf[256];

static void stage_hex(const char *msg, DWORD a, DWORD b);

static void stage(const char *msg)
{
    DWORD n = 0, i = 0;
    if (g_stage_h == INVALID_HANDLE_VALUE)
        return;
    while (msg[i] && i < sizeof(g_stage_buf) - 3) {
        g_stage_buf[i] = msg[i];
        i++;
    }
    g_stage_buf[i++] = '\r';
    g_stage_buf[i++] = '\n';
    WriteFile(g_stage_h, g_stage_buf, i, &n, NULL);
}

static void stage_hex(const char *msg, DWORD a, DWORD b)
{
    static const char *hx = "0123456789abcdef";
    char line[128];
    DWORD n = 0, i = 0, k;
    if (g_stage_h == INVALID_HANDLE_VALUE)
        return;
    while (msg[i] && i < 90) {
        line[i] = msg[i];
        i++;
    }
    line[i++] = ' ';
    for (k = 8; k-- > 0;)
        line[i++] = hx[(a >> (k * 4)) & 0xf];
    line[i++] = '+';
    for (k = 8; k-- > 0;)
        line[i++] = hx[(b >> (k * 4)) & 0xf];
    line[i++] = '\r';
    line[i++] = '\n';
    WriteFile(g_stage_h, line, i, &n, NULL);
}

/* ------------------------------------------------------------- snapshot i/o */

typedef struct Snap {
    BYTE *raw;
    DWORD len;
    SnapHeader *h;
    SnapRegion *r;
} Snap;

static Snap g_noise;   /* the second back-to-back entry snapshot; see noisy() */
static int g_have_noise;
static DWORD g_masked; /* comparisons skipped as measured observer noise */

static int load_snap(const char *path, Snap *s)
{
    HANDLE f;
    DWORD n = 0, sz;

    f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "replay: cannot open %s\n", path);
        return 0;
    }
    sz = GetFileSize(f, NULL);
    s->raw = (BYTE *)arena(sz);
    s->len = sz;
    if (!ReadFile(f, s->raw, sz, &n, NULL) || n != sz) {
        fprintf(stderr, "replay: short read on %s\n", path);
        CloseHandle(f);
        return 0;
    }
    CloseHandle(f);

    s->h = (SnapHeader *)s->raw;
    if (memcmp(s->h->magic, SNAP_MAGIC, 8) != 0) {
        fprintf(stderr, "replay: %s is not a snapshot\n", path);
        return 0;
    }
    if (s->h->version != SNAP_VERSION || s->h->hdr_size != sizeof(SnapHeader)
        || s->h->region_size != sizeof(SnapRegion)) {
        fprintf(stderr,
                "replay: %s has header %lu/%lu/%lu, this build wants %d/%d/%d\n", path,
                s->h->version, s->h->hdr_size, s->h->region_size, (int)SNAP_VERSION,
                (int)sizeof(SnapHeader), (int)sizeof(SnapRegion));
        return 0;
    }
    if (s->h->total_size != sz)
        fprintf(stderr, "replay: %s says %lu bytes, file is %lu\n", path,
                s->h->total_size, sz);
    s->r = (SnapRegion *)(s->raw + s->h->region_off);
    return 1;
}

/* Bytes recorded for [base,base+size) in `s`, or 0 if it recorded no such
 * region. Exit and entry can legitimately differ in shape (an allocation), so
 * this is a lookup, not an index. */
static const BYTE *snap_bytes(Snap *s, DWORD base, DWORD size)
{
    DWORD i;
    for (i = 0; i < s->h->n_regions; i++)
        if (s->r[i].base == base && s->r[i].size == size
            && (s->r[i].rflags & SNAPRF_HAS_BYTES))
            return s->raw + s->r[i].blob_off;
    return 0;
}

/* ------------------------------------------------------- THE SECOND RULE
 *
 * A recorded region is RESTORED and COMPARED only if it is the program's own
 * memory: the game image, its private heap/VirtualAlloc regions, its mapped
 * views, and its stack. The writable data of OTHER loaded modules - wine's
 * ntdll, kernel32, DDRAW, DSOUND, and the game's proprietary MSS32/SMACKW32 -
 * is skipped, and skipped for a reason that is not convenience:
 *
 *   * those pages are the RUNTIME THE HARNESS IS EXECUTING ON. Writing the
 *     game's kernel32 .data over the replay process's kernel32 .data kills the
 *     process on the spot; it was the first thing that happened when this was
 *     not skipped.
 *   * their addresses are assigned per process by the loader, so "the same
 *     address" does not mean "the same thing" across capture and replay.
 *   * for a CLEAN-tier target (`gruntz.audit.iat_tiers`) the entire call
 *     closure never leaves the game image, so nothing the function can read or
 *     write lives there. That is exactly what the tier means.
 *
 * A target that DOES reach those modules is IMPORT tier and needs recorded
 * import call sequences instead - not a wider restore. So this rule is the
 * tier boundary made executable, not a fudge: it never grows, and if it is ever
 * wrong for a target, that target was misclassified.
 *
 * 84 of the 334 regions in the reference capture are in this class - 708 KB
 * against 41.8 MB of game heap.
 */
static int restorable(SnapRegion *r)
{
    return (r->rflags & SNAPRF_HAS_BYTES) && r->klass != SNAPR_OTHER_IMAGE;
}

/* ------------------------------------------------------- the restore plan */

typedef struct Save {
    DWORD base, size;
    BYTE *bytes; /* what WE had there, 0 if the region was free */
    DWORD prot;  /* what protection WE had, 0 if free */
} Save;

static Save g_save[MAX_SAVE];
static DWORD g_nsave;

static int overlaps(DWORD a, DWORD alen, DWORD b, DWORD blen)
{
    return a < b + blen && b < a + alen;
}

static const char *fatal_overlap(DWORD base, DWORD size)
{
    HMODULE self = GetModuleHandleA(NULL);
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)self;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((BYTE *)self + dos->e_lfanew);

    if (overlaps(base, size, (DWORD)self, nt->OptionalHeader.SizeOfImage))
        return "replay.exe's own image";
    if (overlaps(base, size, (DWORD)g_arena, ARENA_SIZE))
        return "the arena";
    if (overlaps(base, size, (DWORD)g_scratch, SCRATCH_SIZE))
        return "the scratch stack";
    return 0;
}

/* Make [base,base+size) present and writable here.
 *
 * A recorded region is one VirtualQuery region in the GAME's address space; in
 * OURS the same range can be several - part free, part reserved inside some
 * allocation, part committed by our own CRT. VirtualProtect refuses a range
 * that spans an allocation boundary, and MEM_RESERVE only takes 64 KB-granular
 * bases, so this walks the range and handles each piece for what it actually
 * is. (Getting this wrong is what the first version did: it looked at the
 * first page, decided "committed", and VirtualProtect failed on 003e0000.) */
static int make_present_ex(DWORD base, DWORD size, int soft)
{
    DWORD a = base, end = base + size;

    while (a < end) {
        MEMORY_BASIC_INFORMATION mbi;
        DWORD chunk, old;
        Save *sv;

        if (VirtualQuery((LPCVOID)a, &mbi, sizeof(mbi)) != sizeof(mbi)) {
            fprintf(stderr, "replay: VirtualQuery(%08lx) failed\n", a);
            return 0;
        }
        chunk = (DWORD)mbi.BaseAddress + mbi.RegionSize - a;
        if (chunk > end - a)
            chunk = end - a;
        if (g_nsave >= MAX_SAVE) {
            fprintf(stderr, "replay: more than %d chunks to restore\n", (int)MAX_SAVE);
            return 0;
        }
        sv = &g_save[g_nsave++];
        sv->base = a;
        sv->size = chunk;
        sv->bytes = 0;
        sv->prot = 0;

        if (mbi.State == MEM_FREE) {
            if (!VirtualAlloc((LPVOID)a, chunk, MEM_RESERVE | MEM_COMMIT,
                              PAGE_EXECUTE_READWRITE)) {
                /* MEM_RESERVE rounds the base down to the 64 KB allocation
                 * granularity, so a free chunk that starts mid-granule next to
                 * something of ours cannot be reserved at all. */
                if (soft) {
                    g_nsave--;
                    return 0;
                }
                fprintf(stderr, "replay: VirtualAlloc(%08lx,%lx) failed (%lu)\n", a,
                        chunk, GetLastError());
                return 0;
            }
        } else if (mbi.State == MEM_RESERVE) {
            /* Inside somebody's reservation but uncommitted: commit it. There
             * is nothing to save - it had no contents - but it must be released
             * differently, so mark it. */
            if (!VirtualAlloc((LPVOID)a, chunk, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
                if (soft) {
                    g_nsave--;
                    return 0;
                }
                fprintf(stderr, "replay: VirtualAlloc(commit %08lx,%lx) failed (%lu)\n",
                        a, chunk, GetLastError());
                return 0;
            }
            sv->prot = 0xffffffffu; /* "decommit me" */
        } else {
            sv->prot = mbi.Protect;
            sv->bytes = (BYTE *)arena(chunk);
            /* Protect BEFORE reading. A committed page can carry PAGE_GUARD -
             * our own thread stack has one - and merely READING it raises
             * STATUS_GUARD_PAGE_VIOLATION, which wine reports as an unhandled
             * stack overflow. VirtualProtect clears the guard bit, and
             * undo_prepare puts the original protection (guard included) back. */
            if (!VirtualProtect((LPVOID)a, chunk, PAGE_EXECUTE_READWRITE, &old)) {
                if (soft) {
                    g_nsave--;
                    return 0;
                }
                fprintf(stderr,
                        "replay: VirtualProtect(%08lx,%lx) failed (%lu) - a mapped "
                        "section view cannot be made writable; use --spawn\n",
                        a, chunk, GetLastError());
                g_nsave--; /* never leave a half-built Save for undo_prepare */
                return 0;
            }
            memcpy(sv->bytes, (const void *)a, chunk);
        }
        a += chunk;
    }
    return 1;
}

static int make_present(DWORD base, DWORD size)
{
    return make_present_ex(base, size, 0);
}

/* Best-effort: used only for the extra room under the restored stack, which is
 * a convenience and not part of the recorded state. */
static DWORD g_headroom;

static void make_headroom(DWORD stack_base)
{
    DWORD n = STACK_HEADROOM;
    while (n >= 0x1000) {
        if (stack_base > n && !fatal_overlap(stack_base - n, n)
            && make_present_ex(stack_base - n, n, 1)) {
            g_headroom = n;
            return;
        }
        n >>= 1;
    }
    g_headroom = 0;
}

static int prepare(Snap *s)
{
    DWORD i;

    for (i = 0; i < s->h->n_regions; i++) {
        SnapRegion *r = &s->r[i];
        const char *what;

        if (!restorable(r))
            continue;
        what = fatal_overlap(r->base, r->size);
        if (what) {
            fprintf(stderr,
                    "replay: recorded region %08lx+%lx overlaps %s - this snapshot "
                    "cannot be replayed in this process layout\n",
                    r->base, r->size, what);
            return 0;
        }
        if (!make_present(r->base, r->size))
            return 0;

        /* Give the restored stack room to grow DOWN. The recorded region stops
         * at the game's committed bottom; below it the game had a guard page
         * that grew the stack on demand, and a plain VirtualAlloc has none. The
         * callee's own frame and any exception dispatch land there, and all of
         * it is below entry_esp, so none of it is ever compared. */
        /* Only when the recorded stack does not already carry enough committed
         * space below entry_esp for the callee's frame. It usually does. */
        if (r->klass == SNAPR_HOOK_STACK && s->h->entry_esp - r->base < STACK_HEADROOM)
            make_headroom(r->base);
    }
    return 1;
}

static void undo_prepare(void)
{
    DWORD i, old;
    for (i = g_nsave; i-- > 0;) {
        Save *sv = &g_save[i];
        if (sv->bytes) {
            memcpy((void *)sv->base, sv->bytes, sv->size);
            VirtualProtect((LPVOID)sv->base, sv->size, sv->prot, &old);
        } else if (sv->prot == 0xffffffffu) {
            VirtualFree((LPVOID)sv->base, sv->size, MEM_DECOMMIT);
        } else {
            VirtualFree((LPVOID)sv->base, 0, MEM_RELEASE);
        }
    }
    g_nsave = 0;
}

static int g_verbose_restore;

static void restore(Snap *s)
{
    DWORD i;
    for (i = 0; i < s->h->n_regions; i++) {
        SnapRegion *r = &s->r[i];
        if (restorable(r)) {
            if (g_verbose_restore)
                stage_hex("    region", r->base, r->size);
            memcpy((void *)r->base, s->raw + r->blob_off, r->size);
        }
    }
}

/* --------------------------------------------------------------- mutations */

typedef struct Setting {
    DWORD addr, value;
} Setting;
static Setting g_set[MAX_SET];
static DWORD g_nset;

/* Applied to the OURS side only: a deliberate, one-sided perturbation. It is
 * the harness's NEGATIVE CONTROL - `IDENTICAL` means nothing unless the same
 * machinery reports DISAGREE when the two sides really are fed different
 * state. Run it once whenever you doubt a green verdict. */
static Setting g_set_ours[MAX_SET];
static DWORD g_nset_ours;
static int g_side_ours;

static void apply_mutations(void)
{
    DWORD i;
    for (i = 0; i < g_nset; i++)
        *(DWORD *)g_set[i].addr = g_set[i].value;
    if (g_side_ours)
        for (i = 0; i < g_nset_ours; i++)
            *(DWORD *)g_set_ours[i].addr = g_set_ours[i].value;
}

/* ------------------------------------------------------------- the call ---
 *
 * The callee runs on the RESTORED stack at the RECORDED esp, with the RECORDED
 * registers, and with [esp] pointing at call_return instead of at retail's
 * caller. That is the only byte range of the recorded state the replay
 * changes, and it is excluded from the diff by name.
 */

static DWORD g_r_eax, g_r_ecx, g_r_edx, g_r_ebx, g_r_esp, g_r_ebp, g_r_esi, g_r_edi,
    g_r_efl;
static DWORD g_o_eax, g_o_ecx, g_o_edx, g_o_ebx, g_o_esp, g_o_ebp, g_o_esi, g_o_edi,
    g_o_efl;
static DWORD g_calltarget;
static DWORD g_saveesp;

/* The TEB describes this thread's stack, and the exception dispatcher believes
 * it: with ESP moved to the scratch region or to the restored game stack, wine
 * reports "Exception frame is not in stack limits" and cannot dispatch. So the
 * bounds move with ESP. */
static DWORD g_teb_base, g_teb_limit;
static DWORD g_wide_base, g_wide_limit;

static void teb_get_stack(DWORD *base, DWORD *limit)
{
    DWORD b, l;
    __asm {
        mov eax, fs:[4]
        mov b, eax
        mov eax, fs:[8]
        mov l, eax
    }
    *base = b;
    *limit = l;
}

static void teb_set_stack(DWORD base, DWORD limit)
{
    __asm {
        mov eax, base
        mov fs:[4], eax
        mov eax, limit
        mov fs:[8], eax
    }
}

void call_return(void);

__declspec(naked) void call_restored(void)
{
    __asm {
        /* Save the C CALLER's callee-saved registers. call_return puts the
         * CALLEE's register file into the globals and then hands control back
         * here, so without this the compiler's ebx/esi/edi/ebp in run_side
         * would silently become the replayed function's - which is exactly how
         * the first version faulted, dereferencing the snapshot pointer that
         * had been living in esi. */
        push ebx
        push esi
        push edi
        push ebp
        mov  g_saveesp, esp
        mov  eax, g_r_efl
        push eax
        popfd
        mov  eax, g_r_eax
        mov  ecx, g_r_ecx
        mov  edx, g_r_edx
        mov  ebx, g_r_ebx
        mov  ebp, g_r_ebp
        mov  esi, g_r_esi
        mov  edi, g_r_edi
        mov  esp, g_r_esp
        jmp  DWORD PTR [g_calltarget]
    }
}

__declspec(naked) void call_return(void)
{
    __asm {
        mov  g_o_eax, eax
        mov  g_o_ecx, ecx
        mov  g_o_edx, edx
        mov  g_o_ebx, ebx
        mov  g_o_ebp, ebp
        mov  g_o_esi, esi
        mov  g_o_edi, edi
        pushfd
        pop  eax
        mov  g_o_efl, eax
        mov  eax, esp
        mov  g_o_esp, eax
        mov  esp, g_saveesp
        pop  ebp
        pop  edi
        pop  esi
        pop  ebx
        mov  eax, g_o_eax
        ret
    }
}

/* --------------------------------------------------- single-step coverage
 *
 * TF plus a VECTORED exception handler. Vectored, not SEH: the callee runs on
 * the restored game stack while the TEB still describes ours, so a stack-based
 * exception registration record would not be believed. A vectored handler is a
 * global list inside ntdll and does not care where the stack is. It is
 * resolved at run time, so a missing export degrades to "no coverage" rather
 * than a link error.
 */

typedef LONG(WINAPI *VEH_HANDLER)(EXCEPTION_POINTERS *);
typedef PVOID(WINAPI *ADD_VEH)(ULONG, VEH_HANDLER);

static DWORD g_trace[MAX_TRACE];
static DWORD g_ntrace;
static DWORD g_steps;
static DWORD g_lo, g_hi;

/* Stepping is ARMED BY A BREAKPOINT, not by setting TF up front.
 *
 * Setting TF in the EFLAGS loaded by call_restored means stepping begins inside
 * the harness's own trampoline, several instructions before the target and
 * across a POPFD - and that killed the process every time. Instead, one 0xCC is
 * written over the target's first byte; the breakpoint handler puts the
 * original byte back, rewinds EIP, and turns TF on there. Stepping then covers
 * exactly the function under test and stops the moment control reaches
 * call_return. Both exception kinds are verified to reach a 32-bit vectored
 * handler under this wine (replay.exe --vehtest). */
static BYTE g_bp_orig;
static int g_bp_armed;
static DWORD g_veh_calls, g_veh_bps;

static LONG WINAPI step_handler(EXCEPTION_POINTERS *ep)
{
    CONTEXT *c = ep->ContextRecord;
    DWORD code = ep->ExceptionRecord->ExceptionCode;

    g_veh_calls++;
    if (code == (DWORD)EXCEPTION_BREAKPOINT) {
        g_veh_bps++;
        /* Wine's wow64 layer reports Eip AT the int3, where Windows reports it
         * one past; accept both rather than depend on which. (--vehtest showed
         * this build reports it AT: an Eip++ there skipped the int3 and ran on
         * correctly, which would have faulted under the other convention.) */
        if (!g_bp_armed || (c->Eip != g_lo && c->Eip != g_lo + 1))
            return EXCEPTION_CONTINUE_SEARCH;
        *(BYTE *)g_lo = g_bp_orig; /* put the real first byte back */
        g_bp_armed = 0;
        c->Eip = g_lo;
        c->EFlags |= 0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code != (DWORD)EXCEPTION_SINGLE_STEP)
        return EXCEPTION_CONTINUE_SEARCH;
    if (++g_steps > STEP_LIMIT || c->Eip == (DWORD)(void *)call_return) {
        c->EFlags &= ~0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (c->Eip >= g_lo && c->Eip < g_hi && g_ntrace < MAX_TRACE)
        g_trace[g_ntrace++] = c->Eip;
    c->EFlags |= 0x100u;
    return EXCEPTION_CONTINUE_EXECUTION;
}

/* --------------------------------------------------------- run and collect */

typedef struct Post {
    BYTE *bytes; /* concatenated, in the ENTRY snapshot's region order */
    DWORD len;
    SnapRegs regs;
    DWORD ntrace;
    DWORD trace[MAX_TRACE];
} Post;

static DWORD post_len(Snap *s)
{
    DWORD i, n = 0;
    for (i = 0; i < s->h->n_regions; i++)
        if (restorable(&s->r[i]))
            n += s->r[i].size;
    return n;
}

static void run_side(Snap *entry, void *fn, Post *out, int trace)
{
    DWORD i, off = 0;

    stage("  restore");
    restore(entry);
    apply_mutations();
    stage("  restored");

    g_r_eax = entry->h->regs.eax;
    g_r_ecx = entry->h->regs.ecx;
    g_r_edx = entry->h->regs.edx;
    g_r_ebx = entry->h->regs.ebx;
    g_r_ebp = entry->h->regs.ebp;
    g_r_esi = entry->h->regs.esi;
    g_r_edi = entry->h->regs.edi;
    g_r_esp = entry->h->entry_esp;
    g_r_efl = entry->h->regs.eflags;
    g_calltarget = (DWORD)fn;
    if (trace) {
        g_bp_orig = *(BYTE *)fn;
        *(BYTE *)fn = 0xcc;
        g_bp_armed = 1;
        g_lo = (DWORD)fn;
        g_hi = g_lo + 0x1000;
    }

    /* The one recorded location the replay rewrites, named in snapshot.h. */
    *(DWORD *)entry->h->entry_esp = (DWORD)(void *)call_return;

    g_ntrace = 0;
    g_steps = 0;
    stage("  call");
    call_restored();
    stage("  returned");

    for (i = 0; i < entry->h->n_regions; i++) {
        SnapRegion *r = &entry->r[i];
        if (!restorable(r))
            continue;
        memcpy(out->bytes + off, (const void *)r->base, r->size);
        off += r->size;
    }
    out->len = off;
    out->regs.eax = g_o_eax;
    out->regs.ecx = g_o_ecx;
    out->regs.edx = g_o_edx;
    out->regs.ebx = g_o_ebx;
    out->regs.esp = g_o_esp;
    out->regs.ebp = g_o_ebp;
    out->regs.esi = g_o_esi;
    out->regs.edi = g_o_edi;
    out->regs.eflags = g_o_efl;
    out->ntrace = g_ntrace;
    for (i = 0; i < g_ntrace && i < MAX_TRACE; i++)
        out->trace[i] = g_trace[i];
}

/* -------------------------------------------------------------- the diff */

typedef struct Hit {
    DWORD addr;
    BYTE a, b;
    DWORD klass;
    char module[40];
} Hit;

typedef struct Verdict {
    DWORD nbytes;
    DWORD nregions;
    DWORD nmissing; /* regions the other side did not record at all */
    Hit hit[MAX_SHOW];
    DWORD nhit;
} Verdict;

/* THE THIRD RULE, and the only one that is MEASURED rather than argued.
 *
 * The capture takes the entry snapshot twice, back to back, with the program
 * frozen and nothing in between. Any byte that differs between those two is not
 * program state: it is what the act of observing costs - wine bookkeeping that
 * moves when 33 MB of address space is walked and copied. `noisy()` is that
 * measurement, and the diff skips exactly those bytes and reports how many.
 *
 * This is deliberately not a list of addresses to ignore. It is re-measured in
 * every capture, so it cannot quietly grow to cover a real difference: if the
 * observer stops perturbing a byte, that byte is compared again the next time.
 */
static int noisy(Snap *entry, SnapRegion *r, const BYTE *probe, DWORD off)
{
    if (!probe)
        return 0;
    return entry->raw[r->blob_off + off] != probe[off];
}

/* THE stack exclusion rule. Nothing else is ever excluded. */
static int excluded(SnapHeader *h, DWORD addr)
{
    if (h->stack_base == 0)
        return 0;
    if (addr < h->stack_base || addr >= h->stack_end)
        return 0;
    return addr < h->entry_esp + 4;
}

static void record(Verdict *v, SnapRegion *r, DWORD addr, BYTE a, BYTE b)
{
    if (!v->nbytes || v->nhit < MAX_SHOW) {
        if (v->nhit < MAX_SHOW) {
            Hit *h = &v->hit[v->nhit++];
            h->addr = addr;
            h->a = a;
            h->b = b;
            h->klass = r->klass;
            memcpy(h->module, r->module, sizeof(h->module));
        }
    }
    v->nbytes++;
}

/* post (concatenated in entry order) against the exit snapshot. */
static void diff_post_snap(Snap *entry, Post *p, Snap *ref, Verdict *v)
{
    DWORD i, off = 0;

    memset(v, 0, sizeof(*v));
    for (i = 0; i < entry->h->n_regions; i++) {
        SnapRegion *r = &entry->r[i];
        const BYTE *b, *probe;
        DWORD j, bad = 0;
        if (!restorable(r))
            continue;
        b = snap_bytes(ref, r->base, r->size);
        if (!b) {
            v->nmissing++;
            off += r->size;
            continue;
        }
        probe = g_have_noise ? snap_bytes(&g_noise, r->base, r->size) : 0;
        for (j = 0; j < r->size; j++) {
            if (p->bytes[off + j] == b[j])
                continue;
            if (excluded(entry->h, r->base + j))
                continue;
            if (noisy(entry, r, probe, j)) {
                g_masked++;
                continue;
            }
            record(v, r, r->base + j, p->bytes[off + j], b[j]);
            bad = 1;
        }
        v->nregions += bad;
        off += r->size;
    }
}

/* THE FUNCTION'S OBSERVABLE EFFECT: the recorded entry against the recorded
 * exit, under exactly the same exclusions the verdicts use. Printed with every
 * run, because "IDENTICAL" means nothing until you know the comparison had
 * something to compare: a function whose effect measures zero bytes is a test
 * that cannot fail, and the harness should say so rather than report a pass. */
static void effect(Snap *entry, Snap *ref, Verdict *v)
{
    DWORD i;

    memset(v, 0, sizeof(*v));
    for (i = 0; i < entry->h->n_regions; i++) {
        SnapRegion *r = &entry->r[i];
        const BYTE *a, *b, *probe;
        DWORD j, bad = 0;
        if (!restorable(r))
            continue;
        b = snap_bytes(ref, r->base, r->size);
        if (!b)
            continue;
        a = entry->raw + r->blob_off;
        probe = g_have_noise ? snap_bytes(&g_noise, r->base, r->size) : 0;
        for (j = 0; j < r->size; j++) {
            if (a[j] == b[j])
                continue;
            if (excluded(entry->h, r->base + j))
                continue;
            if (noisy(entry, r, probe, j))
                continue;
            record(v, r, r->base + j, a[j], b[j]);
            bad = 1;
        }
        v->nregions += bad;
    }
}

static void diff_post_post(Snap *entry, Post *a, Post *b, Verdict *v)
{
    DWORD i, off = 0;

    memset(v, 0, sizeof(*v));
    for (i = 0; i < entry->h->n_regions; i++) {
        SnapRegion *r = &entry->r[i];
        const BYTE *probe;
        DWORD j, bad = 0;
        if (!restorable(r))
            continue;
        probe = g_have_noise ? snap_bytes(&g_noise, r->base, r->size) : 0;
        for (j = 0; j < r->size; j++) {
            if (a->bytes[off + j] == b->bytes[off + j])
                continue;
            if (excluded(entry->h, r->base + j))
                continue;
            if (noisy(entry, r, probe, j)) {
                g_masked++;
                continue;
            }
            record(v, r, r->base + j, a->bytes[off + j], b->bytes[off + j]);
            bad = 1;
        }
        v->nregions += bad;
        off += r->size;
    }
}

/* ------------------------------------------------ the sequence, off-stack */

static Snap g_entry, g_exit;
static Post g_p_ours, g_p_retail;
static Verdict g_v_retail, g_v_ours, g_v_cross;
static int g_trace_on;
static void *g_ours_fn;

static void sequence(void)
{
    /* prepare() runs HERE, not in main(), and that placement is load-bearing.
     * It saves the bytes of every region it is about to clobber - including
     * this thread's real stack - and undo_prepare() puts them back. If it ran
     * before the switch to the scratch stack, the snapshot of our stack would
     * predate on_scratch()'s own pushed registers and return address, and
     * undoing it would restore stale bytes underneath the frame we are about
     * to return through. (That is not hypothetical: it faulted exactly there,
     * reported by wine as an unhandled stack overflow in on_scratch's
     * epilogue.) Taking the save from the scratch stack makes the saved image
     * of our stack final. */
    teb_get_stack(&g_teb_base, &g_teb_limit);
    /* One range wide enough to contain BOTH the scratch stack and the restored
     * game stack. The exception dispatcher validates the current ESP against
     * these bounds, and ESP legitimately moves between the two - a single-step
     * exception taken with ESP on the scratch stack while the bounds described
     * the game stack was reported as "Exception frame is not in stack limits"
     * and killed the process. The bounds are permissive here on purpose; they
     * are restored before main() resumes. */
    {
        DWORD lo = (DWORD)g_scratch;
        DWORD hi = (DWORD)g_scratch + SCRATCH_SIZE;
        if (g_entry.h->stack_base && g_entry.h->stack_base < lo)
            lo = g_entry.h->stack_base;
        if (g_entry.h->stack_end > hi)
            hi = g_entry.h->stack_end;
        g_wide_base = hi;
        g_wide_limit = lo;
    }
    teb_set_stack(g_wide_base, g_wide_limit);
    stage("prepare");
    if (!prepare(&g_entry)) {
        g_fatal = 1;
        undo_prepare();
        stage("prepare FAILED");
        teb_set_stack(g_teb_base, g_teb_limit);
        return;
    }
    stage("prepare ok");
    /* RETAIL first: it is the self-test, and its result frames everything. */
    run_side(&g_entry, (void *)(g_entry.h->image_base + g_entry.h->target_rva),
             &g_p_retail, g_trace_on);
    stage("retail done");
    g_side_ours = 1;
    run_side(&g_entry, g_ours_fn, &g_p_ours, 0);
    g_side_ours = 0;
    stage("ours done");
    undo_prepare();
    teb_set_stack(g_teb_base, g_teb_limit);
    stage("undo done");
}

/* Run `sequence` with ESP in the scratch region, so the restore may overwrite
 * this thread's real stack. */
__declspec(naked) static void on_scratch(void)
{
    __asm {
        push ebx
        push esi
        push edi
        push ebp
        mov  ebx, esp
        mov  esp, g_scratch_esp
        call sequence
        mov  esp, ebx
        pop  ebp
        pop  edi
        pop  esi
        pop  ebx
        ret
    }
}

/* ------------------------------------------------------------- reporting */

static const char *klass_name(DWORD k)
{
    switch (k) {
    case SNAPR_GAME_IMAGE:
        return "game image";
    case SNAPR_OTHER_IMAGE:
        return "other image";
    case SNAPR_PRIVATE:
        return "heap/private";
    case SNAPR_MAPPED:
        return "mapped";
    case SNAPR_HOOK_STACK:
        return "stack";
    }
    return "?";
}

static void show(const char *title, Verdict *v, const char *aname, const char *bname)
{
    DWORD i;
    printf("\n  %s\n", title);
    if (v->nmissing)
        printf("      %lu region(s) present at entry but absent from the reference\n",
               v->nmissing);
    if (!v->nbytes) {
        printf("      IDENTICAL - every compared byte agrees\n");
        return;
    }
    printf("      %lu byte(s) differ across %lu region(s)\n", v->nbytes, v->nregions);
    for (i = 0; i < v->nhit; i++) {
        Hit *h = &v->hit[i];
        printf("      %08lx  %s=%02x  %s=%02x   [%s%s%s]\n", h->addr, aname, h->a, bname,
               h->b, klass_name(h->klass), h->module[0] ? " " : "", h->module);
    }
    if (v->nbytes > v->nhit)
        printf("      ... and %lu more\n", v->nbytes - v->nhit);
}

static int show_regs(const char *title, SnapRegs *got, SnapRegs *want, const char *aname,
                     const char *bname)
{
    int bad = 0;
    /* eax is the return value; ebx/esi/edi/ebp are callee-saved, so they are
     * part of the observable result. ecx/edx are scratch. esp is compared too,
     * because it also proves the callee popped the right argument bytes. */
    struct {
        const char *n;
        DWORD a, b;
    } r[6] = { { "eax", got->eax, want->eax }, { "ebx", got->ebx, want->ebx },
               { "esi", got->esi, want->esi }, { "edi", got->edi, want->edi },
               { "ebp", got->ebp, want->ebp }, { "esp", got->esp, want->esp } };
    int i;
    for (i = 0; i < 6; i++) {
        if (r[i].a == r[i].b)
            continue;
        if (!bad)
            printf("      %s\n", title);
        printf("      %s  %s=%08lx  %s=%08lx\n", r[i].n, aname, r[i].a, bname, r[i].b);
        bad = 1;
    }
    return bad;
}

/* ------------------------------------------------------------------- main */

static void usage(void)
{
    fprintf(stderr,
            "usage: replay.exe <snapdir> [--spawn] [--target N] [--trace]\n"
            "                  [--timeout MS] [--set A=V ...]\n"
            "   --spawn     re-run in a child whose address space was reserved before\n"
            "               its loader ran, with a watchdog. Use this: a plain run\n"
            "               fails whenever wine has already mapped a section inside\n"
            "               the recorded ranges.\n"
            "   <snapdir>   directory holding entry.snap and exit.snap\n"
            "   --target N  index into targets.h (default: match the snapshot rva)\n"
            "   --trace     single-step retail and print the instructions covered\n"
            "   --set-ours A=V  the same, but on OURS only: the negative control.\n"
            "               A run with it MUST report DISAGREE; if it does not, the\n"
            "               comparison is not comparing anything.\n"
            "   --set A=V   write dword V at address A after restoring - a MUTATION,\n"
            "               applied identically to both sides. With any --set the\n"
            "               recorded exit no longer applies and only [3] is a\n"
            "               verdict.\n");
}

/* `--map`: this process's own address space, in the same shape as the capture
 * DLL's census. The two side by side are how you tell whether a snapshot can be
 * restored here at all, and it is the first thing to look at when a
 * VirtualAlloc in make_present() fails. */
static void print_map(void)
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD addr = 0x00010000;

    printf("#     base     size    state  prot   type  module\n");
    while (addr < 0x7ff00000) {
        char mod[MAX_PATH];
        if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)) != sizeof(mbi))
            break;
        if (mbi.RegionSize == 0)
            break;
        mod[0] = 0;
        if (mbi.Type == MEM_IMAGE)
            GetModuleFileNameA((HMODULE)mbi.AllocationBase, mod, sizeof(mod));
        if (mbi.State != MEM_FREE)
            printf("%08lx %8lx %8lx %5lx %6lx  %s\n", (DWORD)mbi.BaseAddress,
                   (DWORD)mbi.RegionSize, mbi.State, mbi.Protect, mbi.Type, mod);
        addr = (DWORD)mbi.BaseAddress + mbi.RegionSize;
    }
}

/* --------------------------------------------------------------- --spawn
 *
 * The address-space problem in its general form: a fresh wine process has
 * already mapped things - a read-only NLS/section view landed squarely inside
 * where GRUNTZ.EXE's .text has to go - and a mapped section view cannot be
 * VirtualProtect'ed to RWX or moved. Saving and clobbering works for private
 * memory; it does not work for someone else's section.
 *
 * The fix is to claim the addresses BEFORE anything else can. A process created
 * CREATE_SUSPENDED has only ntdll mapped and its loader has not run, so
 * reserving the recorded ranges in it with VirtualAllocEx makes every later
 * loader/CRT allocation go somewhere else. Then resume it.
 *
 * That also delivers exactly what fuzzing needs and wine cannot give us
 * directly: fork-per-case isolation and a watchdog. A mutated entry state can
 * crash or fail to terminate - one is a fault in the child, the other is a
 * timeout - and in both cases the parent kills it and moves on.
 */
static char g_report_path[MAX_PATH];

static int spawn_child(const char *self, const char *cmdline, Snap *s, DWORD timeout_ms)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char *cmd = (char *)arena((DWORD)strlen(cmdline) + 1);
    DWORD i, code = 0, reserved = 0, failed = 0;

    strcpy(cmd, cmdline);
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    /* The child does the reporting, so its stdout has to be ours. */
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (!CreateProcessA(self, cmd, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si,
                        &pi)) {
        fprintf(stderr, "replay: CreateProcess(%s) failed (%lu)\n", self,
                GetLastError());
        return 2;
    }
    /* Reserve the UNION of the granule-rounded ranges, not each region
     * separately. Two recorded regions routinely share a 64 KB granule (the
     * game image's header page and its .text, for one), and reserving the
     * second overlapping range fails - which silently left a hole in the middle
     * of GRUNTZ.EXE's .text for the child's loader to map an NLS section into.
     * Merge first; then every reservation is disjoint and can only fail for a
     * real reason. */
    {
        DWORD *lo = (DWORD *)arena(s->h->n_regions * 4);
        DWORD *hi = (DWORD *)arena(s->h->n_regions * 4);
        DWORD n = 0, j;
        for (i = 0; i < s->h->n_regions; i++) {
            SnapRegion *r = &s->r[i];
            if (!restorable(r))
                continue;
            lo[n] = r->base & ~0xffffu;
            hi[n] = (r->base + r->size + 0xffff) & ~0xffffu;
            n++;
        }
        for (i = 0; i + 1 < n; i++) /* the table is already address-ordered */
            for (j = i + 1; j < n; j++)
                if (lo[j] < lo[i]) {
                    DWORD t = lo[i];
                    lo[i] = lo[j];
                    lo[j] = t;
                    t = hi[i];
                    hi[i] = hi[j];
                    hi[j] = t;
                }
        for (i = 0; i < n;) {
            DWORD a = lo[i], b = hi[i];
            j = i + 1;
            while (j < n && lo[j] <= b) {
                if (hi[j] > b)
                    b = hi[j];
                j++;
            }
            if (VirtualAllocEx(pi.hProcess, (LPVOID)a, b - a, MEM_RESERVE,
                               PAGE_READWRITE))
                reserved++;
            else
                failed++;
            i = j;
        }
    }
    printf("replay: spawned child %lu, reserved %lu of %lu merged ranges "
           "(%lu already taken by the kernel: stack, PEB, TEB)\n",
           pi.dwProcessId, reserved, reserved + failed, failed);
    fflush(stdout);
    ResumeThread(pi.hThread);
    if (WaitForSingleObject(pi.hProcess, timeout_ms) == WAIT_TIMEOUT) {
        printf("replay: child exceeded %lu ms - killed (a mutated input that does "
               "not terminate)\n",
               timeout_ms);
        TerminateProcess(pi.hProcess, 3);
        WaitForSingleObject(pi.hProcess, 5000);
        code = 3;
    } else {
        GetExitCodeProcess(pi.hProcess, &code);
        if (code > 0x80000000u) {
            printf("replay: child faulted (%08lx) - a mutated input the program "
                   "could not survive\n",
                   code);
            code = 4;
        }
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    /* The child writes its report to a file rather than to an inherited stdout:
     * a wow64 console child's standard handles did not reach this parent's
     * pipe, and losing the whole report to a handle-inheritance detail is not
     * an acceptable failure mode for a tool whose entire output is that
     * report. */
    {
        FILE *f = fopen(g_report_path, "r");
        char line[512];
        if (!f) {
            printf("replay: child left no report at %s\n", g_report_path);
            return code ? (int)code : 2;
        }
        while (fgets(line, sizeof(line), f))
            fputs(line, stdout);
        fclose(f);
    }
    return (int)code;
}

/* Does this wine/wow64 build deliver debug exceptions to a 32-bit vectored
 * handler at all? `--vehtest` answers it in isolation: one INT3, one TF step.
 * The coverage feature depends entirely on the answer, so it is worth a mode
 * rather than a guess. */
static volatile int g_veh_bp, g_veh_step;

static LONG WINAPI vehtest_handler(EXCEPTION_POINTERS *ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    g_veh_calls++;
    if (code == (DWORD)EXCEPTION_BREAKPOINT) {
        g_veh_bps++;
        g_veh_bp++;
        ep->ContextRecord->Eip++;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == (DWORD)EXCEPTION_SINGLE_STEP) {
        g_veh_step++;
        ep->ContextRecord->EFlags &= ~0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static int vehtest(void)
{
    ADD_VEH add = (ADD_VEH)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                          "AddVectoredExceptionHandler");
    if (!add)
        add = (ADD_VEH)GetProcAddress(GetModuleHandleA("ntdll.dll"),
                                      "RtlAddVectoredExceptionHandler");
    printf("AddVectoredExceptionHandler: %s\n", add ? "resolved" : "MISSING");
    if (!add)
        return 1;
    if (!add(1, vehtest_handler)) {
        printf("registration FAILED\n");
        return 1;
    }
    printf("int3 ...\n");
    fflush(stdout);
    __asm { int 3 }
    printf("  breakpoint delivered: %d\n", g_veh_bp);
    printf("trap flag ...\n");
    fflush(stdout);
    __asm {
        pushfd
        or   DWORD PTR [esp], 0x100
        popfd
        nop
        nop
    }
    printf("  single-step delivered: %d\n", g_veh_step);
    return 0;
}

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    const char *dir = 0;
    int target = -1, i, bad = 0, spawn = 0, child = 0;
    DWORD n, timeout_ms = 30000;

    if (argc == 2 && !strcmp(argv[1], "--map")) {
        print_map();
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--vehtest"))
        return vehtest();

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--target") && i + 1 < argc)
            target = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--trace"))
            g_trace_on = 1;
        else if (!strcmp(argv[i], "--spawn"))
            spawn = 1;
        else if (!strcmp(argv[i], "--child"))
            child = 1;
        else if (!strcmp(argv[i], "--verbose-restore"))
            g_verbose_restore = 1;
        else if (!strcmp(argv[i], "--timeout") && i + 1 < argc)
            timeout_ms = (DWORD)strtoul(argv[++i], 0, 0);
        else if (!strcmp(argv[i], "--set-ours") && i + 1 < argc) {
            char buf[64], *eq;
            lstrcpynA(buf, argv[++i], sizeof(buf));
            eq = strchr(buf, '=');
            if (eq && g_nset_ours < MAX_SET) {
                *eq = 0;
                g_set_ours[g_nset_ours].addr = strtoul(buf, 0, 0);
                g_set_ours[g_nset_ours].value = strtoul(eq + 1, 0, 0);
                g_nset_ours++;
            }
        } else if (!strcmp(argv[i], "--set") && i + 1 < argc) {
            /* Parse into a COPY. Writing the '=' terminator into argv[] in
             * place truncated the argument that --spawn then forwarded to the
             * child, so the child silently ran with no mutation at all and
             * cheerfully reported that everything agreed. */
            char buf[64], *eq;
            lstrcpynA(buf, argv[++i], sizeof(buf));
            eq = strchr(buf, '=');
            if (eq && g_nset < MAX_SET) {
                *eq = 0;
                g_set[g_nset].addr = strtoul(buf, 0, 0);
                g_set[g_nset].value = strtoul(eq + 1, 0, 0);
                g_nset++;
            }
        } else if (argv[i][0] != '-' && !dir) {
            dir = argv[i];
        } else {
            usage();
            return 2;
        }
    }
    if (!dir) {
        usage();
        return 2;
    }

    g_arena = (BYTE *)VirtualAlloc((LPVOID)ARENA_BASE, ARENA_SIZE,
                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!g_arena)
        g_arena = (BYTE *)VirtualAlloc(NULL, ARENA_SIZE, MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);
    g_scratch = (BYTE *)VirtualAlloc(NULL, SCRATCH_SIZE, MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!g_arena || !g_scratch) {
        fprintf(stderr, "replay: arena/scratch allocation failed\n");
        return 2;
    }
    g_scratch_esp = (DWORD)(g_scratch + SCRATCH_SIZE - 0x400);

    sprintf(path, "%s\\entry.snap", dir);
    if (!load_snap(path, &g_entry))
        return 2;
    sprintf(path, "%s\\exit.snap", dir);
    if (!load_snap(path, &g_exit))
        return 2;
    /* Optional, and the harness is much weaker without it - see noisy(). */
    sprintf(path, "%s\\entry2.snap", dir);
    {
        HANDLE probe = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (probe != INVALID_HANDLE_VALUE) {
            CloseHandle(probe);
            g_have_noise = load_snap(path, &g_noise);
        }
    }

    sprintf(g_report_path, "%s\\replay.out", dir);
    if (child) {
        if (!freopen(g_report_path, "w", stdout))
            fprintf(stderr, "replay: cannot write %s\n", g_report_path);
        /* Unbuffered: a child that faults or is killed by the watchdog must
         * still have left everything it printed up to that point on disk. A
         * buffered report is a report you lose exactly when you need it. */
        setvbuf(stdout, NULL, _IONBF, 0);
    }
    if (spawn) {
        /* Re-run ourselves in a child whose address space was claimed before
         * its loader ran. Everything after this point happens in that child. */
        char cmd[4096];
        int k;
        cmd[0] = 0;
        strcat(cmd, "\"");
        strcat(cmd, argv[0]);
        strcat(cmd, "\"");
        for (k = 1; k < argc; k++) {
            if (!strcmp(argv[k], "--spawn"))
                continue;
            if (!strcmp(argv[k], "--timeout")) {
                k++;
                continue;
            }
            strcat(cmd, " ");
            strcat(cmd, argv[k]);
        }
        strcat(cmd, " --child");
        return spawn_child(argv[0], cmd, &g_entry, timeout_ms);
    }

    if (target < 0)
        for (i = 0; i < (int)REPLAY_NTARGETS; i++)
            if (g_targets[i].rva == g_entry.h->target_rva)
                target = i;
    if (target < 0 || target >= (int)REPLAY_NTARGETS) {
        fprintf(stderr, "replay: snapshot rva %08lx (%s) is not in targets.h\n",
                g_entry.h->target_rva, g_entry.h->target_name);
        return 2;
    }
    g_ours_fn = g_targets[target].ours;

    printf("replay: %s\n", g_targets[target].name);
    fflush(stdout);
    printf("  snapshot   rva=%08lx image=%08lx hit=%lu regions=%lu %lu bytes%s\n",
           g_entry.h->target_rva, g_entry.h->image_base, g_entry.h->hit_index,
           g_entry.h->n_regions, g_entry.len,
           (g_entry.h->flags & SNAP_FULL) ? "  [full address space]"
                                          : "  [writable + game image]");
    printf("  entry      esp=%08lx ret=%08lx ecx=%08lx\n", g_entry.h->entry_esp,
           g_entry.h->ret_addr, g_entry.h->regs.ecx);
    printf("  stack      %08lx..%08lx  EXCLUDED below %08lx (+4: the return slot)\n",
           g_entry.h->stack_base, g_entry.h->stack_end, g_entry.h->entry_esp);
    printf("  code       retail=%08lx  ours=%08lx\n",
           g_entry.h->image_base + g_entry.h->target_rva, (DWORD)g_ours_fn);
    for (i = 0; i < (int)g_nset; i++)
        printf("  MUTATION   [%08lx] = %08lx  (both sides)\n", g_set[i].addr,
               g_set[i].value);
    for (i = 0; i < (int)g_nset_ours; i++)
        printf("  CONTROL    [%08lx] = %08lx  (OURS side only - this run MUST "
               "report DISAGREE)\n",
               g_set_ours[i].addr, g_set_ours[i].value);

    n = post_len(&g_entry);
    g_p_ours.bytes = (BYTE *)arena(n);
    g_p_retail.bytes = (BYTE *)arena(n);

    if (g_trace_on) {
        ADD_VEH add = (ADD_VEH)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                              "AddVectoredExceptionHandler");
        if (!add)
            add = (ADD_VEH)GetProcAddress(GetModuleHandleA("ntdll.dll"),
                                          "RtlAddVectoredExceptionHandler");
        if (!add || !add(1, step_handler)) {
            printf("  trace      UNAVAILABLE (no vectored-exception-handler export)\n");
            g_trace_on = 0;
        }
    }

    {
        char sp[MAX_PATH];
        sprintf(sp, "%s\\stages.txt", dir);
        g_stage_h = CreateFileA(sp, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    }
    on_scratch();
    if (g_stage_h != INVALID_HANDLE_VALUE)
        CloseHandle(g_stage_h);
    if (g_fatal)
        return 2;

    /* Everything below runs with our own address space intact again. */
    if (g_trace_on) {
        DWORD k;
        printf("  veh        %lu exception(s) reached the handler, %lu breakpoint(s)\n",
               g_veh_calls, g_veh_bps);
        printf("  covered    %lu instruction(s) of retail, at target offsets:",
               g_p_retail.ntrace);
        for (k = 0; k < g_p_retail.ntrace && k < 48; k++)
            printf(" +%lx", g_p_retail.trace[k] - g_lo);
        printf("%s\n", g_p_retail.ntrace > 48 ? " ..." : "");
    }

    printf("  noise      %s\n",
           g_have_noise ? "measured (entry.snap vs entry2.snap: the observer's own "
                          "footprint is masked)"
                        : "NOT MEASURED - no entry2.snap; observer noise will show "
                          "up as a disagreement");
    {
        Verdict e;
        DWORD saved = g_masked;
        /* Under mutation the recorded exit is not the answer to this input, so
         * the effect is measured against the RETAIL RE-RUN instead. The bytes
         * written by --set itself show up in that count, deliberately: they are
         * part of what changed. */
        if (g_nset)
            diff_post_snap(&g_entry, &g_p_retail, &g_entry, &e);
        else
            effect(&g_entry, &g_exit, &e);
        g_masked = saved;
        printf("  effect     %s changed %lu byte(s) in %lu region(s)"
               " - that is what the verdicts below have to agree about\n",
               g_nset ? "the MUTATED call" : "the recorded call", e.nbytes,
               e.nregions);
        if (!e.nbytes)
            printf("      WARNING: zero measured effect. Any \"IDENTICAL\" below is "
                   "vacuous - pick a call of this function that does something.\n");
        else {
            /* The two producers order (a,b) oppositely: effect() records
             * (entry, exit); the mutated path records (post, entry). Print
             * before/after, not whichever slot happened to be first. */
            DWORD k;
            for (k = 0; k < e.nhit && k < 8; k++) {
                BYTE before = g_nset ? e.hit[k].b : e.hit[k].a;
                BYTE after = g_nset ? e.hit[k].a : e.hit[k].b;
                printf("      %08lx  before=%02x  after=%02x   [%s]\n", e.hit[k].addr,
                       before, after, klass_name(e.hit[k].klass));
            }
            if (e.nbytes > 8)
                printf("      ... and %lu more\n", e.nbytes - 8);
        }
    }
    diff_post_snap(&g_entry, &g_p_retail, &g_exit, &g_v_retail);
    diff_post_snap(&g_entry, &g_p_ours, &g_exit, &g_v_ours);
    diff_post_post(&g_entry, &g_p_ours, &g_p_retail, &g_v_cross);

    if (g_nset == 0) {
        show("[1] RETAIL re-run vs the RECORDED exit   (harness self-test)", &g_v_retail,
             "rerun", "recorded");
        bad |= show_regs("registers:", &g_p_retail.regs, &g_exit.h->regs, "rerun",
                         "recorded")
               || g_v_retail.nbytes != 0;
        show("[2] OURS vs the RECORDED exit            (the oracle verdict)", &g_v_ours,
             "ours", "recorded");
        bad |= show_regs("registers:", &g_p_ours.regs, &g_exit.h->regs, "ours",
                         "recorded")
               || g_v_ours.nbytes != 0;
    } else {
        printf("\n  the entry state was MUTATED: the recorded exit does not apply,\n"
               "  so [1] and [2] are suppressed and only [3] is a verdict.\n");
    }
    show("[3] OURS vs the RETAIL re-run            (valid under mutation too)",
         &g_v_cross, "ours", "retail");
    bad |= show_regs("registers:", &g_p_ours.regs, &g_p_retail.regs, "ours", "retail")
           || g_v_cross.nbytes != 0;

    if (g_masked)
        printf("\n  (%lu byte comparison(s) skipped as measured observer noise)\n",
               g_masked);
    printf("\nVERDICT: %s\n", bad ? "DISAGREE" : "agree on every compared byte");
    fflush(stdout);
    return bad ? 1 : 0;
}

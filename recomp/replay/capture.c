/* capture.c - the record half of record-and-replay, as an injected DLL.
 *
 * ---------------------------------------------------------------------------
 * Why a DLL and not Frida, and not gdb
 * ---------------------------------------------------------------------------
 * The archived tracer (scripts/archive/dynamic-trace/) drove a Frida gadget
 * under wine, and it worked - but it was retired, and its remaining machinery
 * we reuse is the asset provisioning, not the tracing. For STATE capture the
 * in-process DLL is the better instrument, for three concrete reasons:
 *
 *   1. wine here is 11.8 wow64: a 32-bit PE runs inside a 64-bit host process.
 *      A ptrace/gdb stub therefore sees a 64-bit process whose /proc maps carry
 *      Linux protections, not Win32 ones, and whose register file has to be
 *      read through the compat-mode view. Everything we need is a VirtualQuery
 *      away from inside.
 *   2. The replay harness is itself a 32-bit PE and restores through
 *      VirtualAlloc. Capturing through the SAME API as the restore means the
 *      region table means literally the same thing on both sides.
 *   3. It is the same toolchain as everything else here - MSVC 5.0 under wine,
 *      no new pinned 33 MB binary, no JS bridge that cannot express a
 *      fixed-address restore anyway.
 *
 * ---------------------------------------------------------------------------
 * How it gets in, and how it stays in
 * ---------------------------------------------------------------------------
 * `SFManager_SelectBestDevice` (RVA 0x0f8970) starts with an unconditional
 * LoadLibraryA("SFMAN32.DLL"), and is called once from CGruntzMgr::Run+0x9c8.
 * Dropping this DLL in the game directory under that name gets DllMain run
 * mid-init, before the main loop. That is the same door the archived Frida
 * gadget used.
 *
 * The catch the gadget did not have: if GetProcAddress(h,"SFManager") fails the
 * game immediately FreeLibrary's us, which would unmap the hook stubs the
 * patched call sites now point at - an instant crash. So DllMain pins the
 * module with a self LoadLibraryA. (Exporting a fake `SFManager` is the other
 * option and is worse: it is a pointer to a COM-ish interface the game then
 * calls into, so we would have to fake a SoundFont device.)
 *
 * ---------------------------------------------------------------------------
 * How the hook works - call-site patching, not a prologue detour
 * ---------------------------------------------------------------------------
 * A prologue detour needs instruction-length decoding to relocate the bytes it
 * overwrites. Patching the `call rel32` at the CALL SITE needs none: rewrite
 * the 4-byte displacement to point at our stub, and the target function stays
 * byte-for-byte untouched. The stub is then entered with EXACTLY the state the
 * callee would have seen - same ESP, same arguments in place, same ecx.
 *
 * To regain control at the return, the stub overwrites the return address at
 * [esp] with its own trampoline and `jmp`s to the real target. The callee thus
 * runs on an unmodified stack with unmodified registers, and its `ret N` lands
 * in us with eax intact.
 *
 * The stub switches ESP to a private scratch stack before doing any work, so it
 * never writes below the callee's entry ESP. That matters: those bytes are the
 * dead scratch the un-instrumented callee would have inherited, and the
 * snapshot records them faithfully so a replay inherits the same ones.
 *
 * All call sites are un-patched before the snapshot is taken, so the recorded
 * image is retail's bytes and not ours.
 *
 * ---------------------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------------------
 * `capture.cfg` in the game directory, one `key=value` per line:
 *
 *     mode=census|capture    census dumps the region table as text and stops
 *     out=Z:\abs\path        output directory (wine path)
 *     target=0x000f9280      target RVA
 *     name=?Foo@@YAXXZ       label, recorded in the snapshot header
 *     site=0x00012345        a `call rel32` site to patch (repeatable)
 *     hit=0                  capture the Nth call (0-based)
 *     full=0|1               1 = snapshot every committed region (tier 2)
 *     cap=0x8000000          per-phase snapshot buffer cap
 */

#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "snapshot.h"

#define MAX_SITES 512
#define MAX_REGIONS 8192

void hook_entry(void);
void hook_return(void);

/* ------------------------------------------------------------------ config */

static char g_out[MAX_PATH] = "";
static char g_name[64] = "";
static DWORD g_mode_census = 0;
static DWORD g_mode_probe = 0;
static DWORD g_target_rva = 0;
static DWORD g_hit_want = 0;
static DWORD g_full = 0;
static DWORD g_cap = 0x04000000; /* 64 MB per buffer, three of them */
static DWORD g_sites[MAX_SITES];
static DWORD g_nsites = 0;

/* ------------------------------------------------------------------- state */

static BYTE *g_image;               /* GRUNTZ.EXE load base */
static BYTE *g_self;                /* this DLL's load base */
static DWORD g_target;              /* absolute VA of the target */
static DWORD g_orig_disp[MAX_SITES];/* original rel32 at each patched site */
static DWORD g_patched;
static DWORD g_hit;
static DWORD g_done;

static BYTE *g_buf[3];              /* entry, exit, and the noise probe */
static DWORD g_buf_len[3];
static BYTE *g_stackbuf;            /* scratch stack for the hook */
static BYTE *g_stubs;               /* generated probe stubs (probe mode) */
static DWORD g_scratch_esp;

static SnapRegion g_tab[MAX_REGIONS];

/* Entry / exit register files, filled by the naked stubs. */
static DWORD g_e_eax, g_e_ecx, g_e_edx, g_e_ebx, g_e_esp, g_e_ebp, g_e_esi,
    g_e_edi, g_e_efl;
static DWORD g_x_eax, g_x_ecx, g_x_edx, g_x_ebx, g_x_esp, g_x_ebp, g_x_esi,
    g_x_edi, g_x_efl;
static DWORD g_retaddr;
static DWORD g_savedesp;
static DWORD g_hookret; /* address of hook_return, set in DllMain */
static DWORD g_froze;   /* other threads suspended for the capture window */

/* ------------------------------------------------------------------- probes
 *
 * `mode=probe` answers the question that decides which function is worth
 * capturing at all: *which of my candidates does a real play session actually
 * call?* Guessing costs a 90-second game launch per guess; probing answers for
 * 128 candidates in one launch.
 *
 * One 16-byte stub is generated per candidate at run time:
 *
 *     C7 05 <&g_probe_idx> <i>   mov dword ptr [g_probe_idx], i
 *     E9    <rel32>              jmp probe_common
 *
 * and every direct call site of candidate i is pointed at its stub. The common
 * tail counts the hit and jumps on to the real target. It uses only `mov` and
 * `lea`, so it does not disturb EFLAGS, and it preserves eax/ecx - which is
 * what a __thiscall callee is entitled to. It is not re-entrant across threads
 * (g_probe_idx is one global); the game's logic is single-threaded, and a lost
 * count in a hit CENSUS is not a correctness problem the way a lost byte in a
 * snapshot would be.
 */
#define MAX_PROBES 256
#define MAX_PSITES 4096
static DWORD g_psite[MAX_PSITES];       /* call-site RVA */
static DWORD g_psite_owner[MAX_PSITES]; /* which candidate it reaches */
static DWORD g_psite_orig[MAX_PSITES];  /* the displacement we replaced */
static DWORD g_npsites;
static DWORD g_ptarget[MAX_PROBES]; /* absolute VA of candidate i */
static DWORD g_pcount[MAX_PROBES];  /* hits of candidate i */
static DWORD g_prva[MAX_PROBES];    /* its RVA, for the report */
static DWORD g_nprobes;
static DWORD g_probe_idx;
static DWORD g_probe_jmp;
static DWORD g_probe_sav, g_probe_sav2;
void probe_common(void);

/* --------------------------------------------------------------------- log */

static void caplog(const char *fmt, ...)
{
    char path[MAX_PATH], line[1024];
    va_list ap;
    HANDLE h;
    DWORD n;

    va_start(ap, fmt);
    wvsprintfA(line, fmt, ap);
    va_end(ap);
    lstrcatA(line, "\r\n");

    wsprintfA(path, "%s\\capture.log", g_out[0] ? g_out : ".");
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    SetFilePointer(h, 0, NULL, FILE_END);
    WriteFile(h, line, lstrlenA(line), &n, NULL);
    CloseHandle(h);
}

/* ------------------------------------------------------------------ config */

static DWORD parse_u32(const char *s)
{
    DWORD v = 0;
    while (*s == ' ')
        s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            char c = *s++;
            if (c >= '0' && c <= '9')
                v = v * 16 + (DWORD)(c - '0');
            else if (c >= 'a' && c <= 'f')
                v = v * 16 + (DWORD)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                v = v * 16 + (DWORD)(c - 'A' + 10);
            else
                break;
        }
    } else {
        while (*s >= '0' && *s <= '9')
            v = v * 10 + (DWORD)(*s++ - '0');
    }
    return v;
}

/* Both buffers here are sized for the WHOLE runnable worklist, and both say so
 * when they fill. They were 8 KB and 512 B, which the 129-candidate probe list
 * fits into by 1 KB and 110 characters - and neither would have said a word on
 * the day it stopped fitting. A truncated `probe=` line drops call sites, which
 * shows up as "this candidate is never called", i.e. as a fact about the game
 * rather than as a bug. Silence is the failure mode that costs the most here. */
static void read_config(void)
{
    HANDLE h;
    static char buf[262144];
    DWORD n = 0, i = 0;

    h = CreateFileA("capture.cfg", GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    ReadFile(h, buf, sizeof(buf) - 1, &n, NULL);
    CloseHandle(h);
    buf[n] = 0;
    if (n >= sizeof(buf) - 1)
        caplog("CONFIG TRUNCATED: capture.cfg is at least %lu bytes and the reader "
               "takes %lu - candidates were DROPPED",
               n, (DWORD)sizeof(buf) - 1);

    while (i < n) {
        static char line[8192];
        DWORD j = 0;
        while (i < n && buf[i] != '\n' && buf[i] != '\r' && j < sizeof(line) - 1)
            line[j++] = buf[i++];
        line[j] = 0;
        if (j == sizeof(line) - 1 && i < n && buf[i] != '\n' && buf[i] != '\r') {
            caplog("CONFIG LINE TRUNCATED at %lu chars - call sites were DROPPED",
                   j);
            while (i < n && buf[i] != '\n' && buf[i] != '\r')
                i++;
        }
        while (i < n && (buf[i] == '\n' || buf[i] == '\r'))
            i++;
        if (line[0] == 0 || line[0] == '#')
            continue;
        {
            char *eq = strchr(line, '=');
            char *v;
            if (!eq)
                continue;
            *eq = 0;
            v = eq + 1;
            if (!lstrcmpA(line, "mode")) {
                g_mode_census = !lstrcmpA(v, "census");
                g_mode_probe = !lstrcmpA(v, "probe");
            } else if (!lstrcmpA(line, "probe")) {
                /* probe=<targetRva>,<siteRva>,<siteRva>,... */
                DWORD idx = g_nprobes;
                if (g_nprobes >= MAX_PROBES) {
                    caplog("TOO MANY CANDIDATES: MAX_PROBES=%d, dropping the rest",
                           (int)MAX_PROBES);
                    continue;
                }
                g_nprobes++;
                g_prva[idx] = parse_u32(v);
                while ((v = strchr(v, ',')) != NULL) {
                    v++;
                    if (g_npsites >= MAX_PSITES) {
                        caplog("TOO MANY SITES: MAX_PSITES=%d, dropping the rest",
                               (int)MAX_PSITES);
                        break;
                    }
                    g_psite_owner[g_npsites] = idx;
                    g_psite[g_npsites++] = parse_u32(v);
                }
            } else if (!lstrcmpA(line, "out"))
                lstrcpynA(g_out, v, sizeof(g_out));
            else if (!lstrcmpA(line, "name"))
                lstrcpynA(g_name, v, sizeof(g_name));
            else if (!lstrcmpA(line, "target"))
                g_target_rva = parse_u32(v);
            else if (!lstrcmpA(line, "hit"))
                g_hit_want = parse_u32(v);
            else if (!lstrcmpA(line, "full"))
                g_full = parse_u32(v);
            else if (!lstrcmpA(line, "cap"))
                g_cap = parse_u32(v);
            else if (!lstrcmpA(line, "site")) {
                if (g_nsites >= MAX_SITES)
                    caplog("TOO MANY SITES: MAX_SITES=%d, dropping the rest",
                           (int)MAX_SITES);
                else
                    g_sites[g_nsites++] = parse_u32(v);
            }
        }
    }
}

/* ------------------------------------------------------------- region walk */

static int is_writable(DWORD p)
{
    p &= 0xff; /* strip PAGE_GUARD / PAGE_NOCACHE */
    return p == PAGE_READWRITE || p == PAGE_WRITECOPY || p == PAGE_EXECUTE_READWRITE
           || p == PAGE_EXECUTE_WRITECOPY;
}

static int is_readable(DWORD p)
{
    if (p & PAGE_GUARD)
        return 0;
    p &= 0xff;
    return p != 0 && p != PAGE_NOACCESS;
}

static void basename_of(const char *path, char *out, int cap)
{
    const char *b = path, *p;
    for (p = path; *p; p++)
        if (*p == '\\' || *p == '/')
            b = p + 1;
    lstrcpynA(out, b, cap);
}

/* Is this region part of the capture apparatus rather than the program?
 * Our DLL image and our own buffers did not exist in the un-instrumented
 * process, so recording them would be recording the observer. */
static int is_ours(DWORD alloc_base)
{
    DWORD i;
    if (alloc_base == (DWORD)g_self)
        return 1;
    /* Every buffer, indexed - not three hand-written comparisons. Adding the
     * noise-probe buffer without extending this recorded 64 MB of the observer
     * into the snapshot and overflowed it. */
    for (i = 0; i < sizeof(g_buf) / sizeof(g_buf[0]); i++)
        if (g_buf[i] && alloc_base == (DWORD)g_buf[i])
            return 1;
    if (g_stackbuf && alloc_base == (DWORD)g_stackbuf)
        return 1;
    if (g_stubs && alloc_base == (DWORD)g_stubs)
        return 1;
    return 0;
}

/* Fill g_tab; return the region count. */
static DWORD walk_regions(int record_all)
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD addr = 0x00010000;
    DWORD n = 0;

    while (addr < 0x7ff00000 && n < MAX_REGIONS) {
        SnapRegion *r;
        char mod[MAX_PATH];
        int want;

        if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)) != sizeof(mbi))
            break;
        if (mbi.RegionSize == 0)
            break;

        if (mbi.State != MEM_COMMIT || is_ours((DWORD)mbi.AllocationBase))
            goto next;

        r = &g_tab[n];
        memset(r, 0, sizeof(*r));
        r->base = (DWORD)mbi.BaseAddress;
        r->size = mbi.RegionSize;
        r->protect = mbi.Protect;
        r->alloc_base = (DWORD)mbi.AllocationBase;
        r->alloc_protect = mbi.AllocationProtect;
        r->state = mbi.State;
        r->type = mbi.Type;

        mod[0] = 0;
        if (mbi.Type == MEM_IMAGE)
            GetModuleFileNameA((HMODULE)mbi.AllocationBase, mod, sizeof(mod));
        basename_of(mod, r->module, sizeof(r->module));

        if (mbi.AllocationBase == (LPVOID)g_image)
            r->klass = SNAPR_GAME_IMAGE;
        else if (mbi.Type == MEM_IMAGE)
            r->klass = SNAPR_OTHER_IMAGE;
        else if (mbi.Type == MEM_MAPPED)
            r->klass = SNAPR_MAPPED;
        else
            r->klass = SNAPR_PRIVATE;

        if (is_writable(mbi.Protect))
            r->rflags |= SNAPRF_WRITABLE;

        /* Tier 1: writable regions, plus the whole game image (the replay must
         * execute retail's code and read its constants). Tier 2: everything. */
        want = record_all || (r->rflags & SNAPRF_WRITABLE)
               || r->klass == SNAPR_GAME_IMAGE;
        if (want && is_readable(mbi.Protect))
            r->rflags |= SNAPRF_HAS_BYTES;
        n++;

    next:
        addr = (DWORD)mbi.BaseAddress + mbi.RegionSize;
    }
    return n;
}

static void teb_stack(DWORD *lo, DWORD *hi)
{
    DWORD base, limit;
    __asm {
        mov eax, fs:[4]
        mov base, eax
        mov eax, fs:[8]
        mov limit, eax
    }
    *hi = base;
    *lo = limit;
}

/* ------------------------------------------------------------- census mode */

static void census(void)
{
    char path[MAX_PATH], line[512];
    HANDLE h;
    DWORD n, i, w = 0, tot = 0, wrote;

    n = walk_regions(1);
    wsprintfA(path, "%s\\census.txt", g_out[0] ? g_out : ".");
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    wsprintfA(line, "# image=%08lx self=%08lx regions=%lu\r\n"
                    "#     base     size  prot  type klass W module\r\n",
              (DWORD)g_image, (DWORD)g_self, n);
    WriteFile(h, line, lstrlenA(line), &wrote, NULL);
    for (i = 0; i < n; i++) {
        SnapRegion *r = &g_tab[i];
        wsprintfA(line, "%08lx %8lx %5lx %5lx %5lu %d %s\r\n", r->base, r->size,
                  r->protect, r->type, r->klass,
                  (r->rflags & SNAPRF_WRITABLE) ? 1 : 0, r->module);
        WriteFile(h, line, lstrlenA(line), &wrote, NULL);
        tot += r->size;
        if (r->rflags & SNAPRF_WRITABLE)
            w += r->size;
        else if (r->klass == SNAPR_GAME_IMAGE)
            w += r->size;
    }
    wsprintfA(line, "# committed=%lu bytes  tier1(writable+game image)=%lu bytes\r\n",
              tot, w);
    WriteFile(h, line, lstrlenA(line), &wrote, NULL);
    CloseHandle(h);
    caplog("census: %lu regions, committed=%lu, tier1=%lu", n, tot, w);
}

/* ------------------------------------------------------ freezing the world
 *
 * A snapshot of a MULTITHREADED process is only an observable if nothing else
 * is running. Without this the harness's own self-test - retail re-run against
 * the recorded exit - reported 11492 differing bytes in four heap regions for a
 * 48-byte function that writes twelve, because the DirectSound mixer and wine's
 * worker threads kept writing while the 47 MB entry snapshot was being copied,
 * and kept writing between entry and exit. Those bytes were not the function's;
 * they were the rest of the program's.
 *
 * So every other thread in the process is suspended for the whole window: the
 * entry snapshot, the call, and the exit snapshot. The snapshot path allocates
 * nothing and takes no lock, so suspending a thread inside the heap lock cannot
 * deadlock us.
 *
 * ToolHelp is resolved at run time - it is not in the VC5 import libraries -
 * and if it is unavailable the capture still runs and says so, which is better
 * than silently recording a torn snapshot.
 */

typedef struct TH32ENTRY {
    DWORD dwSize;
    DWORD cntUsage;
    DWORD th32ThreadID;
    DWORD th32OwnerProcessID;
    LONG tpBasePri;
    LONG tpDeltaPri;
    DWORD dwFlags;
} TH32ENTRY;

typedef HANDLE(WINAPI *FN_SNAP)(DWORD, DWORD);
typedef BOOL(WINAPI *FN_T32)(HANDLE, TH32ENTRY *);
typedef HANDLE(WINAPI *FN_OPENTHREAD)(DWORD, BOOL, DWORD);

#define TH32CS_SNAPTHREAD_ 0x00000004
#define THREAD_SUSPEND_RESUME_ 0x0002

#define MAX_FROZEN 128
static HANDLE g_frozen[MAX_FROZEN];
static DWORD g_nfrozen;

static DWORD freeze_others(void)
{
    HMODULE k = GetModuleHandleA("kernel32.dll");
    FN_SNAP snap = (FN_SNAP)GetProcAddress(k, "CreateToolhelp32Snapshot");
    FN_T32 first = (FN_T32)GetProcAddress(k, "Thread32First");
    FN_T32 next = (FN_T32)GetProcAddress(k, "Thread32Next");
    FN_OPENTHREAD openthr = (FN_OPENTHREAD)GetProcAddress(k, "OpenThread");
    HANDLE h;
    TH32ENTRY te;
    DWORD me = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();

    g_nfrozen = 0;
    if (!snap || !first || !next || !openthr)
        return 0;
    h = snap(TH32CS_SNAPTHREAD_, 0);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    te.dwSize = sizeof(te);
    if (first(h, &te)) {
        do {
            HANDLE t;
            if (te.th32OwnerProcessID != pid || te.th32ThreadID == me)
                continue;
            if (g_nfrozen >= MAX_FROZEN)
                break;
            t = openthr(THREAD_SUSPEND_RESUME_, FALSE, te.th32ThreadID);
            if (!t)
                continue;
            if (SuspendThread(t) == (DWORD)-1) {
                CloseHandle(t);
                continue;
            }
            g_frozen[g_nfrozen++] = t;
        } while (te.dwSize = sizeof(te), next(h, &te));
    }
    CloseHandle(h);
    return g_nfrozen;
}

static void thaw_others(void)
{
    DWORD i;
    for (i = 0; i < g_nfrozen; i++) {
        ResumeThread(g_frozen[i]);
        CloseHandle(g_frozen[i]);
    }
    g_nfrozen = 0;
}

/* ------------------------------------------------------------ snapshotting */

/* Serialize the current address space into g_buf[phase]. Returns byte length,
 * or 0 on overflow (which is a hard error, not a truncation). */
static DWORD snap(int bufidx, int phase, SnapRegs *regs)
{
    BYTE *buf = g_buf[bufidx];
    SnapHeader *h = (SnapHeader *)buf;
    SnapRegion *tab;
    DWORD n, i, off, lo, hi;

    n = walk_regions(g_full != 0);
    teb_stack(&lo, &hi);

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, SNAP_MAGIC, 8);
    h->version = SNAP_VERSION;
    h->flags = g_full ? SNAP_FULL : SNAP_WRITABLE;
    h->phase = (DWORD)phase;
    h->hdr_size = sizeof(SnapHeader);
    h->region_size = sizeof(SnapRegion);
    h->n_regions = n;
    h->region_off = sizeof(SnapHeader);
    h->image_base = (DWORD)g_image;
    h->target_rva = g_target_rva;
    h->hit_index = g_hit_want;
    h->entry_esp = g_e_esp;
    h->ret_addr = g_retaddr;
    h->stack_end = hi;
    h->tick = GetTickCount();
    h->regs = *regs;
    lstrcpynA(h->target_name, g_name, sizeof(h->target_name));

    tab = (SnapRegion *)(buf + sizeof(SnapHeader));
    off = sizeof(SnapHeader) + n * sizeof(SnapRegion);

    /* Size the whole thing BEFORE copying any of it, and say what it needs.
     * "raise cap=" without a number is a guess-and-relaunch loop at 100 seconds
     * a go: an in-game snapshot is several times the menu one, and the answer
     * is right here. */
    {
        DWORD need = off;
        for (i = 0; i < n; i++)
            if (g_tab[i].rflags & SNAPRF_HAS_BYTES)
                need += g_tab[i].size;
        if (need > g_cap) {
            caplog("snapshot NEEDS %lu bytes, cap=%lu - relaunch with cap=0x%lx",
                   need, g_cap, (need + (need >> 3) + 0xfffff) & ~0xfffffu);
            return 0;
        }
    }

    for (i = 0; i < n; i++) {
        tab[i] = g_tab[i];
        /* Tag the hooked thread's stack, and record its reservation base. */
        if (g_tab[i].base <= lo && lo < g_tab[i].base + g_tab[i].size) {
            tab[i].klass = SNAPR_HOOK_STACK;
            h->stack_base = g_tab[i].alloc_base;
        }
        if (tab[i].rflags & SNAPRF_HAS_BYTES) {
            if (off + tab[i].size > g_cap)
                return 0;
            tab[i].blob_off = off;
            memcpy(buf + off, (const void *)tab[i].base, tab[i].size);
            off += tab[i].size;
        }
    }
    h->total_size = off;
    return off;
}

static void write_file(const char *name, const void *p, DWORD len)
{
    char path[MAX_PATH];
    HANDLE h;
    DWORD w;

    wsprintfA(path, "%s\\%s", g_out[0] ? g_out : ".", name);
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        caplog("write_file: cannot create %s", path);
        return;
    }
    WriteFile(h, p, len, &w, NULL);
    CloseHandle(h);
    if (w != len)
        caplog("write_file: short write on %s (%lu of %lu)", path, w, len);
}

/* ------------------------------------------------------- call-site patching */

/* Point one `call rel32` at `dest`, returning the displacement replaced.
 * Refuses anything that is not an `e8` - the static site scan reads x86
 * linearly and could in principle land inside another instruction. */
static int patch_call(DWORD site_rva, DWORD dest, DWORD *saved)
{
    BYTE *site = g_image + site_rva;
    DWORD old;

    if (site[0] != 0xe8) {
        caplog("site %08lx is not a call rel32 (opcode %02x)", site_rva, site[0]);
        return 0;
    }
    if (!VirtualProtect(site, 8, PAGE_EXECUTE_READWRITE, &old))
        return 0;
    if (saved)
        *saved = *(DWORD *)(site + 1);
    *(DWORD *)(site + 1) = dest - (DWORD)(site + 5);
    VirtualProtect(site, 8, old, &old);
    return 1;
}

static void probe_report(void)
{
    char path[MAX_PATH], line[256];
    HANDLE h;
    DWORD i, w;

    wsprintfA(path, "%s\\hits.txt", g_out[0] ? g_out : ".");
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    wsprintfA(line, "# tick=%lu probes=%lu sites=%lu\r\n", GetTickCount(),
              g_nprobes, g_npsites);
    WriteFile(h, line, lstrlenA(line), &w, NULL);
    for (i = 0; i < g_nprobes; i++) {
        wsprintfA(line, "%08lx %lu\r\n", g_prva[i], g_pcount[i]);
        WriteFile(h, line, lstrlenA(line), &w, NULL);
    }
    CloseHandle(h);
}

static DWORD WINAPI probe_thread(LPVOID unused)
{
    for (;;) {
        Sleep(1500);
        probe_report();
    }
}

__declspec(naked) void probe_common(void)
{
    /* mov/lea only: EFLAGS is untouched, and eax/ecx are restored, so the
     * candidate sees exactly the state its caller set up. */
    __asm {
        mov  g_probe_sav, eax
        mov  g_probe_sav2, ecx
        mov  eax, g_probe_idx
        mov  ecx, DWORD PTR g_pcount[eax*4]
        lea  ecx, [ecx+1]
        mov  DWORD PTR g_pcount[eax*4], ecx
        mov  eax, DWORD PTR g_ptarget[eax*4]
        mov  g_probe_jmp, eax
        mov  eax, g_probe_sav
        mov  ecx, g_probe_sav2
        jmp  DWORD PTR [g_probe_jmp]
    }
}

static void arm_probes(void)
{
    DWORD i, armed = 0;

    g_stubs = (BYTE *)VirtualAlloc(NULL, 16 * MAX_PROBES, MEM_RESERVE | MEM_COMMIT,
                                   PAGE_EXECUTE_READWRITE);
    if (!g_stubs) {
        caplog("probe: stub page alloc failed");
        return;
    }
    for (i = 0; i < g_nprobes; i++) {
        BYTE *s = g_stubs + i * 16;
        g_ptarget[i] = (DWORD)(g_image + g_prva[i]);
        s[0] = 0xc7;
        s[1] = 0x05;
        *(DWORD *)(s + 2) = (DWORD)&g_probe_idx;
        *(DWORD *)(s + 6) = i;
        s[10] = 0xe9;
        *(DWORD *)(s + 11) = (DWORD)probe_common - (DWORD)(s + 15);
    }
    for (i = 0; i < g_npsites; i++)
        armed += (DWORD)patch_call(g_psite[i], (DWORD)(g_stubs + g_psite_owner[i] * 16),
                                   &g_psite_orig[i]);
    caplog("probe: %lu candidates, %lu of %lu sites armed", g_nprobes, armed,
           g_npsites);
    probe_report();
    CreateThread(NULL, 0x10000, probe_thread, NULL, 0, &i);
}

static void patch_sites(int on)
{
    DWORD i;
    for (i = 0; i < g_nsites; i++) {
        BYTE *site = g_image + g_sites[i];
        DWORD old;
        if (on) {
            patch_call(g_sites[i], (DWORD)hook_entry, &g_orig_disp[i]);
        } else if (site[0] == 0xe8) {
            if (!VirtualProtect(site, 8, PAGE_EXECUTE_READWRITE, &old))
                continue;
            *(DWORD *)(site + 1) = g_orig_disp[i];
            VirtualProtect(site, 8, old, &old);
        }
    }
    g_patched = (DWORD)(on != 0);
}

/* -------------------------------------------------------------- the stubs */

/* Called on the scratch stack, between the entry snapshot and the callee. */
static void on_entry(void)
{
    SnapRegs r;
    r.eax = g_e_eax;
    r.ecx = g_e_ecx;
    r.edx = g_e_edx;
    r.ebx = g_e_ebx;
    r.esp = g_e_esp;
    r.ebp = g_e_ebp;
    r.esi = g_e_esi;
    r.edi = g_e_edi;
    r.eflags = g_e_efl;
    r.eip = g_target;
    caplog("hook fired: esp=%08lx ret=%08lx ecx=%08lx", g_e_esp, g_retaddr, g_e_ecx);
    patch_sites(0); /* the snapshot must record retail's bytes, not ours */
    g_froze = freeze_others();
    g_buf_len[0] = snap(0, SNAP_PHASE_ENTRY, &r);
    if (!g_buf_len[0])
        caplog("entry snapshot OVERFLOWED cap=%lu - raise `cap=`", g_cap);

    /* THE NOISE PROBE. Take the entry snapshot a SECOND time, back to back,
     * with nothing in between. The state has not changed, so every byte that
     * differs between the two is the OBSERVER'S OWN FOOTPRINT - what the act of
     * walking and copying 33 MB of address space costs. Measured here rather
     * than argued about later: two counters in a wine bookkeeping page at
     * 00120400 move by 0x106 across a snapshot, and without this the harness
     * self-test blamed them on the function under test. The replay masks
     * exactly these bytes and prints how many. */
    g_buf_len[2] = snap(2, SNAP_PHASE_ENTRY, &r);
}

/* Called on the scratch stack, after the callee returned. */
static void on_exit(void)
{
    SnapRegs r;
    r.eax = g_x_eax;
    r.ecx = g_x_ecx;
    r.edx = g_x_edx;
    r.ebx = g_x_ebx;
    r.esp = g_x_esp;
    r.ebp = g_x_ebp;
    r.esi = g_x_esi;
    r.edi = g_x_edi;
    r.eflags = g_x_efl;
    r.eip = g_retaddr;
    g_buf_len[1] = snap(1, SNAP_PHASE_EXIT, &r);
    if (!g_buf_len[1])
        caplog("exit snapshot OVERFLOWED cap=%lu - raise `cap=`", g_cap);
    thaw_others();

    /* Both snapshots are in memory; only now touch the filesystem, so no heap
     * or handle-table churn from file I/O lands between them. */
    write_file("entry.snap", g_buf[0], g_buf_len[0]);
    write_file("entry2.snap", g_buf[2], g_buf_len[2]);
    write_file("exit.snap", g_buf[1], g_buf_len[1]);
    caplog("captured %s rva=%08lx hit=%lu entry=%lu exit=%lu esp=%08lx ret=%08lx "
         "eax_out=%08lx esp_out=%08lx froze=%lu other thread(s) noiseprobe=%lu",
         g_name, g_target_rva, g_hit, g_buf_len[0], g_buf_len[1], g_e_esp,
         g_retaddr, g_x_eax, g_x_esp, g_froze, g_buf_len[2]);
    g_done = 1;
}

/* The dispatcher: entered INSTEAD of the target, with the callee's exact
 * entry state. Decides whether this call is the one we want. */
static int want_this_hit(void)
{
    if (g_done)
        return 0;
    if (g_hit++ != g_hit_want)
        return 0;
    return 1;
}

__declspec(naked) void hook_entry(void)
{
    __asm {
        mov  g_e_eax, eax
        mov  g_e_ecx, ecx
        mov  g_e_edx, edx
        mov  g_e_ebx, ebx
        mov  g_e_ebp, ebp
        mov  g_e_esi, esi
        mov  g_e_edi, edi
        pushfd
        pop  eax
        mov  g_e_efl, eax
        mov  eax, esp
        mov  g_e_esp, eax
        mov  eax, [esp]
        mov  g_retaddr, eax

        /* Everything below runs on our own stack, so nothing under the
         * callee's entry ESP is disturbed. */
        mov  g_savedesp, esp
        mov  esp, g_scratch_esp
        call want_this_hit
        test eax, eax
        jz   skip
        call on_entry
        mov  esp, g_savedesp
        /* Redirect the callee's return to us. */
        mov  eax, g_hookret
        mov  [esp], eax
        jmp  go
    skip:
        mov  esp, g_savedesp
    go:
        mov  eax, g_e_efl
        push eax
        popfd
        mov  eax, g_e_eax
        mov  ecx, g_e_ecx
        mov  edx, g_e_edx
        mov  ebx, g_e_ebx
        mov  ebp, g_e_ebp
        mov  esi, g_e_esi
        mov  edi, g_e_edi
        jmp  DWORD PTR [g_target]
    }
}

__declspec(naked) void hook_return(void)
{
    __asm {
        mov  g_x_eax, eax
        mov  g_x_ecx, ecx
        mov  g_x_edx, edx
        mov  g_x_ebx, ebx
        mov  g_x_ebp, ebp
        mov  g_x_esi, esi
        mov  g_x_edi, edi
        pushfd
        pop  eax
        mov  g_x_efl, eax
        mov  eax, esp
        mov  g_x_esp, eax

        mov  g_savedesp, esp
        mov  esp, g_scratch_esp
        call on_exit
        mov  esp, g_savedesp

        mov  eax, g_x_efl
        push eax
        popfd
        mov  eax, g_x_eax
        mov  ecx, g_x_ecx
        mov  edx, g_x_edx
        mov  ebx, g_x_ebx
        mov  ebp, g_x_ebp
        mov  esi, g_x_esi
        mov  edi, g_x_edi
        jmp  DWORD PTR [g_retaddr]
    }
}

/* -------------------------------------------------------------- attachment */

static void startup(HINSTANCE self)
{
    g_self = (BYTE *)self;
    g_image = (BYTE *)GetModuleHandleA(NULL);
    read_config();

    if (g_mode_census) {
        census();
        return;
    }
    if (g_mode_probe) {
        arm_probes();
        return;
    }
    if (!g_target_rva || !g_nsites) {
        caplog("no target/site in capture.cfg - nothing to do");
        return;
    }

    g_target = (DWORD)(g_image + g_target_rva);
    g_hookret = (DWORD)hook_return;

    g_buf[0] = (BYTE *)VirtualAlloc(NULL, g_cap, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_READWRITE);
    g_buf[1] = (BYTE *)VirtualAlloc(NULL, g_cap, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_READWRITE);
    g_buf[2] = (BYTE *)VirtualAlloc(NULL, g_cap, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_READWRITE);
    g_stackbuf = (BYTE *)VirtualAlloc(NULL, 0x100000, MEM_RESERVE | MEM_COMMIT,
                                      PAGE_READWRITE);
    if (!g_buf[0] || !g_buf[1] || !g_buf[2] || !g_stackbuf) {
        caplog("VirtualAlloc failed (cap=%lu) - no capture", g_cap);
        return;
    }
    g_scratch_esp = (DWORD)(g_stackbuf + 0x100000 - 0x100);

    patch_sites(1);
    caplog("armed: image=%08lx target=%08lx (%s) sites=%lu hit=%lu full=%lu",
         (DWORD)g_image, g_target, g_name, g_nsites, g_hit_want, g_full);
}

BOOL WINAPI DllMain(HINSTANCE self, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        char me[MAX_PATH];
        DisableThreadLibraryCalls(self);
        /* Pin ourselves: the game FreeLibrary's this DLL as soon as
         * GetProcAddress("SFManager") fails, which would unmap the stubs the
         * patched call sites point at. */
        GetModuleFileNameA(self, me, sizeof(me));
        LoadLibraryA(me);
        startup(self);
    }
    return TRUE;
}

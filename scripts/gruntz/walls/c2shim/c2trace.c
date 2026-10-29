/* Stand-in mspdb50.dll for a private copy of cl 5.0's c2.exe.

   Every export jumps to the same entry of the real DLL, loaded as
   mspdb50r.dll. When GRUNTZ_C2_TRACE names a log file, DllMain also patches
   logging hooks into the /Ob inline pass (c2.exe has no relocations, so its
   code sits at fixed addresses); each hook re-executes the instructions it
   displaced and resumes, so the traced compile emits the same object.
   Without the variable the DLL only forwards. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* c2.exe function record fields (FUN_0042491e / FUN_004249cb). */
#define RECORD_NAME 0x18
#define RECORD_HANDLE 0x24
#define RECORD_CB 0x64
#define RECORD_FLAGS 0x6a

/* FUN_004249cb frame, as esp offsets inside its body. */
#define FRAME_NODE 0x14
#define FRAME_OWNER 0x18
#define FRAME_REMAINING 0x2c
#define FRAME_DEPTH 0x30
#define FRAME_BUDGET 0x48

#define RUNNING_SIZE (*(const long *)0x490e70)

static FILE *g_log;
static const unsigned char *g_root;

static unsigned long g_resumeRoot = 0x424933;
static unsigned long g_resumeSite = 0x424ef5;
static unsigned long g_resumeArgsMatch = 0x424f1d;
static unsigned long g_resumeDecline = 0x424a67;
static unsigned long g_resumeAdmit = 0x424f5e;
static unsigned long g_resumeCharge = 0x424f7f;
static unsigned long g_resumeBody = 0x424c10;
static unsigned long g_walk = 0x4249cb;

static const char *recordName(const unsigned char *record)
{
    const char *name = *(const char *const *)(record + RECORD_NAME);
    return IsBadStringPtr(name, 4096) ? "?" : name;
}

static void logSymbol(const unsigned char *record)
{
    fprintf(g_log, "sym %08lx %s\n", (unsigned long)record, recordName(record));
}

static void __cdecl logRoot(const unsigned char *const *function)
{
    g_root = *function;
    logSymbol(g_root);
    fprintf(g_log, "main %08lx cb=%d handle=%lx flags=%lx\n", (unsigned long)g_root,
            *(const short *)(g_root + RECORD_CB),
            *(const unsigned long *)(g_root + RECORD_HANDLE),
            *(const unsigned long *)(g_root + RECORD_FLAGS));
}

static void __cdecl logSite(const unsigned char *callee, const unsigned char *tuple,
                            const unsigned char *frame)
{
    const unsigned char *owner = **(const unsigned char *const *const *)(frame + FRAME_OWNER);
    const unsigned char *node = *(const unsigned char *const *)(frame + FRAME_NODE);
    logSymbol(callee);
    logSymbol(owner);
    fprintf(g_log, "site root=%08lx owner=%08lx callee=%08lx tuple=%08lx depth=%ld "
            "allow=%d budget=%ld remain=%ld cb=%d running=%ld handle=%lx flags=%lx\n",
            (unsigned long)g_root, (unsigned long)owner, (unsigned long)callee,
            (unsigned long)tuple, *(const long *)(frame + FRAME_DEPTH), node[8],
            *(const long *)(frame + FRAME_BUDGET), *(const long *)(frame + FRAME_REMAINING),
            *(const short *)(callee + RECORD_CB), RUNNING_SIZE,
            *(const unsigned long *)(callee + RECORD_HANDLE),
            *(const unsigned long *)(callee + RECORD_FLAGS));
}

static void __cdecl logArgs(long actual, long formal)
{
    fprintf(g_log, "args actual=%ld formal=%ld\n", actual, formal);
}

static void __cdecl logAdmit(void)
{
    fprintf(g_log, "admit running=%ld\n", RUNNING_SIZE);
}

static void __cdecl logCharge(void)
{
    fprintf(g_log, "charge running=%ld\n", RUNNING_SIZE);
}

static void __cdecl logBody(const unsigned char *const *function, long share, long depth)
{
    fprintf(g_log, "body callee=%08lx share=%ld depth=%ld\n",
            (unsigned long)*function, share, depth);
}

static __declspec(naked) void hookRoot(void)
{
    __asm {
        pushad
        pushfd
        push esi
        call logRoot
        add esp, 4
        popfd
        popad
        mov eax, [esi]
        movsx eax, word ptr [eax+0x64]
        jmp dword ptr [g_resumeRoot]
    }
}

static __declspec(naked) void hookSite(void)
{
    __asm {
        pushad
        pushfd
        lea eax, [esp+36]
        push eax
        push edi
        push ebx
        call logSite
        add esp, 12
        popfd
        popad
        mov edx, [ebx+0x3a]
        mov cl, [ebx+0x36]
        jmp dword ptr [g_resumeSite]
    }
}

static __declspec(naked) void hookArgs(void)
{
    __asm {
        pushad
        pushfd
        push edx
        push esi
        call logArgs
        add esp, 8
        popfd
        popad
        cmp esi, edx
        jne mismatch
        jmp dword ptr [g_resumeArgsMatch]
    mismatch:
        jmp dword ptr [g_resumeDecline]
    }
}

static __declspec(naked) void hookAdmit(void)
{
    __asm {
        pushad
        pushfd
        call logAdmit
        popfd
        popad
        mov al, [edi+0x12]
        test al, al
        jmp dword ptr [g_resumeAdmit]
    }
}

/* The running-size add every charged site passes (the budget charge of a
   cost over 40 precedes it). */
static __declspec(naked) void hookCharge(void)
{
    __asm {
        pushad
        pushfd
        call logCharge
        popfd
        popad
        mov ecx, dword ptr ds:[0x490e70]
        jmp dword ptr [g_resumeCharge]
    }
}

/* Entered in place of `call FUN_004249cb`: ecx is the callee's function,
   edx the nested depth, [esp] the nested budget share. */
static __declspec(naked) void hookBody(void)
{
    __asm {
        pushad
        pushfd
        push edx
        push dword ptr [esp+40]
        push ecx
        call logBody
        add esp, 12
        popfd
        popad
        push dword ptr [g_resumeBody]
        jmp dword ptr [g_walk]
    }
}

struct Hook {
    unsigned long at;
    const char *expect;
    int length;
    void (*target)(void);
};

static const struct Hook g_hooks[] = {
    { 0x42492d, "\x8b\x06\x0f\xbf\x40\x64", 6, hookRoot },
    { 0x424eef, "\x8b\x53\x3a\x8a\x4b\x36", 6, hookSite },
    { 0x424f15, "\x3b\xf2\x0f\x85\x4a\xfb\xff\xff", 8, hookArgs },
    { 0x424f59, "\x8a\x47\x12\x84\xc0", 5, hookAdmit },
    { 0x424f79, "\x8b\x0d\x70\x0e\x49\x00", 6, hookCharge },
    { 0x424c0b, "\xe8\xbb\xfd\xff\xff", 5, hookBody },
};

static int install(void)
{
    int i;
    for (i = 0; i < sizeof g_hooks / sizeof g_hooks[0]; i++)
        if (memcmp((const void *)g_hooks[i].at, g_hooks[i].expect, g_hooks[i].length))
            return 0;
    for (i = 0; i < sizeof g_hooks / sizeof g_hooks[0]; i++) {
        unsigned char *at = (unsigned char *)g_hooks[i].at;
        DWORD old;
        VirtualProtect(at, g_hooks[i].length, PAGE_EXECUTE_READWRITE, &old);
        at[0] = 0xe9;
        *(long *)(at + 1) = (long)g_hooks[i].target - (long)(at + 5);
        memset(at + 5, 0x90, g_hooks[i].length - 5);
        VirtualProtect(at, g_hooks[i].length, old, &old);
    }
    return 1;
}

/* mspdb50r.dll entry points, resolved at load; each export jumps through one. */
static const char *const g_forwardNames[] = {
    "StreamAppend",
    "StreamReplace",
    "ILModGetILVer",
    "ILModGetIL",
    "StreamQueryCb",
    "StreamRead",
    "ILModRelease",
    "ILStoreRelease",
    "StreamRelease",
    "PDBCommit",
    "SigForPbCb",
    "PDBClose",
    "ILStoreGetILMod",
    "PDBOpen",
    "PDBOpenStream",
    "ILStoreOpen",
};
static FARPROC g_forward[16];

#define FORWARD(index, name) \
    __declspec(naked) void forward##name(void) { __asm jmp dword ptr [g_forward + 4 * index] }
FORWARD(0, StreamAppend)
FORWARD(1, StreamReplace)
FORWARD(2, ILModGetILVer)
FORWARD(3, ILModGetIL)
FORWARD(4, StreamQueryCb)
FORWARD(5, StreamRead)
FORWARD(6, ILModRelease)
FORWARD(7, ILStoreRelease)
FORWARD(8, StreamRelease)
FORWARD(9, PDBCommit)
FORWARD(10, SigForPbCb)
FORWARD(11, PDBClose)
FORWARD(12, ILStoreGetILMod)
FORWARD(13, PDBOpen)
FORWARD(14, PDBOpenStream)
FORWARD(15, ILStoreOpen)

static int resolveForwards(void)
{
    HMODULE real = LoadLibrary("mspdb50r.dll");
    int i;
    if (!real)
        return 0;
    for (i = 0; i < sizeof g_forward / sizeof g_forward[0]; i++)
        if (!(g_forward[i] = GetProcAddress(real, g_forwardNames[i])))
            return 0;
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    char path[MAX_PATH];
    if (reason == DLL_PROCESS_ATTACH) {
        if (!resolveForwards())
            return FALSE;
        if (!GetEnvironmentVariable("GRUNTZ_C2_TRACE", path, sizeof path))
            return TRUE;
        g_log = fopen(path, "a");
        if (!g_log)
            return FALSE;
        if (!install()) {
            fprintf(g_log, "refused c2.exe differs at a hook site\n");
            fclose(g_log);
            return FALSE;
        }
        fprintf(g_log, "begin\n");
    } else if (reason == DLL_PROCESS_DETACH && g_log) {
        fprintf(g_log, "end\n");
        fclose(g_log);
    }
    return TRUE;
}

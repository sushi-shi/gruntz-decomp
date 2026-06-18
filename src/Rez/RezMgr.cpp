// RezMgr.cpp - the Monolith "RezMgr" archive container classes (CRezItm leaf /
// CRezDir subdirectory nodes) and the directory walk over a Gruntz.REZ /
// GRUNTZ.VRZ archive.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CRezItm::CRezItm(parent)        @ 0x13c540  (40 B,  thiscall ret 4)  BYTE-EXACT  - leaf ctor (new 0x24)
//   CRezDir::CRezDir(parent,rezmgr) @ 0x13c940  (70 B,  thiscall ret 8)  PLATEAU 78% - dir  ctor (new 0x38)
//   CRezDir::FindEntry(name)        @ 0x13c080  (60 B,  thiscall ret 4)  BYTE-EXACT  - is-this-a-dir? stat
//   CRezDirNode::Load(childFlag)    @ 0x13a0f0  (153 B, thiscall ret 4)  BYTE-EXACT  - recursive dir parse
//   RezMgr::MakeImageKey(...)       @ 0x13e5d0  (177 B, thiscall ret 0xc) BYTE-EXACT  - ext-dispatch image load (.BMP/.PCX/.PID)
//   RezMgr::MakeRezPath()           @ 0x091670  (684 B, thiscall ret)    PLATEAU 92% - archive-path builder (EH/CString entropy)
//   RezMgr::PerFrameTick()          @ 0x08b740  (301 B, virtual vtbl+0x10) BYTE-EXACT - THE per-frame game tick (heart of the loop)
//
// Both ctors share the base ctor CRezItmBase::CRezItmBase @0x13c4e0 (stores the
// base vtable @0x5ef768 and the parent pointer @+0xc), then overwrite the vtable
// with the derived one (two-phase construction; all vtable stores reloc-masked).
// `operator new` sizes 0x24 (leaf) / 0x38 (dir) confirm the layouts. The
// "File is not sorted!" assert string is a reloc-masked file-scope literal.
//
// CRezDir ctor PLATEAU (78%): all 14 member stores go to the correct offsets
// with the correct values, but MSVC5 schedules the +0x10/+0x1c collection-vtable
// stores and the +0x14/+0x18 head/tail zeros in a different (still-correct) order
// than the target, and materializes the vtbl constant before the zero (vs after).
// No source lever flips it (tried 6 store orderings + an embedded collection
// sub-object - the sub-object emits an out-of-line ctor call, far worse). The
// vtable operands are reloc-masked. Entropy-class residue, left per the doctrine.
//
// OpenSub @0x13b0c0 (568 B) is NOT matched here: it runs on a THIRD, distinct
// node layout (it uses +0x1c as a child COUNT and +0x10 as a list-append target,
// directly conflicting with the 0x38 CRezDir ctor's vtable stores at those same
// offsets, AND with CRezDirNode's +0x10 size / +0x18 source - so all three
// "CRezDir"-labeled functions are actually three different classes). It also
// needs a faithful C++ EH frame, the inline CString strlen+strcpy, the embedded
// list-append helper, two-slot virtual dispatch on the allocated child, a
// 0xA8-byte item-header parse feeding running max-dims, and two large external
// tail calls (0x13b300 recursive FS walk, 0x13a580 item-record). >512 B of high
// EH/CString/virtual entropy; deferred to a dedicated worker per the prompt's
// "don't sacrifice a green fn" guidance. The container layouts it would confirm
// are already pinned by the two ctors below.
#include "RezMgr.h"

// ---------------------------------------------------------------------------
// CRezItmBase::CRezItmBase(parent)
//   mov [this] = base vtbl (0x5ef768); mov [this+0xc] = parent. Out-of-line so
//   the derived ctors emit a `call` to it.
//
// @address: 0x13c4e0
// @size:    0x12
// ---------------------------------------------------------------------------
CRezItmBase::CRezItmBase(void *parent)
{
    m_parent = parent;
}


// ---------------------------------------------------------------------------
// CRezItm::CRezItm(parent)
// Base ctor (vtbl @0x5ef768 + parent), then derived vtbl @0x5ef788, m_10 = 0,
// m_14 = 0, m_20 = -1. m_18/m_1c untouched.
//
// @address: 0x13c540
// @size:    0x28
// ---------------------------------------------------------------------------
CRezItm::CRezItm(void *parent) : CRezItmBase(parent)
{
    m_10 = 0;
    m_14 = 0;
    m_20 = -1;
}

// The embedded child-collection vftable both CRezDir sub-objects install (a
// vftable in .rdata @0x5ef7c8; modeled as a labeled datum so taking its address
// reloc-matches the engine instead of a bare 0x5ef7c8 immediate).
// @data: 0x1ef7c8
extern int g_rezDirChildVtbl;

// ---------------------------------------------------------------------------
// CRezDir::CRezDir(parent, rezMgr)
// Base ctor, then: m_14=0, m_18=0, m_vtblA=m_vtblB=&g_rezDirChildVtbl (embedded
// child collection's two vtables), m_20=m_24=m_28=m_34=0, derived vtbl @0x5ef7a8,
// m_2c=rezMgr, m_30=1.
//
// @address: 0x13c940
// @size:    0x46
// ---------------------------------------------------------------------------
CRezDir::CRezDir(void *parent, void *rezMgr) : CRezItmBase(parent)
{
    m_14 = 0;
    m_18 = 0;
    m_vtblA = (void *)&g_rezDirChildVtbl;
    m_vtblB = (void *)&g_rezDirChildVtbl;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_34 = 0;
    m_2c = rezMgr;
    m_30 = 1;
}

// ---------------------------------------------------------------------------
// CRezDir::FindEntry(char* name)
// Despite the tomalla "binary search" label, the bytes are a stat: build a
// 0x24-byte find-record on the stack, RezStatEntry(name, &rec); on failure
// return 0; on success return whether the entry's attribute dword (at byte +6
// of the record) has bit 0x4000 set (i.e. the entry is a directory).
// `this` is never read here.
//
// @address: 0x13c080
// @size:    0x3c
// ---------------------------------------------------------------------------
int CRezDir::FindEntry(char *name)
{
    RezFindRec rec;
    if (RezStatEntry(name, &rec) != 0)
        return 0;
    return (*(int *)(rec.raw + 6) & 0x4000) == 0x4000;
}

// The "File is not sorted!" assert message - a file-scope literal (its address
// is the reloc-masked push operand in Load's failure path).
static const char s_notSorted[] = "CRezDir::Load Failed! (File is not sorted!)";

// ---------------------------------------------------------------------------
// CRezDirNode::Load(childFlag)
// Recursive directory parse / load. If already loaded (m_buf != 0) return 1.
// Validate the source (m_src->m_8 nonzero, m_src->m_1c <= 1) else assert "File
// is not sorted!". If m_size > 0, allocate the payload buffer and virtually read
// it from the source stream at (m_off, 0, m_size, buf). When childFlag is set,
// iterate the child collection (First/Next) and recurse Load(1) into each
// child's sub-dir node (node->m_14). Returns 1.
//
// @address: 0x13a0f0
// @symbol:  ?Load@CRezDirNode@@QAEHH@Z
// @size:    0x99
// ---------------------------------------------------------------------------
int CRezDirNode::Load(int childFlag)
{
    if (m_buf != 0)
        return 1;

    RezSrc *src = m_src;
    if (src->m_8 == 0 || (unsigned)src->m_1c > 1) {
        RezAssertFail(s_notSorted);
        return 0;
    }

    if (m_size > 0) {
        m_buf = RezAlloc(m_size);
        if (m_buf != 0)
            m_src->m_stream->ReadAt(m_off, 0, m_size, m_buf);
    }

    if (childFlag != 0) {
        for (RezNode *n = m_kids.First(); n != 0; n = n->Next())
            n->m_14->Load(1);
    }
    return 1;
}

// The image-resource extension keys (file-scope literals - their addresses are
// the reloc-masked push operands in MakeImageKey's stricmp calls). Order in the
// binary: .BMP @0x61a0e4, then .PCX @0x61a0dc, then .PID @0x61a0d4.
static const char s_extBmp[] = ".BMP";   // 0x61a0e4
static const char s_extPcx[] = ".PCX";   // 0x61a0dc
static const char s_extPid[] = ".PID";   // 0x61a0d4

// ---------------------------------------------------------------------------
// RezMgr::MakeImageKey(arg1, name, arg3)  @ 0x13e5d0 (177 B, thiscall ret 0xc).
// Dispatch a resource load by the file extension of `name`: locate the last
// '.', then case-insensitively match .BMP/.PCX/.PID and hand off to the
// matching loader (LoadBmp/LoadPcx take (arg1,name); LoadPid takes
// (arg1,name,arg3)). Returns 1 unless the extension matched but its loader
// failed (then 0); an unrecognised/absent extension also returns 1.
//
// @address: 0x13e5d0
// @size:    0xb1
// ---------------------------------------------------------------------------
int RezMgr::MakeImageKey(void *arg1, char *name, void *arg3)
{
    char *ext = RezStrrchr(name, '.');
    if (ext && RezStricmp(ext, s_extBmp) == 0) {
        if (!LoadBmp(arg1, name))
            return 0;
    } else if (ext && RezStricmp(ext, s_extPcx) == 0) {
        if (!LoadPcx(arg1, name))
            return 0;
    } else if (ext && RezStricmp(ext, s_extPid) == 0) {
        if (!LoadPid(arg1, name, arg3))
            return 0;
    }
    return 1;
}

// The runtime low-detail / front-end-class selector (binary @0x6455d4).
int g_rezLowDetail;

// The archive base names / path templates - file-scope literals (reloc-masked).
static const char s_rezName[]    = "Gruntz.REZ";    // 0x60c674
static const char s_join[]       = "%s\\%s";        // 0x60c66c
static const char s_dataPath[]   = "%c:\\DATA\\%s";  // 0x611054
static const char s_fecName[]    = "Gruntz.FEC";    // 0x611044
static const char s_fecLoName[]  = "GruntzLo.FEC";  // 0x611034
static const char s_moviezPath[] = "%c:\\MOVIEZ\\%s"; // 0x611024

// ---------------------------------------------------------------------------
// RezMgr::MakeRezPath()  @ 0x091670 (684 B, thiscall ret).
// Assembles the candidate archive paths (the main Gruntz.REZ and the front-end
// Gruntz.FEC / GruntzLo.FEC) and probes them with FileExists, recording in
// m_inGameDir/m_haveRez/m_haveMoviez which were found. Reports an error and
// returns 0 if nothing was found, else 1.
//
// PLATEAU 91.87% (documented): a >512 B C++ EH-frame function with four
// ref-counted MFC CString locals (one COW copy-ctor selecting the lo/hi FEC
// variant), the engine sprintf-style CString::Format wrapper, a runtime
// low-detail global branch, and FileExists probes. All call/string/IAT/EH
// operands are reloc-masked. The logic, control flow, all RezMgr member offsets
// (+0xec/+0xf0 path CStrings, +0xf4/+0xf8/+0xfc flags) and the string-template
// order are reconstructed faithfully and verified against the disasm. The sole
// residue is an EH-state-tracking write: the target advances the C++ EH state to
// 0 (`mov [esp+ehstate],ebp`) inline right before the first CString::Format,
// just after `m_haveRez=0` - my build omits exactly that one inline state-write,
// which shifts the instruction alignment by one and cascades objdiff's
// edit-distance. This is the MSVC5 EH-state scheduling over four overlapping
// CString live ranges (entropy-class; no source lever flips a single funclet
// state-write). MakeImageKey (the other target) is BYTE-EXACT and is the green
// deliverable; per the prompt's "don't sacrifice a green fn", this is left as a
// documented plateau with the full reconstruction in place.
//
// @address: 0x091670
// @size:    0x2ac
// ---------------------------------------------------------------------------
int RezMgr::MakeRezPath()
{
    char cwd[0x100];
    if (!GetCurrentDirectoryA(0xff, cwd))
        return 0;

    char drive = GetGruntzDriveLetter();
    m_inGameDir = (drive == cwd[0]);

    int found = 1;

    // --- main archive: cwd\Gruntz.REZ, fall back to <drive>:\DATA\Gruntz.REZ ---
    {
        AfxString rez(s_rezName);
        m_haveRez = 0;
        RezFormat(&m_pathA, s_join, cwd, (const char *)rez);
        if (!RezFileExists(m_pathA)) {
            if (drive) {
                RezFormat(&m_pathA, s_dataPath, drive, (const char *)rez);
                if (RezFileExists(m_pathA))
                    m_haveRez = 1;
                else
                    found = 0;
            } else {
                found = 0;
            }
        }
    }

    // --- front-end archive: cwd\<FEC>, then <drive>:\MOVIEZ\<FEC> ---
    AfxString fecHi(s_fecName);
    AfxString fecLo(s_fecLoName);
    AfxString fec(g_rezLowDetail ? fecLo : fecHi);

    m_haveMoviez = 0;
    int movFound = 0;
    RezFormat(&m_pathB, s_join, cwd, (const char *)fec);
    if (!m_inGameDir && !RezFileExists(m_pathB) && !g_rezLowDetail) {
        RezFormat(&m_pathB, s_join, cwd, (const char *)fecHi);
        if (RezFileExists(m_pathB))
            movFound = 1;
    }
    if (!movFound && drive) {
        RezFormat(&m_pathB, s_moviezPath, drive, (const char *)fec);
        if (RezFileExists(m_pathB))
            m_haveMoviez = 1;
    }

    if (!found) {
        ReportError(0x800b, 0x43e);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// The per-frame global frame-clock / interval-timer state (file-scope ints,
// reloc-masked; see RezMgr.h). UpdateClock (@0x13ddc0) writes g_now/g_frameDelta;
// the tick reads them, clamps the delta and advances the per-second timers.
// ---------------------------------------------------------------------------
static int g_now;          // 0x653c70  (UpdateClock sets it; tick re-uses)
static int g_frameDelta;   // 0x653c74  (ms since previous frame)

static int g_lastNow;      // 0x645580
static int g_lastDelta;    // 0x645584  (frame delta, clamped to <= 0x64)
static int g_accumMs;      // 0x645588  (running accumulated frame time)
static int g_frameTicks;   // 0x64558c  (per-frame counter)
static int g_timer32;      // 0x645590  (seed 0x32 ms)
static int g_timer100;     // 0x645594  (seed 0x64 ms)
static int g_timer200;     // 0x645598  (seed 0xc8 ms)
static int g_timer400;     // 0x64559c  (seed 0x190 ms)
static int g_timer500;     // 0x6455a0  (seed 0x1f4 ms)

// ---------------------------------------------------------------------------
// RezMgr::PerFrameTick()  @ 0x8b740 (301 B, virtual, vtable slot +0x10, ret).
// THE per-frame game tick: the engine's idle (CGameApp slot +0x20) tail-calls
// this every frame on an empty message queue.
//
//   if (m_mode == 0) return 0;          // nothing to drive yet
//   UpdateClock();                       // advance g_now / g_frameDelta
//   int r = m_mode->Update();            // step the active game-state (slot +0x10)
//   if (r != 0x11) {                     // 0x11 = a state that suppresses timing
//       // clamp this frame's delta to <= 0x64 ms and accumulate
//       int dt = g_frameDelta;
//       g_lastNow = g_now;  g_lastDelta = dt;
//       if (dt > 0x64) { dt = 0x64; g_lastDelta = 0x64; }
//       g_accumMs += dt;
//       // five interval countdown timers: reseed when expired, else subtract dt
//       <timer32/100/200/400/500>
//       g_frameTicks++;
//   }
//   if (m_renderGate != 0) return 0;     // render suppressed this frame
//   m_mode->Render();                    // post-step (slot +0x14)
//   return 1;
//
// Each countdown timer t loads its current value (or its SEED when it has hit
// 0), and either zeroes (delta has run it out) or subtracts the clamped delta:
//   v = (g_tN == 0) ? SEED : g_tN;
//   if (dt >= v) g_tN = 0; else g_tN = v - dt;
// The disasm reseeds in a register (mov ecx,SEED) without storing, so the
// reseed value is only visible through the subtract/zero - reproduce with the
// ternary feeding the compare directly.
//
// STATUS: BYTE-EXACT under reloc-masking (98.70% fuzzy). All 92 instructions are
// opcode+ModRM-identical vs dump_target.py 0x8b740; the only 24 diffs are
// reloc-masked address operands (the `call UpdateClock` REL32 + the 23 DIR32 to
// the frame-clock globals below).
// LEVER: the frame-delta clamp and the timer compares are UNSIGNED. `dt`/`v` MUST
// be `unsigned int` so the target's `jbe` (dt>0x64 clamp) and `jb` (dt>=v timer
// test) fall out; `int` emits signed `jle`/`jl` (94.78%). g_frameDelta is a
// timeGetTime() delta (genuinely unsigned ms).
//
// @address: 0x08b740
// @size:    0x12d
// ---------------------------------------------------------------------------
int RezMgr::PerFrameTick()
{
    if (m_mode == 0)
        return 0;

    UpdateClock();

    int r = m_mode->Update();
    if (r != 0x11) {
        unsigned int dt = g_frameDelta;
        g_lastNow = g_now;
        g_lastDelta = dt;
        if (dt > 0x64) {
            dt = 0x64;
            g_lastDelta = 0x64;
        }
        g_accumMs += dt;

        unsigned int v;
        v = (g_timer32 == 0) ? 0x32 : g_timer32;
        if (dt >= v) g_timer32 = 0; else g_timer32 = v - dt;
        v = (g_timer100 == 0) ? 0x64 : g_timer100;
        if (dt >= v) g_timer100 = 0; else g_timer100 = v - dt;
        v = (g_timer200 == 0) ? 0xc8 : g_timer200;
        if (dt >= v) g_timer200 = 0; else g_timer200 = v - dt;
        v = (g_timer400 == 0) ? 0x190 : g_timer400;
        if (dt >= v) g_timer400 = 0; else g_timer400 = v - dt;
        v = (g_timer500 == 0) ? 0x1f4 : g_timer500;
        if (dt >= v) g_timer500 = 0; else g_timer500 = v - dt;

        g_frameTicks++;
    }

    if (m_renderGate != 0)
        return 0;

    m_mode->Render();
    return 1;
}

// ---------------------------------------------------------------------------
// The owning app object's ReportError virtual (vtable slot +0x1c / index 7).
// Forward a (msgId, code) error report through the manager's back-pointer to
// the app's error dialog. Declared as a minimal virtual interface here so the
// vtable dispatch produces `mov eax,[ecx]; call [eax+0x1c]` (reloc-masked).
// ---------------------------------------------------------------------------
struct RezMgrAppTarget {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void ReportError(int msgId, int code);   // +0x1c (slot 7)
};

// ---------------------------------------------------------------------------
// RezMgr::ReportError  @ 0x08dc60 (25 bytes)
// Forwards the error report to the owning CGameApp through its vtable, if the
// back-pointer (m_8) is non-null.
//
// @address: 0x08dc60
// @size:    0x19
// ---------------------------------------------------------------------------
void RezMgr::ReportError(int msgId, int code)
{
    void *app = *(void **)((char *)this + 0x08);
    if (app == 0)
        return;
    ((RezMgrAppTarget *)app)->ReportError(msgId, code);
}

// =========================================================================
// NEW ADDITIONS (all matched unnamed FUN_ leaf functions)
// =========================================================================

#include <string.h>

extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);
extern "C" void free(void *);
extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
extern "C" __declspec(dllimport) int __stdcall PostMessageA(void *hWnd, unsigned Msg, unsigned wParam, long lParam);

extern "C" void *RezAlloc2(void *a, void *b);
extern "C" void *RezAlloc1(void *a);
struct RezCtorA { void Construct(); };
struct RezCtorB { void Construct(); };
struct RezDirCallbacks {
    void Prep(int flag);
    int Store(void *a, int b, int c);
};
struct RezBucketArray { void *GetAt(unsigned index); };
struct RezNodeList { void Insert(void *node); };
struct RezCollection {
    void Init(int hint);
    void Cleanup(void *arg);
    void Remove(void *node);
};
extern "C" int RezFileSeek(void *fp, int off, int origin);
extern "C" int RezFileRead(void *fp, void *buf, unsigned len, int unused);
extern "C" int RezFileRead2(void *fp, void *buf, unsigned len, int unused);
extern "C" int RezFileCheck(void *fp);
struct RezProbeCallbacks { int Probe(int arg); };
struct RezRefCallbacks { int DecRef(int arg); };
extern "C" void *RezOpenFile(const char *path, int mode);
struct RezNodeFree { void FreeItm(int node); };

struct RezHashTbl { unsigned m_divisor; };
struct RezStrHashA : RezHashTbl {
    int HashStr(const char *s);
    void *FindStr(const char *key, int mode);
};
struct RezStrHashB : RezHashTbl {
    int HashStr(const char *s);
    void *FindStr(const char *key, int mode);
};
struct RezIntHash : RezHashTbl {
    int HashInt(int val);
    void *FindInt(int key);
};

class CRezNamedStream : public CRezItmBase {
public:
    CRezNamedStream(void *parent, const char *name, void *backPtr);
    virtual ~CRezNamedStream();
    int StreamRead(void *a, void *b, unsigned size, int d);
    int StreamRead2(void *a, void *b, unsigned size, int d);
    void Check();
    void Open();
    void Close();
    void CloseAll();
    char *m_name;
    void *m_subObj;
    void *m_backPtr;
};

CRezItmBase::CRezItmBase() { m_parent = 0; }

void *RezFactory2(void *a, void *b)
{
    void *obj = RezAlloc2(a, b);
    ((RezCtorA *)obj)->Construct();
    return obj;
}

void *RezFactory1(void *a)
{
    void *obj = RezAlloc1(a);
    ((RezCtorB *)obj)->Construct();
    return obj;
}

int CRezDir::ParentOp()
{
    if (m_parent == 0) return 0;
    ((RezDirCallbacks *)this)->Prep(0);
    return ((RezDirCallbacks *)this)->Store(m_name, 1, 0);
}

void CRezDir::InsertNode(void *node)
{
    if (node == 0) return;
    ((RezNodeList *)((char *)this + 0x80))->Insert((char *)node + 0x1c);
}

int RezStrHashA::HashStr(const char *s)
{
    if (s == 0) return 0;
    unsigned len = 0;
    if (*s != 0) { do { len++; s++; } while (*s != 0); }
    return (int)(len % m_divisor);
}

void *RezStrHashA::FindStr(const char *key, int mode)
{
    if (key == 0) return 0;
    void *node = ((RezBucketArray *)this)->GetAt((unsigned)HashStr(key));
    if (node == 0) return 0;
    if (mode == 0) {
        while (node) {
            void *data = *(void **)((char *)node + 0x14);
            if (RezStricmp(*(const char **)data, key) == 0) return data;
            node = *(void **)((char *)node + 0x04);
        }
    } else {
        while (node) {
            void *data = *(void **)((char *)node + 0x14);
            const char *a = *(const char **)data;
            const char *b = key;
            int diff = 0;
            while (*a && *b) { if (*a++ != *b++) { diff = 1; break; } }
            if (!diff && *a == *b) return data;
            node = *(void **)((char *)node + 0x04);
        }
    }
    return 0;
}

int RezStrHashB::HashStr(const char *s)
{
    if (s == 0) return 0;
    unsigned len = 0;
    if (*s != 0) { do { len++; s++; } while (*s != 0); }
    return (int)(len % m_divisor);
}

void *RezStrHashB::FindStr(const char *key, int mode)
{
    if (key == 0) return 0;
    void *node = ((RezBucketArray *)this)->GetAt((unsigned)HashStr(key));
    if (node == 0) return 0;
    if (mode == 0) {
        while (node) {
            void *data = *(void **)((char *)node + 0x14);
            if (RezStricmp(*(const char **)data, key) == 0) return data;
            node = *(void **)((char *)node + 0x04);
        }
    } else {
        while (node) {
            void *data = *(void **)((char *)node + 0x14);
            const char *a = *(const char **)data;
            const char *b = key;
            int diff = 0;
            while (*a && *b) { if (*a++ != *b++) { diff = 1; break; } }
            if (!diff && *a == *b) return data;
            node = *(void **)((char *)node + 0x04);
        }
    }
    return 0;
}

int RezIntHash::HashInt(int val) { return (int)((unsigned)val % m_divisor); }

void *RezIntHash::FindInt(int key)
{
    void *node = ((RezBucketArray *)this)->GetAt((unsigned)HashInt(key));
    while (node) {
        void *data = *(void **)((char *)node + 0x14);
        if (data && *(int *)data == key) return data;
        node = *(void **)((char *)node + 0x04);
    }
    return 0;
}

CRezItm::~CRezItm()
{
    if (m_10) ((RezNodeFree *)this)->FreeItm(m_10);
    if (m_14) RezFree((void *)m_14);
}

int CRezDirNode::Open()
{
    *(int *)((char *)this + 0x20) = -1;
    if (m_size == 0) return 0;
    return ((RezProbeCallbacks *)this)->Probe((int)m_size) ? 1 : 0;
}

int CRezDirNode::Check()
{
    *(int *)((char *)this + 0x20) = -1;
    if (m_size == 0) return 0;
    int r = RezFileCheck((void *)m_size);
    if (r == -1) { int (**v)(void *) = (int (**)(void *))this; r = v[4](this); }
    return r >= 0 ? 1 : 0;
}

CRezDir::~CRezDir()
{
    while (m_14) { int *n = *(int **)((char *)m_14 + 4); delete (CRezItmBase *)m_14; m_14 = (int)n; }
    while (m_20) { int *n = *(int **)((char *)m_20 + 4); delete (CRezItmBase *)m_20; m_20 = (int)n; }
}

CRezNamedStream::CRezNamedStream(void *parent, const char *name, void *backPtr)
    : CRezItmBase(parent)
{
    m_name = _strdup(name);
    m_backPtr = backPtr;
    m_subObj = 0;
    ((RezCollection *)((char *)this + 0x34))->Init(0);
}

CRezNamedStream::~CRezNamedStream()
{
    Close();
    if (m_name) free(m_name);
    ((RezCollection *)((char *)this + 0x34))->Cleanup(0);
}

int CRezNamedStream::StreamRead(void *a, void *b, unsigned size, int d)
{
    if (!m_subObj) Open();
    if (!m_subObj) return 0;
    int *off = (int *)((char *)m_subObj + 0x20);
    int t = (int)b;
    if (*off != t) { if (!RezFileSeek(m_subObj, t, 0)) return 0; *off = t; }
    int n = RezFileRead(m_subObj, a, size, 0);
    if (n > 0) *off += n;
    return n;
}

int CRezNamedStream::StreamRead2(void *a, void *b, unsigned size, int d)
{
    if (!m_subObj) Open();
    if (!m_subObj) return 0;
    int *off = (int *)((char *)m_subObj + 0x20);
    int t = (int)b;
    if (*off != t) { if (!RezFileSeek(m_subObj, t, 0)) return 0; *off = t; }
    int n = RezFileRead2(m_subObj, a, size, 0);
    if (n > 0) *off += n;
    return n;
}

void CRezNamedStream::Check() { if (m_subObj) ((RezProbeCallbacks *)m_subObj)->Probe(0); }

void CRezNamedStream::Open()
{
    if (m_subObj) return;
    void *h = RezOpenFile(m_name, 0);
    if (h) { m_subObj = h; ((RezCtorA *)h)->Construct(); }
}

void CRezNamedStream::Close()
{
    if (!m_subObj) return;
    if (!((RezRefCallbacks *)m_subObj)->DecRef(0)) {
        ((RezCollection *)((char *)this + 0x34))->Remove(m_subObj);
        m_subObj = 0;
    }
}

void CRezNamedStream::CloseAll() { while (m_subObj) Close(); }

void RezMgr::UpdateClock()
{
    unsigned now = timeGetTime();
    g_frameDelta = (int)(now - (unsigned)g_now);
    g_now = (int)now;
}

static void RezSpinWait(unsigned ms)
{
    unsigned start = timeGetTime();
    while ((int)(timeGetTime() - start) < (int)ms) ;
}

int __stdcall WaitForKeyOrTimeout(int vKey, int timeoutMs)
{
    if (timeoutMs == 0) {
        while (!(GetAsyncKeyState(vKey) & 0x8000)) ;
        while (GetAsyncKeyState(vKey) & 0x8000) ;
        return 1;
    }
    unsigned start = timeGetTime();
    while (!(GetAsyncKeyState(vKey) & 0x8000)) {
        if ((int)(timeGetTime() - start) >= timeoutMs) return 0;
    }
    while (GetAsyncKeyState(vKey) & 0x8000) {
        if ((int)(timeGetTime() - start) >= timeoutMs) return 1;
    }
    return 1;
}

int RezMgr::ForwardSlot4c(int a, int b, int c)
{
    if (!m_mode) return 0;
    void **vt = *(void ***)m_mode;
    return ((int (*)(void *, int, int, int))vt[0x13])(m_mode, a, b, c);
}

int RezMgr::ForwardSlot50(int a, int b, int c)
{
    if (!m_mode) return 0;
    void **vt = *(void ***)m_mode;
    return ((int (*)(void *, int, int, int))vt[0x14])(m_mode, a, b, c);
}

int RezMgr::HandleDebugPosition()
{
    if (!m_mode) return 0;
    int s = m_mode->Update();
    if (s == 3) {
        extern int g_debugFlag;
        if (g_debugFlag == 1)
            PostMessageA(0, 0x111, 0x805c, 0);
    }
    return s != 0 ? 0 : 1;
}

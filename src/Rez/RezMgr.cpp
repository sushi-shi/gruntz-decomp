// RezMgr.cpp - the Monolith "RezMgr" archive container classes (CRezItm leaf /
// CRezDir subdirectory nodes) and the directory walk over a Gruntz.REZ /
// GRUNTZ.VRZ archive.
//
// Functions matched in this TU:
//   CRezItm::CRezItm(parent)        BYTE-EXACT  - leaf ctor (new 0x24)
//   CRezDir::CRezDir(parent,rezmgr) PLATEAU 78% - dir  ctor (new 0x38)
//   CRezDir::FindEntry(name)        BYTE-EXACT  - is-this-a-dir? stat
//   CRezDirNode::Load(childFlag)    BYTE-EXACT  - recursive dir parse
//   RezMgr::MakeImageKey(...)       BYTE-EXACT  - ext-dispatch image load (.BMP/.PCX/.PID)
//   RezMgr::MakeRezPath()           PLATEAU 92% - archive-path builder (EH/CString entropy)
//   RezMgr::PerFrameTick()          BYTE-EXACT  - THE per-frame game tick (heart of the loop, vtbl +0x10)
//
// Both ctors share the base ctor CRezItmBase::CRezItmBase (stores the
// base vtable and the parent pointer @+0xc), then overwrite the vtable
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
// OpenSub is NOT matched here: it runs on a THIRD, distinct
// node layout (it uses +0x1c as a child COUNT and +0x10 as a list-append target,
// directly conflicting with the 0x38 CRezDir ctor's vtable stores at those same
// offsets, AND with CRezDirNode's +0x10 size / +0x18 source - so all three
// "CRezDir"-labeled functions are actually three different classes). It also
// needs a faithful C++ EH frame, the inline CString strlen+strcpy, the embedded
// list-append helper, two-slot virtual dispatch on the allocated child, a
// 0xA8-byte item-header parse feeding running max-dims, and two large external
// tail calls (a recursive FS walk + an item-record reader). >512 B of high
// EH/CString/virtual entropy; deferred to a dedicated worker per the prompt's
// "don't sacrifice a green fn" guidance. The container layouts it would confirm
// are already pinned by the two ctors below.
#include <Rez/RezMgr.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CRezItmBase::CRezItmBase(parent)
//   mov [this] = base vtbl; mov [this+0xc] = parent. Out-of-line so
//   the derived ctors emit a `call` to it.
RVA(0x0013c4e0, 0x12)
CRezItmBase::CRezItmBase(void* parent) {
    m_parent = parent;
}

// ---------------------------------------------------------------------------
// CRezItm::CRezItm(parent)
// Base ctor (vtbl + parent), then derived vtbl, m_10 = 0,
// m_14 = 0, m_20 = -1. m_18/m_1c untouched.
RVA(0x0013c540, 0x28)
CRezItm::CRezItm(void* parent) : CRezItmBase(parent) {
    m_10 = 0;
    m_14 = 0;
    m_20 = -1;
}

// The embedded child-collection vftable both CRezDir sub-objects install (a
// vftable in .rdata; modeled as a labeled datum so taking its address
// reloc-matches the engine instead of a bare immediate).
DATA(0x001ef7c8)
extern i32 g_rezDirChildVtbl;

// ---------------------------------------------------------------------------
// CRezDir::CRezDir(parent, rezMgr)
// Base ctor, then: m_14=0, m_18=0, m_vtblA=m_vtblB=&g_rezDirChildVtbl (embedded
// child collection's two vtables), m_20=m_24=m_28=m_34=0, derived vtbl,
// m_2c=rezMgr, m_30=1.
RVA(0x0013c940, 0x46)
CRezDir::CRezDir(void* parent, void* rezMgr) : CRezItmBase(parent) {
    m_14 = 0;
    m_18 = 0;
    m_vtblA = (void*)&g_rezDirChildVtbl;
    m_vtblB = (void*)&g_rezDirChildVtbl;
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
RVA(0x0013c080, 0x3c)
i32 CRezDir::FindEntry(char* name) {
    RezFindRec rec;
    if (RezStatEntry(name, &rec) != 0) {
        return 0;
    }
    return (*(i32*)(rec.raw + 6) & 0x4000) == 0x4000;
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
SYMBOL(?Load@CRezDirNode@@QAEHH@Z)
RVA(0x0013a0f0, 0x99)
i32 CRezDirNode::Load(i32 childFlag) {
    if (m_buf != 0) {
        return 1;
    }

    RezSrc* src = m_src;
    if (src->m_8 == 0 || (u32)src->m_1c > 1) {
        RezAssertFail(s_notSorted);
        return 0;
    }

    if (m_size > 0) {
        m_buf = RezAlloc(m_size);
        if (m_buf != 0) {
            m_src->m_stream->ReadAt(m_off, 0, m_size, m_buf);
        }
    }

    if (childFlag != 0) {
        for (RezNode* n = m_kids.First(); n != 0; n = n->Next()) {
            n->m_14->Load(1);
        }
    }
    return 1;
}

// The image-resource extension keys (file-scope literals - their addresses are
// the reloc-masked push operands in MakeImageKey's stricmp calls). Order in the
// binary: .BMP, then .PCX, then .PID.
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extPid[] = ".PID";

// ---------------------------------------------------------------------------
// RezMgr::MakeImageKey(arg1, name, arg3)
// Dispatch a resource load by the file extension of `name`: locate the last
// '.', then case-insensitively match .BMP/.PCX/.PID and hand off to the
// matching loader (LoadBmp/LoadPcx take (arg1,name); LoadPid takes
// (arg1,name,arg3)). Returns 1 unless the extension matched but its loader
// failed (then 0); an unrecognised/absent extension also returns 1.
RVA(0x0013e5d0, 0xb1)
i32 RezMgr::MakeImageKey(void* arg1, char* name, void* arg3) {
    char* ext = RezStrrchr(name, '.');
    if (ext && RezStricmp(ext, s_extBmp) == 0) {
        if (!LoadBmp(arg1, name)) {
            return 0;
        }
    } else if (ext && RezStricmp(ext, s_extPcx) == 0) {
        if (!LoadPcx(arg1, name)) {
            return 0;
        }
    } else if (ext && RezStricmp(ext, s_extPid) == 0) {
        if (!LoadPid(arg1, name, arg3)) {
            return 0;
        }
    }
    return 1;
}

// The runtime low-detail / front-end-class selector.
i32 g_rezLowDetail;

// The archive base names / path templates - file-scope literals (reloc-masked).
static const char s_rezName[] = "Gruntz.REZ";
static const char s_join[] = "%s\\%s";
static const char s_dataPath[] = "%c:\\DATA\\%s";
static const char s_fecName[] = "Gruntz.FEC";
static const char s_fecLoName[] = "GruntzLo.FEC";
static const char s_moviezPath[] = "%c:\\MOVIEZ\\%s";

// ---------------------------------------------------------------------------
// RezMgr::MakeRezPath()
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
RVA(0x00091670, 0x2ac)
i32 RezMgr::MakeRezPath() {
    char cwd[0x100];
    if (!GetCurrentDirectoryA(0xff, cwd)) {
        return 0;
    }

    char drive = GetGruntzDriveLetter();
    m_inGameDir = (drive == cwd[0]);

    i32 found = 1;

    // --- main archive: cwd\Gruntz.REZ, fall back to <drive>:\DATA\Gruntz.REZ ---
    {
        CString rez(s_rezName);
        m_haveRez = 0;
        RezFormat(&m_pathA, s_join, cwd, (const char*)rez);
        if (!RezFileExists(m_pathA)) {
            if (drive) {
                RezFormat(&m_pathA, s_dataPath, drive, (const char*)rez);
                if (RezFileExists(m_pathA)) {
                    m_haveRez = 1;
                } else {
                    found = 0;
                }
            } else {
                found = 0;
            }
        }
    }

    // --- front-end archive: cwd\<FEC>, then <drive>:\MOVIEZ\<FEC> ---
    CString fecHi(s_fecName);
    CString fecLo(s_fecLoName);
    CString fec(g_rezLowDetail ? fecLo : fecHi);

    m_haveMoviez = 0;
    i32 movFound = 0;
    RezFormat(&m_pathB, s_join, cwd, (const char*)fec);
    if (!m_inGameDir && !RezFileExists(m_pathB) && !g_rezLowDetail) {
        RezFormat(&m_pathB, s_join, cwd, (const char*)fecHi);
        if (RezFileExists(m_pathB)) {
            movFound = 1;
        }
    }
    if (!movFound && drive) {
        RezFormat(&m_pathB, s_moviezPath, drive, (const char*)fec);
        if (RezFileExists(m_pathB)) {
            m_haveMoviez = 1;
        }
    }

    if (!found) {
        ReportError(0x800b, 0x43e);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// The per-frame global frame-clock / interval-timer state (file-scope ints,
// reloc-masked; see RezMgr.h). UpdateClock writes g_now/g_frameDelta;
// the tick reads them, clamps the delta and advances the per-second timers.
// ---------------------------------------------------------------------------
static i32 g_now;        // (UpdateClock sets it; tick re-uses)
static i32 g_frameDelta; // (ms since previous frame)

static i32 g_lastNow;
static i32 g_lastDelta;  // (frame delta, clamped to <= 0x64)
static i32 g_accumMs;    // (running accumulated frame time)
static i32 g_frameTicks; // (per-frame counter)
static i32 g_timer32;    // (seed 0x32 ms)
static i32 g_timer100;   // (seed 0x64 ms)
static i32 g_timer200;   // (seed 0xc8 ms)
static i32 g_timer400;   // (seed 0x190 ms)
static i32 g_timer500;   // (seed 0x1f4 ms)

// ---------------------------------------------------------------------------
// RezMgr::PerFrameTick()  (virtual, vtable slot +0x10).
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
// opcode+ModRM-identical vs dump_target.py; the only 24 diffs are
// reloc-masked address operands (the `call UpdateClock` REL32 + the 23 DIR32 to
// the frame-clock globals below).
// LEVER: the frame-delta clamp and the timer compares are UNSIGNED. `dt`/`v` MUST
// be `unsigned int` so the target's `jbe` (dt>0x64 clamp) and `jb` (dt>=v timer
// test) fall out; `int` emits signed `jle`/`jl` (94.78%). g_frameDelta is a
// timeGetTime() delta (genuinely unsigned ms).
RVA(0x0008b740, 0x12d)
i32 RezMgr::PerFrameTick() {
    if (m_mode == 0) {
        return 0;
    }

    UpdateClock();

    i32 r = m_mode->Update();
    if (r != 0x11) {
        u32 dt = g_frameDelta;
        g_lastNow = g_now;
        g_lastDelta = dt;
        if (dt > 0x64) {
            dt = 0x64;
            g_lastDelta = 0x64;
        }
        g_accumMs += dt;

        u32 v;
        v = (g_timer32 == 0) ? 0x32 : g_timer32;
        if (dt >= v) {
            g_timer32 = 0;
        } else {
            g_timer32 = v - dt;
        }
        v = (g_timer100 == 0) ? 0x64 : g_timer100;
        if (dt >= v) {
            g_timer100 = 0;
        } else {
            g_timer100 = v - dt;
        }
        v = (g_timer200 == 0) ? 0xc8 : g_timer200;
        if (dt >= v) {
            g_timer200 = 0;
        } else {
            g_timer200 = v - dt;
        }
        v = (g_timer400 == 0) ? 0x190 : g_timer400;
        if (dt >= v) {
            g_timer400 = 0;
        } else {
            g_timer400 = v - dt;
        }
        v = (g_timer500 == 0) ? 0x1f4 : g_timer500;
        if (dt >= v) {
            g_timer500 = 0;
        } else {
            g_timer500 = v - dt;
        }

        g_frameTicks++;
    }

    if (m_renderGate != 0) {
        return 0;
    }

    m_mode->Render();
    return 1;
}

// ---------------------------------------------------------------------------
// RezMgr::HandleDebugPosition()
// When the active mode's per-frame state step reports 3, look up the
// "DEBUG_POSITION" debug value (via the external CheckDbgVal helper, reached at
// the call site through the 0x2bb7 thunk); if it is set, fetch the
// owning window handle (this+4 -> owner, owner+4 -> hwnd) and post it a
// WM_COMMAND (0x111) with command id 0x805c. Returns nonzero iff the lookup
// reported a hit. The CheckDbgVal call's E8 rel32 is reloc-masked by objdiff.
RVA(0x0008e470, 0x50)
i32 RezMgr::HandleDebugPosition() {
    i32 r = 0;
    if (m_mode && m_mode->Update() == 3) {
        r = CheckDbgVal("DEBUG_POSITION", 0x402d0b, 1);
        if (r == 1) {
            HWND hwnd = m_4->m_hWnd;
            PostMessageA(hwnd, 0x111, 0x805c, 0);
        }
    }
    return r != 0;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: rez-trace
// @stub
RVA(0x0013b0c0, 0x238)
void CRezDir::Stub_13b0c0() {}

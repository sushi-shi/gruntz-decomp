// RezMgr.cpp - the Monolith "RezMgr" archive container classes (CRezItm leaf /
// CRezDir subdirectory nodes) and the directory walk over a Gruntz.REZ /
// GRUNTZ.VRZ archive.
//
// Both ctors share the base ctor CRezItmBase::CRezItmBase (stores the base vtable
// and the parent pointer @+0xc), then overwrite the vtable with the derived one
// (two-phase construction; all vtable stores reloc-masked). `operator new` sizes
// 0x24 (leaf) / 0x38 (dir) confirm the layouts. The "File is not sorted!" assert
// string is a reloc-masked file-scope literal.
//
// OpenSub is NOT matched here: it runs on a THIRD, distinct node layout (it uses
// +0x1c as a child COUNT and +0x10 as a list-append target, conflicting with both
// the 0x38 CRezDir ctor's vtable stores and CRezDirNode's +0x10 size / +0x18 source
// - so the three "CRezDir"-labeled functions are actually three different classes).
// The container layouts it would confirm are already pinned by the two ctors below.
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

// ---------------------------------------------------------------------------
// CRezItmBase::~CRezItmBase()
// The base destructor: restore the base vtbl (auto, since polymorphic) and clear
// the parent pointer. Out-of-line so the derived dtor emits a `call` to it.
RVA(0x0013c520, 0xe)
CRezItmBase::~CRezItmBase() {
    m_parent = 0;
}

// ---------------------------------------------------------------------------
// CRezItm::~CRezItm()
// Derived dtor: if a FILE* is open, Close() it; free the read buffer; chain to
// the base dtor. A destructible state -> the /GX EH frame (push -1 / handler).
RVA(0x0013c590, 0x66)
CRezItm::~CRezItm() {
    if (m_10 != 0) {
        Close();
    }
    if (m_14 != 0) {
        RezFree(m_14);
    }
}

// ---------------------------------------------------------------------------
// CRezItm::Read(off, base, count, buf)  (vtable slot 2)
// Seek to absolute position (off+base) if the cursor isn't already there,
// recovering through the owner's Retry() gate on a seek failure; then fread
// `count` bytes into buf, retrying through the same gate on a short read. On
// success advances the +0x20 cursor and returns the bytes read; 0 / cursor=-1
// on a zero count or a gate that gives up.
//
// 99.77% (reloc-masked plateau): EVERY code byte matches the disasm. The entry
// guard now spells `if(count <= 0)` (unsigned `count`, so `<= 0` == `== 0`) which
// lowers to retail's `test edi,edi; ja` - the equality `== 0` form gave `jne`, and
// wrapping the body in `if(count>0)` cascaded (99.3%->83.4%); the `<= 0`
// early-return flips the one opcode with no cascade (docs/patterns/
// unsigned-zero-guard-le-not-eq.md). Only residual: the four buffered-IO calls,
// which the delinker names bare `fseek`/`fread` (Ghidra FLIRT) vs our
// `_RezFSeek`/`_RezFRead` - identical `call rel32`, a delinker bare-name artifact.
RVA(0x0013c600, 0xbd)
i32 CRezItm::Read(i32 off, i32 base, u32 count, void* buf) {
    if (count <= 0) {
        return 0;
    }

    i32 pos = base + off;

    if (m_20 != pos) {
        while (RezFSeek(m_10, pos, 0) != 0) {
            if (((CRezItmOwner*)m_parent)->Retry() == 0) {
                m_20 = -1;
                return 0;
            }
        }
    }

    u32 got = RezFRead(buf, 1, count, m_10);
    while (got != count) {
        if (((CRezItmOwner*)m_parent)->Retry() == 0) {
            m_20 = -1;
            return 0;
        }
        got = RezFRead(buf, 1, count, m_10);
    }

    m_20 = got + pos;
    return got;
}

// ---------------------------------------------------------------------------
// CRezItm::Write(base, off, count, buf)
// The write counterpart of Read: invalidate the cursor (m_20 = -1), seek to the
// absolute position (base+off) recovering through the owner's Retry() gate on a
// seek failure, then fwrite `count` bytes from buf, retrying the write through
// the same gate on a short write. Returns 0 on a zero count or a gate that gives
// up; the write count otherwise. Unlike Read, the cursor is left invalid.
// ---------------------------------------------------------------------------
// @early-stop
// 99.69% (reloc-masked plateau) - every code byte matches retail. The count guard
// now spells `if(count <= 0)` (unsigned), lowering to retail's `test;jbe` (the
// `== 0` form gave `je`; see Read's note + docs/patterns/
// unsigned-zero-guard-le-not-eq.md). Only residual: the buffered-IO calls the
// delinker names bare `fseek`/`fread` (Ghidra FLIRT) vs our `_RezFSeek`/`_RezFWrite`
// - identical `call rel32`, a delinker bare-name artifact (not steerable from src).
RVA(0x0013c6c0, 0x97)
i32 CRezItm::Write(i32 base, i32 off, u32 count, void* buf) {
    m_20 = -1;
    if (count <= 0) {
        return 0;
    }

    i32 pos = off + base;

    while (RezFSeek(m_10, pos, 0) != 0) {
        if (((CRezItmOwner*)m_parent)->Retry() == 0) {
            return 0;
        }
    }

    u32 put = RezFWrite(buf, 1, count, m_10);
    while (put != count) {
        if (((CRezItmOwner*)m_parent)->Retry() == 0) {
            return 0;
        }
        put = RezFWrite(buf, 1, count, m_10);
    }
    return put;
}

// ---------------------------------------------------------------------------
// CRezItm::Close()  (vtable slot 5)
// fclose the FILE*, retrying through the owner's Retry() gate; then free the
// read buffer and reset the cursor. Returns 1 on success, 0 if there was no open
// FILE* or the gate gave up.
// @early-stop
// regalloc wall (zero-register-pinning): structure is byte-exact but retail pins
// this->esi / the loop-flag ok->edi, while my cl swaps them (this->edi, ok->esi)
// -- the prologue interleave `push esi; mov ecx,esi; push edi` vs mine
// `push esi; push edi; mov ecx,edi`, cascading the esi<->edi names through the
// whole body. `while(ok==0)`+`ok=0` init beats `do-while` (81.3% vs 56.5%); the
// swap is the documented MSVC5 callee-save coin-flip, not source-steerable.
RVA(0x0013c830, 0x63)
i32 CRezItm::Close() {
    if (m_10 == 0) {
        return 0;
    }

    i32 ok = 0;
    while (ok == 0) {
        if (RezFClose(m_10) == 0) {
            ok = 1;
        } else {
            ok = 0;
            if (((CRezItmOwner*)m_parent)->Retry() == 0) {
                return 0;
            }
        }
    }

    m_10 = 0;
    if (m_14 != 0) {
        RezFree(m_14);
    }
    m_14 = 0;
    m_20 = -1;
    return ok;
}

// ---------------------------------------------------------------------------
// CRezDir::CRezDir(parent, rezMgr)
// Base ctor, then the two embedded child-collection list members auto-construct
// (each stamps ??_7CRezDirList @0x1ef7c8 and zeroes head/tail), the derived vtbl
// is stamped, then m_28=0, m_34=0, m_2c=rezMgr, m_30=1.
// @early-stop
// vptr-schedule wall (ALL-VTABLES): real-polymorphic list members auto-stamp their
// vptr FIRST (vptr,head,tail) and the compiler zeroes m_28/m_34 after the derived
// vptr, vs retail's head,tail,vtbl store order + pre-vptr field zeroing. The two
// child collections + the CRezItmBase base are byte-faithful; converted per the
// ALL-VTABLES mandate (was the hand-rolled child-collection double-stamp).
RVA(0x0013c940, 0x46)
CRezDir::CRezDir(void* parent, void* rezMgr) : CRezItmBase(parent) {
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
            ((CRezDirNode*)n->m_14)->Load(1);
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

// The run-state / pacing globals UpdateClock (0x13ddc0) maintains alongside the
// frame clock above (reloc-masked; modeled as file-scope like g_now). g_clockReset
// is unsigned so its `!=0` gate + the elapsed<budget test emit the unsigned compare.
static i32 g_run7c;      // (run-state countdown; reseeded from g_run80)
static i32 g_run80;      // (run-state reload value)
static u32 g_clockReset; // (last clock-reset tick; == ?g_wap32ClockReset@@3HA @0x253c78)

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

// WINMM timeGetTime (the frame clock) comes from <Mfc.h> (via RezMgr.h).

// -------------------------------------------------------------------------
// RezMgr::UpdateClock() (0x13ddc0) - the frame-clock advance helper PerFrameTick
// calls (re-homed from src/Stub/RezMgr.cpp). Sample timeGetTime, derive the
// per-frame delta into g_now/g_frameDelta, run down the run-state countdown, then
// (when the pacing gate m_1c is armed) busy-wait to the ms budget and, every ~2s
// window, fold the frame count into the smoothed m_18 and rearm the window.
// @confidence: med
// @source: reloc-correlation (1 caller)
RVA(0x0013ddc0, 0xaa)
i32 RezMgr::UpdateClock() {
    u32 now = timeGetTime();
    u32 delta = now - (u32)g_now;
    g_now = now;
    g_frameDelta = delta;
    u32 run7c = g_run7c;
    if (run7c == 0) {
        g_run7c = g_run80;
    } else if (delta >= run7c) {
        g_run7c = 0;
    } else {
        g_run7c = run7c - delta;
    }

    if (m_1c > 0) {
        if (g_clockReset > 0) {
            u32 elapsed = timeGetTime() - g_clockReset;
            if (elapsed < (u32)m_28) {
                SpinWaitUntil(m_28 - elapsed);
            }
        }
        g_clockReset = timeGetTime();
    }

    u32 count = m_20 + 1;
    m_20 = count;
    if ((u32)g_now - (u32)m_24 >= 0x7d0) {
        m_18 = count >> 1;
        InitTimeFields(0);
    }
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: rez-trace
// @stub
RVA(0x0013b0c0, 0x238)
void CRezDir::Stub_13b0c0() {}

// -------------------------------------------------------------------------
// CRezParseNode::Construct (0x13cac0) - a CRezItmBase-derived rez tree node,
// re-homed from src/Stub/MallocConstructors. xref (gruntz.analysis.xref): built by
// CSymParser::ParseRecords (0x13b300, the .rez directory/symbol parser). Base-ctors
// via CRezItmBase::CRezItmBase (0x13c4e0), stamps its own vtable (0x5ef7d0),
// heap-copies its name string into +0x10, stores its parent at +0x18, and links
// itself into the parent dir's child CRezList via CRezList::AddHead (0x1851e0). A
// third CRezItmBase-derived class alongside CRezItm/CRezDir; exact name unresolved
// (non-RTTI vtable 0x5ef7d0), modeled as a plain shell so the stub is
// matching-neutral. Reconstruction deferred.
struct CRezParseNode {
    CRezParseNode* Construct(void* parent, void* nameSrc, void* owner); // 0x13cac0
};
// @confidence: high
// @source: xref
// @stub
RVA(0x0013cac0, 0x9b)
CRezParseNode* CRezParseNode::Construct(void* parent, void* nameSrc, void* owner) {
    return this;
}

// ===========================================================================
// Class-metadata annotations for the RezMgr.h classes. Hosted at EOF of this TU
// (not in the header): RezMgr.h is pulled into the /O2-sensitive Image.cpp for
// RezAlloc/RezFree, where any header-injected typedef reschedules DecodePcxData
// (verified). Placed after all function bodies so this TU is unperturbed too.
// ===========================================================================
SIZE(RezFindRec, 0x24);        // RE'd WIN32-find-style fixed record
SIZE_UNKNOWN(CRezItmOwner);    // slot-dispatch gate view (no emitted vtable)
SIZE(CRezItmBase, 0x10);       // "16 bytes" base (derived fields start at +0x10)
VTBL(CRezItmBase, 0x001ef768); // base vtable stamp from ctor 0x13c4e0
SIZE(CRezItm, 0x24);           // operator new leaf size 0x24
VTBL(CRezItm, 0x001ef788);     // derived vtable stamp from ctor 0x13c540
SIZE(CRezDirList, 0xc);        // embedded child-collection list {vptr,head,tail}
SIZE_UNKNOWN(CRezDir);         // model pads to +0x68 runtime fields; ctor alloc 0x38
SIZE_UNKNOWN(RezStream);       // abstract slot-view (pure virtuals, no vtable)
SIZE_UNKNOWN(RezSrc);
SIZE_UNKNOWN(CRezDirNode);
SIZE_UNKNOWN(CGameMode); // slot-dispatch view (no emitted vtable)
SIZE_UNKNOWN(RezMgrOwner);
SIZE_UNKNOWN(RezMgr);        // model pads to +0xfc; retail alloc is 0xa30
SIZE_UNKNOWN(CRezParseNode); // 0x13cac0 CRezItmBase-derived parse node (name TBD)

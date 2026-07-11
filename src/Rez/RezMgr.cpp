// RezMgr.cpp - the CGruntzMgr (WAP32 CGameMgr) manager MAIN cluster: the per-frame
// game tick, the debug-position probe, and the archive-path assembler - three methods
// of the "RezMgr" manager view (0x08b740 / 0x08e470 / 0x91670).
//
// The CRez* archive-container file-system family (ONE contiguous retail .text obj at
// 0x13c4e0-0x13ceec: CRezItmBase/CRezItm/CRezDir/CRezParseNode directory nodes +
// CRezFile/CRezFileMgr open-file wrapper) was carved into src/Rez/RezFile.cpp
// (holding-TU drain, 2026-07-11) - a distinct, separately-linked obj. (UpdateClock /
// SetFrameRate / ... @0x13ddc0.. moved to GameApp.cpp; MakeImageKey @0x13e5d0 to
// DDSurface.cpp - see RezFile.cpp's breadcrumbs.)
#include <Rez/RezMgr.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// The per-frame frame-clock globals PerFrameTick reads are the CANONICAL WAP32
// clock cells (0x253c70/0x253c74, DATA-bound in Globals.cpp) - the same cells
// GameApp.cpp's CGameMgr::InitializeTimeGlobal seeds and RezMgr::UpdateClock
// (now in GameApp.cpp, its original TU) advances. The per-second accumulators
// below stay file-scope statics of this TU (reloc-masked).
// ---------------------------------------------------------------------------
extern i32 g_wap32Now;        // 0x253c70 (last timeGetTime sample)
extern i32 g_wap32FrameDelta; // 0x253c74 (ms since previous frame)

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
//   UpdateClock();                       // advance g_wap32Now / g_wap32FrameDelta
//   int r = m_mode->Update();            // step the active game-state (slot +0x10)
//   if (r != 0x11) {                     // 0x11 = a state that suppresses timing
//       // clamp this frame's delta to <= 0x64 ms and accumulate
//       int dt = g_wap32FrameDelta;
//       g_lastNow = g_wap32Now;  g_lastDelta = dt;
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
// test) fall out; `int` emits signed `jle`/`jl` (94.78%). g_wap32FrameDelta is a
// timeGetTime() delta (genuinely unsigned ms).
RVA(0x0008b740, 0x12d)
i32 RezMgr::PerFrameTick() {
    if (m_mode == 0) {
        return 0;
    }

    UpdateClock();

    i32 r = m_mode->Update();
    if (r != 0x11) {
        u32 dt = g_wap32FrameDelta;
        g_lastNow = g_wap32Now;
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
        RezFormat(&m_pathA, s_join, cwd, (LPCTSTR)rez);
        if (!RezFileExists(m_pathA)) {
            if (drive) {
                RezFormat(&m_pathA, s_dataPath, drive, (LPCTSTR)rez);
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
    RezFormat(&m_pathB, s_join, cwd, (LPCTSTR)fec);
    if (!m_inGameDir && !RezFileExists(m_pathB) && !g_rezLowDetail) {
        RezFormat(&m_pathB, s_join, cwd, (LPCTSTR)fecHi);
        if (RezFileExists(m_pathB)) {
            movFound = 1;
        }
    }
    if (!movFound && drive) {
        RezFormat(&m_pathB, s_moviezPath, drive, (LPCTSTR)fec);
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

// ===========================================================================
// Class-metadata annotations for the RezMgr manager class (EOF-hosted; RezMgr.h is
// pulled into /O2-sensitive TUs like Image.cpp, so the header stays untouched).
// ===========================================================================
SIZE_UNKNOWN(CGameMode);   // abstract per-frame mode interface (no storage here)
SIZE_UNKNOWN(RezMgrOwner); // partial view of the owning-window holder (+0x04 HWND)
SIZE(RezMgr, 0xa30);       // the CGruntzMgr (WAP32::CGameMgr); alloc 0xa30, modeled to +0xfc

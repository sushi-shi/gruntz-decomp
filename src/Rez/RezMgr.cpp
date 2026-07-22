#include <Gruntz/GruntzMgr.h> // CGruntzMgr - the real owner of the three methods below
#include <Rez/RezMgr.h>       // RezFormat/RezFileExists helper decls (rez-path externs)
#include <Rez/FrameClock.h>
#include <rva.h>
#include <Rez/RezSync.h> // ex Globals.h
#include <Wap32/GameApp.h> // ex Globals.h

typedef CGameMgr CGameMgrBase;

DATA(0x00245580)
i32 g_lastNow = 0; // last timeGetTime() sample
DATA(0x00245584)
i32 g_frameDelta = 0; // frame delta, clamped to <= 0x64
// 0x245588 - the running accumulated frame time. This TU is the SOLE writer
// (g_frameTime += dt, below) and owns this contiguous 0x245580-0x2455a0 .bss band,
// so the definition lives here (was misfiled in Projectile.cpp).
DATA(0x00245588)
u32 g_frameTime = 0; // unsigned running clock (canonical type in <Rez/FrameClock.h>)
DATA(0x0024558c)
i32 g_frameTicks = 0; // per-frame counter
DATA(0x00245590)
i32 g_timer32 = 0; // interval countdown, seed 0x32
DATA(0x00245598)
i32 g_timer200 = 0; // interval countdown, seed 0xc8
DATA(0x0024559c)
i32 g_timer400 = 0; // interval countdown, seed 0x190
DATA(0x002455a0)
i32 g_timer500 = 0; // interval countdown, seed 0x1f4
DATA(0x00245594)
i32 g_timer100 = 0; // interval countdown, seed 0x64

RVA(0x0008b740, 0x12d)
i32 CGruntzMgr::PerFrameTick() {
    if (m_curState == 0) {
        return 0;
    }

    CGameMgrBase::PerFrameTick(); // the qualified base call - clock advance @0x13ddc0

    i32 r = m_curState->Update();
    if (r != GAMESTATE_NONE) {
        u32 dt = g_wap32FrameDelta;
        g_lastNow = g_wap32Now;
        g_frameDelta = dt;
        if (dt > 0x64) {
            dt = 0x64;
            g_frameDelta = 0x64;
        }
        g_frameTime += dt;

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

    m_curState->Render();
    return 1;
}

// The DEBUG_POSITION dialog proc: WarpDialogProc (GruntzMgr.cpp @0x8e4e0),
// address-taken through its ILT thunk (0x2d0b, byte-verified `jmp 0x8e4e0`);
// bound to the thunk rva (the pushed DIR32 is reloc-masked) - the same idiom as
// GruntzMgr.cpp's other dialog-proc thunk externs.
DATA_SYMBOL(0x00002d0b, 0x0, _WarpDialogProcThunk)

RVA(0x0008e470, 0x50)
i32 CGruntzMgr::HandleDebugPosition() {
    i32 r = 0;
    if (m_curState && m_curState->Update() == GAMESTATE_PLAY) {
        r = RunModalDialog("DEBUG_POSITION", static_cast<void*>(&WarpDialogProcThunk), 1);
        if (r == 1) {
            HWND hwnd = m_gameWnd->m_hwnd;
            PostMessageA(hwnd, 0x111, 0x805c, 0);
        }
    }
    return r != 0;
}

static const char s_rezName[] = "Gruntz.REZ";
static const char s_join[] = "%s\\%s";
static const char s_dataPath[] = "%c:\\DATA\\%s";
static const char s_fecName[] = "Gruntz.FEC";
static const char s_fecLoName[] = "GruntzLo.FEC";
static const char s_moviezPath[] = "%c:\\MOVIEZ\\%s";

// ---------------------------------------------------------------------------
// CGruntzMgr::MakeRezPath()
// Assembles the candidate archive paths (the main Gruntz.REZ into m_strRezPath
// and the front-end Gruntz.FEC / GruntzLo.FEC into m_strMoviePath) and probes
// them with FileExists, recording in m_inGameDir/m_haveRez/m_haveMoviez which
// were found. Reports an error and returns 0 if nothing was found, else 1.
// The low-detail selector is g_disableHqMovie (0x2455d4, <Globals.h> - the
// "Disable High Quality Movie" registry flag RezSync loads; the retail DIR32s
// here bind to that exact cell. The ex file-local `g_rezLowDetail` definition
// was a DIVERGENT duplicate: a second bss cell nothing ever wrote).
//
// PLATEAU 91.87% (documented): a >512 B C++ EH-frame function with four
// ref-counted MFC CString locals (one COW copy-ctor selecting the lo/hi FEC
// variant), the engine sprintf-style CString::Format wrapper, a runtime
// low-detail global branch, and FileExists probes. All call/string/IAT/EH
// operands are reloc-masked. The logic, control flow, all member offsets
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
// @early-stop
// EH-state-write scheduling residue (see the PLATEAU note above); logic complete.
RVA(0x00091670, 0x2ac)
i32 CGruntzMgr::MakeRezPath() {
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
        RezFormat(&m_strRezPath, s_join, cwd, static_cast<LPCTSTR>(rez));
        if (!RezFileExists(m_strRezPath)) {
            if (drive) {
                RezFormat(&m_strRezPath, s_dataPath, drive, static_cast<LPCTSTR>(rez));
                if (RezFileExists(m_strRezPath)) {
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
    CString fec(g_disableHqMovie ? fecLo : fecHi);

    m_haveMoviez = 0;
    i32 movFound = 0;
    RezFormat(&m_strMoviePath, s_join, cwd, static_cast<LPCTSTR>(fec));
    if (!m_inGameDir && !RezFileExists(m_strMoviePath) && !g_disableHqMovie) {
        RezFormat(&m_strMoviePath, s_join, cwd, static_cast<LPCTSTR>(fecHi));
        if (RezFileExists(m_strMoviePath)) {
            movFound = 1;
        }
    }
    if (!movFound && drive) {
        RezFormat(&m_strMoviePath, s_moviezPath, drive, static_cast<LPCTSTR>(fec));
        if (RezFileExists(m_strMoviePath)) {
            m_haveMoviez = 1;
        }
    }

    if (!found) {
        ReportError(0x800b, 0x43e);
        return 0;
    }
    return 1;
}

#include <Gruntz/GruntzMgr.h>
#include <Rez/RezMgr.h>
#include <Rez/FrameClock.h>
#include <rva.h>
#include <Rez/RezSync.h>
#include <Wap32/GameApp.h>

typedef CGameMgr CGameMgrBase;

DATA(0x00245580)
i32 g_lastNow = 0;
DATA(0x00245584)
u32 g_frameDelta = 0;

DATA(0x00245588)
u32 g_frameTime = 0;
DATA(0x0024558c)
i32 g_frameTicks = 0;
DATA(0x00245590)
i32 g_timer32 = 0;
DATA(0x00245598)
i32 g_timer200 = 0;
DATA(0x0024559c)
i32 g_timer400 = 0;
DATA(0x002455a0)
i32 g_timer500 = 0;
DATA(0x00245594)
i32 g_timer100 = 0;

DATA(0x0020c674)
static const char s_rezName[] = "Gruntz.REZ";
DATA(0x0020c66c)
static const char s_join[] = "%s\\%s";
DATA(0x00211054)
static const char s_dataPath[] = "%c:\\DATA\\%s";
DATA(0x00211044)
static const char s_fecName[] = "Gruntz.FEC";
DATA(0x00211034)
static const char s_fecLoName[] = "GruntzLo.FEC";
DATA(0x00211024)
static const char s_moviezPath[] = "%c:\\MOVIEZ\\%s";

RVA(0x0008b740, 0x12d)
i32 CGruntzMgr::PerFrameTick() {
    if (m_curState == 0) {
        return 0;
    }

    CGameMgrBase::PerFrameTick();

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

RVA(0x0008e470, 0x50)
i32 CGruntzMgr::HandleDebugPosition() {
    i32 r = 0;
    if (m_curState->Update() == GAMESTATE_PLAY) {
        r = RunModalDialog("DEBUG_POSITION", WarpDialogProc, 1);
        if (r == 1) {
            HWND hwnd = m_gameWnd->m_hwnd;
            PostMessageA(hwnd, 0x111, 0x805c, 0);
        }
    }
    return r != 0;
}

// @early-stop
RVA(0x00091670, 0x2ac)
i32 CGruntzMgr::MakeRezPath() {
    char cwd[0x100];
    if (!GetCurrentDirectoryA(0xff, cwd)) {
        return 0;
    }

    char drive = GetGruntzDriveLetter();
    m_inGameDir = (drive == cwd[0]);

    i32 found = 1;

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

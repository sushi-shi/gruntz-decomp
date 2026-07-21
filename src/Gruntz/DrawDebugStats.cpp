#include <Mfc.h> // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde / += 0x1ba0c8) + windows.h
#include <Gruntz/GameRegMfcPtr.h>
#include <ddraw.h> // real IDirectDrawSurface (the debug-overlay DC host: GetDC/ReleaseDC)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Play.h>              // the real CPlay : CState (the method owner)
#include <Gruntz/View.h>              // the CDDrawSurfaceMgr chain (render state, draw surface)
#include <DDrawMgr/DDrawChildGroup.h> // renderer A - the real CDDrawChildGroup (m_list.GetCount @+0x1c)
#include <Gruntz/GameLevel.h>   // canonical CGameLevel/CLevelPlane (the m_24 level + scroll origin)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (render-flip surface; +0x08 held COM surface)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (m_surface)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (full def)
#include <Gruntz/GruntzMgr.h>          // CGruntzMgr (base CGameMgr::m_fps @+0x18)
#include <stdio.h>                     // engine sprintf (reloc-masked)
#include <string.h>                    // inline strcat/strlen intrinsics (/O2)

#include <rva.h>
#include <Globals.h>

CString FormatElapsed(i32 count);

RVA(0x000cf770, 0x35e)
void CPlay::DrawDebugStats() {
    if (g_debugDisplayFlags & 0x20) {
        return;
    }

    char buf[0x1f0];
    char scratch[0x40];
    buf[0] = 0;

    if (g_debugDisplayFlags & 0x10) {
        sprintf(scratch, "Fps = %i ", m_mgr->m_fps);
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x1) {
        sprintf(scratch, " Objs = %i ", m_world->m_childGroup->m_list.GetCount());
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x4) {
        CLevelPlane* p = m_world->m_level->m_mainPlane;
        sprintf(scratch, " Pos = %i,%i", p->m_originX, p->m_originY);
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x40) {
        strcat(buf, " Timing = On ");
    }
    if (g_debugDisplayFlags & 0x80) {
        CString t = FormatElapsed(g_frameTime);
        t += " ";
        strcat(buf, t);
        t += " ";
    }
    if (g_debugDisplayFlags & 0x2) {
        sprintf(
            scratch,
            " Sent = %i, Rcvd = %i, Frame = %i Counter = %lu",
            m_packetsSent,
            m_packetsRcvd,
            GetFrame(),
            g_frameTime
        );
        strcat(buf, scratch);
    }

    CDDSurface* host = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = 0;
    host->m_8->GetDC(&hdc);
    if (hdc == 0) {
        return;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    SetBkColor(hdc, 0);
    PostSetup(hdc);

    if (buf[0] != 0) {
        RECT rb;
        RECT lr;
        CopyRect(&lr, g_gameReg->GetRect(&rb));
        RECT dr;
        dr.left = lr.left;
        dr.top = lr.bottom - 0x1c;
        dr.right = lr.right;
        dr.bottom = lr.bottom;
        if (lr.left > 0) {
            DrawTextA(hdc, buf, -1, &dr, 0x20);
        } else {
            TextOutA(hdc, 0, dr.top, buf, strlen(buf));
        }
    }
    host->m_8->ReleaseDC(hdc);
}

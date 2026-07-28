#include <Gruntz/DrawDebugStats.h> // this TU's external declarations
#include <Mfc.h> // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde / += 0x1ba0c8) + windows.h
#include <Gruntz/GameRegMfcPtr.h>
#include <ddraw.h> // real IDirectDrawSurface (the debug-overlay DC host: GetDC/ReleaseDC)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Play.h>              // the real CPlay : CState (the method owner)
#include <Gruntz/View.h>              // the CDDrawSurfaceMgr chain (render state, draw surface)
#include <DDrawMgr/DDrawChildGroup.h> // renderer A - the real CDDrawChildGroup (m_list.GetCount @+0x1c)
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CDDrawWorkerHost (the m_24 level + scroll origin)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (render-flip surface; +0x08 held COM surface)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (m_surface)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (full def)
#include <Gruntz/GruntzMgr.h>          // CGruntzMgr (base CGameMgr::m_fps @+0x18)
#include <stdio.h>                     // engine sprintf (reloc-masked)
#include <string.h>                    // inline strcat/strlen intrinsics (/O2)

#include <rva.h>

RVA(0x000cf770, 0x35e)
void CPlay::DrawDebugStats() {
    if (g_debugDisplayFlags & 0x20) {
        return;
    }

    // 0x200 + 0x40: the frame is sub esp,0x268 == 4 (hdc) + 4 (the CString) +
    // 0x10 (dr) + 0x10 (lr) + 0x40 (scratch) + 0x200 (buf), with NO separate slot
    // for the GetRect out-rect - retail hands GetRect the scratch buffer itself
    // (see the reinterpret_cast below).
    char buf[0x200];
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
        CDDrawWorkerHost* p = m_world->m_level->m_mainPlane;
        // The debug "Pos" is the plane's SNAPPED SCROLL ORIGIN (+0x84/+0x88), not
        // m_viewRect (+0x40/+0x44) - retail reads [eax+0x84]/[eax+0x88].
        sprintf(scratch, " Pos = %i,%i", p->m_snappedX, p->m_snappedY);
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x40) {
        strcat(buf, " Timing = On ");
    }
    if (g_debugDisplayFlags & 0x80) {
        CString t = FormatElapsedTime(g_frameTime);
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
    host->m_ddSurface->GetDC(&hdc);
    if (hdc == 0) {
        return;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    SetBkColor(hdc, 0);
    PostSetup(hdc);

    if (buf[0] != 0) {
        RECT lr;
        // The query rect IS the sprintf scratch buffer, and the frame PROVES it:
        // retail passes esp+0x38 - the very slot the format buffer occupies - and
        // sub esp,0x268 leaves no room for a third RECT. Dead storage by then (the
        // last strcat is long past); byte-forced overlay at this one seam.
        CopyRect(&lr, g_gameReg->GetRect(reinterpret_cast<RECT*>(scratch)));
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
    host->m_ddSurface->ReleaseDC(hdc);
}

#include <Gruntz/DrawDebugStats.h>
#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <ddraw.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Play.h>
#include <Gruntz/View.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GameLevel.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GruntzMgr.h>
#include <stdio.h>
#include <string.h>

#include <rva.h>
#include <Pix16.h>

RVA(0x000cf770, 0x35e)
void CPlay::DrawDebugStats() {
    if (g_debugDisplayFlags & 0x20) {
        return;
    }

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

        RecordBytes reuse;
        reuse.m_chars = scratch;
        CopyRect(&lr, g_gameReg->GetRect(static_cast<RECT*>(reuse.m_rec)));
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

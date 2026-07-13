// DrawDebugStats.cpp - CPlay::DrawDebugStats (0xcf770), the in-game debug-overlay
// text renderer. Recovered from the $SG string set ("Fps = %i ", " Objs = %i ",
// " Pos = %i,%i", " Timing = On ", " Sent = %i, Rcvd = %i, Frame = %i Counter =
// %lu") + the GDI call set (SetBkMode/SetTextColor/SetBkColor/DrawTextA/TextOutA).
//
// IDENTITY (fake-view burndown, 2026-07-05): the former TU-local "CDbgView" owner
// + its Dbg* sub-views were a per-TU shadow of the real CPlay: `sema xref 0xcf770`
// shows the callers are ?Render@CPlay (0xc8cf0) and CMulti::PumpB (CMulti : CPlay
// per RTTI), Play.h already declared the SAME RVA as its reloc-masked
// "ProfFlushTail", and every deref maps onto the canonical shapes: m_4/m_c are
// CState's CGruntzMgr*/CSpriteFactoryHolder*, the fps is CGameMgr::m_fps (+0x18), the object
// count CRenderer::m_1c (renderer A), the position CDrawSurface::m_5c->CameraGeom
// {m_84,m_88}, the DC host the render-flip CDDSurface (RenderState::m_14->m_2c)
// whose +0x08 holds the real IDirectDrawSurface (GetDC slot 17 +0x44 / ReleaseDC
// slot 26 +0x68, __stdcall), and the net counters are CPlay +0x2d0/+0x2d4.
// GetFrame (slot 27, +0x6c) and PostSetup (slot 37, +0x94) are the inherited
// CState fat-interface virtuals (State.h).
//
// Structure: a debug-flags byte (g_debugFlags @0x6455f4) gates the whole thing
// (bit 0x20 = master off) and each text piece (0x10 Fps, 0x1 Objs, 0x4 Pos,
// 0x40 Timing, 0x80 elapsed-time, 0x2 net stats). Each piece sprintf's into a
// scratch buffer and inline-strcat's onto the accumulator; the 0x80 piece builds
// a CString (FormatElapsed @0x1190f0, "%i:%02i:%02i") which gives the routine its
// /GX exception frame, so it lives in an `eh` unit. The tail draws the text
// bottom-aligned in the level's rect (g_dbgMgr->GetRect, then DrawTextA when
// rect.left>0 else TextOutA).

#include <Mfc.h> // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde / += 0x1ba0c8) + windows.h
#include <ddraw.h> // real IDirectDrawSurface (the debug-overlay DC host: GetDC/ReleaseDC)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Play.h>              // the real CPlay : CState (the method owner)
#include <Gruntz/View.h>              // the CSpriteFactoryHolder chain (render state, draw surface)
#include <DDrawMgr/DDrawChildGroup.h> // renderer A - the real CDDrawChildGroup (m_count @+0x1c)
#include <Gruntz/GameLevel.h>   // canonical CGameLevel/CLevelPlane (the m_24 level + scroll origin)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (render-flip surface; +0x08 held COM surface)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDrawTarget pages (m_surface)
#include <Gruntz/GruntzMgr.h>          // CGruntzMgr (base CGameMgr::m_fps @+0x18)
#include <stdio.h>                     // engine sprintf (reloc-masked)
#include <string.h>                    // inline strcat/strlen intrinsics (/O2)

#include <rva.h>
#include <Globals.h>

DATA(0x00245588)
extern "C" u32 g_frameTime; // a wrap-safe draw/elapsed counter (FormatElapsed arg + %lu)

// FUN_001190f0 __cdecl: format the counter as "%i:%02i:%02i" into a returned
// CString (copy-construct into the caller's hidden return slot).
CString FormatElapsed(i32 count);

// The 0x64556c singleton IS CGruntzMgr (RTTI-confirmed); GetRect (@0x8e3a0) is its
// method - the old `g_dbgMgr` CGameRegistry alias emitted the phantom
// ?GetRect@CGameRegistry@@QAEPAXPAX@Z, which no obj and no .LIB can ever define.
DATA(0x0024556c)
extern "C" CGruntzMgr* g_gameReg; // *0x64556c the one singleton

// @source: string-xref
// Code bytes are byte-EXACT vs retail (verified instruction-by-instruction with
// llvm-objdump -dr, base obj vs delinked target). The 99.55% fuzzy residual is
// entirely the reloc/EH/import scoring artifact: differently-NAMED symbol operands
// (sprintf vs _sprintf, FormatElapsed vs Unmatched_1190f0, CString op/dtor, the
// g_dbgMgr alias vs _g_mgrSettings), the Win32 __imp__ import-thunk vs direct-call
// delink form, and the /GX __except_list/__CxxFrameHandler EH-table relocs + the
// CString cleanup unwind funclet. No instruction-byte difference - green-enough (§2a).
RVA(0x000cf770, 0x35e)
void CPlay::DrawDebugStats() {
    if (g_debugFlags & 0x20) {
        return;
    }

    char buf[0x1f0];
    char scratch[0x40];
    buf[0] = 0;

    if (g_debugFlags & 0x10) {
        sprintf(scratch, "Fps = %i ", m_4->m_fps);
        strcat(buf, scratch);
    }
    if (g_debugFlags & 0x1) {
        sprintf(scratch, " Objs = %i ", m_c->m_childGroup->m_count);
        strcat(buf, scratch);
    }
    if (g_debugFlags & 0x4) {
        CLevelPlane* p = m_c->m_24->m_mainPlane;
        sprintf(scratch, " Pos = %i,%i", p->m_originX, p->m_originY);
        strcat(buf, scratch);
    }
    if (g_debugFlags & 0x40) {
        strcat(buf, " Timing = On ");
    }
    if (g_debugFlags & 0x80) {
        CString t = FormatElapsed(g_frameTime);
        t += " ";
        strcat(buf, t);
        t += " ";
    }
    if (g_debugFlags & 0x2) {
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

    CDDSurface* host = m_c->m_drawTarget->m_14->m_surface;
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

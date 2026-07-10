// StateDrawText.cpp - two CState render helpers physically sited in the shared
// attract/state-render source (RVA-contiguous with Attract.cpp's RunTitleSeq/Paint),
// kept in their own unit so they don't perturb Attract.cpp's already-matched /GX
// CAttract methods (the EngStrRenderText.cpp precedent). Both draw onto the CState
// frame surface reached through the shared CSpriteFactoryHolder holder:
//   * CState::Vslot17 (0x0fa6b0, vtable slot 23 / +0x5c, inherited by every state) -
//     the frame-surface GDI text overlay: GetDC on the IDirectDrawSurface, SetBkMode/
//     SetTextColor, TextOutA(x,y,str), ReleaseDC.
//   * CState::ShadeScreen (0x0faf50) - the once-suppressed screen dim: consume the
//     g_suppress latch on the first armed frame, else shade the draw surface.
#include <Mfc.h>   // <windows.h> GDI (HDC/SetBkMode/SetTextColor/TextOutA/RECT)
#include <ddraw.h> // IDirectDrawSurface (frame-surface GetDC/ReleaseDC COM slots 17/26)
#include <string.h>

#include <Gruntz/State.h>            // CState (m_c holder + the Vslot17 declaration)
#include <Gruntz/GameRegistry.h>    // CSpriteFactoryHolder (m_c -> m_drawTarget)
#include <Gruntz/ResMgr.h>          // CDrawTarget (m_10 frame surface / m_14 draw surface)
#include <DDrawMgr/DDSurface.h>     // CDDSurface::ShadeRect (m_14->m_2c)
#include <rva.h>

// The once-per-arm screen-dim suppression latch (?g_suppress_64e360@@3HA); when set,
// ShadeScreen consumes it (skips the shade for one frame) instead of shading.
DATA(0x0064e360)
extern i32 g_suppress_64e360;

// CState::Vslot17 (0x0fa6b0, slot 23): draw `str` at (x,y) onto the frame surface via
// GDI, in bk-mode `bkMode` and text color `color`. Bails (0) on a null string, a
// missing frame surface, or a failed GetDC; returns 1 on success. Inline /Oi strlen
// (repnz scasb) feeds TextOutA's length. All states inherit this one body.
RVA(0x000fa6b0, 0xa7)
i32 CState::Vslot17(i32 x, i32 y, char* str, i32 color, i32 bkMode) {
    if (str == 0) {
        return 0;
    }
    CDrawTarget::SurfaceA::Surface2c* s = m_c->m_drawTarget->m_10->m_2c;
    if (s == 0) {
        return 0;
    }
    HDC hdc = 0;
    s->m_8->GetDC(&hdc);
    if (hdc == 0) {
        return 0;
    }
    SetBkMode(hdc, bkMode);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, str, strlen(str));
    s->m_8->ReleaseDC(hdc);
    return 1;
}

// CState::ShadeScreen (0x0faf50): consume the g_suppress latch (return its old value)
// on the frame it is armed; otherwise shade the whole draw surface `pct` percent.
RVA(0x000faf50, 0x31)
i32 CState::ShadeScreen(i32 pct) {
    i32 v = g_suppress_64e360;
    if (v != 0) {
        g_suppress_64e360 = 0;
        return v;
    }
    return m_c->m_drawTarget->m_14->m_2c->ShadeRect(pct, 0);
}

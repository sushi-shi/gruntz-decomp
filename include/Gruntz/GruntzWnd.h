// GruntzWnd.h - CGruntzWnd, the Gruntz main window (the game's CGameWnd
// subclass; RTTI class, ??_7 @0x1ea2d4, SIZE 0x10 == the CGameWnd base - it adds
// no fields). ONE canonical definition: was defined .cpp-locally twice
// (GruntzWnd.cpp full class + GruntzApp.cpp's reduced `new`-site view), unified
// here 2026-07-16.
//
// CGruntzWnd overrides the CGameWnd message handlers (slot map:
// vtable_hierarchy --class CGruntzWnd: 16 override / 6 inherited / 0 new); each
// override forwards the message to the running CGruntzMgr (reached through the
// owning CGameApp: m_owner->m_gameMgr) and, where retail does, chains the
// CGameWnd base handler. Bodies + the RVA bindings live in
// src/Gruntz/GruntzWnd.cpp (0x94640..0x94bf1); slot 2 (Wap32GameWndVfunc2) is
// still declared-only (unreconstructed; the vftable reloc masks).
#ifndef GRUNTZ_GRUNTZWND_H
#define GRUNTZ_GRUNTZWND_H

#include <Ints.h>
#include <Wap32/Wap32.h>
#include <rva.h>

class CGruntzMgr;

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();                                                  // 0x94640
    virtual ~CGruntzWnd() OVERRIDE;                                // 0x946a0
    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) OVERRIDE; // slot 1  0x94790
    virtual i32 Wap32GameWndVfunc2(i32, i32, i32) OVERRIDE;        // slot 2  (declared-only)
    virtual i32 OnClose() OVERRIDE;                                // slot 4  0x94b90
    virtual i32 OnPaint() OVERRIDE;                                // slot 7  0x94bc0
    virtual i32 OnChar(WPARAM, LPARAM) OVERRIDE;                   // slot 8  0x948a0
    virtual i32 OnKeyDown(WPARAM, LPARAM) OVERRIDE;                // slot 9  0x948e0
    virtual i32 OnKeyUp(WPARAM, LPARAM) OVERRIDE;                  // slot 10 0x94920
    virtual i32 OnActivateApp(WPARAM, LPARAM) OVERRIDE;            // slot 12 0x94b20
    virtual i32 OnLButtonDown(WPARAM, i32, i32) OVERRIDE;          // slot 14 0x94960
    virtual i32 OnRButtonDown(WPARAM, i32, i32) OVERRIDE;          // slot 15 0x94a20
    virtual i32 OnLButtonUp(WPARAM, i32, i32) OVERRIDE;            // slot 16 0x949a0
    virtual i32 OnRButtonUp(WPARAM, i32, i32) OVERRIDE;            // slot 17 0x94a60
    virtual i32 OnMouseMove(WPARAM, i32, i32) OVERRIDE;            // slot 18 0x949e0
    virtual i32 OnLButtonDblClk(WPARAM, i32, i32) OVERRIDE;        // slot 19 0x94aa0
    virtual i32 OnRButtonDblClk(WPARAM, i32, i32) OVERRIDE;        // slot 20 0x94ae0

    i32 Wap32GameWndVfunc0(); // non-virtual: vftable emitted by the 16 real overrides

    // Reaches the running game manager through the owning CGameApp.
    CGruntzMgr* GameMgr() {
        return (CGruntzMgr*)m_owner->m_gameMgr;
    }
};
SIZE(CGruntzWnd, 0x10); // recovered from the operator-new site (gruntz.analysis.news)

#endif // GRUNTZ_GRUNTZWND_H

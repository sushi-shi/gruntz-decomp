#ifndef GRUNTZ_GRUNTZWND_H
#define GRUNTZ_GRUNTZWND_H

#include <rva.h>

#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <Wap32/Wap32.h>

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd() OVERRIDE;

    i32 CreateAndShow(CREATESTRUCTA* params, CGameApp* owner);
    void Destroy();

    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) OVERRIDE;
    virtual i32 HandleWindowCommand(i32, i32, i32) OVERRIDE;
    virtual i32 OnClose() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 OnChar(WPARAM charCode, LPARAM keyData) OVERRIDE;
    virtual i32 OnKeyDown(WPARAM virtualKey, LPARAM keyData) OVERRIDE;
    virtual i32 OnKeyUp(WPARAM virtualKey, LPARAM keyData) OVERRIDE;
    virtual i32 OnActivateApp(WPARAM, LPARAM) OVERRIDE;
    virtual i32 OnLButtonDown(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnRButtonDown(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnLButtonUp(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnRButtonUp(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnMouseMove(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnLButtonDblClk(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;
    virtual i32 OnRButtonDblClk(WPARAM keyFlags, i32 x, i32 y) OVERRIDE;

    i32 UnusedWindowQuery();

    CGruntzMgr* GameMgr() {
        return static_cast<CGruntzMgr*>(m_owner->m_gameMgr);
    }
};

#endif // GRUNTZ_GRUNTZWND_H

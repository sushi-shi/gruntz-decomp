#ifndef GRUNTZ_GRUNTZWND_H
#define GRUNTZ_GRUNTZWND_H

#include <Ints.h>
#include <Wap32/Wap32.h>
#include <rva.h>

#include <Gruntz/GruntzMgr.h>

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd() OVERRIDE;
    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) OVERRIDE;
    virtual i32 Wap32GameWndVfunc2(i32, i32, i32) OVERRIDE;
    virtual i32 OnClose() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 OnChar(WPARAM, LPARAM) OVERRIDE;
    virtual i32 OnKeyDown(WPARAM, LPARAM) OVERRIDE;
    virtual i32 OnKeyUp(WPARAM, LPARAM) OVERRIDE;
    virtual i32 OnActivateApp(WPARAM, LPARAM) OVERRIDE;
    virtual i32 OnLButtonDown(WPARAM, i32, i32) OVERRIDE;
    virtual i32 OnRButtonDown(WPARAM, i32, i32) OVERRIDE;
    virtual i32 OnLButtonUp(WPARAM, i32, i32) OVERRIDE;
    virtual i32 OnRButtonUp(WPARAM, i32, i32) OVERRIDE;
    virtual i32 OnMouseMove(WPARAM, i32, i32) OVERRIDE;
    virtual i32 OnLButtonDblClk(WPARAM, i32, i32) OVERRIDE;
    virtual i32 OnRButtonDblClk(WPARAM, i32, i32) OVERRIDE;

    i32 Wap32GameWndVfunc0();

    CGruntzMgr* GameMgr() {
        return static_cast<CGruntzMgr*>(m_owner->m_gameMgr);
    }
};
SIZE(0x10);

#endif // GRUNTZ_GRUNTZWND_H

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
    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) OVERRIDE;
    virtual i32 HandleWindowCommand(i32, i32, i32) OVERRIDE;
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

    i32 UnusedWindowQuery();

    CGruntzMgr* GameMgr() {
        return static_cast<CGruntzMgr*>(m_owner->m_gameMgr);
    }
};

#endif // GRUNTZ_GRUNTZWND_H

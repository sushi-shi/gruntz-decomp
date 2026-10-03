#ifndef GRUNTZ_GRUNTZWND_H
#define GRUNTZ_GRUNTZWND_H

#include <Ints.h>

#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <Wap32/Wap32.h>

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd()  ;

    i32 CreateAndShow(CREATESTRUCTA* params, CGameApp* owner);
    void Destroy();

    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM)  ;
    virtual i32 HandleWindowCommand(i32, i32, i32)  ;
    virtual i32 OnClose()  ;
    virtual i32 OnPaint()  ;
    virtual i32 OnChar(WPARAM charCode, LPARAM keyData)  ;
    virtual i32 OnKeyDown(WPARAM virtualKey, LPARAM keyData)  ;
    virtual i32 OnKeyUp(WPARAM virtualKey, LPARAM keyData)  ;
    virtual i32 OnActivateApp(WPARAM, LPARAM)  ;
    virtual i32 OnLButtonDown(WPARAM keyFlags, i32 x, i32 y)  ;
    virtual i32 OnRButtonDown(WPARAM keyFlags, i32 x, i32 y)  ;
    virtual i32 OnLButtonUp(WPARAM keyFlags, i32 x, i32 y)  ;
    virtual i32 OnRButtonUp(WPARAM keyFlags, i32 x, i32 y)  ;
    virtual i32 OnMouseMove(WPARAM keyFlags, i32 x, i32 y)  ;
    virtual i32 OnLButtonDblClk(WPARAM keyFlags, i32 x, i32 y)  ;
    virtual i32 OnRButtonDblClk(WPARAM keyFlags, i32 x, i32 y)  ;

    CGruntzMgr* GameMgr() {
        return static_cast<CGruntzMgr*>(m_owner->m_gameMgr);
    }
};

#endif

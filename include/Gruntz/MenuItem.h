#ifndef GRUNTZ_MENUITEM_H
#define GRUNTZ_MENUITEM_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>

GZ_ENUM_FORWARD(MenuItemState);

class CMenuPage;
class CMenuItem;
class CDDrawSurfacePair;

class CDDrawSurfaceMgr;

class CMenuItem {
public:
    CMenuItem();
    virtual ~CMenuItem();

    virtual i32 Init(
        CMenuPage* page,
        const char* name,
        const char* spriteKey,
        i32 cmdId,
        const char* key,
        i32 flags
    );

    virtual void Cleanup();
    virtual void Reset();
    virtual i32 GetWidth();
    virtual i32 GetFrameWidth();
    virtual void Disable(MenuItemState mode);

    virtual i32 Detach();

    virtual i32 Notify(u32 dt);
    virtual i32 Place(CDDrawSurfacePair* target, i32 x, i32 y);

    virtual i32 Configure(i32 notify);
    virtual i32 Release();
    virtual i32 Trigger();
    virtual i32 OnInit();

    RVA(0x001845b0, 0x20)
    CString GetName() {
        return m_name;
    }
    RVA(0x001845d0, 0x20)
    CString GetLeftName() {
        return m_leftName;
    }
    RVA(0x001845f0, 0x20)
    CString GetRightName() {
        return m_rightName;
    }
    CString GetUpName();
    CString GetDownName();
    i32 NotifyCmd();
    i32 Hit(i32 x, i32 y);

    CDDrawSurfaceMgr* m_owner;

    class CChatBox* m_host;
    CMenuPage* m_template;
    CString m_name;
    CString m_key;
    i32 m_cmdId;
    i32 m_secondaryCmdId;
    i32 m_flags;
    MenuItemState m_state;
    CObject* m_sprite;

    POSITION m_listPos;

    i32 m_cmdParam;
    i32 m_hitLeft;
    i32 m_hitTop;
    i32 m_hitRight;
    i32 m_hitBottom;
    i32 m_fixedX;
    i32 m_fixedY;
    CString m_leftName;
    CString m_rightName;
    CString m_upName;
    CString m_downName;
};
SIZE(0x5c);

inline CMenuItem::~CMenuItem() {
    Cleanup();
}

inline CMenuItem::CMenuItem() {
    Reset();
}

#endif // GRUNTZ_MENUITEM_H

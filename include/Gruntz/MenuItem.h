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

// Reset lives HERE, not in MenuItem.cpp: retail inlines it into three of
// CMenuPage's four new-sites (AddItem 0x183460, AddSubItem 0x1835a0,
// AddItem2 0x1836f0) and, its inline budget spent, emits a real
// `call ?Reset@CMenuItem@@UAEXXZ` in the fourth (AddSubItem2 0x183850).
// The out-of-line COMDAT is pinned in MenuItem.cpp, which emits the vtable.
inline void CMenuItem::Reset() {
    m_host = NULL;
    m_template = NULL;
    m_sprite = NULL;
    m_owner = NULL;
    m_listPos = NULL;
    m_hitLeft = UNINIT_FILL;
    m_fixedX = UNINIT_FILL;
    m_leftName.Empty();
    m_rightName.Empty();
    m_upName.Empty();
    m_downName.Empty();
}

inline CMenuItem::CMenuItem() {
    Reset();
}

#endif // GRUNTZ_MENUITEM_H

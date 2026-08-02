#ifndef GRUNTZ_MENUITEM_H
#define GRUNTZ_MENUITEM_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

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
    virtual void Disable(i32 mode);

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
    i32 m_state;
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

inline CMenuItem::CMenuItem() {
    m_host = 0;
    m_template = 0;
    m_sprite = 0;
    m_owner = 0;
    m_listPos = 0;
    m_hitLeft = static_cast<i32>(0xeeeeeeee);
    m_fixedX = static_cast<i32>(0xeeeeeeee);
    m_leftName.Empty();
    m_rightName.Empty();
    m_upName.Empty();
    m_downName.Empty();
}

#endif // GRUNTZ_MENUITEM_H

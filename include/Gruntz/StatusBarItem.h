#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SbiCommandId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItemKind.h>
#include <Gruntz/StatusBarTab.h>
#include <Ints.h>

#include <stddef.h>

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CStatusBarItem {
public:
    CStatusBarItem();
    virtual ~CStatusBarItem();

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 unusedFrame
    );
    virtual void Reset();
    virtual i32 Refresh(i32 deltaMs);
    virtual i32 Render();

    virtual i32 OnPointerMove(i32 keyFlags, i32 x, i32 y);
    virtual i32 OnDoubleClick(i32 keyFlags, i32 x, i32 y);
    virtual i32 UnusedPointerAction(i32, i32, i32);
    virtual i32 OnPointerDrag(i32 keyFlags, i32 x, i32 y);
    virtual void RequestRedraw();

    void SetEnabled(i32 on) {
        m_enabled = on;
    }

    b32 m_enabled;
    StatusBarItemKind m_kind;
    SbiCommandId m_cmd;
    StatusBarTab m_tab;

    RECT m_rect;
    class CDDrawSurfaceMgr* m_host;
    i32 m_redrawFrames;
    class CStatusBarMgr* m_owner;
};

RVA(0x001005d0, 0x17)
inline CStatusBarItem::CStatusBarItem() {
    m_enabled = false;
    m_kind = SBI_KIND_BASE;
    m_host = NULL;
    m_redrawFrames = 0;
}

inline CStatusBarItem::~CStatusBarItem() {
    Reset();
}

#endif // STATUSBARITEM_H

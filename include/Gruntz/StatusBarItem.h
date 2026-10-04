#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include <rva.h>

#include <Globals.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiCommandId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItemKind.h>
#include <Gruntz/StatusBarTab.h>
#include <Ints.h>
#include <MakeRect.h>

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

    SbiCommandId GetCommandId() const {
        return m_cmd;
    }

    StatusBarTab GetTab() const {
        return m_tab;
    }

    StatusBarItemKind GetKind() const {
        return m_kind;
    }

    b32 IsEnabled() const {
        return m_enabled;
    }

    b32 ContainsPoint(i32 x, i32 y) const {
        return ::PtInRect(&m_rect, x, y);
    }

    void SetEnabled(i32 on) {
        m_enabled = on;
    }

    void
    Initialize(CStatusBarMgr* owner, StatusBarTab tab, CDDrawSurfaceMgr* host, b32 enabled = true) {
        m_owner = owner;
        m_tab = tab;
        m_host = host;
        m_redrawFrames = 0;
        SetEnabled(enabled);
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
    SetEnabled(false);
    m_kind = SBI_KIND_BASE;
    m_host = NULL;
    m_redrawFrames = 0;
}

inline CStatusBarItem::~CStatusBarItem() {
    Reset();
}

#endif // STATUSBARITEM_H

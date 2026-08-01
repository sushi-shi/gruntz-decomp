#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

#include <Gruntz/SbGeom.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <rva.h>

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CStatusBarItem {
public:
#ifdef SBI_ITEM_OWN_CTOR
    CStatusBarItem();
#else
    CStatusBarItem() {
        m_enabled = 0;
        m_kind = 0;
        m_24 = 0;
        m_28 = 0;
    }
#endif
    virtual ~CStatusBarItem();

    virtual i32 SerializeFields(CFileMemBase* ar, i32 kind, i32 a, i32 b);

    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
        RECT rc,
        const char* key,
        i32 a10
    );
    virtual void Reset();
    virtual i32 Refresh(i32 a);
    virtual i32 Render();

    virtual i32 OnPointerMove(i32, i32, i32);
    virtual i32 Click1c(i32 a, i32 b, i32 c);
    virtual i32 UnusedPointerAction(i32, i32, i32);
    virtual i32 Click24(i32 a, i32 b, i32 c);
    virtual void SetSubtype();

    i32 m_enabled;
    i32 m_kind;
    i32 m_cmd;
    i32 m_tab;

    RECT m_rect14;
    class CDDrawSurfaceMgr* m_24;
    i32 m_28;
    class CStatusBarMgr* m_2c;
};
SIZE_UNKNOWN();

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_ITEM_OWN_DTOR)
inline CStatusBarItem::~CStatusBarItem() {
    Reset();
}
#endif

extern "C" i32 g_curPlayer;

#endif // STATUSBARITEM_H

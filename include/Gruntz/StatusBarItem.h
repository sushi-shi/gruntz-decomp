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
    // Two entities (docs/patterns/two-shapes-need-two-entities.md).  Retail carries
    // ??0CStatusBarItem@@QAE@XZ as a standalone 23-byte COMDAT at 0x1005d0 AND
    // expands the same four stores at other sites, so the source had an out-of-line
    // body plus an inline sibling.  The four `sema xref 0x1005d0` callers are
    // BuildStatusBarTabs, BuildGameMenu, BuildTabzDialog and LoadTabSprites, and in
    // all four every `new CSBI_Image` (size 0x34) calls it.
    CStatusBarItem();
    enum ENoSeed {
        NO_SEED
    };
    CStatusBarItem(ENoSeed) {
        m_enabled = 0;
        m_kind = SBI_KIND_BASE;
        m_host = NULL;
        m_redrawFrames = 0;
    }
    virtual ~CStatusBarItem();

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b);

    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
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
    StatusBarItemKind m_kind;
    SbiCommandId m_cmd;
    StatusBarTab m_tab;

    RECT m_rect14;
    class CDDrawSurfaceMgr* m_host;
    i32 m_redrawFrames;
    class CStatusBarMgr* m_owner;
};
SIZE_UNKNOWN();

inline CStatusBarItem::~CStatusBarItem() {
    Reset();
}

#endif // STATUSBARITEM_H

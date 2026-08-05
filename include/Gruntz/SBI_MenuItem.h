#ifndef SBI_MENUITEM_H
#define SBI_MENUITEM_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

#include <stddef.h>

class CDDrawWorker;

class CDDrawSurfaceMgr;

class CSBI_MenuItem : public CSBI_Image {
public:
    CSBI_MenuItem() {
        m_kind = SBI_KIND_MENU_ITEM;
        m_state = MENUITEM_UNSET;
        m_frame = NULL;
        m_record = NULL;
    }

    virtual ~CSBI_MenuItem() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 frame,
        i32 unused
    ) OVERRIDE;

    i32 ResolveFrame(const char* key, i32 a);
    i32 SetState(SbiMenuItemState state, i32 a);
    i32 ProbeState(SbiMenuItemState state);
    i32 Blit();

    SbiMenuItemState m_state;

    CDDrawWorker* m_record;
};
SIZE_UNKNOWN();

inline CSBI_MenuItem::~CSBI_MenuItem() {
    Reset();
}

#endif // SBI_MENUITEM_H

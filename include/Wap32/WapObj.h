#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/Object.h>

#include <stddef.h>

VTBL_ABSENT(CWapObj);
class CDDrawSurfaceMgr;

class CWapObj : public CObject {
public:
    // Pure: no retail vtable binds a CWapObj::IsLoaded body. Every one of the 22
    // classes reaching this slot supplies its own (vtable_hierarchy --csv slot 5).
    virtual i32 IsLoaded() = 0;

    virtual i32 IsReady();

    i32 m_id;
    i32 m_flags;
    class CDDrawSurfaceMgr* m_ownerCtx;

    // 0xd5d70: CImage's base-subobject destructor. Retail's ??1CImage EH funclet
    // CALLS it, and CImage's RTTI base array is CImage -> CWapObj -> CObject, so
    // the three-field reset belongs to CWapObj, not to a class between them.
    virtual ~CWapObj() OVERRIDE {
        m_id = -1;
        m_flags = 0;
        m_ownerCtx = NULL;
    }

    CWapObj() {}

    CWapObj(i32 id, class CDDrawSurfaceMgr* owner) {
        m_id = id;
        m_flags = 0;
        m_ownerCtx = owner;
    }
    class CDDrawSurfaceMgr* OwnerMgr() const {
        return m_ownerCtx;
    }
};
SIZE(0x10);

#endif // WAP32_CWAPOBJ_H

#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/Object.h>

VTBL_ABSENT(CWapObj);
class CDDrawSurfaceMgr;

class CWapObj : public CObject {
public:
    virtual i32 IsLoaded();

    virtual i32 IsReady();

    i32 m_id;
    i32 m_flags;
    class CDDrawSurfaceMgr* m_ownerCtx;

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

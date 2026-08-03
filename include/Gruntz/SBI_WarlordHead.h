#ifndef SBI_WARLORDHEAD_H
#define SBI_WARLORDHEAD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Ints.h>

struct CShadeTable;

class CSBI_WarlordHead : public CSBI_ImageSet {
public:
    CSBI_WarlordHead() {
        m_frame = 0;
        m_kind = 0xb;
        m_frameSet = 0;
    }

    virtual ~CSBI_WarlordHead() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, i32 pObj)
        OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
        RECT rc,
        const char* key,
        i32 frame,
        i32 extra
    ) OVERRIDE;

    i32 ShowFrames(i32 show, CShadeTable* palDescr);

    i32 SetState(i32 dir);

    i32 m_direction;
};
SIZE_UNKNOWN();

#endif // SBI_WARLORDHEAD_H

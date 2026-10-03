#ifndef SBI_WARLORDHEAD_H
#define SBI_WARLORDHEAD_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

struct CShadeTable;
GZ_ENUM_FORWARD(ShadeMode);

class CSBI_WarlordHead : public CSBI_ImageSet {
public:
    CSBI_WarlordHead() {
        m_kind = SBI_KIND_WARLORD_HEAD;
        m_frameSet = NULL;
    }

    virtual ~CSBI_WarlordHead()  ;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
         ;
    virtual i32 Render()  ;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 frame,
        i32 extra
    )  ;

    i32 ShowFrames(ShadeMode show, CShadeTable* palDescr);

    i32 SetState(i32 dir);

    i32 m_direction;
};

#endif

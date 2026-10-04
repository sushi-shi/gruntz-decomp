#ifndef SBI_WARLORDHEAD_H
#define SBI_WARLORDHEAD_H

#include <rva.h>

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

    virtual ~CSBI_WarlordHead() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CGameWorld* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 frame,
        i32 extra
    ) OVERRIDE;

    i32 SetHeadShading(ShadeMode shadeMode, CShadeTable* shadeTable);

    i32 SetDisplayState(i32 state);

    i32 m_displayState;
};

#endif // SBI_WARLORDHEAD_H

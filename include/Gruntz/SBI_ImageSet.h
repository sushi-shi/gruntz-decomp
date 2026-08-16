#ifndef GRUNTZ_SBI_IMAGESET_H
#define GRUNTZ_SBI_IMAGESET_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

#include <stddef.h>

class CDDrawWorker;

class CSBI_ImageSet : public CSBI_Image {
public:
    CSBI_ImageSet();
    virtual ~CSBI_ImageSet() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 pObj)
        OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;
    virtual i32 SetupImage(
        CStatusBarMgr*,
        CDDrawSurfaceMgr*,
        SbiCommandId,
        StatusBarTab,
        RECT,
        const char*,
        i32,
        i32
    ) OVERRIDE;

    virtual void Notify(i32 on);

    CDDrawWorker* m_frameSet;
    i32 m_frameIndex;
};

inline CSBI_ImageSet::CSBI_ImageSet() {
    m_kind = SBI_KIND_IMAGE_SET;
    m_frameSet = NULL;
}

inline CSBI_ImageSet::~CSBI_ImageSet() {
    Reset();
}

#endif // GRUNTZ_SBI_IMAGESET_H

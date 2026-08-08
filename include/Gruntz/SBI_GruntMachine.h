#ifndef SBI_GRUNTMACHINE_H
#define SBI_GRUNTMACHINE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>

#include <stddef.h>

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CSBI_GruntMachine : public CStatusBarItem {
public:
    CSBI_GruntMachine() : CStatusBarItem(CStatusBarItem::NO_SEED) {
        m_kind = SBI_KIND_GRUNT_MACHINE;
        m_frameA = NULL;
        m_frameB = NULL;
        m_standaloneFrame = NULL;
        m_config = NULL;
    }

    virtual ~CSBI_GruntMachine() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 BuildResourceTabStatusBar(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT g,
        const char* key,
        i32 idxA,
        i32 idxB
    );

    void SetFrames(i32 idxA, i32 idxB);

    CDDrawWorker* m_config;
    CImage* m_frameA;
    i32 m_frameIdxA;
    CImage* m_frameB;
    i32 m_frameIdxB;
    CImage* m_standaloneFrame;
};
SIZE(0x48);

#endif // SBI_GRUNTMACHINE_H

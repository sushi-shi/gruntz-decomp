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

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CSBI_GruntMachine : public CStatusBarItem {
public:
    CSBI_GruntMachine() {
        m_kind = 9;
        m_frameA = 0;
        m_frameB = 0;
        m_standaloneFrame = 0;
        m_config = 0;
    }

    virtual ~CSBI_GruntMachine() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 BuildResourceTabStatusBar(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
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
SIZE_UNKNOWN();

#endif // SBI_GRUNTMACHINE_H

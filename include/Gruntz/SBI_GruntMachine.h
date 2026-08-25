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
    CSBI_GruntMachine() {
        m_kind = SBI_KIND_GRUNT_MACHINE;
        m_leftFrame = NULL;
        m_rightFrame = NULL;
        m_standaloneFrame = NULL;
        m_config = NULL;
    }

    virtual ~CSBI_GruntMachine() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 deltaMs) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 BuildResourceTabStatusBar(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT g,
        const char* key,
        i32 leftFrameIndex,
        i32 rightFrameIndex
    );

    void SetFrames(i32 leftFrameIndex, i32 rightFrameIndex);

    CDDrawWorker* m_config;
    CImage* m_leftFrame;
    i32 m_leftFrameIndex;
    CImage* m_rightFrame;
    i32 m_rightFrameIndex;
    CImage* m_standaloneFrame;
};

#endif // SBI_GRUNTMACHINE_H

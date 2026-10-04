#ifndef SBI_GRUNTMACHINE_H
#define SBI_GRUNTMACHINE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <MakeRect.h>

#include <stddef.h>

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CSBI_GruntMachine : public CStatusBarItem {
public:
    CSBI_GruntMachine() {
        m_kind = SBI_KIND_GRUNT_MACHINE;
        m_leftFrame = NULL;
        m_rightFrame = NULL;
        m_backgroundImage = NULL;
        m_machineFrames = NULL;
    }

    virtual ~CSBI_GruntMachine() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 deltaMs) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 Initialize(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rect,
        const char* frameSetName,
        i32 leftFrameIndex,
        i32 rightFrameIndex
    );

    void SetFrames(i32 leftFrameIndex, i32 rightFrameIndex);

    CDDrawWorker* m_machineFrames;
    CImage* m_leftFrame;
    i32 m_leftFrameIndex;
    CImage* m_rightFrame;
    i32 m_rightFrameIndex;
    CImage* m_backgroundImage;
};

#endif // SBI_GRUNTMACHINE_H

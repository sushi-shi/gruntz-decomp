#ifndef GRUNTZ_SBI_SIDETAB_H
#define GRUNTZ_SBI_SIDETAB_H

#include <Ints.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusSampleMode.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

class CStatusBarMgr;

class CSBI_SideTab : public CStatusBarItem {
public:
    CSBI_SideTab() {
        m_topFrame = NULL;
        m_bottomFrame = NULL;
        m_sampledValue = -1;
        m_sampleMode = STATUS_SAMPLE_UNINITIALIZED;
    }
    virtual ~CSBI_SideTab()  ;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
         ;
    virtual void Reset()  ;
    virtual i32 Refresh(i32 deltaMs)  ;
    virtual i32 Render()  ;

    i32 BuildStatzTabStatusBar(
        CStatusBarMgr* parent,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* unused,

        i32 rowIndex,
        i32 colIndex,
        StatusSampleMode enabled,
        i32 onLeft
    );

    i32 BuildHandle();

    CImage* m_topFrame;
    CImage* m_bottomFrame;
    i32 m_sampledValue;
    i32 m_rowIndex;
    i32 m_colIndex;
    StatusSampleMode m_sampleMode;
    Coord m_drawPosition;
    i32 m_bottomFrameDy;
    i32 m_onLeft;
    i32 m_drawGate;
};

#endif

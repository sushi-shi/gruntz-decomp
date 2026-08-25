#ifndef GRUNTZ_SBI_IMAGESETANI_H
#define GRUNTZ_SBI_IMAGESETANI_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarDock.h>
#include <Ints.h>

#include <stddef.h>

class CSBI_ImageSetAni : public CSBI_ImageSet {
public:
    CSBI_ImageSetAni() {
        m_kind = SBI_KIND_IMAGE_SET_ANI;
        m_frameSet = NULL;
        m_loop = 0;
        m_interval = 0x64;
    }

    virtual ~CSBI_ImageSetAni() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual i32 Refresh(i32 deltaMs) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 Init(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 frameStart,
        i32 frameEnd,
        i32 intervalMs,
        i32 loop,
        i32 step
    );

    virtual void SetRange(i32 start, i32 end, i32 step, i32 loop, i32 interval);

    i32 m_interval;
    i32 m_lastTime;
    i32 m_loop;
    i32 m_step;
    i32 m_frameEnd;
    i32 m_frameStart;
};

inline CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    Reset();
}

class CSBI_StatzTabArrow : public CSBI_ImageSetAni {
public:
    CSBI_StatzTabArrow() {
        m_kind = SBI_KIND_STATZ_TAB_ARROW;
    }

    virtual ~CSBI_StatzTabArrow() OVERRIDE;

    void SetUnsampledDirection(StatusBarDock position, i32 animate);
    void SetSampledDirection(StatusBarDock position, i32 animate);
};

#endif // GRUNTZ_SBI_IMAGESETANI_H

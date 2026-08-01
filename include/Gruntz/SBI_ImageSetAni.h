#ifndef GRUNTZ_SBI_IMAGESETANI_H
#define GRUNTZ_SBI_IMAGESETANI_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_ImageSet.h>

class CSBI_ImageSetAni : public CSBI_ImageSet {
public:
    CSBI_ImageSetAni() {
        m_frame = 0;
        m_kind = 8;
        m_34 = 0;
        m_loop = 0;
        m_interval = 0x64;
    }

    virtual ~CSBI_ImageSetAni() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 Init(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
        RECT rc,
        const char* key,
        i32 b0,
        i32 b1,
        i32 b2,
        i32 b3,
        i32 b4
    );

    virtual void SetRange(i32 start, i32 end, i32 step, i32 loop, i32 interval);

    i32 m_interval;
    i32 m_lastTime;
    i32 m_loop;
    i32 m_step;
    i32 m_frameEnd;
    i32 m_frameStart;
};
SIZE_UNKNOWN();

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGESETANI_DTOR)
inline CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    Reset();
}
#endif

class CSBI_StatzTabArrow : public CSBI_ImageSetAni {
public:
    CSBI_StatzTabArrow() {
        m_kind = 5;
    }

    virtual ~CSBI_StatzTabArrow() OVERRIDE;

    void SetDirection(i32 position, i32 animate);
    void SetDirectionAlt(i32 position, i32 animate);
};
SIZE(0x54);
VTBL(CSBI_StatzTabArrow, 0x001eac94);

#endif // GRUNTZ_SBI_IMAGESETANI_H

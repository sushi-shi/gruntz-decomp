#ifndef GRUNTZ_GRUNTZ_RESOLVENODE_H
#define GRUNTZ_GRUNTZ_RESOLVENODE_H

#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

struct WwdDirtyRect {
    // Out-of-line in retail: CDDrawChildGroup's factories CALL 0x15b270 (from
    // 0x1592c8/0x1594b8/0x159673). The body lives in src/Wwd/WwdObjMgr.cpp.
    WwdDirtyRect();

    ~WwdDirtyRect() {}

    enum ENoSeed {
        NO_SEED
    };
    WwdDirtyRect(ENoSeed) {}

    // The same seed, INLINE: CResolveNode's ctors expand it (0x1549d0 stores
    // [+0x20]=COORD_UNSET and [+0x38]=-1 straight into the parent).
    enum EInlineSeed {
        INLINE_SEED
    };
    WwdDirtyRect(EInlineSeed) {
        m_rect.left = COORD_UNSET;
        m_armed = -1;
    }
    i32 m_lastX;
    i32 m_lastY;
    RECT m_rect;
    i32 m_w;
    i32 m_h;
    i32 m_armed;
};
SIZE(0x24);

class CResolveNode : public CLoadable {
public:
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;

    virtual i32 SetPosition(i32 x, i32 y);

    CResolveNode();

    // Out of line at 0x15b2c0 in WwdObjMgr.cpp; <Gruntz/ResolveNodeCtorInline.h> is the
    // opt-in inline view of the same body for the one TU that expands it.
    CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08);

    enum ENoSeed {
        NO_SEED
    };
    CResolveNode(ENoSeed) : m_dirty(WwdDirtyRect::NO_SEED) {}
    i32 Init(
        CDDrawSurfaceMgr* owner,
        i32 field04,
        i32 resolveX,
        i32 resolveY,
        i32 field40,
        i32 field08
    );

    virtual ~CResolveNode() OVERRIDE {
        m_screenX = COORD_UNSET;
        m_dirty.m_rect.left = COORD_UNSET;
        m_dirty.m_armed = -1;
    }

    i32 m_plotDX;
    i32 m_plotDY;

    WwdDirtyRect m_dirty;

    class CGameLevel* m_level;
    SpriteStateFlags m_stateFlags;
    i32 m_flashCountdown;
    i32 m_flashInterval;
    CShadeTable* m_drawFillArg;
    ShadeMode m_drawFillCmd;
    i32 m_fillFraction;
    i32 m_drawActive;
    i32 m_screenX;

    i32 m_screenY;

    RECT m_clip;
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H

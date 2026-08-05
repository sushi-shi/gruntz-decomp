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
    WwdDirtyRect();

    ~WwdDirtyRect() {}

    enum ENoSeed {
        NO_SEED
    };
    WwdDirtyRect(ENoSeed) {}
    i32 m_lastX;
    i32 m_lastY;
    RECT m_rect;
    i32 m_w;
    i32 m_h;
    i32 m_armed;
};
SIZE(0x24);

#ifndef WWDDIRTYRECT_OOL_CTOR
inline WwdDirtyRect::WwdDirtyRect() {
    m_rect.left = COORD_UNSET;
    m_armed = -1;
}
#endif

class CResolveNode : public CLoadable {
public:
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;

    virtual i32 SetPosition(i32 x, i32 y);

    CResolveNode();

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
VTBL(CResolveNode, 0x001efbc0);

#ifndef CRESOLVENODE_OOL_CTOR
inline CResolveNode::CResolveNode(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CLoadable(field04, field08, owner) {

    m_screenX = COORD_UNSET;
    m_clip.left = COORD_UNSET;
    m_level = NULL;
    m_stateFlags = SPRITE_STATE_NONE;
}
#endif

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H

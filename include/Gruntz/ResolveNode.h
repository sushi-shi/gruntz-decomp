#ifndef GRUNTZ_GRUNTZ_RESOLVENODE_H
#define GRUNTZ_GRUNTZ_RESOLVENODE_H

#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

struct WwdDirtyRect {
    WwdDirtyRect();

    RVA(0x0015b290, 0x10)
    ~WwdDirtyRect() {}

    enum ENoSeed {
        NO_SEED
    };
    WwdDirtyRect(ENoSeed) {}

    enum EInlineSeed {
        INLINE_SEED
    };
    WwdDirtyRect(EInlineSeed) {
        m_rect.left = COORD_UNSET;
        m_armed = -1;
    }

    void Reset() {
        m_rect.left = COORD_UNSET;
        m_armed = -1;
    }
    Coord m_lastPosition;
    RECT m_rect;
    SIZE
    m_size;
    i32 m_armed;
};

class CResolveNode : public CWapObj {
public:
    virtual i32 IsLoaded() OVERRIDE;
    RVA(0x00154a80, 0x13)
    virtual void Unload() OVERRIDE {
        m_screenPosition.m_x = COORD_UNSET;
        m_dirty.Reset();
    }

    virtual i32 SetPosition(i32 x, i32 y);

    Coord ScreenPos() const {
        return m_screenPosition;
    }

    void SetScreenPos(Coord position) {
        m_screenPosition = position;
    }

    void SetScreenPos(i32 x, i32 y) {
        SetScreenPos(Coord(x, y));
    }

    inline void SetDrawFill(ShadeMode mode, CShadeTable* table) {
        m_drawActive = true;
        m_drawFillCmd = mode;
        m_drawFillArg = table;
    }

    inline void SetDrawFillFraction(ShadeMode mode, i32 fraction) {
        m_drawActive = true;
        m_drawFillCmd = mode;
        m_fillFraction = fraction;
    }

    CResolveNode();

    CResolveNode(CDDrawSurfaceMgr* owner, i32 id, i32 flags);

    enum EInlineSeed {
        INLINE_SEED
    };
    CResolveNode(CDDrawSurfaceMgr* owner, i32 id, i32 flags, EInlineSeed);

    enum ENoSeed {
        NO_SEED
    };
    CResolveNode(ENoSeed) : m_dirty(WwdDirtyRect::NO_SEED) {}
    i32 Init(
        CDDrawSurfaceMgr* owner,
        i32 id,
        i32 resolveX,
        i32 resolveY,
        GZ_ENUM_PARAM(SpriteStateFlags, i32) stateFlags,
        i32 flags
    );

    virtual ~CResolveNode() OVERRIDE {
        m_screenPosition.m_x = COORD_UNSET;
        m_dirty.Reset();
    }

    Coord m_plotOffset;

    WwdDirtyRect m_dirty;

    class CGameLevel* m_level;
    SpriteStateFlags m_stateFlags;
    i32 m_flashCountdown;
    i32 m_flashInterval;
    CShadeTable* m_drawFillArg;
    ShadeMode m_drawFillCmd;
    i32 m_fillFraction;
    b32 m_drawActive;
    Coord m_screenPosition;

    RECT m_clip;
};

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H

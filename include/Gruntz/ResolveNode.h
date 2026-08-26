#ifndef GRUNTZ_GRUNTZ_RESOLVENODE_H
#define GRUNTZ_GRUNTZ_RESOLVENODE_H

#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>
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
    i32 m_lastX;
    i32 m_lastY;
    RECT m_rect;
    i32 m_w;
    i32 m_h;
    i32 m_armed;
};

class CResolveNode : public CWapObj {
public:
    virtual i32 IsLoaded() OVERRIDE;
    RVA(0x00154a80, 0x13)
    virtual void Unload() OVERRIDE {
        m_screenX = COORD_UNSET;
        m_dirty.Reset();
    }

    virtual i32 SetPosition(i32 x, i32 y);

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
        m_screenX = COORD_UNSET;
        m_dirty.Reset();
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
    b32 m_drawActive;
    i32 m_screenX;

    i32 m_screenY;

    RECT m_clip;
};

#define SET_DRAW_FILL(node, mode, table)                                                           \
    node->m_drawActive = true;                                                                     \
    node->m_drawFillCmd = mode;                                                                    \
    node->m_drawFillArg = table

#define SET_DRAW_FILL_REVERSED(node, mode, table)                                                  \
    node->m_drawActive = true;                                                                     \
    node->m_drawFillArg = table;                                                                   \
    node->m_drawFillCmd = mode

#define SET_DRAW_FILL_SPLIT(activeNode, node, mode, table)                                         \
    activeNode->m_drawActive = true;                                                               \
    node->m_drawFillCmd = mode;                                                                    \
    node->m_drawFillArg = table

#define SET_DRAW_FILL_ARG_FIRST(node, mode, table)                                                 \
    node->m_drawFillArg = table;                                                                   \
    node->m_drawActive = true;                                                                     \
    node->m_drawFillCmd = mode

#define SET_DRAW_FILL_FRACTION(node, mode, fraction)                                               \
    node->m_drawActive = true;                                                                     \
    node->m_drawFillCmd = mode;                                                                    \
    node->m_fillFraction = fraction

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H

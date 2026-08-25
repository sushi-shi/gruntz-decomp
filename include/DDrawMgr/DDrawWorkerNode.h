#ifndef GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
#define GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

#include <rva.h>

#include <Gruntz/ResolveNode.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

class CDDrawWorker;

class CDDrawSurfacePair;

class CDDrawWorkerBase : public CResolveNode {
public:
    virtual ~CDDrawWorkerBase() OVERRIDE {
        m_dirty.Reset();
    }

    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetPosition(i32 x, i32 y) OVERRIDE;

    virtual void RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay);

    i32 m_refCount;

    union {
        i32 m_frameValue;
        class CImage* m_frame;
        char m_pixelValue;
    };

    CDDrawWorkerBase() {}

    CDDrawWorkerBase(CDDrawSurfaceMgr* ctx) : CResolveNode(NO_SEED) {
        m_id = 0;
        m_ownerCtx = ctx;
        m_flags = 0;
        m_dirty.m_rect.left = COORD_UNSET;
        m_dirty.m_armed = -1;
        m_screenX = COORD_UNSET;
        m_clip.left = COORD_UNSET;
        m_level = NULL;
        m_stateFlags = SPRITE_STATE_NONE;
    }
};

// +0x78 holds a raw pixel VALUE, not a CImage*, so the three CDDrawWorkerBase
// virtuals that treat it as a pointer are overridden back (retail vtable
// 0x1efea0 slots 5/7/8 point at 0x157060/0x157130/0x1570a0, not the base's
// 0x157200/0x157310/0x157210).
struct CDDrawWorkerA : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerA() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual void RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) OVERRIDE;
    CDDrawWorkerA() {}
    CDDrawWorkerA(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_pixelValue = 0;
    }
    virtual i32 PlacePixel(i32 x, i32 y, i32 pixelValue);
};

struct CDDrawWorkerB : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerB() OVERRIDE;

    virtual void RenderFrame(CDDrawSurfacePair* backBuffer, CDDrawSurfacePair* overlay) OVERRIDE;
    CDDrawWorkerB() {}
    CDDrawWorkerB(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_frameValue = 0;
    }
    virtual i32 PlaceFrame(i32 x, i32 y, const char* workerName, i32 frameIndex);
    virtual i32 PlaceFrame(i32 x, i32 y, CDDrawWorker* source, i32 frameIndex);
    virtual i32 PlaceFrame(i32 x, i32 y, CImage* frame);

    i32 ResolveFrame(const char* workerName, i32 frameIndex);
};

#endif // GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

#ifndef GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H
#define GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

#include <rva.h>

#include <Gruntz/ResolveNode.h>
#include <Ints.h>

class CDDrawSurfaceMgr;

class CDDrawWorker;

class CDDrawSurfacePair;

VTBL_ABSENT(CDDrawWorkerBase);
class CDDrawWorkerBase : public CResolveNode {
public:
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual i32 GetClassId() OVERRIDE;

    virtual i32 SetPosition(i32 x, i32 y) OVERRIDE;

    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b);

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
        m_dirty.m_rect.left = static_cast<i32>(0x80000000);
        m_dirty.m_armed = -1;
        m_screenX = static_cast<i32>(0x80000000);
        m_clip.left = static_cast<i32>(0x80000000);
        m_level = 0;
        m_stateFlags = 0;
    }
};
SIZE(0x7c);

struct CDDrawWorkerA : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerA() OVERRIDE;

    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    CDDrawWorkerA() {}
    CDDrawWorkerA(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_pixelValue = 0;
    }
    virtual i32 PlaceFrameValue(i32 x, i32 y, i32 frame);
};
SIZE(0x7c);

struct CDDrawWorkerB : public CDDrawWorkerBase {
    virtual ~CDDrawWorkerB() OVERRIDE;

    virtual void RenderFrame(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    CDDrawWorkerB() {}
    CDDrawWorkerB(CDDrawSurfaceMgr* ctx) : CDDrawWorkerBase(ctx) {
        m_frameValue = 0;
    }
    virtual i32 PlaceFrameValue(i32 x, i32 y, i32 frame);
    virtual i32 PlaceFrame(i32 x, i32 y, CDDrawWorker* src, i32 frameIndex);

    virtual i32 PlaceBound(i32 x, i32 y, const char* key, i32 frameIndex);

    i32 Helper(const char* key, i32 idx);
};
SIZE(0x7c);

#endif // GRUNTZ_GRUNTZ_CDDRAWWORKERNODE_H

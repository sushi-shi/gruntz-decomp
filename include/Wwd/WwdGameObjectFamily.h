#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

#include <Ints.h>
#include <Mfc.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <rva.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/WwdGridIter.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Wwd/WwdObjMgr.h>

class CDDrawSurfacePair;
class CWwdGameObject;
class CDDrawWorker;

class CImage;
struct LeafCue;
class CAniElement;

#define WORKER_FREE(p)                                                                             \
    do {                                                                                           \
        if (p) {                                                                                   \
            delete (p);                                                                            \
            (p) = 0;                                                                               \
        }                                                                                          \
    } while (0)

struct CGameObject : public CResolveNode {
public:
    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags);
    virtual ~CGameObject() OVERRIDE {
        Unload();
    }

    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015b5d0, 0x7c)
    virtual void Unload() OVERRIDE {
        WORKER_FREE(m_animWorker);
        WORKER_FREE(m_hitWorker);
        WORKER_FREE(m_attackWorker);
        WORKER_FREE(m_collideWorker);
        m_shadow.m_rect.left = static_cast<i32>(0x80000000);
        m_shadow.m_armed = -1;
        m_screenX = static_cast<i32>(0x80000000);
        m_dirty.m_rect.left = static_cast<i32>(0x80000000);
        m_dirty.m_armed = -1;
    }

    virtual i32 Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl);

    virtual void Render(CDDrawSurfacePair* ctx) = 0;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) = 0;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) = 0;
    virtual void
    BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) = 0;

    virtual i32 Play(CFileMemBase* ar, i32 mode, i32 typeId, void* self);

    void Notify(void* p);

    i32 Serialize(CFileMemBase* ar);
    i32 WriteSnapshot(CFileMemBase* dst, i32 unused);
    i32 SerializeObjectState(CFileMemBase* ar);
    i32 ResolveLinkedObject(i32 gate);

    i32 EnsureWorker80(AnimWorkerObj* src);
    i32 EnsureWorker88(AnimWorkerObj* src);
    i32 EnsureWorker90(AnimWorkerObj* src);
    void AddLogicHit(char* key);
    void AddLogicAttack(char* key);
    void AddLogicBump(char* key);
    i32 NotifyHooked(i32 arg);

    i32 m_sortKey;

    POSITION m_posCache;

    AnimWorkerObj* m_animWorker;
    AnimWorkerObj* m_hitWorker;
    CGameObject* m_84;
    AnimWorkerObj* m_attackWorker;
    CGameObject* m_8c;
    AnimWorkerObj* m_collideWorker;
    CGameObject* m_hitOther;

    CGameObject* m_carrier;

    WwdRegion m_region;

    WwdDirtyRect m_shadow;

    CString m_dc;

    i32 m_e0;

    i32 m_moveMode;
    u32 m_collCategory;

    i32 m_ec;
    i32 m_f0;

    u32 m_collMask;
    i32 m_strideX;
    i32 m_strideY;
    i32 m_100;
    i32 m_104;
    i32 m_108;
    i32 m_10c;
    i32 m_110;
    i32 m_114;
    i32 m_118;
    i32 m_11c;
    i32 m_120;

    i32 m_124;
    i32 m_placeMode;

    i32 m_12c;
    i32 m_130;

    RECT m_extent;

    RECT m_area;

    RECT m_switchRect;

    i32 m_164;
    i32 m_168;
    i32 m_16c;
    i32 m_170;
    i32 m_deltaX;
    i32 m_deltaY;
    i32 m_17c;
    i32 m_180;
    i32 m_184;
    i32 m_188;
};
SIZE_UNKNOWN();

#ifndef CGAMEOBJECT_OOL_CTOR
inline CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CResolveNode(owner, id, stateFlags) {
    m_screenX = static_cast<i32>(0x80000000);
    m_posCache = 0;
    m_animWorker = new AnimWorkerObj(owner, id, 0);
    m_carrier = 0;
    m_hitWorker = 0;
    m_attackWorker = 0;
    m_collideWorker = 0;
    m_188 = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}
#endif

class CWwdGameObjectA : public CGameObject {
public:
    CWwdGameObjectA(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CGameObject(owner, id, stateFlags), m_1a0(owner, id, stateFlags) {
        m_18c = -1;
        m_190 = -1;
        m_layer = 0;
        m_194 = 0;
        m_19c = 0;
    }
    virtual ~CWwdGameObjectA() OVERRIDE;

    RVA(0x0015b980, 0x96)
    virtual void Unload() OVERRIDE {
        m_18c = -1;
        m_190 = -1;
        m_layer = 0;
        m_194 = 0;
        CGameObject::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;
    virtual i32 Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual i32 Play(CFileMemBase* ar, i32 mode, i32 typeId, void* self) OVERRIDE;

    void ApplyLookupSprite(const char* key, i32 frame);
    void ApplyName(const char* name);
    i32 ApplyLookupGeometry(const char* key, i32 flag);
    i32 LookupAnimSprite(const char* name);
    void ApplyGeometryDirect(CAniElement* srcSprite, i32 applyDefault);
    i32 Test();

    void ClampFirst();
    void ClampLast();
    i32 SerializeSpriteName(CFileMemBase* ar);
    i32 ReadState(CFileMemBase* src);

    i32 m_18c;
    i32 m_190;
    union {

        char* m_194;
        CDDrawWorker* m_sprite;
        CDDrawWorker* m_imageSet;
    };
    CImage* m_layer;
    union {

        LeafCue* m_19c;
        CDDrawWorker* m_19cSprite;
    };
    CAniAdvanceCursor m_1a0;
};
SIZE(0x1dc);

class CWwdGameObject : public CWwdGameObjectA {
public:
    CWwdGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CWwdGameObjectA(owner, id, stateFlags), m_1dc(0xa) {
        m_1f8 = 0;
    }
    virtual ~CWwdGameObject() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015bf00, 0xa1)
    virtual void Unload() OVERRIDE {
        Clear();
        m_1f8 = 0;
        m_18c = -1;
        m_190 = -1;
        m_layer = 0;
        m_194 = 0;
        CGameObject::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;

    virtual i32 Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;

    void Clear();
    i32 AddChild(CGameObject* child);
    i32 RemoveChild(CGameObject* child);
    i32 WalkChildWorkers();

    CWwdGameObject*
    CreateObject(int id, int x, int y, int sortKey, AnimWorkerObj* tmpl, int stateFlags);
    CWwdGameObject*
    CreateNamed(int id, int x, int y, int sortKey, const char* name, int stateFlags);

    CObList m_1dc;

    i32 m_1f8;
};
SIZE(0x1fc);

class CWwdGameObjectF : public CGameObject {
public:
    CWwdGameObjectF(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CGameObject(owner, id, stateFlags) {}
    virtual ~CWwdGameObjectF() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015bc50, 0x7c)
    virtual void Unload() OVERRIDE {
        CGameObject::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;

    virtual i32 SetupDeferred(i32 sortKey, AnimWorkerObj* tmpl);
};
SIZE(0x18c);

class CWwdGameObjectC : public CGameObject {
public:
    CWwdGameObjectC(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CGameObject(owner, id, stateFlags) {
        m_dotColor = 0;
    }
    virtual ~CWwdGameObjectC() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015c200, 0x82)
    virtual void Unload() OVERRIDE {
        m_dotColor = 0;
        CGameObject::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;

    virtual i32 SetupFlagged(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl, i32 flag);
    virtual u8 GetDotColor();
    virtual void SetDotColor(u8 c8);

    u8 m_dotColor;
    char _p18d[0x190 - 0x18d];
};
SIZE(0x190);

inline CGameObject* CDDrawChildGroup::NextChild(POSITION& pos) {
    return static_cast<CGameObject*>(m_list.GetNext(pos));
}
inline CGameObject* CDDrawChildGroup::HeadChild() const {
    return static_cast<CGameObject*>(m_list.GetHead());
}

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

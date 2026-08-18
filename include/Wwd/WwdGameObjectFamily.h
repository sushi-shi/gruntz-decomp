#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WwdGridIter.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdObjMgr.h>

GZ_ENUM_FORWARD(MoveMode);

class CDDrawSurfacePair;
class CWwdGameObject;
class CDDrawWorker;

class CImage;
struct LeafCue;
class CAniElement;

struct CGameObject : public CResolveNode {
public:
    // Tag type: picks the inline sibling of the out-of-line 0x15b390 ctor.
    enum EInlineBase {
        INLINE_BASE
    };

    // Same expansion, but m_region's ctor is expanded too (`call ??0WwdGridNode`
    // + the m_object store).  CWwdGameObjectC / CWwdGameObjectF take this one;
    // CWwdGameObjectA, whose m_animCursor eats the rest of the inline budget,
    // keeps the plain `call ??0WwdRegion` and takes INLINE_BASE above.
    enum EInlineBaseAndRegion {
        INLINE_BASE_AND_REGION
    };

    // Out of line at 0x15b390.  Retail `call`s it from CWwdGameObject's chain and
    // from the two CWwdGameObjectA sites in LevelPlane / WwdGameObjectRender.
    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags);
    // The expanded sibling: CreateSpriteObject / CreateDotObject /
    // CreateDeferredObject carry this body inline.
    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags, EInlineBase);
    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags, EInlineBaseAndRegion);
    virtual ~CGameObject() OVERRIDE {
        Unload();
    }

    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015b5d0, 0x7c)
    virtual void Unload() OVERRIDE {
        if (m_animWorker) {
            delete m_animWorker;
            m_animWorker = 0;
        }
        if (m_hitWorker) {
            delete m_hitWorker;
            m_hitWorker = 0;
        }
        if (m_attackWorker) {
            delete m_attackWorker;
            m_attackWorker = 0;
        }
        if (m_collideWorker) {
            delete m_collideWorker;
            m_collideWorker = 0;
        }
        m_shadow.Reset();
        m_screenX = COORD_UNSET;
        m_dirty.Reset();
    }

    virtual i32 Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl);

    virtual void Render(CDDrawSurfacePair* ctx) = 0;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) = 0;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) = 0;
    virtual void
    BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c) = 0;

    virtual i32 Play(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* self);

    void Notify(void* p);

    i32 Serialize(CFileMemBase* ar);
    i32 WriteSnapshot(CFileMemBase* dst, LogicTypeId unused);
    i32 SerializeObjectState(CFileMemBase* ar);
    i32 ResolveLinkedObject(i32 gate);

    i32 EnsureHitWorker(AnimWorkerObj* src);
    i32 EnsureAttackWorker(AnimWorkerObj* src);
    i32 EnsureBumpWorker(AnimWorkerObj* src);
    void AddLogicHit(char* key);
    void AddLogicAttack(char* key);
    void AddLogicBump(char* key);
    i32 NotifyHooked(i32 arg);

    void AttachToOwner(CDDrawSurfaceMgr* owner, i32 id);

    i32 m_sortKey;

    POSITION m_posCache;

    AnimWorkerObj* m_animWorker;
    AnimWorkerObj* m_hitWorker;
    CGameObject* m_hitSource;
    AnimWorkerObj* m_attackWorker;
    CGameObject* m_attackTarget;
    AnimWorkerObj* m_collideWorker;
    CGameObject* m_hitOther;

    CGameObject* m_carrier;

    WwdRegion m_region;

    WwdDirtyRect m_shadow;

    CString m_name;

    i32 m_reservede0;

    MoveMode m_moveMode;
    u32 m_objectType;

    i32 m_hitTypeFlags;
    i32 m_attackTypeMask;

    u32 m_collMask;
    i32 m_strideX;
    i32 m_strideY;
    i32 m_reserved100;
    i32 m_spawnX;
    i32 m_spawnY;
    i32 m_spawnSortKey;
    i32 m_reserved110;
    i32 m_score;
    i32 m_points;
    i32 m_powerup;
    i32 m_damage;

    i32 m_smarts;
    i32 m_health;

    i32 m_direction;
    i32 m_faceDirection;

    RECT m_extent;

    RECT m_area;

    RECT m_switchRect;

    i32 m_speedX;
    i32 m_speedY;
    i32 m_reserved16c;
    i32 m_reserved170;
    i32 m_deltaX;
    i32 m_deltaY;
    i32 m_reserved17c;
    i32 m_reserved180;
    i32 m_carrierId;
    i32 m_objectId;
};

// The one textual copy of the ctor body.  Both CGameObject ctor entities expand it.
inline void CGameObject::AttachToOwner(CDDrawSurfaceMgr* owner, i32 id) {
    m_screenX = COORD_UNSET;
    m_posCache = NULL;
    m_animWorker = new AnimWorkerObj(owner, id, 0);
    m_carrier = NULL;
    m_hitWorker = NULL;
    m_attackWorker = NULL;
    m_collideWorker = NULL;
    m_objectId = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}

inline CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags, EInlineBase)
    : CResolveNode(owner, id, stateFlags) {
    AttachToOwner(owner, id);
}

inline CGameObject::CGameObject(
    CDDrawSurfaceMgr* owner,
    i32 id,
    i32 stateFlags,
    EInlineBaseAndRegion
)
    : CResolveNode(owner, id, stateFlags), m_region(WwdRegion::BASE_CALL) {
    AttachToOwner(owner, id);
}

class CWwdGameObjectA : public CGameObject {
public:
    // Calls the pinned base ctor: CWwdGameObject's chain and LevelPlane's
    // ReadPlaneObjects take this one.  These sites carry CAniAdvanceCursor's ctor
    // EXPANDED (`call ??0CLoadable`, then the 0x5f0128 vptr and the three NULLs) -
    // 0x15b730 has exactly one retail caller, CreateSpriteObject, which is the
    // INLINE_BASE overload below.
    CWwdGameObjectA(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CGameObject(owner, id, stateFlags),
          m_animCursor(owner, id, stateFlags, CAniAdvanceCursor::INLINE_CURSOR) {
        ResetSpriteFields();
    }
    // Same, except the cursor's own CWapObj base is expanded too - the one site
    // that shows it is CWwdGameObject::CreateObject.
    CWwdGameObjectA(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags, CWapObj::ENoSeed)
        : CGameObject(owner, id, stateFlags),
          m_animCursor(owner, id, stateFlags, CWapObj::NO_SEED) {
        ResetSpriteFields();
    }
    // Expands the base ctor: CDDrawChildGroup::CreateSpriteObject takes this one.
    CWwdGameObjectA(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags, EInlineBase)
        : CGameObject(owner, id, stateFlags, INLINE_BASE), m_animCursor(owner, id, stateFlags) {
        ResetSpriteFields();
    }
    void ResetSpriteFields() {
        m_reserved18c = -1;
        m_frameIndex = -1;
        m_layer = NULL;
        m_frameSet = NULL;
        m_soundCue = NULL;
    }
    virtual ~CWwdGameObjectA() OVERRIDE {
        Unload();
    }

    RVA(0x0015b980, 0x96)
    virtual void Unload() OVERRIDE {
        m_reserved18c = -1;
        m_frameIndex = -1;
        m_layer = NULL;
        m_frameSet = NULL;
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;
    virtual i32 Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual i32 Play(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* self)
        OVERRIDE;

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

    i32 m_reserved18c; // reset to -1 with m_frameIndex; never read
    i32 m_frameIndex;
    CDDrawWorker* m_frameSet;
    CImage* m_layer;
    LeafCue* m_soundCue;
    CAniAdvanceCursor m_animCursor;
};

class CWwdGameObject : public CWwdGameObjectA {
public:
    CWwdGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CWwdGameObjectA(owner, id, stateFlags), m_children(0xa) {
        m_reserved1f8 = 0;
    }
    virtual ~CWwdGameObject() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015bf00, 0xa1)
    virtual void Unload() OVERRIDE {
        Clear();
        m_reserved1f8 = 0;
        m_reserved18c = -1;
        m_frameIndex = -1;
        m_layer = NULL;
        m_frameSet = NULL;
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;

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

    CObList m_children;

    i32 m_reserved1f8; // zeroed in ctor/Unload only
};

class CWwdGameObjectF : public CGameObject {
public:
    // Only created by CDDrawChildGroup::CreateDeferredObject, which expands the base.
    CWwdGameObjectF(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CGameObject(owner, id, stateFlags, INLINE_BASE_AND_REGION) {}
    virtual ~CWwdGameObjectF() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015bc50, 0x7c)
    virtual void Unload() OVERRIDE {
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE;
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, CDDrawSurfacePair* c)
        OVERRIDE;

    virtual i32 SetupDeferred(i32 sortKey, AnimWorkerObj* tmpl);
};

class CWwdGameObjectC : public CGameObject {
public:
    // Only created by CDDrawChildGroup::CreateDotObject, which expands the base.
    CWwdGameObjectC(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
        : CGameObject(owner, id, stateFlags, INLINE_BASE_AND_REGION) {
        m_dotColor = 0;
    }
    virtual ~CWwdGameObjectC() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015c200, 0x82)
    virtual void Unload() OVERRIDE {
        m_dotColor = 0;
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;
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

inline CGameObject* CDDrawChildGroup::NextChild(POSITION& pos) {
    return static_cast<CGameObject*>(m_list.GetNext(pos));
}
inline CGameObject* CDDrawChildGroup::HeadChild() const {
    return static_cast<CGameObject*>(m_list.GetHead());
}

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

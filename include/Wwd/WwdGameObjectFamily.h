#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/LogicRecord.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WwdGridIter.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdGameObjectFlags.h>
#include <Wwd/WwdObjMgr.h>

GZ_ENUM_FORWARD(MoveMode);

class CDDrawSurfacePair;
class CDrawSubWorker;
class CWwdGameObject;
class CDDrawWorker;

class CImage;
struct SoundCue;
class CAniElement;

struct CGameObject : public CResolveNode {
public:
    enum EInlineBase {
        INLINE_BASE
    };

    enum EInlineBaseAndRegion {
        INLINE_BASE_AND_REGION
    };

    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags);
    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags, EInlineBase);
    CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags, EInlineBaseAndRegion);
    virtual ~CGameObject() OVERRIDE {
        Unload();
    }

    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015b5d0, 0x7c)
    virtual void Unload() OVERRIDE {
        if (m_logicRecord) {
            delete m_logicRecord;
            m_logicRecord = NULL;
        }
        if (m_hitLogic) {
            delete m_hitLogic;
            m_hitLogic = NULL;
        }
        if (m_attackLogic) {
            delete m_attackLogic;
            m_attackLogic = NULL;
        }
        if (m_collisionLogic) {
            delete m_collisionLogic;
            m_collisionLogic = NULL;
        }
        m_shadow.Reset();
        m_screenPosition.m_x = COORD_UNSET;
        m_dirty.Reset();
    }

    virtual i32 Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate);

    virtual void Render(CDDrawSurfacePair* ctx) = 0;
    virtual void BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) = 0;
    virtual void
    BltDirtyEx(CDrawSubWorker* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc) = 0;
    virtual void BltDirtyRegions(
        CDDrawSurfacePair* dst,
        CDDrawSurfacePair* src,
        CDDrawSurfacePair* restoreSrc
    ) = 0;

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);

    void Notify(CGameObject* p);

    i32 AttackBits(CGameObject* target) const;

    i32 PrepareSave(CFileMemBase* ar);
    i32 Serialize(CFileMemBase* ar);
    i32 WriteSnapshot(CFileMemBase* dst, LogicTypeId unused);
    i32 SerializeObjectState(CFileMemBase* ar);
    i32 ResolveLinkedObject(b32 gate);

    i32 EnsureHitLogic(CLogicRecord* logicTemplate);
    i32 EnsureAttackLogic(CLogicRecord* logicTemplate);
    i32 EnsureBumpLogic(CLogicRecord* logicTemplate);
    void AddLogicHit(char* key);
    void AddLogicAttack(char* key);
    void AddLogicBump(char* key);
    i32 NotifyForEventCode(i32 eventCode);

    void AttachToOwner(CDDrawSurfaceMgr* owner, i32 id);

    inline void SetSortKey(i32 sortKey) {
        if (m_sortKey != sortKey) {
            m_sortKey = sortKey;
            m_flags |= IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING);
        }
    }

    i32 m_sortKey;

    POSITION m_posCache;

    CLogicRecord* m_logicRecord;
    CLogicRecord* m_hitLogic;
    CGameObject* m_hitSource;
    CLogicRecord* m_attackLogic;
    CGameObject* m_attackTarget;
    CLogicRecord* m_collisionLogic;
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
    Coord m_stride;
    i32 m_reserved100;
    Coord m_spawnPosition;
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

    Coord m_speed;
    i32 m_reserved16c;
    i32 m_reserved170;
    Coord m_delta;
    i32 m_reserved17c;
    i32 m_reserved180;
    i32 m_carrierId;
    i32 m_objectId;
};

inline i32 CGameObject::AttackBits(CGameObject* target) const {
    return static_cast<i32>(target->m_objectType) & m_attackTypeMask;
}

inline void CGameObject::AttachToOwner(CDDrawSurfaceMgr* owner, i32 id) {
    m_screenPosition.m_x = COORD_UNSET;
    m_posCache = NULL;
    m_logicRecord = new CLogicRecord(owner, id, 0);
    m_carrier = NULL;
    m_hitLogic = NULL;
    m_attackLogic = NULL;
    m_collisionLogic = NULL;
    m_objectId = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}

inline CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags, EInlineBase)
    : CResolveNode(owner, id, objectFlags) {
    AttachToOwner(owner, id);
}

inline CGameObject::CGameObject(
    CDDrawSurfaceMgr* owner,
    i32 id,
    i32 objectFlags,
    EInlineBaseAndRegion
)
    : CResolveNode(owner, id, objectFlags), m_region(WwdRegion::BASE_CALL) {
    AttachToOwner(owner, id);
}

class CWwdSpriteObject : public CGameObject {
public:
    CWwdSpriteObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags)
        : CGameObject(owner, id, objectFlags),
          m_animationCursor(owner, id, objectFlags, CAniAdvanceCursor::INLINE_CURSOR) {
        ResetSpriteFields();
    }
    CWwdSpriteObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags, CWapObj::ENoSeed)
        : CGameObject(owner, id, objectFlags),
          m_animationCursor(owner, id, objectFlags, CWapObj::NO_SEED) {
        ResetSpriteFields();
    }
    CWwdSpriteObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags, EInlineBase)
        : CGameObject(owner, id, objectFlags, INLINE_BASE),
          m_animationCursor(owner, id, objectFlags) {
        ResetSpriteFields();
    }
    void ResetSpriteFields() {
        m_reserved18c = -1;
        m_frameIndex = -1;
        m_frameImage = NULL;
        m_imageSet = NULL;
        m_soundCue = NULL;
    }
    virtual ~CWwdSpriteObject() OVERRIDE {
        Unload();
    }

    RVA(0x0015b980, 0x96)
    virtual void Unload() OVERRIDE {
        m_reserved18c = -1;
        m_frameIndex = -1;
        m_frameImage = NULL;
        m_imageSet = NULL;
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;
    virtual i32 Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) OVERRIDE;
    virtual void
    BltDirtyEx(CDrawSubWorker* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc) OVERRIDE;
    virtual void
    BltDirtyRegions(CDDrawSurfacePair* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc)
        OVERRIDE;
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE;

    void SetImageFrameByName(const char* key, i32 frame);
    void SetImageSetByName(const char* name);
    i32 SetAnimationByName(const char* key, i32 advanceImmediately);
    i32 SetSoundCueByName(const char* name);
    void SetAnimation(CAniElement* animation, i32 advanceImmediately);
    i32 IntersectsViewport();

    void ClampToFirstFrame();
    void ClampToLastFrame();
    i32 ReadSpriteState(CFileMemBase* stream);
    i32 WriteSpriteState(CFileMemBase* stream);

    i32 m_reserved18c; // reset to -1 with m_frameIndex; never read
    i32 m_frameIndex;
    CDDrawWorker* m_imageSet;
    CImage* m_frameImage;
    SoundCue* m_soundCue;
    CAniAdvanceCursor m_animationCursor;
};

class CWwdGameObject : public CWwdSpriteObject {
public:
    CWwdGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags)
        : CWwdSpriteObject(owner, id, objectFlags), m_children(0xa) {
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
        m_frameImage = NULL;
        m_imageSet = NULL;
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) OVERRIDE;
    virtual void
    BltDirtyEx(CDrawSubWorker* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc) OVERRIDE;
    virtual void
    BltDirtyRegions(CDDrawSurfacePair* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc)
        OVERRIDE;

    void Clear();
    i32 AddChild(CGameObject* child);
    i32 RemoveChild(CGameObject* child);
    i32 WalkChildWorkers();

    CWwdGameObject*
    CreateObject(int id, int x, int y, int sortKey, CLogicRecord* logicTemplate, int objectFlags);
    CWwdGameObject*
    CreateNamed(int id, int x, int y, int sortKey, const char* name, int objectFlags);

    CObList m_children;

    i32 m_reserved1f8; // zeroed in ctor/Unload only
};

class CWwdDeferredObject : public CGameObject {
public:
    CWwdDeferredObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags)
        : CGameObject(owner, id, objectFlags, INLINE_BASE_AND_REGION) {}
    virtual ~CWwdDeferredObject() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015bc50, 0x7c)
    virtual void Unload() OVERRIDE {
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) OVERRIDE;
    virtual void
    BltDirtyEx(CDrawSubWorker* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc) OVERRIDE;
    virtual void
    BltDirtyRegions(CDDrawSurfacePair* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc)
        OVERRIDE;

    virtual i32 SetupDeferred(i32 sortKey, CLogicRecord* logicTemplate);
};

class CWwdDotObject : public CGameObject {
public:
    CWwdDotObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags)
        : CGameObject(owner, id, objectFlags, INLINE_BASE_AND_REGION) {
        m_dotColor = 0;
    }
    virtual ~CWwdDotObject() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    RVA(0x0015c200, 0x82)
    virtual void Unload() OVERRIDE {
        m_dotColor = 0;
        CGameObject::Unload();
    }
    virtual LoadableClassId GetClassId() OVERRIDE;
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;
    virtual void BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) OVERRIDE;
    virtual void
    BltDirtyEx(CDrawSubWorker* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc) OVERRIDE;
    virtual void
    BltDirtyRegions(CDDrawSurfacePair* dst, CDDrawSurfacePair* src, CDDrawSurfacePair* restoreSrc)
        OVERRIDE;

    virtual i32 SetupDot(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate, i32 dotColor);
    virtual u8 GetDotColor();
    virtual void SetDotColor(u8 dotColor);

    u8 m_dotColor;
    char _p18d[0x190 - 0x18d];
};

#define NEXT_CHILD_FROM_LIST(list, pos) static_cast<CGameObject*>(list.GetNext(pos))

inline CGameObject* CDDrawChildGroup::NextChild(POSITION& pos) {
    return static_cast<CGameObject*>(m_list.GetNext(pos));
}
inline CGameObject* CDDrawChildGroup::HeadChild() const {
    return static_cast<CGameObject*>(m_list.GetHead());
}

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

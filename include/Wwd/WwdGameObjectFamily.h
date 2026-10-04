#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawWorker.h>
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

class CRenderBuffer;
class CRenderSurface;
class CWwdGameObject;
class CImageSet;

class CImage;
struct SoundCue;
class CAnimationSequence;

struct CGameObject : public CRenderState {
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
        CRenderState::Unload();
    }

    virtual i32 Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate);

    virtual void Render(CRenderBuffer* ctx) = 0;
    virtual void BltDirty(CRenderBuffer* dst, CRenderBuffer* src) = 0;
    virtual void BltDirtyEx(CRenderSurface* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc) = 0;
    virtual void
    BltDirtyRegions(CRenderBuffer* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc) = 0;

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object);

    void Notify(CGameObject* p);
    i32 NotifyCollision(CGameObject* other) {
        m_hitOther = other;
        LogicRecordDispatchFn dispatch = m_collisionLogic->GetDispatch();
        return dispatch(this);
    }

    MoveMode GetMoveMode() const {
        return m_moveMode;
    }
    void SetMoveMode(MoveMode mode) {
        m_moveMode = mode;
    }

    i32 GetFaceDirection() const {
        return m_faceDirection;
    }

    i32 GetPowerup() const {
        return m_powerup;
    }
    void SetPowerup(i32 powerup) {
        m_powerup = powerup;
    }

    i32 GetDamage() const {
        return m_damage;
    }
    void SetDamage(i32 damage) {
        m_damage = damage;
    }

    i32 GetScore() const {
        return m_score;
    }
    void SetScore(i32 score) {
        m_score = score;
    }

    i32 GetPoints() const {
        return m_points;
    }
    void SetPoints(i32 points) {
        m_points = points;
    }

    void SetSpeedX(i32 speed) {
        m_speedX = speed;
    }
    void SetSpeedY(i32 speed) {
        m_speedY = speed;
    }

    i32 GetObjectId() const {
        return m_objectId;
    }

    BOOL HasMovementBounds() const {
        return m_extent.left != COORD_UNSET;
    }
    BOOL HasHitBounds() const {
        return m_area.left != COORD_UNSET;
    }
    BOOL HasAttackBounds() const {
        return m_switchRect.left != COORD_UNSET;
    }

    i32 AttackBits(CGameObject* target) const;
    i32 CollisionBits(CGameObject* target) const {
        return static_cast<i32>(target->m_objectType) & m_collMask;
    }
    RECT& ExtentAt(const i32& x, const i32& y, RECT& bounds) const {
        i32 left = m_extent.left + x;
        i32 top = y;
        top += m_extent.top;
        i32 right = x + m_extent.right;
        i32 bottom = m_extent.bottom + y;
        bounds.left = left;
        bounds.top = top;
        bounds.right = right;
        bounds.bottom = bottom;
        return bounds;
    }

    BOOL ExtentsOverlapAt(const i32& x, const i32& y, CGameObject* b, RECT& bounds) const {
        const RECT& aBounds = ExtentAt(x, y, bounds);
        i32 bLeft = b->m_screenX + b->m_extent.left;
        i32 bTop = b->m_extent.top + b->m_screenY;
        i32 bBottom = b->m_screenY + b->m_extent.bottom;
        i32 bRight = b->m_screenX + b->m_extent.right;
        return aBounds.left <= bRight && aBounds.right >= bLeft && aBounds.top <= bBottom
               && aBounds.bottom >= bTop;
    }

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

    const i32& GetSortKey() const {
        return m_sortKey;
    }

    void SetSortKey(i32 key) {
        if (m_sortKey != key) {
            m_sortKey = key;
            AddFlags(IDX(WWD_GAME_OBJECT_FLAG_SORT_PENDING));
        }
    }

    i32 m_sortKey;

    POSITION m_posCache;

    CLogicRecord* const& GetLogicRecord() const {
        return m_logicRecord;
    }

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

    // @identity-TODO: this word is only initialized; the later reserved scalar
    // members are save-streamed without a game-object operation reading them.
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

    const i32& GetSmarts() const {
        return m_smarts;
    }
    void SetSmarts(i32 value) {
        m_smarts = value;
    }

    i32 m_smarts;
    i32 GetHealth() const {
        return m_health;
    }
    void SetHealth(i32 health) {
        m_health = health;
    }

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

inline i32 CGameObject::AttackBits(CGameObject* target) const {
    i32 bits = static_cast<i32>(target->m_objectType) & m_attackTypeMask;
    return bits;
}

inline void CGameObject::AttachToOwner(CDDrawSurfaceMgr* owner, i32 id) {
    m_screenX = COORD_UNSET;
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
    : CRenderState(owner, id, objectFlags) {
    AttachToOwner(owner, id);
}

inline CGameObject::CGameObject(
    CDDrawSurfaceMgr* owner,
    i32 id,
    i32 objectFlags,
    EInlineBaseAndRegion
)
    : CRenderState(owner, id, objectFlags), m_region(WwdRegion::BASE_CALL) {
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
    virtual void Render(CRenderBuffer* ctx) OVERRIDE;
    virtual void BltDirty(CRenderBuffer* dst, CRenderBuffer* src) OVERRIDE;
    virtual void BltDirtyEx(CRenderSurface* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
        OVERRIDE;
    virtual void BltDirtyRegions(CRenderBuffer* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
        OVERRIDE;
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE;

    CAniAdvanceCursor& GetAnimationCursor() {
        return m_animationCursor;
    }

    CImageSet* GetImageSet() const {
        return m_imageSet;
    }

    CImage* GetFrameImage() const {
        return m_frameImage;
    }

    void SetImageFrame(i32 frame) {
        CImageSet* imageSet = m_imageSet;
        if (imageSet != NULL) {
            CImage* image = imageSet->GetAt(frame);
            m_frameImage = image;
            m_frameIndex = frame;
        }
    }
    void SetImageFrameByName(const char* key, i32 frame);
    void SetImageSetByName(const char* name);
    i32 SetAnimationByName(const char* key, i32 advanceImmediately);
    i32 SetSoundCueByName(const char* name);
    void SetAnimation(CAnimationSequence* animation, i32 advanceImmediately);
    i32 IntersectsViewport();

    void ClampToFirstFrame();
    void ClampToLastFrame();
    i32 ReadSpriteState(CFileMemBase* stream);
    i32 WriteSpriteState(CFileMemBase* stream);

    i32 m_reserved18c; // reset to -1 with m_frameIndex; never read
    i32 m_frameIndex;
    CImageSet* m_imageSet;
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
    virtual void Render(CRenderBuffer* ctx) OVERRIDE;
    virtual void BltDirty(CRenderBuffer* dst, CRenderBuffer* src) OVERRIDE;
    virtual void BltDirtyEx(CRenderSurface* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
        OVERRIDE;
    virtual void BltDirtyRegions(CRenderBuffer* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
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
    virtual void Render(CRenderBuffer* ctx) OVERRIDE;
    virtual void BltDirty(CRenderBuffer* dst, CRenderBuffer* src) OVERRIDE;
    virtual void BltDirtyEx(CRenderSurface* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
        OVERRIDE;
    virtual void BltDirtyRegions(CRenderBuffer* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
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
    virtual void Render(CRenderBuffer* ctx) OVERRIDE;
    virtual void BltDirty(CRenderBuffer* dst, CRenderBuffer* src) OVERRIDE;
    virtual void BltDirtyEx(CRenderSurface* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
        OVERRIDE;
    virtual void BltDirtyRegions(CRenderBuffer* dst, CRenderBuffer* src, CRenderBuffer* restoreSrc)
        OVERRIDE;

    virtual i32 SetupDot(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate, i32 dotColor);
    virtual u8 GetDotColor();
    virtual void SetDotColor(u8 dotColor);

    u8 m_dotColor;
};

inline CGameObject* CDDrawChildGroup::NextChild(POSITION& pos) {
    return static_cast<CGameObject*>(m_list.GetNext(pos));
}
inline CGameObject* CDDrawChildGroup::HeadChild() const {
    return static_cast<CGameObject*>(m_list.GetHead());
}

#define CLEAR_WWD_GAME_OBJECT_CHILDREN                                                             \
    POSITION pos = m_children.GetHeadPosition();                                                   \
    while (pos != NULL) {                                                                          \
        CObject* child = m_children.GetNext(pos);                                                  \
        if (child != NULL) {                                                                       \
            delete child;                                                                          \
        }                                                                                          \
    }                                                                                              \
    m_children.RemoveAll()

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

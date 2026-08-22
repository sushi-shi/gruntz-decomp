#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WwdGridIter.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdGameObjectFamily.h>

struct CGameObject;
struct LeafCue;
class CDDrawSurfacePair;
class CUserLogic;

class CDDrawWorker;

class CImage;

extern i32 g_logicTypesRegistered;

class CFileMemBase;

class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase() {}
    RVA(0x000087d0, 0x8)
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) {
        return 1;
    }
    RVA(0x000087f0, 0x3)
    virtual LogicTypeId GetTypeTag() {
        return LOGIC_UNSET;
    }
};

class CUserLogic : public CUserBase {
public:
    // Tag type: picks the inline sibling of the out-of-line 0x58cd0 ctor.
    enum EInlineBase {
        INLINE_BASE
    };

    // Two entities, same tag type.  Retail's SerialObjectFactory `call`s
    // ??0CUserLogic@@QAE@XZ (0x138d0) at 45 of its 57 direct-derived `new` sites and
    // expands it at the other 11 (CRollingBall .. CBehindCandyAni), so the split is
    // per CLASS - two-shapes-need-two-entities.md.
    CUserLogic();
    // The expanded sibling for those 11 classes.
    CUserLogic(EInlineBase) {}
    // Out of line at 0x58cd0.  Only CMovingLogic (CGrunt / CProjectile) and
    // CDoNothingNormal reach it - retail's three `call` sites.
    CUserLogic(CGameObject* obj);
    // The expanded sibling: every other derived ctor carries this body inline.
    CUserLogic(CGameObject* obj, EInlineBase);
    virtual ~CUserLogic() OVERRIDE {}
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00008840, 0x4)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

    virtual void XferName(char* name);

    virtual void FireActivation(i32 id);

    virtual void FinalizeStep(char* name);

    RVA(0x000088d0, 0x1)
    virtual void Activate() {}
    RVA(0x000088f0, 0x6)
    virtual i32 AdvanceAnimation() {
        return 1;
    }
    RVA(0x00008910, 0x6)
    virtual i32 RecordFrameTick() {
        return 1;
    }

    RVA(0x00008930, 0x6)
    virtual i32 StepAttackFire() {
        return 1;
    }

    RVA(0x00008950, 0x1)
    virtual void OnLeaveActiveRegion() {}
    RVA(0x00008970, 0x1)
    virtual void OnObjectRemoved() {}
    RVA(0x00008990, 0x1)
    virtual void AfterLoad() {}
    RVA(0x000089b0, 0x1)
    virtual void AfterSave() {}
    RVA(0x000089d0, 0x1)
    virtual void PrepareSave() {}
    RVA(0x000089f0, 0x1)
    virtual void AfterLoadReferences() {}

    void GetScreenPos(Coord* out);

    // Header-inline: retail never emits it out of line - every caller carries
    // the `call GetScreenPos` followed by both halves loaded, `sar 5`-ed and
    // stored back in place.
    void GetScreenTile(Coord* out);

    void RegisterLogicTypesOnce();
    void BuildLogicTypeTable(CGameObject* obj);

    void LoadGruntTuningConstants(i32);

    typedef i32 (CUserLogic::*ActCallback)();
    ActCallback m_deferredCallback;
    ActCallback m_gatedCallback;
    CGameObject* m_logicObject;

    CWwdGameObjectA* m_object;

    AnimWorkerObj* m_objAux;
    zBitVec m_actBits;
    i32 m_gatedActKey;
    i32 m_reserved2c;

    i32 m_prevAnimSetNode;
};

#define SET_ANIMATION_ACT(key)                                                                     \
    m_prevAnimSetNode = m_objAux->m_actKey;                                                        \
    m_objAux->m_actKey = ActFindId(key)

#define ANIMATION_ACT_EQUALS(key) (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), key) == 0)

#define ANIMATION_ACT_DIFFERS(key) (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), key) != 0)

#define ANIMATION_ACT_EQUALS_FOR(logic, key)                                                       \
    (strcmp(*g_typeColl.GetNameRecord(logic->m_objAux->m_actKey), key) == 0)

#define ANIMATION_ACT_DIFFERS_FOR(logic, key)                                                      \
    (strcmp(*g_typeColl.GetNameRecord(logic->m_objAux->m_actKey), key) != 0)

#define APPLY_NAME_INLINE(name) m_wwdObject->ApplyName(name)

#define APPLY_LOOKUP_SPRITE_INLINE(name, frame) m_wwdObject->ApplyLookupSprite(name, frame)

#define ADVANCE_CURRENT_ANIMATION_CURSOR(cursor, elapsed)                                          \
    m_wwdObject->m_animCursor.Advance(elapsed);                                                    \
    CAniAdvanceCursor* cursor = &m_wwdObject->m_animCursor;

#define GET_SCREEN_TILE_Y_FIRST(logic, out)                                                        \
    (logic)->GetScreenPos((&out));                                                                 \
    out.m_y >>= TILE_SHIFT_PX;                                                                     \
    out.m_x >>= TILE_SHIFT_PX;

#define DECLARE_CURRENT_ANIMATION_FRAME(frame, animation, record)                                  \
    CAniElement* animation = m_wwdObject->m_animCursor.m_animation;                                \
    CAniRecordView* record = static_cast<CAniRecordView*>(GetAniElementAt(animation, 0));          \
    i32 frame = record->m_param;

#define SET_OBJECT_FLAGS_INLINE(bits) m_wwdObject->m_flags |= bits

#define HIDE_OBJECT_INLINE() m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN

#define SET_OBJECT_FLAGS_AND_HIDE_INLINE(bits)                                                     \
    SET_OBJECT_FLAGS_INLINE(bits);                                                                 \
    HIDE_OBJECT_INLINE();

#define INITIALIZE_DEFAULT_CYCLE_ANIMATION                                                         \
    SET_ANIMATION_ACT("A");                                                                        \
    if (m_wwdObject->m_animCursor.m_animation == NULL) {                                           \
        SwitchGeometry("GAME_CYCLE100", 0);                                                        \
    }

#define MARK_OBJECT_COMPLETE_IF(condition)                                                         \
    if (condition) {                                                                               \
        SET_OBJECT_FLAGS_INLINE(0x10000);                                                          \
    }

#define APPLY_CURRENT_ANIMATION_FRAME_SPRITE(name, animation, record)                              \
    CAniElement* animation = m_wwdObject->m_animCursor.m_animation;                                \
    CAniRecordView* record = static_cast<CAniRecordView*>(GetAniElementAt(animation, 0));          \
    APPLY_LOOKUP_SPRITE_INLINE(name, record->m_param);

#define SET_OBJECT_AREA(value)                                                                     \
    m_object->m_area.left = value;                                                                 \
    m_object->m_area.right = value;                                                                \
    m_object->m_area.top = value;                                                                  \
    m_object->m_area.bottom = value;

#define CLEAR_OBJECT_AREA                                                                          \
    m_object->m_area.left = 0;                                                                     \
    m_object->m_area.right = 0;                                                                    \
    m_object->m_area.top = 0;                                                                      \
    m_object->m_area.bottom = 0;

#define SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, object)                                   \
    if (!CUserLogic::SerializeMove(ar, mode, typeId, object)) {                                    \
        return 0;                                                                                  \
    }

#define SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, object)                                   \
    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, object)                                       \
    return Chain(ar, mode, typeId, object) != 0;

#define SERIALIZE_USER_LOGIC_AND_CHAIN_OR_RETURN(ar, mode, typeId, object)                         \
    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, object)                                       \
    if (!Chain(ar, mode, typeId, object)) {                                                        \
        return 0;                                                                                  \
    }

#define SERIALIZE_USER_LOGIC_AND_CHAIN_FROM(baseAr, chainAr, mode, typeId, object)                 \
    if (!CUserLogic::SerializeMove(baseAr, mode, typeId, object)) {                                \
        return 0;                                                                                  \
    }                                                                                              \
    return Chain(chainAr, mode, typeId, object) != 0;

#define SERIALIZE_USER_LOGIC_AND_CHAIN_FROM_OR_RETURN(baseAr, chainAr, mode, typeId, object)       \
    if (!CUserLogic::SerializeMove(baseAr, mode, typeId, object)) {                                \
        return 0;                                                                                  \
    }                                                                                              \
    if (!Chain(chainAr, mode, typeId, object)) {                                                   \
        return 0;                                                                                  \
    }

inline void CUserLogic::GetScreenTile(Coord* out) {
    GetScreenPos(out);
    out->m_x >>= TILE_SHIFT_PX;
    out->m_y >>= TILE_SHIFT_PX;
}

inline void CUserLogic::RegisterLogicTypesOnce() {
    if (!g_logicTypesRegistered) {
        BuildLogicTypeTable(m_logicObject);
        g_logicTypesRegistered = 1;
    }
}

// The one textual copy of the ctor body.  Both CUserLogic ctor entities expand it.
//
// A MACRO, not an inline member: MSVC 5 has no __forceinline, and as a real inline
// member cl's /Ob1 budget DECLINES it in the two largest derived ctors - measured
// 2026-08-14: CWarlord (0x42d40) 78.11 -> 70.99 and CInGameIcon (0x95b10) 98.26 ->
// 93.24, where retail expands it verbatim.  A textual macro is the period device for
// a block that must expand at every site
// (docs/patterns/inline-expanded-twice-costs-a-register.md).
#define USERLOGIC_ATTACH_TO_OBJECT(obj)                                                            \
    m_logicObject = (obj);                                                                         \
    m_object = static_cast<CWwdGameObjectA*>(obj);                                                 \
    m_objAux = (obj)->m_animWorker;                                                                \
    {                                                                                              \
        zBitVec tmp("", 0);                                                                        \
        m_actBits = tmp;                                                                           \
    }                                                                                              \
    RegisterLogicTypesOnce();                                                                      \
    m_object->AddLogicHit("LogicHit");                                                             \
    m_object->AddLogicAttack("LogicAttack");                                                       \
    m_object->AddLogicBump("LogicBump");                                                           \
    m_deferredCallback = 0;                                                                        \
    m_gatedCallback = 0;                                                                           \
    m_gatedActKey = IDX(ACT_NONE);                                                                 \
    m_reserved2c = 2;

// Inline in the shared header: retail expands this whole body into ~57 derived
// logic constructors (they show the two vptr stamps, the m_actBits zBitVec assign and
// the g_logicTypesRegistered guard verbatim) and only CGrunt / CProjectile /
// CreateDoNothingNormal reach the 0x58cd0 out-of-line copy.
inline CUserLogic::CUserLogic(CGameObject* obj, EInlineBase) {
    USERLOGIC_ATTACH_TO_OBJECT(obj);
}

class CWapX {
public:
    CWapX() {}
    CWapX(CGameObject* obj) {
        m_gameObject = obj;
        m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
        m_animWorker = obj->m_animWorker;
    }
    RVA(0x00008be0, 0x1)
    ~CWapX() {}

    i32 Chain(CFileMemBase* arc, SerialMode mode, LogicTypeId unused, CGameObject* obj);

    void Apply(class CAniElement* a, i32 b);

    CGameObject* m_gameObject;
    CWwdGameObjectA* m_wwdObject;

    AnimWorkerObj* m_animWorker;

    class CAniElement* m_value;
    char m_blob[0x10];

    // These MUST stay inline members OF THIS class, not calls through
    // m_wwdObject: the receiver load has to sit inside the expansion or cl 5.0
    // hoists it over the preceding store
    // (docs/patterns/ctor-body-first-statement-is-an-inline-member.md,
    // docs/patterns/animation-switch-pair-is-one-inline-member.md).
    void Hide() {
        m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }

    void SetObjectFlags(i32 bits) {
        m_wwdObject->m_flags |= bits;
    }

    void ApplyLookupSprite(const char* name, i32 flag) {
        m_wwdObject->ApplyLookupSprite(name, flag);
    }

    void ApplyName(const char* name) {
        m_wwdObject->ApplyName(name);
    }

    void SwitchAnimation(CAniElement* anim) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(anim);
    }

    void SwitchGeometryDirect(CAniElement* anim, i32 applyDefault) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyGeometryDirect(anim, applyDefault);
    }

    i32 SwitchGeometry(const char* key, i32 flag) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        return m_wwdObject->ApplyLookupGeometry(key, flag);
    }
};

class CTileTrigger : public CUserLogic, public CWapX {
public:
    RVA(0x000111f0, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE{SERIALIZE_USER_LOGIC_AND_CHAIN(ar, mode, typeId, pObj)} RVA(0x000111d0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILETRIGGER;
    }

public:
    // Two entities, same tag type.  The out-of-line one at 0x11160 EXPANDS its
    // CUserLogic base (it stamps ??_7CUserBase and `call`s ??0zBitVec);
    // CTileSecretTrigger and CGiantRock reach it.
    CTileTrigger();
    // The inline sibling, whose CUserLogic base stays a `call`: `new CTileTrigger`
    // and CCoveredPowerup expand this body.
    CTileTrigger(CUserLogic::EInlineBase) {}
    CTileTrigger(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif // GRUNTZ_USERLOGIC_H

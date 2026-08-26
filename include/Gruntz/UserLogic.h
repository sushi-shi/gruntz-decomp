#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/LogicRecord.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WwdGridIter.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdGameObjectFlags.h>

struct CGameObject;
struct SoundCue;
class CDDrawSurfacePair;
class CUserLogic;

class CDDrawWorker;

class CImage;

extern b32 g_logicTypesRegistered;

class CFileMemBase;

class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase() {}
    RVA(0x000087e0, 0x8)
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) {
        return 1;
    }
    RVA(0x00008800, 0x3)
    virtual LogicTypeId GetTypeTag() {
        return LOGIC_UNSET;
    }
};

class CUserLogic : public CUserBase {
public:
    enum EInlineBase {
        INLINE_BASE
    };

    CUserLogic();
    CUserLogic(EInlineBase) {}
    CUserLogic(CGameObject* obj);
    CUserLogic(CGameObject* obj, EInlineBase);
    virtual ~CUserLogic() OVERRIDE {}
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00008850, 0x4)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

    virtual void StepBehavior(char* animationActName);

    virtual void FireActivation(i32 id);

    virtual void FinalizeStep(char* name);

    RVA(0x000088e0, 0x1)
    virtual void Activate() {}
    RVA(0x00008900, 0x6)
    virtual i32 AdvanceAnimation() {
        return 1;
    }
    RVA(0x00008920, 0x6)
    virtual i32 RecordFrameTick() {
        return 1;
    }

    RVA(0x00008940, 0x6)
    virtual i32 StepAttackFire() {
        return 1;
    }

    RVA(0x00008960, 0x1)
    virtual void OnLeaveActiveRegion() {}
    RVA(0x00008980, 0x1)
    virtual void OnObjectRemoved() {}
    RVA(0x000089a0, 0x1)
    virtual void AfterLoad() {}
    RVA(0x000089c0, 0x1)
    virtual void AfterSave() {}
    RVA(0x000089e0, 0x1)
    virtual void PrepareSave() {}
    RVA(0x00008a00, 0x1)
    virtual void AfterLoadReferences() {}

    void GetScreenPos(Coord* out);

    void GetScreenTile(Coord* out);

    void RegisterLogicTypesOnce();
    void BuildLogicTypeTable(CGameObject* obj);

    void LoadGruntTuningConstants(i32);

    typedef i32 (CUserLogic::*ActCallback)();
    ActCallback m_deferredCallback;
    ActCallback m_gatedCallback;
    CGameObject* m_logicObject;

    CWwdSpriteObject* m_object;

    CLogicRecord* m_logicRecord;
    zBitVec m_actBits;
    i32 m_gatedCallbackCode;
    i32 m_reserved2c;

    i32 m_previousAnimationActId;
};

#define SET_ANIMATION_ACT(key)                                                                     \
    m_previousAnimationActId = m_logicRecord->m_eventCode;                                         \
    m_logicRecord->m_eventCode = ActFindId(key)

#define ANIMATION_ACT_EQUALS(key)                                                                  \
    (strcmp(*g_typeColl.GetNameRecord(m_logicRecord->m_eventCode), key) == 0)

#define ANIMATION_ACT_DIFFERS(key)                                                                 \
    (strcmp(*g_typeColl.GetNameRecord(m_logicRecord->m_eventCode), key) != 0)

#define ANIMATION_ACT_EQUALS_FOR(logic, key)                                                       \
    (strcmp(*g_typeColl.GetNameRecord(logic->m_logicRecord->m_eventCode), key) == 0)

#define ANIMATION_ACT_DIFFERS_FOR(logic, key)                                                      \
    (strcmp(*g_typeColl.GetNameRecord(logic->m_logicRecord->m_eventCode), key) != 0)

#define APPLY_NAME_INLINE(name) m_wwdObject->SetImageSetByName(name)

#define APPLY_LOOKUP_SPRITE_INLINE(name, frame) m_wwdObject->SetImageFrameByName(name, frame)

#define ADVANCE_CURRENT_ANIMATION_CURSOR(cursor, elapsed)                                          \
    m_wwdObject->m_animationCursor.Advance(elapsed);                                               \
    CAniAdvanceCursor* cursor = &m_wwdObject->m_animationCursor;

#define GET_SCREEN_TILE_Y_FIRST(logic, out)                                                        \
    (logic)->GetScreenPos((&out));                                                                 \
    out.m_y >>= TILE_SHIFT_PX;                                                                     \
    out.m_x >>= TILE_SHIFT_PX;

#define DECLARE_CURRENT_ANIMATION_FRAME(frame, animation, record)                                  \
    CAniElement* animation = m_wwdObject->m_animationCursor.m_animation;                           \
    CAniRecordView* record = static_cast<CAniRecordView*>(GetAniElementAt(animation, 0));          \
    i32 frame = record->m_param;

#define SET_OBJECT_FLAGS_INLINE(bits) m_wwdObject->m_flags |= bits

#define HIDE_OBJECT_INLINE() m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN

#define SET_OBJECT_FLAGS_AND_HIDE_INLINE(bits)                                                     \
    SET_OBJECT_FLAGS_INLINE(bits);                                                                 \
    HIDE_OBJECT_INLINE();

#define INITIALIZE_DEFAULT_CYCLE_ANIMATION                                                         \
    SET_ANIMATION_ACT("A");                                                                        \
    if (m_wwdObject->m_animationCursor.m_animation == NULL) {                                      \
        SwitchAnimationByName("GAME_CYCLE100", 0);                                                 \
    }

#define MARK_OBJECT_COMPLETE_IF(condition)                                                         \
    if (condition) {                                                                               \
        SET_OBJECT_FLAGS_INLINE(0x10000);                                                          \
    }

#define APPLY_CURRENT_ANIMATION_FRAME_SPRITE(name, animation, record)                              \
    CAniElement* animation = m_wwdObject->m_animationCursor.m_animation;                           \
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
    if (!CUserLogic::SerializeDispatch(ar, mode, typeId, object)) {                                \
        return 0;                                                                                  \
    }

#define SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)                         \
    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, object)                                       \
    return SerializeAnimationState(ar, mode, typeId, object) != 0;

#define SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)               \
    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, object)                                       \
    if (!SerializeAnimationState(ar, mode, typeId, object)) {                                      \
        return 0;                                                                                  \
    }

#define SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM(baseAr, stateAr, mode, typeId, object)       \
    if (!CUserLogic::SerializeDispatch(baseAr, mode, typeId, object)) {                            \
        return 0;                                                                                  \
    }                                                                                              \
    return SerializeAnimationState(stateAr, mode, typeId, object) != 0;

#define SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM_OR_RETURN(                                   \
    baseAr,                                                                                        \
    stateAr,                                                                                       \
    mode,                                                                                          \
    typeId,                                                                                        \
    object                                                                                         \
)                                                                                                  \
    if (!CUserLogic::SerializeDispatch(baseAr, mode, typeId, object)) {                            \
        return 0;                                                                                  \
    }                                                                                              \
    if (!SerializeAnimationState(stateAr, mode, typeId, object)) {                                 \
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
        g_logicTypesRegistered = true;
    }
}

#define USERLOGIC_ATTACH_TO_OBJECT(obj)                                                            \
    m_logicObject = (obj);                                                                         \
    m_object = static_cast<CWwdSpriteObject*>(obj);                                                \
    m_logicRecord = (obj)->m_logicRecord;                                                          \
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
    m_gatedCallbackCode = IDX(ACT_NONE);                                                           \
    m_reserved2c = 2;

inline CUserLogic::CUserLogic(CGameObject* obj, EInlineBase) {
    USERLOGIC_ATTACH_TO_OBJECT(obj);
}

class CWapX {
public:
    CWapX() {}
    CWapX(CGameObject* obj) {
        m_gameObject = obj;
        m_wwdObject = static_cast<CWwdSpriteObject*>(obj);
        m_ownerLogicRecord = obj->m_logicRecord;
    }
    RVA(0x00008bf0, 0x1)
    ~CWapX() {}

    i32 SerializeAnimationState(
        CFileMemBase* archive,
        SerialMode mode,
        LogicTypeId unusedTypeId,
        CGameObject* object
    );

    void ApplyAnimation(class CAniElement* animation, i32 advanceImmediately);

    CGameObject* m_gameObject;
    CWwdSpriteObject* m_wwdObject;

    CLogicRecord* m_ownerLogicRecord;

    class CAniElement* m_value;
    char m_blob[0x10];

    void Hide() {
        m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }

    void SetObjectFlags(i32 bits) {
        m_wwdObject->m_flags |= bits;
    }

    void SetImageFrameByName(const char* name, i32 flag) {
        m_wwdObject->SetImageFrameByName(name, flag);
    }

    void SetImageSetByName(const char* name) {
        m_wwdObject->SetImageSetByName(name);
    }

    void SwitchAnimation(CAniElement* anim) {
        m_value = m_wwdObject->m_animationCursor.m_animation;
        m_wwdObject->m_animationCursor.SetAnimation(anim);
    }

    void SwitchAnimationAndMaybeAdvance(CAniElement* anim, i32 advanceImmediately) {
        m_value = m_wwdObject->m_animationCursor.m_animation;
        m_wwdObject->SetAnimation(anim, advanceImmediately);
    }

    i32 SwitchAnimationByName(const char* key, i32 advanceImmediately) {
        m_value = m_wwdObject->m_animationCursor.m_animation;
        return m_wwdObject->SetAnimationByName(key, advanceImmediately);
    }
};

class CTileTrigger : public CUserLogic, public CWapX {
public:
    RVA(0x00011200, 0x47)
    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
        OVERRIDE{
            SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
        } RVA(0x000111e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILETRIGGER;
    }

public:
    CTileTrigger();
    CTileTrigger(CUserLogic::EInlineBase) {}
    CTileTrigger(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif // GRUNTZ_USERLOGIC_H

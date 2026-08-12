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
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj);
    virtual LogicTypeId GetTypeTag();
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
    virtual LogicTypeId GetTypeTag() OVERRIDE;

    virtual void XferName(char* name);

    virtual void FireActivation(i32 id);

    virtual void FinalizeStep(char* name);

    virtual void Activate();
    virtual i32 AdvanceAnimation();
    virtual i32 RecordFrameTick();

    virtual i32 StepAttackFire();

    virtual void OnLeaveActiveRegion();
    virtual void OnObjectRemoved();
    virtual void AfterLoad();
    virtual void AfterSave();
    virtual void PrepareSave();
    virtual void AfterLoadReferences();

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
// A MACRO, not an inline member: MSVC 5 has no __forceinline and its inline budget
// declines the body in the two largest derived ctors - CWarlord (0x750 B) and
// CInGameIcon (0x15f0 B) - where retail expands it verbatim, leaving a
// `call ?AttachToObject@CUserLogic@@` at ctor+0x48 in both.  A textual macro is the
// period device for a block that must expand at every site
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
    ~CWapX() {}

    i32 Chain(CFileMemBase* arc, SerialMode mode, LogicTypeId unused, CGameObject* obj);

    void Apply(class CAniElement* a, i32 b);

    CGameObject* m_gameObject;
    CWwdGameObjectA* m_wwdObject;

    AnimWorkerObj* m_animWorker;

    class CAniElement* m_value;
    char m_blob[0x10];
};

class CTileTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE;

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

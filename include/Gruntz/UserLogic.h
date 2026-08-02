#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h>

#include <Gruntz/UserBaseLink.h>

#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/WwdGridIter.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/SerialArchive.h>

struct CGameObject;
struct LeafCue;
class CDDrawSurfacePair;
class CUserLogic;
struct Coord;

class CDDrawWorker;

class CImage;

extern CButeMgr g_buteMgr;

extern i32 g_logicTypesRegistered;

class CFileMemBase;

class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase() {}
    virtual i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj);
    virtual LogicTypeId GetTypeTag();
};
SIZE_UNKNOWN();

class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
    virtual ~CUserLogic() OVERRIDE {}
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
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

    i32 IsAtSavedScreenPos();

    void RegisterLogicTypesOnce();
    void BuildLogicTypeTable(CGameObject* obj);

    void LoadGruntTuningConstants(i32);

    typedef i32 (CUserLogic::*ActCallback)();
    ActCallback m_deferredCallback;
    ActCallback m_gatedCallback;
    CGameObject* m_logicObject;

    CWwdGameObjectA* m_object;

    AnimWorkerObj* m_objAux;
    CUserBaseLink m_link;
    i32 m_gatedActKey;
    i32 m_2c;

    i32 m_prevAnimSetNode;
};
SIZE(0x34);

#ifndef USERLOGIC_OOL_CTOR
inline CUserLogic::CUserLogic(CGameObject* obj) {
    m_logicObject = obj;
    m_object = static_cast<CWwdGameObjectA*>(obj);
    m_objAux = obj->m_animWorker;
    {
        zBitVec tmp(g_emptyString, 0);
        m_link.m_str = tmp;
    }
    RegisterLogicTypesOnce();
    m_object->AddLogicHit("LogicHit");
    m_object->AddLogicAttack("LogicAttack");
    m_object->AddLogicBump("LogicBump");
    m_deferredCallback = 0;
    m_gatedCallback = 0;
    m_gatedActKey = 0x3e9;
    m_2c = 2;
}
#endif

inline void CUserLogic::RegisterLogicTypesOnce() {
    if (!g_logicTypesRegistered) {
        BuildLogicTypeTable(m_logicObject);
        g_logicTypesRegistered = 1;
    }
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

    i32 Chain(CFileMemBase* arc, i32 mode, i32 unused, CGameObject* obj);

    void Apply(class CAniElement* a, i32 b);

    CGameObject* m_gameObject;
    CWwdGameObjectA* m_wwdObject;

    AnimWorkerObj* m_animWorker;

    class CAniElement* m_value;
    char m_blob[0x10];
};
SIZE(0x20);

class CTileTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE;

public:
    CTileTrigger();
    CTileTrigger(CGameObject* obj);
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();
};
SIZE(0x54);

extern "C" i32 LogicHitFactory(CGameObject* obj);
extern "C" i32 LogicAttackFactory(CGameObject* obj);
extern "C" i32 LogicBumpFactory(CGameObject* obj);

#endif // GRUNTZ_USERLOGIC_H

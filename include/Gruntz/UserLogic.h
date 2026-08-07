#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserBaseLink.h>
#include <Gruntz/WwdGridIter.h>
#include <Wwd/WwdGameObjectFamily.h>

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
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj);
    virtual LogicTypeId GetTypeTag();
};
SIZE_UNKNOWN();

class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
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
    i32 m_reserved2c;

    i32 m_prevAnimSetNode;
};
SIZE(0x34);

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

    i32 Chain(CFileMemBase* arc, SerialMode mode, LogicTypeId unused, CGameObject* obj);

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
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
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

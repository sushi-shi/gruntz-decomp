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
    RVA(0x000087d0, 0x8)
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) {
        return 1;
    }
    RVA(0x000087f0, 0x3)
    virtual LogicTypeId GetTypeTag() {
        return static_cast<LogicTypeId>(0);
    }
};
SIZE_UNKNOWN();

class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
    virtual ~CUserLogic() OVERRIDE {}
    RVA(0x0016e7f0, 0x1cf)
    virtual i32
    SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (arc == 0) {
            return 0;
        }
        switch (mode) {
            case SERIAL_LOAD: {

                i32 len;
                arc->Read(&len, 4);
                void* buf = ::operator new(len);
                arc->Read(buf, len);
                istrstream accum(static_cast<char*>(buf), len);
                accum >> m_link.m_str;
                ::operator delete(buf);
                arc->Read(&m_gatedActKey, 4);
                arc->Read(&m_reserved2c, 4);
                arc->Read(&g_logicTypesRegistered, 4);
                arc->Read(&m_prevAnimSetNode, 4);
                m_logicObject = pObj;
                m_object = static_cast<CWwdGameObjectA*>(pObj);
                m_objAux = (pObj)->m_animWorker;
                m_deferredCallback = 0;
                m_gatedCallback = 0;
                m_gatedActKey = 0x3e9;

                break;
            }
            case SERIAL_SAVE: {

                char buf[0x100];
                ostrstream accum(buf, 0x100);
                accum << m_link.m_str;
                i32 len = accum.pcount();
                arc->Write(&len, 4);
                arc->Write(accum.str(), len);
                arc->Write(&m_gatedActKey, 4);
                arc->Write(&m_reserved2c, 4);
                arc->Write(&g_logicTypesRegistered, 4);
                arc->Write(&m_prevAnimSetNode, 4);

                break;
            }
        }
        return 1;
    }
    RVA(0x00008840, 0x4)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

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
    m_reserved2c = 2;
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
    RVA(0x000111f0, 0x47)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
            return 0;
        }
        return Chain(ar, mode, typeId, pObj) != 0;
    }
    RVA(0x000111d0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TILETRIGGER;
    }

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

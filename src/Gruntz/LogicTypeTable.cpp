#include <Gruntz/GameObjectFactory.h> // the Logic*Factory registrants (ex .cpp externs)
#include <Mfc.h> // real MFC CMapStringToOb (the logic-name map's Lookup @0x1b8438)
#include <rva.h>
#include <Gruntz/Grunt.h>     // CAnimLookupNode (the m_14 aux the guard reads at +0x1c)
#include <Gruntz/UserLogic.h> // CUserLogic - the real owner of 0x8b90 (ex CFinalize8b90)


#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

RVA(0x00008a40, 0xc8)
void __stdcall BuildLogicTypeTable(CGameObject* obj) {
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicHit", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicAttack", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicBump", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
        }
    }
}

typedef void (CUserLogic::*UserLogicCallback)(); // 4 bytes (complete class, single inheritance)

RVA(0x00008b90, 0x40)
void CUserLogic::FinalizeStep(i32 /*unused*/) {
    if (m_deferredCallback == 0) {
        return;
    }
    if (m_gatedCallback != 0 && reinterpret_cast<i32>(m_objAux->m_1c) == m_28) {
        (this->*reinterpret_cast<UserLogicCallback&>(m_gatedCallback))();
        m_gatedCallback = 0;
    }
    (this->*reinterpret_cast<UserLogicCallback&>(m_deferredCallback))();
    m_deferredCallback = 0;
    m_28 = 0x3e9;
}

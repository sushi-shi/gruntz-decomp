#include <rva.h>

#include <Gruntz/ToobSpikez.h>

#include <Bute/ButeTree.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/XferArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

#include <stddef.h>

RVA_COMPGEN(0x00012c30, 0x1e, ??_GCToobSpikez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012c60, 0x44, ??1CToobSpikez@@UAE@XZ)

RVA(0x00114480, 0xf1)
i32 CreateToobSpikez(CGameObject* obj) {
    AnimWorkerObj* rec = obj->m_animWorker;
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
            CToobSpikez* inst = new CToobSpikez(obj);
            inst->Activate();
            rec->m_logic = inst;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(rec->m_logic);
            break;
    }
    return 1;
}

VTBL(CToobSpikez, 0x001e7774);
template<> DATA(0x0024e978)
CActReg CActRegPool<CToobSpikez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 2);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_object->m_speedX = m_object->m_screenX >> TILE_SHIFT_PX;
    m_object->m_speedY = m_object->m_screenY >> TILE_SHIFT_PX;
    if (m_object->m_sortKey != SORTKEY_TOOB_SPIKE) {
        m_object->m_sortKey = SORTKEY_TOOB_SPIKE;
        m_object->m_flags |= 0x20000;
    }
}

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00012bc0, 0x47)
i32 CToobSpikez::SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA(0x00114860, 0x102)
void CToobSpikez::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CToobSpikez>::s_table.ResolveEntry(coord));
    if (*e != 0) {
        CActHandler* e2 = (CActRegPool<CToobSpikez>::s_table.ResolveEntry(coord));
        (this->*(*e2))();
    }
}

RVA(0x001149c0, 0x18d)
void CToobSpikez::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != NULL) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CToobSpikez>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CToobSpikez::AdvanceAnim);
}

RVA(0x00114bc0, 0x17)
i32 CToobSpikez::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

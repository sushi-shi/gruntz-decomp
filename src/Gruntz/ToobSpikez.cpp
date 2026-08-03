#include <rva.h>

#include <Gruntz/ToobSpikez.h>

#include <Bute/ButeTree.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/XferArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_COMPGEN(0x00012c30, 0x1e, ??_GCToobSpikez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012c60, 0x44, ??1CToobSpikez@@UAE@XZ)

RVA(0x00114480, 0xf1)
i32 CreateToobSpikez(CGameObject* obj) {
    AnimWorkerObj* rec = obj->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CToobSpikez* inst = new CToobSpikez(obj);
            inst->Activate();
            rec->m_logic = inst;
            break;
        }
        case 0x1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            rec->m_logic->PrepareSave();
            break;
        case 0x51:
            rec->m_logic->AfterSave();
            break;
        case 0x52:
            rec->m_logic->AfterLoad();
            break;
        case 0x53:
            rec->m_logic->AfterLoadReferences();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(rec->m_logic);
            break;
    }
    return 1;
}

VTBL(CToobSpikez, 0x001e7774);
template<> DATA(0x0024e978)
CActReg CActRegPool<CToobSpikez>::s_table(2000, 2010);

RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 2);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_object->m_speedX = m_object->m_screenX >> 5;
    m_object->m_speedY = m_object->m_screenY >> 5;
    if (m_object->m_sortKey != 0xc) {
        m_object->m_sortKey = 0xc;
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

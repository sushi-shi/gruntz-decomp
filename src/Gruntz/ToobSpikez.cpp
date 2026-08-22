#include <rva.h>

#include <Gruntz/ToobSpikez.h>

#include <Bute/ButeTree.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
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
    switch (rec->WorkerAct()) {
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

RVA_DYNINIT(0x001147c0, 0xa, CActRegPool<CToobSpikez>::s_table)
RVA_DYNINIT(0x001147e0, 0x15, CActRegPool<CToobSpikez>::s_table)
RVA_DYNINIT(0x00114810, 0xe, CActRegPool<CToobSpikez>::s_table)
RVA_DYNINIT(0x00114830, 0x1f, CActRegPool<CToobSpikez>::s_table)
template<> DATA(0x0024e978)
CActReg CActRegPool<CToobSpikez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @early-stop
// The vptr stamp is transposed with the body's first m_wwdObject read; the rest is
// a scratch-register rotation.  docs/patterns/vptr-stamp-transposed-with-second-base-member-load.md
RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SwitchGeometry("GAME_CYCLE100", 2);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    SetObjectFlags(2);
    m_object->m_speedX = m_object->m_screenX >> TILE_SHIFT_PX;
    m_object->m_speedY = m_object->m_screenY >> TILE_SHIFT_PX;
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_TOOB_SPIKE)
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
    ACT_NAME_ID(id, "A")
    *CActRegPool<CToobSpikez>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CToobSpikez::AdvanceAnim);
}

RVA(0x00114bc0, 0x17)
i32 CToobSpikez::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

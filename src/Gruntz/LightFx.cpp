#include <rva.h>

#include <Gruntz/LightFx.h>

#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/XferArchive.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

#include <stddef.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

static inline CAniElement* LookupAnimation(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* result = NULL;
    MapLookup(map, name, result);
    return result;
}

RVA_DYNINIT(0x0009d120, 0xa, CActRegPool<CLightFx>::s_table)
RVA_DYNINIT(0x0009d140, 0x15, CActRegPool<CLightFx>::s_table)
RVA_DYNINIT(0x0009d170, 0xe, CActRegPool<CLightFx>::s_table)
RVA_DYNINIT(0x0009d190, 0x1f, CActRegPool<CLightFx>::s_table)
template<> DATA(0x00245ad0)
CActReg CActRegPool<CLightFx>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00012400, 0x1e, ??_GCLightFx@@UAEPAXI@Z)
RVA_COMPGEN(0x00012430, 0x44, ??1CLightFx@@UAE@XZ)

RVA(0x0009cdc0, 0xf1)
i32 CreateLightFx(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    switch (aux->WorkerAct()) {
        case ACT_UNINITIALISED:
            aux->SetWorkerAct(ACT_LIVE);
            {
                CLightFx* p = new CLightFx(obj);
                (static_cast<CUserLogic*>(p))->Activate();
                aux->m_logic = p;
            }
            break;
        case ACT_OBJECT_REMOVED:
            aux->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            aux->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            aux->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            aux->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            aux->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

// @early-stop the same out-parameter `= NULL` store schedule as Activate; the only
// other diff rows are the `fs:0` reloc-masking artifact. FLAT across 24 TU states.
RVA(0x0009cf00, 0x1a5)
CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_anchorA = 2;
    m_anchorB = 1;
}
RVA(0x0009d1c0, 0x102)
void CLightFx::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CLightFx>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CLightFx>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0009d320, 0x18d)
void CLightFx::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CLightFx>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CLightFx::AdvanceAnim);
}

// @early-stop
RVA(0x0009d520, 0xfd)
void CLightFx::Activate(const char* spec, const char* effect, i32 anchorA, i32 anchorB) {
    CDDrawWorker* en =
        LookupWorker(m_animWorker->m_ownerCtx->m_imageRegistry->m_workersByName, spec);
    g_gameReg->m_logicPump->Push(en, anchorA, SHADE_DST_BY_SRC_16);
    CWwdGameObjectA* o = m_wwdObject;
    if (en != NULL) {

        i32 key = en->m_minIndex;

        o->m_frameSet = en;
        CImage* val = en->GetAt(key);
        o->m_layer = val;
        o->m_frameIndex = key;
    }
    CAniElement* node = NULL;
    SetObjectFlags(2);
    m_anchorA = anchorA;
    m_anchorB = anchorB;

    MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, effect, node);
    if (node != NULL) {
        SwitchAnimation(
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, effect)
        );
        RebindNode();
    }
}

RVA(0x0009d660, 0xc8)
i32 CLightFx::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            (ar)->Write(&m_anchorA, sizeof(m_anchorA));
            (ar)->Write(&m_anchorB, sizeof(m_anchorB));
            break;
        case SERIAL_LOAD:
            (ar)->Read(&m_anchorA, sizeof(m_anchorA));
            (ar)->Read(&m_anchorB, sizeof(m_anchorB));
            break;
        case SERIAL_POSTLOAD:
            g_gameReg
                ->m_logicPump

                ->Push(m_wwdObject->m_frameSet, m_anchorA, SHADE_DST_BY_SRC_16);
            break;
    }
    return 1;
}

RVA(0x0009d770, 0x25)
i32 CLightFx::RebindNode() {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    return 0;
}

RVA(0x0009d7b0, 0x40)
i32 CLightFx::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (IsAniCursorComplete(&m_wwdObject->m_animCursor) && m_anchorB) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

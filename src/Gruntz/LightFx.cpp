#include <rva.h>

#include <Gruntz/LightFx.h>

#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
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

template<> DATA(0x00245ad0)
CActReg CActRegPool<CLightFx>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

VTBL(CLightFx, 0x001e7af4);

RVA_COMPGEN(0x00012400, 0x1e, ??_GCLightFx@@UAEPAXI@Z)
RVA_COMPGEN(0x00012430, 0x44, ??1CLightFx@@UAE@XZ)

RVA(0x0009cdc0, 0xf1)
i32 CreateLightFx(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    switch (static_cast<u32>(static_cast<size_t>(aux->ActKey()))) {
        case 0:
            aux->SetActKey(0x3e8);
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

// @early-stop
RVA(0x0009cf00, 0x1a5)
CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
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
    (*((CActRegPool<CLightFx>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CLightFx::AdvanceAnim);
}

// @early-stop
RVA(0x0009d520, 0xfd)
i32 CLightFx::Activate(const char* spec, const char* effect, i32 anchorA, i32 anchorB) {
    void* node = 0;
    CObject* nodeOb = 0;

    m_animWorker->m_ownerCtx->m_imageRegistry->m_10map.Lookup(spec, nodeOb);
    node = nodeOb;
    void* found = node;
    g_gameReg->m_logicPump->Push(static_cast<CDDrawWorker*>(found), anchorA, SHADE_DST_BY_SRC_16);
    if (found != NULL) {

        CDDrawWorker* en = static_cast<CDDrawWorker*>(found);
        i32 key = en->m_minIndex;

        m_wwdObject->m_frameSet = en;
        CImage* val;
        if (key < en->m_minIndex || key > en->m_maxIndex) {
            val = NULL;
        } else {
            val = static_cast<CImage*>(en->m_items.GetAt(key));
        }
        m_wwdObject->m_layer = val;
        m_wwdObject->m_frameIndex = key;
    }
    node = NULL;
    m_wwdObject->m_flags |= 2;
    m_anchorA = anchorA;
    m_anchorB = anchorB;

    MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, effect, node);
    if (node != NULL) {
        node = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, effect, node);
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(static_cast<CAniElement*>(node));
        RebindNode();
    }
    return 0;
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
    if (m_wwdObject->m_animCursor.m_finished && !m_wwdObject->m_animCursor.m_frameTicksLeft
        && m_anchorB) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

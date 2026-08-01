#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/ActNameRegistry.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/XferArchive.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h>
#include <Image/ImageSet.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Wap32/ZVec.h>
#include <Utils/MapTyped.h>

template<> DATA(0x00245ad0)
CActReg CActRegPool<CLightFx>::s_table(2000, 2010);

VTBL(CLightFx, 0x001e7af4);

RVA_COMPGEN(0x00012400, 0x1e, ??_GCLightFx@@UAEPAXI@Z)
RVA_COMPGEN(0x00012430, 0x44, ??1CLightFx@@UAE@XZ)

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
            if (list != 0) {
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

    m_3c->m_ownerCtx->m_imageRegistry->m_10map.Lookup(spec, nodeOb);
    node = nodeOb;
    void* found = node;
    g_gameReg->m_logicPump->Push(static_cast<CDDrawWorker*>(found), anchorA, 7);
    if (found != 0) {

        CDDrawWorker* en = static_cast<CDDrawWorker*>(found);
        i32 key = en->m_minIndex;

        m_38->m_sprite = en;
        CImage* val;
        if (key < en->m_minIndex || key > en->m_maxIndex) {
            val = 0;
        } else {
            val = static_cast<CImage*>(en->m_items.GetAt(key));
        }
        m_38->m_layer = val;
        m_38->m_190 = key;
    }
    node = 0;
    m_38->m_flags |= 2;
    m_anchorA = anchorA;
    m_anchorB = anchorB;

    MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, effect, node);
    if (node != 0) {
        node = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, effect, node);
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(static_cast<CAniElement*>(node));
        RebindNode();
    }
    return 0;
}

RVA(0x0009d660, 0xc8)
i32 CLightFx::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            (ar)->Write(&m_anchorA, 4);
            (ar)->Write(&m_anchorB, 4);
            break;
        case 7:
            (ar)->Read(&m_anchorA, 4);
            (ar)->Read(&m_anchorB, 4);
            break;
        case 8:
            g_gameReg
                ->m_logicPump

                ->Push(m_38->m_sprite, m_anchorA, 7);
            break;
    }
    return 1;
}

RVA(0x0009d770, 0x25)
i32 CLightFx::RebindNode() {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    return 0;
}

RVA(0x0009d7b0, 0x40)
i32 CLightFx::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    if (m_38->m_1a0.m_finished && !m_38->m_1a0.m_frameTicksLeft && m_anchorB) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

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
        case 0x1d:
            aux->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            aux->m_logic->PrepareSave();
            break;
        case 0x51:
            aux->m_logic->AfterSave();
            break;
        case 0x52:
            aux->m_logic->AfterLoad();
            break;
        case 0x53:
            aux->m_logic->AfterLoadReferences();
            break;
        case 0x3e8:
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

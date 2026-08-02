

#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/ActNameRegistry.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/TileTrigger.h>
#include <Gruntz/TileTriggerSwitch.h>
#include <Gruntz/WarpStonePad.h>
#include <Gruntz/CheckpointTrigger.h>
#include <Gruntz/Play.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/Timer.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GameLevel.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/Random.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/GameRegistry.h>
#include <string.h>
#include <rva.h>
#include <DDrawMgr/AniAdvance.h>
#include <Image/CImage.h>
#include <Gruntz/TileLogicPump.h>

template<> DATA(0x0024e6a0)
CActReg CActRegPool<CWarpStonePad>::s_table(2000, 2010);
template<> DATA(0x0024e798)
CActReg CActRegPool<CTileTriggerSwitch>::s_table(2000, 2010);
template<> DATA(0x0024e810)
CActReg CActRegPool<CTileTrigger>::s_table(2000, 2010);
template<> DATA(0x0024e7c0)
CActReg CActRegPool<CBrickz>::s_table(2000, 2010);
template<> DATA(0x0024e7e8)
CActReg CActRegPool<CCheckpointTrigger>::s_table(2000, 2010);

VTBL(CWarpStonePad, 0x001e71ac);
VTBL(CBrickz, 0x001e7c54);
VTBL(CGiantRock, 0x001e7d5c);
VTBL(CTileTriggerTransition, 0x001e7db4);
VTBL(CCoveredPowerup, 0x001e7e0c);
VTBL(CTileSecretTrigger, 0x001e7e64);
VTBL(CCheckpointTrigger, 0x001e7ebc);
VTBL(CTileTrigger, 0x001e7f14);
VTBL(CTileTriggerSwitch, 0x001e7f6c);

template<> DATA(0x0024e720)
CActReg CActRegPool<CTileTriggerTransition>::s_table(2000, 2010);

#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                               \
    AnimWorkerObj* ctl = obj->m_animWorker;                                                        \
    switch (static_cast<u32>(ctl->ActKey())) {                                                     \
        case 0: {                                                                                  \
            ctl->SetActKey(0x3e8);                                                                 \
            LEAF* t = new LEAF(obj);                                                               \
            t->Activate();                                                                         \
            ctl->m_logic = t;                                                                      \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            ctl->m_logic->OnObjectRemoved();                                                       \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            ctl->m_logic->OnLeaveActiveRegion();                                                   \
            break;                                                                                 \
        case 0x50:                                                                                 \
            ctl->m_logic->PrepareSave();                                                           \
            break;                                                                                 \
        case 0x51:                                                                                 \
            ctl->m_logic->AfterSave();                                                             \
            break;                                                                                 \
        case 0x52:                                                                                 \
            ctl->m_logic->AfterLoad();                                                             \
            break;                                                                                 \
        case 0x53:                                                                                 \
            ctl->m_logic->AfterLoadReferences();                                                   \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            ProjTypeXfer(ctl->m_logic);                                                            \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x00010f20, 0x47)
i32 CWarpStonePad::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x00010f90, 0x1e, ??_GCWarpStonePad@@UAEPAXI@Z)
RVA_COMPGEN(0x00010fc0, 0x44, ??1CWarpStonePad@@UAE@XZ)

RVA(0x00011030, 0x6)
LogicTypeId CTileTriggerSwitch::GetTypeTag() {
    return LOGIC_TILETRIGGERSWITCH;
}

RVA(0x00011050, 0x47)
i32 CTileTriggerSwitch::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x000110c0, 0x1e, ??_GCTileTriggerSwitch@@UAEPAXI@Z)
RVA_COMPGEN(0x000110f0, 0x44, ??1CTileTriggerSwitch@@UAE@XZ)

RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

RVA(0x000111d0, 0x6)
LogicTypeId CTileTrigger::GetTypeTag() {
    return LOGIC_TILETRIGGER;
}

RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x00011260, 0x1e, ??_GCTileTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011290, 0x44, ??1CTileTrigger@@UAE@XZ)

RVA(0x00011320, 0x47)
i32 CBrickz::SerializeMove(CFileMemBase* a, i32 b, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x00011390, 0x1e, ??_GCBrickz@@UAEPAXI@Z)
RVA_COMPGEN(0x000113c0, 0x44, ??1CBrickz@@UAE@XZ)

RVA(0x00011430, 0x6)
LogicTypeId CCheckpointTrigger::GetTypeTag() {
    return LOGIC_CHECKPOINTTRIGGER;
}

RVA_COMPGEN(0x00011450, 0x1e, ??_GCCheckpointTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011480, 0x44, ??1CCheckpointTrigger@@UAE@XZ)

RVA(0x000114f0, 0x6)
LogicTypeId CTileSecretTrigger::GetTypeTag() {
    return LOGIC_TILESECRETTRIGGER;
}

RVA_COMPGEN(0x00011510, 0x1e, ??_GCTileSecretTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011540, 0x44, ??1CTileSecretTrigger@@UAE@XZ)

RVA(0x000115b0, 0x6)
LogicTypeId CGiantRock::GetTypeTag() {
    return LOGIC_GIANTROCK;
}

RVA_COMPGEN(0x000115d0, 0x1e, ??_GCGiantRock@@UAEPAXI@Z)
RVA_COMPGEN(0x00011600, 0x44, ??1CGiantRock@@UAE@XZ)

RVA(0x00011670, 0x6)
LogicTypeId CCoveredPowerup::GetTypeTag() {
    return LOGIC_COVEREDPOWERUP;
}

RVA_COMPGEN(0x00011690, 0x1e, ??_GCCoveredPowerup@@UAEPAXI@Z)
RVA_COMPGEN(0x000116c0, 0x44, ??1CCoveredPowerup@@UAE@XZ)

RVA(0x00011730, 0x6)
LogicTypeId CTileTriggerTransition::GetTypeTag() {
    return LOGIC_TILETRIGGERTRANSITION;
}

RVA(0x00011750, 0x47)
i32 CTileTriggerTransition::SerializeMove(
    CFileMemBase* ar,
    i32 mode,
    i32 typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x000117c0, 0x1e, ??_GCTileTriggerTransition@@UAEPAXI@Z)
RVA_COMPGEN(0x000117f0, 0x44, ??1CTileTriggerTransition@@UAE@XZ)

RVA(0x0010cb10, 0xf1)
i32 CreateTileTrigger(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTrigger)}

RVA(0x0010cc50, 0xf1)
i32 CreateTileTriggerSwitch(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerSwitch)}

RVA(0x0010cd90, 0xf1)
i32 CreateTileSecretTrigger(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileSecretTrigger)}

RVA(0x0010ced0, 0xf1)
i32 CreateGiantRock(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CGiantRock)}

RVA(0x0010d010, 0xf1)
i32 CreateCoveredPowerup(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CCoveredPowerup)}

RVA(0x0010d150, 0xf1)
i32 StepController(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerTransition)}

RVA(0x0010d290, 0xf4)
i32 CreateCheckpointTrigger(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    switch (static_cast<u32>(ctl->ActKey())) {
        case 0: {
            ctl->SetActKey(0x3e8);
            CCheckpointTrigger* t = new CCheckpointTrigger(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            ctl->m_logic->PrepareSave();
            break;
        case 0x51:
            ctl->m_logic->AfterSave();
            break;
        case 0x52:
            ctl->m_logic->AfterLoad();
            break;
        case 0x53:
            ctl->m_logic->AfterLoadReferences();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

RVA(0x0010d3d0, 0xf1)
i32 CreateBrickz(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CBrickz)}

RVA(0x0010d510, 0xf1)
i32 CreateWarpStonePad(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CWarpStonePad)}

// @early-stop
RVA(0x0010d650, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    if (g_gameReg->m_134 == 1) {
        m_wwdObject->m_stateFlags |= 1;
        m_wwdObject->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
}

RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CWarpStonePad>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010da20, 0x18d)
void CWarpStonePad::RegisterActs() {
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
    (*((CActRegPool<CWarpStonePad>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CWarpStonePad::AdvanceAnim);
}

RVA(0x0010dc20, 0x3)
i32 CWarpStonePad::AdvanceAnim() {
    return 0;
}

RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");

    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_stateFlags |= 1;
}

RVA(0x0010dea0, 0x102)
void CTileTriggerSwitch::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010e000, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
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
    (*((CActRegPool<CTileTriggerSwitch>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerSwitch::AdvanceAnim);
}

RVA(0x0010e200, 0x3)
i32 CTileTriggerSwitch::AdvanceAnim() {
    return 0;
}

RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_stateFlags |= 1;

    CWwdGameObjectA* o = m_object;
    i32 tileX = o->m_screenX >> 5;
    i32 tileY = o->m_screenY >> 5;
    o->m_164 = tileX;
    o->m_168 = tileY;
    o->m_id = (tileX << 8) + tileY;
}

RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTileTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
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
    (*((CActRegPool<CTileTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTrigger::AdvanceAnim);
}

// @early-stop
RVA(0x0010e800, 0x17d)
CBrickz::CBrickz(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_stateFlags |= 1;

    CWwdGameObjectA* o = m_object;
    i32 tileX = o->m_screenX >> 5;
    i32 tileY = o->m_screenY >> 5;
    o->m_164 = tileX;
    o->m_168 = tileY;
    o->m_id = (tileX << 8) + tileY;
}

RVA(0x0010ea80, 0x102)
void CBrickz::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CBrickz>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010ebe0, 0x18d)
void CBrickz::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (cnt-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CBrickz>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CBrickz::Trigger);
}

RVA(0x0010ede0, 0x3)
i32 CBrickz::Trigger() {
    return 0;
}

RVA(0x0010ee00, 0x3)
i32 CTileTrigger::AdvanceAnim() {
    return 0;
}

// @early-stop
RVA(0x0010ee20, 0x27d)
CCheckpointTrigger::CCheckpointTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;

    CWwdGameObjectA* o = m_object;
    i32 zk = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
    if (o->m_sortKey != zk) {
        o->m_sortKey = zk;
        o->m_flags |= 0x20000;
    }
    memset(m_state, 0, sizeof(m_state));
    if (m_object->m_extent.left == 0x80000000) {
        m_object->m_extent.left = 0;
    }
    if (m_object->m_area.left == 0x80000000) {
        m_object->m_area.left = 0;
    }
    if (m_object->m_switchRect.left == 0x80000000) {
        m_object->m_switchRect.left = 0;
    }
    if (m_object->m_clip.left == 0x80000000) {
        m_object->m_clip.left = 0;
    }
    m_state[0] = m_object->m_extent.left;
    m_state[1] = m_object->m_extent.top;
    m_state[2] = m_object->m_extent.right;
    m_state[3] = m_object->m_extent.bottom;
    m_state[4] = m_object->m_area.left;
    m_state[5] = m_object->m_area.top;
    m_state[6] = m_object->m_area.right;
    m_state[7] = m_object->m_area.bottom;
    m_state[8] = m_object->m_switchRect.left;
    m_state[9] = m_object->m_switchRect.top;
    m_state[10] = m_object->m_switchRect.right;
    m_state[11] = m_object->m_switchRect.bottom;
    m_state[12] = m_object->m_clip.left;
    m_state[13] = m_object->m_clip.top;
    m_state[14] = m_object->m_clip.right;

    i32 found = 0;
    m_firstEmpty = 0;
    while (found == 0 && m_firstEmpty < 15) {

        if (m_state[m_firstEmpty] != 0) {
            m_firstEmpty++;
        } else {
            found = 1;
        }
    }
}

RVA(0x0010f1e0, 0x102)
void CCheckpointTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CCheckpointTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010f340, 0x2ac)
void CCheckpointTrigger::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
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
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::Act);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "B";
        g_typeCounter++;
    }
    (*((CActRegPool<CCheckpointTrigger>::s_table.ResolveEntryCallReport(id2)))) =
        static_cast<i32 (CUserLogic::*)()>(&CCheckpointTrigger::AdvanceCheckpointAnimation);
}

RVA(0x0010f6a0, 0x235)
i32 CCheckpointTrigger::Act() {
    CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);

    for (i32 i = 0; i < m_firstEmpty; i++) {
        i32 key = m_state[i];
        if (key == 0) {
            return 0;
        }
        CTileTriggerSwitchLogic* child = play->m_beginMarker->FindChild(key, 8);
        if (child == 0) {
            g_gameReg->ReportError(0x80dd, 0x44c);
            return 0;
        }
        if (child->m_linkGate == 0) {
            return 0;
        }
    }

    m_prevAnimSetNode = m_objAux->m_1c;

    m_objAux->m_1c = ActFindId("B");
    m_value = m_wwdObject->m_1a0.m_14;
    m_wwdObject->ApplyLookupGeometry("GAME_CHECKPOINTFLAGSET", 0);

    if (play->m_frameMarker != 0) {
        i32 a = m_object->m_114;
        i32 b = m_object->m_118;
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
            b += b;
            a += a;
            if (b > 0x3b) {
                a++;
                b -= 0x3c;
            }
        }
        play->m_frameMarker->AddTime(a, b);
    }

    CObject* cue = m_wwdObject->OwnerMgr()->m_soundRegistry->Lookup("GAME_FLAGRISE");
    if (cue != 0) {
        static_cast<LeafCue*>(cue)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
    }
    g_gameReg->OnCheckpointReached();

    i32 hi = m_firstEmpty - 1;

    CGruntzMgr* reg = g_gameReg;
    i32 span = hi + 1;
    i32 pick;
    if (span == 0) {
        i32 seed;
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            pick = 0;
        } else {
            pick = hi;
        }
    } else {
        pick = reg->Rand() % span;
    }

    CTileTriggerSwitchLogic* pad = play->m_beginMarker->FindChild(m_state[pick], 8);
    if (pad == 0) {
        g_gameReg->ReportError(0x80dd, 0x44c);
        return 0;
    }

    i32 gy = pad->m_key0c;
    i32 gx = pad->m_tileX;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 owner;
    if (static_cast<u32>(gx) < grid->m_width && static_cast<u32>(gy) < grid->m_height) {
        owner = grid->m_rows[gy][gx].m_4;
    } else {
        owner = -1;
    }
    if (owner == -1) {
        return 0;
    }

    i32 ownerCol = (owner >> 8) & 0xff;
    owner &= 0xff;
    CGrunt* g = g_gameReg->m_cmdGrid->m_grid[ownerCol * TM_GRID_COLS + owner];
    if (g == 0) {
        return 0;
    }

    i32 sy = g->m_object->m_screenY;
    i32 sx = g->m_object->m_screenX;
    RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
    if (sx >= view->right) {
        return 0;
    }
    if (sx < view->left) {
        return 0;
    }
    if (sy >= view->bottom) {
        return 0;
    }
    if (sy < view->top) {
        return 0;
    }
    g_gameReg->m_cueSink->SpawnVoiceDriver(g, 0x334, -1, 0, -1, -1);
    return 0;
}

RVA(0x0010f970, 0x17)
i32 CCheckpointTrigger::AdvanceCheckpointAnimation() {
    m_wwdObject->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x0010f9a0, 0x8f)
i32 CCheckpointTrigger::SerializeMove(CFileMemBase* arc, i32 mode, i32 typeId, CGameObject* pObj) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
    switch (mode) {
        case 7:
            sa->Read(m_state, 0x3c);
            sa->Read(&m_firstEmpty, 4);
            break;
        case 4:
            sa->Write(m_state, 0x3c);
            sa->Write(&m_firstEmpty, 4);
            break;
    }
    if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(sa, mode, typeId, pObj) ? 1 : 0;
}

RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

RVA(0x0010faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 0x1000000;

    CGameObject* o = m_object;
    if (o->m_sortKey != 0) {
        o->m_sortKey = 0;
        o->m_flags |= 0x20000;
    }
}

RVA(0x0010fd10, 0x102)
void CTileTriggerTransition::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010fe70, 0x18d)
void CTileTriggerTransition::RegisterActs() {
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
    (*((CActRegPool<CTileTriggerTransition>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CTileTriggerTransition::TransitionAct);
}

RVA(0x00110070, 0x71)
i32 CTileTriggerTransition::ApplyAnimation(char* sprite, char* geom) {
    m_value = m_wwdObject->m_1a0.m_14;
    if (m_wwdObject->ApplyLookupGeometry(geom, 0) == 0) {
        return 0;
    }
    CAniElement* desc = m_wwdObject->m_1a0.m_14;
    CAniDesc* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    m_wwdObject->ApplyLookupSprite(sprite, elem->m_param);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    return 1;
}

RVA(0x00110110, 0x39)
i32 CTileTriggerTransition::TransitionAct() {
    m_wwdObject->m_1a0.Advance(g_engineFrameDelta);
    if (m_wwdObject->m_1a0.m_finished != 0 && m_wwdObject->m_1a0.m_frameTicksLeft == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

#include <rva.h>

#include <Gruntz/TileLogicPump.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/CheckpointTrigger.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/Random.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileTrigger.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitch.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WarpStonePad.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

#include <string.h>

template<> DATA(0x0024e6a0)
CActReg CActRegPool<CWarpStonePad>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024e798)
CActReg CActRegPool<CTileTriggerSwitch>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024e810)
CActReg CActRegPool<CTileTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024e7c0)
CActReg CActRegPool<CBrickz>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024e7e8)
CActReg CActRegPool<CCheckpointTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<> DATA(0x0024e720)
CActReg CActRegPool<CTileTriggerTransition>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                               \
    AnimWorkerObj* ctl = obj->m_animWorker;                                                        \
    AnimWorkerAct act = ctl->WorkerAct();                                                          \
    switch (act) {                                                                                 \
        case ACT_UNINITIALISED: {                                                                  \
            ctl->SetWorkerAct(ACT_LIVE);                                                           \
            LEAF* t = new LEAF(obj);                                                               \
            t->Activate();                                                                         \
            ctl->m_logic = t;                                                                      \
            break;                                                                                 \
        }                                                                                          \
        case ACT_OBJECT_REMOVED:                                                                   \
            ctl->m_logic->OnObjectRemoved();                                                       \
            break;                                                                                 \
        case ACT_LEAVE_ACTIVE_REGION:                                                              \
            ctl->m_logic->OnLeaveActiveRegion();                                                   \
            break;                                                                                 \
        case ACT_PREPARE_SAVE:                                                                     \
            ctl->m_logic->PrepareSave();                                                           \
            break;                                                                                 \
        case ACT_AFTER_SAVE:                                                                       \
            ctl->m_logic->AfterSave();                                                             \
            break;                                                                                 \
        case ACT_AFTER_LOAD:                                                                       \
            ctl->m_logic->AfterLoad();                                                             \
            break;                                                                                 \
        case ACT_AFTER_LOAD_REFERENCES:                                                            \
            ctl->m_logic->AfterLoadReferences();                                                   \
            break;                                                                                 \
        case ACT_LIVE:                                                                             \
            break;                                                                                 \
        default:                                                                                   \
            ProjTypeXfer(ctl->m_logic);                                                            \
            break;                                                                                 \
    }                                                                                              \
    return 1;

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00010f20, 0x47)
i32 CWarpStonePad::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x00010f90, 0x1e, ??_GCWarpStonePad@@UAEPAXI@Z)
RVA_COMPGEN(0x00010fc0, 0x44, ??1CWarpStonePad@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011030, 0x6)
LogicTypeId CTileTriggerSwitch::GetTypeTag() {
    return LOGIC_TILETRIGGERSWITCH;
}

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011050, 0x47)
i32 CTileTriggerSwitch::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x000110c0, 0x1e, ??_GCTileTriggerSwitch@@UAEPAXI@Z)
RVA_COMPGEN(0x000110f0, 0x44, ??1CTileTriggerSwitch@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000111d0, 0x6)
LogicTypeId CTileTrigger::GetTypeTag() {
    return LOGIC_TILETRIGGER;
}

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x00011260, 0x1e, ??_GCTileTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011290, 0x44, ??1CTileTrigger@@UAE@XZ)

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011320, 0x47)
i32 CBrickz::SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x00011390, 0x1e, ??_GCBrickz@@UAEPAXI@Z)
RVA_COMPGEN(0x000113c0, 0x44, ??1CBrickz@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011430, 0x6)
LogicTypeId CCheckpointTrigger::GetTypeTag() {
    return LOGIC_CHECKPOINTTRIGGER;
}

RVA_COMPGEN(0x00011450, 0x1e, ??_GCCheckpointTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011480, 0x44, ??1CCheckpointTrigger@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000114f0, 0x6)
LogicTypeId CTileSecretTrigger::GetTypeTag() {
    return LOGIC_TILESECRETTRIGGER;
}

RVA_COMPGEN(0x00011510, 0x1e, ??_GCTileSecretTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00011540, 0x44, ??1CTileSecretTrigger@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000115b0, 0x6)
LogicTypeId CGiantRock::GetTypeTag() {
    return LOGIC_GIANTROCK;
}

RVA_COMPGEN(0x000115d0, 0x1e, ??_GCGiantRock@@UAEPAXI@Z)
RVA_COMPGEN(0x00011600, 0x44, ??1CGiantRock@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011670, 0x6)
LogicTypeId CCoveredPowerup::GetTypeTag() {
    return LOGIC_COVEREDPOWERUP;
}

RVA_COMPGEN(0x00011690, 0x1e, ??_GCCoveredPowerup@@UAEPAXI@Z)
RVA_COMPGEN(0x000116c0, 0x44, ??1CCoveredPowerup@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011730, 0x6)
LogicTypeId CTileTriggerTransition::GetTypeTag() {
    return LOGIC_TILETRIGGERTRANSITION;
}

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011750, 0x47)
i32 CTileTriggerTransition::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
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
i32 CreateTileTriggerTransition(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerTransition)}

RVA(0x0010d290, 0xf4)
i32 CreateCheckpointTrigger(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    AnimWorkerAct act = ctl->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            ctl->SetWorkerAct(ACT_LIVE);
            CCheckpointTrigger* t = new CCheckpointTrigger(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case ACT_OBJECT_REMOVED:
            ctl->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            ctl->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            ctl->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            ctl->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            ctl->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
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
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
        m_wwdObject->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
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
            if (list != NULL) {
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
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");

    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
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
            if (list != NULL) {
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

// @early-stop
// The three m_object reloads now match retail; the residual is a callee-saved
// register rotation (edi/ebp/ecx) around the flag read-modify-writes.
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;

    i32 tileX = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = m_object->m_screenY >> TILE_SHIFT_PX;
    m_object->m_speedX = tileX;
    m_object->m_speedY = tileY;
    m_object->m_id = (tileX << 8) + tileY;
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
            if (list != NULL) {
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
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;

    i32 tileX = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = m_object->m_screenY >> TILE_SHIFT_PX;
    m_object->m_speedX = tileX;
    m_object->m_speedY = tileY;
    m_object->m_id = (tileX << 8) + tileY;
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
            if (list != NULL) {
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
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_flags |= 1;

    CWwdGameObjectA* o = m_object;
    i32 zk = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
    if (o->m_sortKey != zk) {
        o->m_sortKey = zk;
        o->m_flags |= 0x20000;
    }
    memset(m_state, 0, sizeof(m_state));
    if (m_object->m_extent.left == COORD_UNSET) {
        m_object->m_extent.left = 0;
    }
    if (m_object->m_area.left == COORD_UNSET) {
        m_object->m_area.left = 0;
    }
    if (m_object->m_switchRect.left == COORD_UNSET) {
        m_object->m_switchRect.left = 0;
    }
    if (m_object->m_clip.left == COORD_UNSET) {
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
            if (list != NULL) {
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
            if (list != NULL) {
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
        CTileTriggerSwitchLogic* child =
            play->m_beginMarker->FindChild(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (child == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), 0x44c);
            return 0;
        }
        if (child->m_linkGate == 0) {
            return 0;
        }
    }

    m_prevAnimSetNode = m_objAux->m_actKey;

    m_objAux->m_actKey = ActFindId("B");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CHECKPOINTFLAGSET", 0);

    if (play->m_frameMarker != NULL) {
        i32 a = m_object->m_score;
        i32 b = m_object->m_points;
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
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
    if (cue != NULL) {
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

    CTileTriggerSwitchLogic* pad =
        play->m_beginMarker->FindChild(m_state[pick], TRIGID_CHECKPOINT_SWITCH_8);
    if (pad == NULL) {
        g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), 0x44c);
        return 0;
    }

    i32 gy = pad->m_tileY;
    i32 gx = pad->m_tileX;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 owner;
    if (static_cast<u32>(gx) < grid->m_width && static_cast<u32>(gy) < grid->m_height) {
        owner = grid->m_rows[gy][gx].m_occupantId;
    } else {
        owner = -1;
    }
    if (owner == -1) {
        return 0;
    }

    i32 ownerCol = (owner >> 8) & 0xff;
    owner &= 0xff;
    CGrunt* g = g_gameReg->m_cmdGrid->m_grid[ownerCol * TM_GRID_COLS + owner];
    if (g == NULL) {
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
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x0010f9a0, 0x8f)
i32 CCheckpointTrigger::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
    switch (mode) {
        case SERIAL_LOAD:
            sa->Read(m_state, 0x3c);
            sa->Read(&m_firstEmpty, sizeof(m_firstEmpty));
            break;
        case SERIAL_SAVE:
            sa->Write(m_state, 0x3c);
            sa->Write(&m_firstEmpty, sizeof(m_firstEmpty));
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

// @early-stop
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
            if (list != NULL) {
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
    m_value = m_wwdObject->m_animCursor.m_animation;
    if (m_wwdObject->ApplyLookupGeometry(geom, 0) == 0) {
        return 0;
    }
    CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
    CAniRecordView* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0)) : 0;
    m_wwdObject->ApplyLookupSprite(sprite, elem->m_param);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    return 1;
}

RVA(0x00110110, 0x39)
i32 CTileTriggerTransition::TransitionAct() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (m_wwdObject->m_animCursor.m_finished != 0
        && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

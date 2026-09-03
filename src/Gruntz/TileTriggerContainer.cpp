#include <rva.h>

#include <Gruntz/TileTriggerContainer.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <new>

static inline i32 CellKey(i32 tileX, i32 tileY) {
    return (tileX << 8) + tileY;
}

RVA_DYNINIT(0x00115c30, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x00115c50, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x00115c80, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x00115ca0, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x00115cd0, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00115cf0, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00115d20, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x00115d40, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x00115d70, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x00115d90, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x00115dc0, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00115de0, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00115e10, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x00115e30, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x00115e60, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x00115e80, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x00115eb0, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x00115ed0, 0x1a, s_gruntDirCenter)

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00115b60, 0x97)
i32 DrawPageDebugText(
    CDDrawSurfaceMgr* mgr,
    const CString* text,
    RECT* dst,
    i32 fontFlag,
    b32 useFrontPage,
    i32 r,
    i32 g,
    i32 b
) {
    if (mgr == NULL) {
        return 0;
    }
    CDrawSubWorker* page;
    if (useFrontPage != false) {
        page = mgr->m_drawTarget->m_frontSurface;
        if (page == NULL) {
            return 0;
        }
    } else {
        page = mgr->m_drawTarget->m_backPair;
        if (page == NULL) {
            return 0;
        }
    }
    CDDSurface* surf = page->m_surface;
    if (surf == NULL) {
        return 0;
    }

    HDC hdc = NULL;
    surf->m_ddSurface->GetDC(&hdc);
    g_gameReg->m_chatLog->Draw3DText(text, hdc, dst, fontFlag, r, g, b, 1, 2, 3);
    surf->m_ddSurface->ReleaseDC(hdc);
    return 1;
}

RVA(0x00115f00, 0x13)
i32 CTileTriggerContainer::Initialize() {
    if (m_initialized != false) {
        return 0;
    }
    m_initialized = true;
    return 1;
}

RVA(0x00115f30, 0x18)
void CTileTriggerContainer::Shutdown() {
    if (m_initialized != false) {
        RemoveAll();
        m_initialized = false;
    }
}

RVA(0x00115f60, 0x300)
CTileTriggerSwitchLogic* CTileTriggerContainer::AddSwitchLogic(
    TrigLogicId logicType,
    i32 tileX,
    i32 tileY,
    i32 cellKey,

    RECT extent,
    RECT area,
    RECT switchRect,
    RECT clip,
    RECT switchRectA,
    RECT switchRectB,
    b32 isMatch,
    i32 damageParam,
    i32 checkpointType
) {
    CTileTriggerSwitchLogic* obj = NULL;
    switch (logicType) {
        case TRIGID_SWITCH_1:
        case TRIGID_SWITCH_2:
        case TRIGID_SWITCH_5:
            obj = new CTileTriggerSwitchLogic;
            break;
        case TRIGID_MULTI_SWITCH_3:
            obj = new CTileMultiTriggerSwitchLogic;
            break;
        case TRIGID_EXCLUSIVE_SWITCH_4:
            obj = new CTileExclusiveTriggerSwitchLogic;
            break;
        case TRIGID_SECRET_SWITCH_6:
            obj = new CTileSecretTriggerSwitchLogic;
            break;
        case TRIGID_TIME_SWITCH_7:
            obj = new CTileTimeTriggerSwitchLogic;
            break;
        case TRIGID_CHECKPOINT_SWITCH_8:
            obj = new CCheckpointTriggerSwitchLogic;
            break;
    }
    if (obj == NULL) {
        return NULL;
    }

    RECT local[6];
    local[0] = extent;
    local[1] = area;
    local[2] = switchRect;
    local[3] = clip;
    local[4] = switchRectA;
    local[5] = switchRectB;

    if (obj->BuildSmall(
            this,
            logicType,
            tileX,
            tileY,
            cellKey,
            local,
            isMatch,
            damageParam,
            checkpointType
        )
        == TRIGID_ANY) {

        delete obj;
        return NULL;
    }
    m_switchLogics.AddTail(obj);
    return obj;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00116320, 0x66)
i32 CTileTriggerContainer::RemoveSwitchLogic(i32 cellKey, TrigLogicId logicType) {
    POSITION pos = m_switchLogics.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerSwitchLogic* logic =
            static_cast<CTileTriggerSwitchLogic*>(m_switchLogics.GetNext(pos));
        if (logic->m_typeId == logicType && logic->m_cellKey == cellKey) {

            delete logic;
            m_switchLogics.RemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

RVA(0x001163b0, 0xb2)
CTileTriggerLogic* CTileTriggerContainer::AddLogicDefaults(
    TileCollisionKind tileType,
    TrigLogicId logicType,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    CRect empty(0, 0, 0, 0);
    return AddLogic(
        tileType,
        logicType,
        tileX,
        tileY,
        cellKey,
        empty,
        empty,
        empty,
        empty,
        empty,
        empty,
        tileToken,
        dutyOnSpan,
        leadInSpan,
        dutyOffSpan
    );
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001164a0, 0x116)
void CTileTriggerContainer::AddLogicFromRecord(
    TileCollisionKind tileType,
    TrigLogicId logicType,
    CGameObject* object
) {
    AddLogic(
        tileType,
        logicType,
        object->m_speed.m_x,
        object->m_speed.m_y,
        object->m_id,
        object->m_extent,
        object->m_area,
        object->m_switchRect,
        object->m_clip,
        object->m_logicRecord->m_userRect1,
        object->m_logicRecord->m_userRect2,
        object->m_smarts,
        object->m_damage,
        object->m_points,
        object->m_health
    );
}

__inline i32 CTileTriggerLogic::Build(
    CTileTriggerContainer* owner,
    TrigLogicId typeTag,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rects,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    if (m_initGate != false) {
        return 0;
    }
    memcpy(m_linkKeys, rects, sizeof(m_linkKeys));
    return Setup(
        owner,
        typeTag,
        tileX,
        tileY,
        cellKey,
        tileToken,
        dutyOnSpan,
        leadInSpan,
        dutyOffSpan
    );
}

__inline i32 CTileTriggerLogic::Setup(
    CTileTriggerContainer* owner,
    TrigLogicId typeTag,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    if (m_initGate != false) {
        return 0;
    }
    m_tile.Set(tileX, tileY);
    m_owner = owner;
    m_typeTag = typeTag;
    m_cellKey = cellKey;
    m_initGate = true;
    m_tileToken = tileToken;
    m_startClock = g_frameTime;
    m_leadInSpan = leadInSpan;
    m_dutyOn = false;
    m_dutyOnSpan = dutyOnSpan;
    m_dutyOffSpan = dutyOffSpan;
    if (typeTag != TRIGID_COVERED_POWERUP_26 && dutyOffSpan == 0) {
        m_dutyOffSpan = dutyOnSpan;
        m_startClock = g_frameTime;
    }
    return 1;
}

// @early-stop
RVA(0x00116610, 0x350)
CTileTriggerLogic* CTileTriggerContainer::AddLogic(
    TileCollisionKind tileType,
    TrigLogicId logicType,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    RECT extent,
    RECT area,
    RECT switchRect,
    RECT clip,
    RECT switchRectA,
    RECT switchRectB,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    CTileTriggerLogic* obj = NULL;
    switch (logicType) {
        case TRIGID_TILE_TRIGGER_21:
        case TRIGID_TILE_TRIGGER_24:
            obj = new CTileTriggerLogic;
            break;
        case TRIGID_SECRET_TRIGGER_25:
            obj = new CTileSecretTriggerLogic;
            break;
        case TRIGID_COVERED_POWERUP_26:
            obj = new CCoveredPowerupLogic;
            break;
        case TRIGID_TIME_TRIGGER_23:
            obj = new CTileTimeTriggerLogic;
            break;
    }
    if (obj == NULL) {
        return NULL;
    }

    RECT local[6];
    local[0] = extent;
    local[1] = area;
    local[2] = switchRect;
    local[3] = clip;
    local[4] = switchRectA;
    local[5] = switchRectB;

    if (obj->Build(
            this,
            logicType,
            tileX,
            tileY,
            cellKey,
            local,
            tileToken,
            dutyOnSpan,
            leadInSpan,
            dutyOffSpan
        )
        == TRIGID_ANY) {
        delete obj;
        return NULL;
    }

    if (logicType == TRIGID_TIME_TRIGGER_23) {
        m_timedLogics.AddTail(obj);
    } else {
        m_idleLogics.AddTail(obj);
    }
    if (logicType == TRIGID_TILE_TRIGGER_21
        && (tileType == TILEKIND_PYRAMID_LATCH_A || tileType == TILEKIND_PYRAMID_LATCH_B)) {
        m_latchedLeaf = obj;
    }
    return obj;
}

RVA(0x00116a40, 0xf5)
CTileActionEvent* CTileTriggerContainer::AddActionEvent(
    BrickTileId actionCode,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    RECT playerFlags
) {
    CTileActionEvent* event = new CTileActionEvent;
    if (event == NULL) {
        return NULL;
    }
    if (event->m_live == false) {
        event->m_tile.Set(tileX, tileY);
        event->m_cellKey = cellKey;
        event->m_playerFlags[0] = playerFlags.left;
        event->m_playerFlags[1] = playerFlags.top;
        event->m_playerFlags[3] = playerFlags.bottom;
        event->m_actionCode = actionCode;
        event->m_owner = this;
        event->m_live = true;
        event->m_playerFlags[2] = playerFlags.right;
        event->SetActionCode(actionCode);
        m_actionEvents.AddTail(event);
        return event;
    }
    delete event;
    return NULL;
}

RVA(0x00116b80, 0x120)
CTileActionEvent* CTileTriggerContainer::AddSwitchActionEvent(
    BrickTileId actionCode,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 playerSlot
) {
    CTileActionEvent* event = new CTileActionEvent;
    if (event == NULL) {
        return NULL;
    }
    i32 playerFlags[4] = {0, 0, 0, 0};
    CTileActionEvent* result = NULL;
    switch (static_cast<PlayerSlot>(playerSlot)) {
        case PLAYER_SLOT_1:
            playerFlags[1] = 1;
            break;
        case PLAYER_SLOT_2:
            playerFlags[2] = 1;
            break;
        case PLAYER_SLOT_3:
            playerFlags[3] = 1;
            break;
        case PLAYER_SLOT_ALL:
            playerFlags[1] = playerFlags[2] = playerFlags[3] = 1;
        case PLAYER_SLOT_0:
            playerFlags[0] = 1;
            break;
    }
    if (event->m_live == false) {
        event->m_tile.Set(tileX, tileY);
        event->m_cellKey = cellKey;
        event->m_actionCode = actionCode;
        event->m_owner = this;
        event->m_live = true;
        for (i32 i = 0; i < 4; i++) {
            event->m_playerFlags[i] = playerFlags[i];
        }
        event->SetActionCode(actionCode);
        m_actionEvents.AddTail(event);
        result = event;
    } else {
        delete event;
    }
    return result;
}

RVA(0x00116cf0, 0x111)
CGiantRockLogic* CTileTriggerContainer::AddGiantRockLogic(
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32* block9,
    i32 powerupType,
    i32 textId,
    i32 dutyOffSpan
) {
    CGiantRockLogic* e = new CGiantRockLogic;
    if (e == NULL) {
        return NULL;
    }
    if (e->m_initGate == false) {
        memcpy(e->m_matrix, block9, sizeof(e->m_matrix));
        e->m_powerupType = static_cast<PickupType>(powerupType);
        e->m_textId = textId;
        e->m_typeTag = TRIGID_GIANT_ROCK_22;
        e->m_tile.Set(tileX, tileY);
        e->m_cellKey = cellKey;
        e->m_owner = this;
        e->m_initGate = true;
        e->m_dutyOn = false;
        e->m_startClock = g_frameTime;
        e->m_dutyOnSpan = 0;
        e->m_tileToken = 0;
        e->m_leadInSpan = 0;
        e->m_dutyOffSpan = 0;
        e->m_startClock = g_frameTime;
        e->m_dutyOffSpan = dutyOffSpan;
        m_idleLogics.AddTail(e);
        return e;
    }
    delete e;
    return NULL;
}

RVA(0x00116e60, 0x59)
i32 CTileTriggerContainer::RemoveIdleLogic(CTileTriggerLogic* logic) {
    POSITION pos = m_idleLogics.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_idleLogics.GetNext(pos));
        if (elem == logic) {

            delete elem;
            m_idleLogics.RemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic*
CTileTriggerContainer::FindSwitchLogic(i32 cellKey, TrigLogicId logicType) {
    POSITION pos = m_switchLogics.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerSwitchLogic* logic =
            static_cast<CTileTriggerSwitchLogic*>(m_switchLogics.GetNext(pos));
        if (logic->m_cellKey == cellKey) {
            if (logicType == TRIGID_ANY || logic->m_typeId == logicType) {
                return logic;
            }
        }
    }
    return NULL;
}

RVA(0x00116f20, 0x51)
CTileTriggerLogic* CTileTriggerContainer::FindLogic(i32 cellKey, TrigLogicId logicType) {
    POSITION pos = m_idleLogics.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_idleLogics.GetNext(pos));
        if (elem->m_cellKey == cellKey) {
            if (logicType == TRIGID_ANY) {
                return elem;
            }
            if (elem->m_typeTag == logicType) {
                return elem;
            }
        }
    }
    pos = m_timedLogics.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_timedLogics.GetNext(pos));
        if (elem->m_cellKey == cellKey) {
            if (logicType == TRIGID_ANY) {
                return elem;
            }
            if (elem->m_typeTag == logicType) {
                return elem;
            }
        }
    }
    return NULL;
}

RVA(0x00116fa0, 0xc7)
void CTileTriggerContainer::RemoveAll() {
    POSITION pos = m_idleLogics.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_idleLogics.GetNext(pos));
        delete elem;
    }
    m_idleLogics.RemoveAll();
    pos = m_switchLogics.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerSwitchLogic* elem =
            static_cast<CTileTriggerSwitchLogic*>(m_switchLogics.GetNext(pos));
        delete elem;
    }
    m_switchLogics.RemoveAll();
    pos = m_timedLogics.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_timedLogics.GetNext(pos));
        delete elem;
    }
    m_timedLogics.RemoveAll();
    pos = m_actionEvents.GetHeadPosition();
    while (pos != NULL) {
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(m_actionEvents.GetNext(pos));
        delete elem;
    }
    m_actionEvents.RemoveAll();
    m_latchedLeaf = NULL;
}

RVA(0x001170b0, 0x72)
i32 CTileTriggerContainer::UpdateTimedLogics(i32 unusedFrameDelta) {
    POSITION pos = m_timedLogics.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_timedLogics.GetNext(pos));
        i32 disposition = elem->Classify(unusedFrameDelta);
        if (disposition == 0) {
            m_timedLogics.RemoveAt(cur);
            delete elem;
        } else if (disposition == -1) {
            m_timedLogics.RemoveAt(cur);
            m_idleLogics.AddTail(elem);
        }
    }
    return 1;
}

RVA(0x00117150, 0x53)
i32 CTileTriggerContainer::ActivateTimedLogic(CTileTriggerLogic* logic) {
    POSITION pos = m_idleLogics.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_idleLogics.GetNext(pos));
        if (elem == logic) {
            m_idleLogics.RemoveAt(cur);
            m_timedLogics.AddTail(elem);
            elem->m_dutyOn = false;
            return 1;
        }
    }
    return 0;
}

RVA(0x001171d0, 0x20)
CTileActionEvent* CTileTriggerContainer::FindActionByCellKey(i32 cellKey) {
    POSITION pos = m_actionEvents.GetHeadPosition();
    while (pos != NULL) {
        CTileActionEvent* data = static_cast<CTileActionEvent*>(m_actionEvents.GetNext(pos));
        if (data->m_cellKey == cellKey) {
            return data;
        }
    }
    return NULL;
}

RVA(0x00117200, 0x53)
i32 CTileTriggerContainer::RemoveActionEvent(CTileActionEvent* event) {
    POSITION pos = m_actionEvents.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur_node = pos;
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(m_actionEvents.GetNext(pos));
        if (elem == event) {
            delete elem;
            m_actionEvents.RemoveAt(cur_node);
            return 1;
        }
    }
    return 0;
}

RVA(0x00117280, 0x2ec)
i32 CTileTriggerContainer::Serialize(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (archive == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE: {
            POSITION pos;
            i32 count = m_switchLogics.GetCount();
            archive->Write(&count, sizeof(count));
            pos = m_switchLogics.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerSwitchLogic* logic =
                    static_cast<CTileTriggerSwitchLogic*>(m_switchLogics.GetNext(pos));
                if (SerializeSwitchLogic(archive, SERIAL_SAVE, typeId, payload, logic) == 0) {
                    return 0;
                }
            }
            count = m_idleLogics.GetCount();
            archive->Write(&count, sizeof(count));
            pos = m_idleLogics.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* logic =
                    static_cast<CTileTriggerLogic*>(m_idleLogics.GetNext(pos));
                if (SerializeTriggerLogic(archive, SERIAL_SAVE, typeId, payload, logic) == 0) {
                    return 0;
                }
            }
            count = m_timedLogics.GetCount();
            archive->Write(&count, sizeof(count));
            pos = m_timedLogics.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* logic =
                    static_cast<CTileTriggerLogic*>(m_timedLogics.GetNext(pos));
                if (SerializeTriggerLogic(archive, SERIAL_SAVE, typeId, payload, logic) == 0) {
                    return 0;
                }
            }
            count = m_actionEvents.GetCount();
            archive->Write(&count, sizeof(count));
            pos = m_actionEvents.GetHeadPosition();
            while (pos != NULL) {
                CTileActionEvent* event =
                    static_cast<CTileActionEvent*>(m_actionEvents.GetNext(pos));
                if (event->Serialize(archive, SERIAL_SAVE, typeId, payload) == 0) {
                    return 0;
                }
            }
            if (SaveInitialized(archive) == 0) {
                return 0;
            }
            break;
        }
        case SERIAL_LOAD: {
            RemoveAll();
            u32 n;
            u32 i;
            archive->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                CTileTriggerSwitchLogic* logic = static_cast<CTileTriggerSwitchLogic*>(
                    DeserializeLogic(archive, SERIAL_LOAD, typeId, payload)
                );
                if (logic == NULL) {
                    return 0;
                }
                m_switchLogics.AddTail(static_cast<void*>(logic));
            }
            archive->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                CTileTriggerLogic* logic = static_cast<CTileTriggerLogic*>(
                    DeserializeLogic(archive, SERIAL_LOAD, typeId, payload)
                );
                if (logic == NULL) {
                    return 0;
                }
                m_idleLogics.AddTail(static_cast<void*>(logic));
            }
            archive->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                CTileTriggerLogic* logic = static_cast<CTileTriggerLogic*>(
                    DeserializeLogic(archive, SERIAL_LOAD, typeId, payload)
                );
                if (logic == NULL) {
                    return 0;
                }
                m_timedLogics.AddTail(static_cast<void*>(logic));
            }
            archive->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                CTileActionEvent* event = new CTileActionEvent;
                if (event->Serialize(archive, SERIAL_LOAD, typeId, payload) == 0) {
                    return 0;
                }
                event->m_owner = this;
                m_actionEvents.AddTail(event);
            }
            if (LoadInitialized(archive) == 0) {
                return 0;
            }
            break;
        }
    }

    return 1;
}

RVA(0x00117630, 0xa4)
i32 CTileTriggerContainer::SerializeSwitchLogic(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload,
    CTileTriggerSwitchLogic* logic
) {
    if (logic == NULL) {
        return 0;
    }
    TrigLogicId tag = logic->m_typeId;
    archive->Write(&tag, sizeof(tag));

    switch (tag) {
        case TRIGID_SWITCH_1:

            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_SWITCH_2:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_MULTI_SWITCH_3:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_EXCLUSIVE_SWITCH_4:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_SWITCH_5:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_SECRET_SWITCH_6:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_TIME_SWITCH_7:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_CHECKPOINT_SWITCH_8:

            if (logic->SerializeDispatch(archive, mode, typeId, payload) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

RVA(0x00117710, 0xc0)
i32 CTileTriggerContainer::SerializeTriggerLogic(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload,
    CTileTriggerLogic* logic
) {
    if (logic == NULL) {
        return 0;
    }
    TrigLogicId tag = logic->m_typeTag;
    archive->Write(&tag, sizeof(tag));

    switch (tag) {
        case TRIGID_GIANT_ROCK_22:
            if ((static_cast<CGiantRockLogic*>(logic))
                    ->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_TILE_TRIGGER_21:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_TIME_TRIGGER_23:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_TILE_TRIGGER_24:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_SECRET_TRIGGER_25:
            if (logic->SerializeDispatch(archive, mode, typeId, payload)) {
                break;
            }
            return 0;
        case TRIGID_COVERED_POWERUP_26:
            if (logic->SerializeDispatch(archive, mode, typeId, payload) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

// @early-stop
RVA(0x00117800, 0x4d6)
void* CTileTriggerContainer::DeserializeLogic(
    CFileMemBase* reader,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (reader == NULL) {
        return NULL;
    }
    if (mode != SERIAL_LOAD) {
        return NULL;
    }
    TrigLogicId id;
    reader->Read(&id, sizeof(id));
    switch (id) {
        case TRIGID_SWITCH_1: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_SWITCH_2: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_MULTI_SWITCH_3: {
            CTileTriggerSwitchLogic* obj = new CTileMultiTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_EXCLUSIVE_SWITCH_4: {
            CTileTriggerSwitchLogic* obj = new CTileExclusiveTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_SWITCH_5: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_SECRET_SWITCH_6: {
            CTileTriggerSwitchLogic* obj = new CTileSecretTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_TIME_SWITCH_7: {
            CTileTriggerSwitchLogic* obj = new CTileTimeTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_CHECKPOINT_SWITCH_8: {
            CTileTriggerSwitchLogic* obj = new CCheckpointTriggerSwitchLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_TILE_TRIGGER_21: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;

            CGameLevel* level = g_gameReg->m_world->m_level;
            Coord tilePosition = obj->m_tile;
            tilePosition.Max(Coord(0, 0));
            tilePosition.Min(Coord(
                level->m_mainPlane->m_tileGridSize.cx - 1,
                level->m_mainPlane->m_tileGridSize.cy - 1
            ));
            i32 cell = level->m_mainPlane->m_tileRowOffsets[tilePosition.m_y] + tilePosition.m_x;
            i32 tile = level->m_mainPlane->m_tileHandles[cell];
            TileCollisionKind tileKind;
            if (tile == UNINIT_FILL || tile == -1) {
                tileKind = TILEKIND_PASSABLE;
            } else {

                CTileImageSet* rec = static_cast<CTileImageSet*>(
                    level->m_imageSets.GetData()[tile & WWD_TILE_IMAGE_SET_INDEX_MASK]
                );
                tileKind = rec->GetCollisionAt(0, 0);
            }
            if (tileKind == TILEKIND_PYRAMID_LATCH_A || tileKind == TILEKIND_PYRAMID_LATCH_B) {
                this->m_latchedLeaf = obj;
            }
            return obj;
        }
        case TRIGID_GIANT_ROCK_22: {
            CGiantRockLogic* obj = new CGiantRockLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_TIME_TRIGGER_23: {
            CTileTriggerLogic* obj = new CTileTimeTriggerLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_TILE_TRIGGER_24: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_SECRET_TRIGGER_25: {
            CTileTriggerLogic* obj = new CTileSecretTriggerLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_COVERED_POWERUP_26: {
            CTileTriggerLogic* obj = new CCoveredPowerupLogic;
            if (obj->SerializeDispatch(reader, SERIAL_LOAD, typeId, payload) == 0) {
                return NULL;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        default:
            return NULL;
    }
}

RVA(0x00117e20, 0x36)
i32 CTileTriggerContainer::SaveInitialized(CFileMemBase* archive) {
    if (archive == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    archive->Write(&m_initialized, sizeof(m_initialized));
    return 1;
}

RVA(0x00117e70, 0x36)
i32 CTileTriggerContainer::LoadInitialized(CFileMemBase* archive) {
    if (archive == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    archive->Read(&m_initialized, sizeof(m_initialized));
    return 1;
}

RVA(0x00117ec0, 0x7f)
CGiantRockLogic* CTileTriggerContainer::ScanNeighborhood(i32 tileX, i32 tileY) {
    for (i32 scanX = tileX - 1; scanX < tileX + 2; scanX++) {
        for (i32 scanY = tileY - 1; scanY < tileY + 2; scanY++) {

            CGiantRockLogic* logic = static_cast<CGiantRockLogic*>(
                FindLogic(CellKey(scanX, scanY), TRIGID_GIANT_ROCK_22)
            );
            if (logic != NULL) {
                return logic;
            }
        }
    }
    return NULL;
}

// @early-stop
RVA(0x00117f60, 0xa1)
i32 CTileTriggerContainer::SetCell(i32 tileX, i32 tileY, i32 playerSlot) {
    CTileActionEvent* elem = FindActionByCellKey(CellKey(tileX, tileY));
    if (elem != NULL) {
        if (playerSlot == IDX(PLAYER_SLOT_ALL)) {
            i32* flags = elem->m_playerFlags;
            for (i32 i = 0; i < 4; i++) {
                flags[i] = 1;
            }
        } else {
            elem->m_playerFlags[playerSlot] = 1;
        }
        elem->SetActionCode(elem->m_actionCode);
        return 1;
    }

    if (FindLogic(CellKey(tileX, tileY), TRIGID_COVERED_POWERUP_26) != NULL) {
        return 1;
    }
    CGiantRockLogic* found = ScanNeighborhood(tileX, tileY);
    return found != NULL;
}

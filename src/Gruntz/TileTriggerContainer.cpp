#include <rva.h>

#include <Gruntz/TileTriggerContainer.h>

#include <Mfc.h>

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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00115b60, 0x97)
i32 DrawPageDebugText(
    CDDrawSurfaceMgr* mgr,
    const CString* text,
    RECT* dst,
    i32 fontFlag,
    i32 useFrontPage,
    i32 r,
    i32 g,
    i32 b
) {
    if (mgr == NULL) {
        return 0;
    }
    CDrawSubWorker* page;
    if (useFrontPage != 0) {
        page = mgr->m_drawTarget->m_frontPair;
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

    HDC hdc = 0;
    surf->m_ddSurface->GetDC(&hdc);
    g_gameReg->m_chatLog->Draw3DText(text, hdc, dst, fontFlag, r, g, b, 1, 2, 3);
    surf->m_ddSurface->ReleaseDC(hdc);
    return 1;
}

RVA(0x00115f00, 0x13)
i32 CTileTriggerContainer::GetFlag74() {
    if (m_built != 0) {
        return 0;
    }
    m_built = 1;
    return 1;
}

RVA(0x00115f30, 0x18)
void CTileTriggerContainer::DtorBase() {
    if (m_built != 0) {
        RemoveAll();
        m_built = 0;
    }
}

RVA(0x00115f60, 0x300)
CTileTriggerSwitchLogic* CTileTriggerContainer::AddSwitchLogic(
    TrigLogicId tag,
    i32 col,
    i32 row,
    i32 key,

    RECT extent,
    RECT area,
    RECT switchRect,
    RECT clip,
    RECT switchRectA,
    RECT switchRectB,
    i32 isMatch,
    i32 damageParam,
    i32 checkpointType
) {
    CTileTriggerSwitchLogic* obj = 0;
    switch (tag) {
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
        return 0;
    }

    RECT local[6];
    local[0] = extent;
    local[1] = area;
    local[2] = switchRect;
    local[3] = clip;
    local[4] = switchRectA;
    local[5] = switchRectB;

    if (obj->BuildSmall(this, tag, col, row, key, local, isMatch, damageParam, checkpointType)
        == 0) {

        delete obj;
        return 0;
    }
    m_base.AddTail(obj);
    return obj;
}

RVA(0x00116320, 0x66)
i32 CTileTriggerContainer::RemoveByKeys(i32 k1, TrigLogicId k2) {
    POSITION pos = m_base.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerSwitchLogic* data = static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
        if (data->m_typeId == k2 && data->m_cellKey == k1) {

            delete data;
            m_base.RemoveAt(cur);
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
    RECT empty = {0, 0, 0, 0};
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

RVA(0x001164a0, 0x116)
void CTileTriggerContainer::AddLogicFromRecord(
    TileCollisionKind tileType,
    TrigLogicId logicType,
    CGameObject* object
) {
    AddLogic(
        tileType,
        logicType,
        object->m_speedX,
        object->m_speedY,
        object->m_id,
        object->m_extent,
        object->m_area,
        object->m_switchRect,
        object->m_clip,
        object->m_animWorker->m_userRect1,
        object->m_animWorker->m_userRect2,
        object->m_smarts,
        object->m_damage,
        object->m_points,
        object->m_health
    );
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
    CTileTriggerLogic* obj = 0;
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
        return 0;
    }

    RECT local[6];
    local[0] = extent;
    local[1] = area;
    local[2] = switchRect;
    local[3] = clip;
    local[4] = switchRectA;
    local[5] = switchRectB;

    i32 ok = 0;
    if (obj->m_initGate == 0) {
        memcpy(obj->m_linkKeys, local, sizeof(local));
        if (obj->m_initGate == 0) {
            obj->m_tileY = tileY;
            obj->m_tileX = tileX;
            obj->m_owner = this;
            obj->m_typeTag = logicType;
            obj->m_cellKey = cellKey;
            obj->m_initGate = 1;
            obj->m_tileToken = tileToken;
            obj->m_startClock = g_frameTime;
            obj->m_leadInSpan = leadInSpan;
            obj->m_dutyOn = 0;
            obj->m_dutyOnSpan = dutyOnSpan;
            obj->m_dutyOffSpan = dutyOffSpan;
            if (logicType != TRIGID_COVERED_POWERUP_26 && dutyOffSpan == 0) {
                obj->m_dutyOffSpan = dutyOnSpan;
                obj->m_startClock = g_frameTime;
            }
            ok = 1;
        }
    }

    if (ok == 0) {
        delete obj;
        return 0;
    }

    CPtrList* list = logicType == TRIGID_TIME_TRIGGER_23 ? &m_list2 : &m_list1;
    list->AddTail(obj);
    if (logicType == TRIGID_TILE_TRIGGER_21
        && (tileType == TILEKIND_PYRAMID_LATCH_A || tileType == TILEKIND_PYRAMID_LATCH_B)) {
        m_latchedLeaf = obj;
    }
    return obj;
}

// @early-stop
RVA(0x00116a40, 0xf5)
CTileActionEvent* CTileTriggerContainer::AddToList3(
    BrickTileId actionCode,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 player0,
    i32 player1,
    i32 player2,
    i32 player3
) {
    CTileActionEvent* m = new CTileActionEvent;
    if (m == NULL) {
        return 0;
    }
    if (m->m_live == 0) {
        m->m_tileX = tileX;
        m->m_tileY = tileY;
        m->m_cellKey = cellKey;
        m->m_playerFlags[0] = player0;
        m->m_playerFlags[1] = player1;
        m->m_playerFlags[3] = player3;
        m->m_actionCode = actionCode;
        m->m_owner = this;
        m->m_live = 1;
        m->m_playerFlags[2] = player2;
        m->SetActionCode(actionCode);
        m_list3.AddTail(m);
        return m;
    }
    delete m;
    return 0;
}

// @early-stop
RVA(0x00116b80, 0x120)
CTileActionEvent* CTileTriggerContainer::AddToList3Switch(
    BrickTileId actionCode,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 playerSlot
) {
    CTileActionEvent* m = new CTileActionEvent;
    if (m == NULL) {
        return 0;
    }
    i32 a = 0, b = 0, c = 0, d = 0;
    switch (static_cast<PlayerSlot>(playerSlot)) {
        case PLAYER_SLOT_1:
            c = 1;
            break;
        case PLAYER_SLOT_2:
            b = 1;
            break;
        case PLAYER_SLOT_3:
            a = 1;
            break;
        case PLAYER_SLOT_ALL:
            a = b = c = 1;
            // falls through to PLAYER_SLOT_0's d = 1
        case PLAYER_SLOT_0:
            d = 1;
            break;
    }
    if (m->m_live != 0) {
        delete m;
        return 0;
    }
    m->m_tileX = tileX;
    m->m_tileY = tileY;
    m->m_cellKey = cellKey;
    m->m_playerFlags[2] = b;
    m->m_actionCode = actionCode;
    m->m_owner = this;
    m->m_live = 1;
    m->m_playerFlags[0] = d;
    m->m_playerFlags[1] = c;
    m->m_playerFlags[3] = a;
    m->SetActionCode(actionCode);
    m_list3.AddTail(m);
    return m;
}

// @early-stop
RVA(0x00116cf0, 0x111)
CGiantRockLogic* CTileTriggerContainer::AddToList1(
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
        return 0;
    }
    if (e->m_initGate == 0) {
        memcpy(e->m_matrix, block9, sizeof(e->m_matrix));
        e->m_powerupType = static_cast<PickupType>(powerupType);
        e->m_textId = textId;
        e->m_typeTag = TRIGID_GIANT_ROCK_22;
        e->m_tileX = tileX;
        e->m_tileY = tileY;
        e->m_cellKey = cellKey;
        e->m_owner = this;
        e->m_initGate = 1;
        e->m_dutyOn = 0;
        e->m_startClock = g_frameTime;
        e->m_dutyOnSpan = 0;
        e->m_tileToken = 0;
        e->m_leadInSpan = 0;
        e->m_dutyOffSpan = 0;
        e->m_startClock = g_frameTime;
        e->m_dutyOffSpan = dutyOffSpan;
        m_list1.AddTail(e);
        return e;
    }

    CTileTriggerLogic* dead = e;
    delete dead;
    return 0;
}

RVA(0x00116e60, 0x59)
i32 CTileTriggerContainer::DelFromList1(CTileTriggerLogic* want) {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
        if (elem == want) {

            delete elem;
            m_list1.RemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic* CTileTriggerContainer::FindChild(i32 k1, TrigLogicId k2) {
    POSITION pos = m_base.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerSwitchLogic* data = static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
        if (data->m_cellKey == k1) {
            if (k2 == TRIGID_ANY || data->m_typeId == k2) {
                return data;
            }
        }
    }
    return 0;
}

RVA(0x00116f20, 0x51)
CTileTriggerLogic* CTileTriggerContainer::FindInLists12(i32 a, TrigLogicId b) {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
        if (elem->m_cellKey == a) {
            if (b == TRIGID_ANY) {
                return elem;
            }
            if (elem->m_typeTag == b) {
                return elem;
            }
        }
    }
    pos = m_list2.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
        if (elem->m_cellKey == a) {
            if (b == TRIGID_ANY) {
                return elem;
            }
            if (elem->m_typeTag == b) {
                return elem;
            }
        }
    }
    return 0;
}

RVA(0x00116fa0, 0xc7)
void CTileTriggerContainer::RemoveAll() {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
        delete elem;
    }
    m_list1.RemoveAll();
    pos = m_base.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerSwitchLogic* elem = static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
        delete elem;
    }
    m_base.RemoveAll();
    pos = m_list2.GetHeadPosition();
    while (pos != NULL) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
        delete elem;
    }
    m_list2.RemoveAll();
    pos = m_list3.GetHeadPosition();
    while (pos != NULL) {
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
        delete elem;
    }
    m_list3.RemoveAll();
    m_latchedLeaf = NULL;
}

RVA(0x001170b0, 0x72)
i32 CTileTriggerContainer::FilterList2(i32 arg) {
    POSITION pos = m_list2.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
        i32 r = elem->Classify(arg);
        if (r == 0) {
            m_list2.RemoveAt(cur);
            delete elem;
        } else if (r == -1) {
            m_list2.RemoveAt(cur);
            m_list1.AddTail(elem);
        }
    }
    return 1;
}

RVA(0x00117150, 0x53)
i32 CTileTriggerContainer::MoveList1ToList2(void* data) {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur = pos;
        void* elem = m_list1.GetNext(pos);
        if (elem == data) {
            m_list1.RemoveAt(cur);
            m_list2.AddTail(elem);
            *(static_cast<i32*>(elem) + 14) = 0;
            return 1;
        }
    }
    return 0;
}

RVA(0x001171d0, 0x20)
CTileActionEvent* CTileTriggerContainer::FindActionByCellKey(i32 cellKey) {
    POSITION pos = m_list3.GetHeadPosition();
    while (pos != NULL) {
        CTileActionEvent* data = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
        if (data->m_cellKey == cellKey) {
            return data;
        }
    }
    return 0;
}

RVA(0x00117200, 0x53)
i32 CTileTriggerContainer::DelFromList3(CTileActionEvent* want) {
    POSITION pos = m_list3.GetHeadPosition();
    while (pos != NULL) {
        POSITION cur_node = pos;
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
        if (elem == want) {
            delete elem;
            m_list3.RemoveAt(cur_node);
            return 1;
        }
    }
    return 0;
}

// @early-stop
// Residue: cl folds the two terminal `return 0`/`return 1` pairs into
// neg/sbb/neg where retail branches, and lays the switch's implicit default out
// as its own `mov eax,1` block instead of jumping to the shared return.
RVA(0x00117280, 0x2ec)
i32 CTileTriggerContainer::Serialize(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj) {
    if (s == NULL) {
        return 0;
    }
    switch (op) {
        case SERIAL_SAVE: {
            POSITION pos;
            i32 cnt = m_base.GetCount();
            s->Write(&cnt, sizeof(cnt));
            pos = m_base.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerSwitchLogic* e0 =
                    static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
                if (SerializeApplyA(s, SERIAL_SAVE, typeId, pObj, e0) == 0) {
                    return 0;
                }
            }
            cnt = m_list1.GetCount();
            s->Write(&cnt, sizeof(cnt));
            pos = m_list1.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* e1 = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
                if (SerializeApplyB(s, SERIAL_SAVE, typeId, pObj, e1) == 0) {
                    return 0;
                }
            }
            cnt = m_list2.GetCount();
            s->Write(&cnt, sizeof(cnt));
            pos = m_list2.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* e2 = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
                if (SerializeApplyB(s, SERIAL_SAVE, typeId, pObj, e2) == 0) {
                    return 0;
                }
            }
            cnt = m_list3.GetCount();
            s->Write(&cnt, sizeof(cnt));
            pos = m_list3.GetHeadPosition();
            while (pos != NULL) {
                CTileActionEvent* e3 = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
                if (e3->Serialize(s, SERIAL_SAVE, typeId, pObj) == 0) {
                    return 0;
                }
            }
            if (TransferFlag74(s) == 0) {
                return 0;
            }
            return 1;
        }
        case SERIAL_LOAD: {
            RemoveAll();
            u32 n;
            u32 i;
            void* e;
            s->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                e = LoadElement(s, SERIAL_LOAD, typeId, pObj);
                if (e == NULL) {
                    return 0;
                }
                m_base.AddTail(e);
            }
            s->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                e = LoadElement(s, SERIAL_LOAD, typeId, pObj);
                if (e == NULL) {
                    return 0;
                }
                m_list1.AddTail(e);
            }
            s->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                e = LoadElement(s, SERIAL_LOAD, typeId, pObj);
                if (e == NULL) {
                    return 0;
                }
                m_list2.AddTail(e);
            }
            s->Read(&n, sizeof(n));
            for (i = 0; i < n; i++) {
                CTileActionEvent* m = new CTileActionEvent;
                if (m->Serialize(s, SERIAL_LOAD, typeId, pObj) == 0) {
                    return 0;
                }
                m->m_owner = this;
                m_list3.AddTail(m);
            }
            if (LoadFlag74(s) == 0) {
                return 0;
            }
            return 1;
        }
    }

    return 1;
}

RVA(0x00117630, 0xa4)
i32 CTileTriggerContainer::SerializeApplyA(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 pObj,
    CTileTriggerSwitchLogic* o
) {
    if (o == NULL) {
        return 0;
    }
    TrigLogicId tag = o->m_typeId;
    s->Write(&tag, sizeof(tag));

    switch (tag) {
        case TRIGID_SWITCH_1:

            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_SWITCH_2:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_MULTI_SWITCH_3:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_EXCLUSIVE_SWITCH_4:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_SWITCH_5:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_SECRET_SWITCH_6:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_TIME_SWITCH_7:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_CHECKPOINT_SWITCH_8:

            if (o->ValidateByType(s, mode, typeId, pObj) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

RVA(0x00117710, 0xc0)
i32 CTileTriggerContainer::SerializeApplyB(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 pObj,
    CTileTriggerLogic* o
) {
    if (o == NULL) {
        return 0;
    }
    TrigLogicId tag = o->m_typeTag;
    s->Write(&tag, sizeof(tag));

    switch (tag) {
        case TRIGID_GIANT_ROCK_22:
            if ((static_cast<CGiantRockLogic*>(o))->ApplyByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_TILE_TRIGGER_21:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_TIME_TRIGGER_23:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_TILE_TRIGGER_24:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_SECRET_TRIGGER_25:
            if (o->ValidateByType(s, mode, typeId, pObj)) {
                break;
            }
            return 0;
        case TRIGID_COVERED_POWERUP_26:
            if (o->ValidateByType(s, mode, typeId, pObj) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

// @early-stop
// residue: two switch arms whose identical tails cl emits in full where retail
// cross-jumps them into the family's single copy.
RVA(0x00117800, 0x4d6)
void* CTileTriggerContainer::LoadElement(
    CFileMemBase* reader,
    SerialMode kind,
    LogicTypeId typeId,
    i32 pObj
) {
    if (reader == NULL) {
        return 0;
    }
    if (kind != SERIAL_LOAD) {
        return 0;
    }
    // Ingest: the archive stores the logic tag as a raw dword.
    TrigLogicId id;
    reader->Read(&id, sizeof(id));
    switch (id) {
        case TRIGID_SWITCH_1: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_SWITCH_2: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_MULTI_SWITCH_3: {
            CTileTriggerSwitchLogic* obj = new CTileMultiTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_EXCLUSIVE_SWITCH_4: {
            CTileTriggerSwitchLogic* obj = new CTileExclusiveTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_SWITCH_5: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_SECRET_SWITCH_6: {
            CTileTriggerSwitchLogic* obj = new CTileSecretTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_TIME_SWITCH_7: {
            CTileTriggerSwitchLogic* obj = new CTileTimeTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_CHECKPOINT_SWITCH_8: {
            CTileTriggerSwitchLogic* obj = new CCheckpointTriggerSwitchLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeId = id;
            return obj;
        }
        case TRIGID_TILE_TRIGGER_21: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;

            CGameLevel* level = g_gameReg->m_world->m_level;
            i32 x = obj->m_tileX;
            i32 y = obj->m_tileY;
            if (x < 0) {
                x = 0;
            } else if (x >= level->m_mainPlane->m_gridW) {
                x = level->m_mainPlane->m_gridW - 1;
            }
            if (y < 0) {
                y = 0;
            } else if (y >= level->m_mainPlane->m_gridH) {
                y = level->m_mainPlane->m_gridH - 1;
            }
            i32 cell = level->m_mainPlane->m_colOffsets[y] + x;
            i32 tile = level->m_mainPlane->m_tileGrid[cell];
            TileCollisionKind tileKind;
            if (tile == UNINIT_FILL || tile == -1) {
                tileKind = TILEKIND_PASSABLE;
            } else {

                CTileImageSet* rec =
                    static_cast<CTileImageSet*>(level->m_imageSets.GetData()[tile & 0xffff]);
                // Ingest: the raw WWD attribute byte for this cell.
                tileKind = rec->GetCollisionAt(0, 0);
            }
            if (tileKind == TILEKIND_PYRAMID_LATCH_A || tileKind == TILEKIND_PYRAMID_LATCH_B) {
                this->m_latchedLeaf = obj;
            }
            return obj;
        }
        case TRIGID_GIANT_ROCK_22: {
            CGiantRockLogic* obj = new CGiantRockLogic;
            if (obj->ApplyByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_TIME_TRIGGER_23: {
            CTileTriggerLogic* obj = new CTileTimeTriggerLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_TILE_TRIGGER_24: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_SECRET_TRIGGER_25: {
            CTileTriggerLogic* obj = new CTileSecretTriggerLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case TRIGID_COVERED_POWERUP_26: {
            CTileTriggerLogic* obj = new CCoveredPowerupLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        default:
            return 0;
    }
}

RVA(0x00117e20, 0x36)
i32 CTileTriggerContainer::TransferFlag74(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Write(&m_built, sizeof(m_built));
    return 1;
}

RVA(0x00117e70, 0x36)
i32 CTileTriggerContainer::LoadFlag74(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Read(&m_built, sizeof(m_built));
    return 1;
}

RVA(0x00117ec0, 0x7f)
CGiantRockLogic* CTileTriggerContainer::ScanNeighborhood(i32 x, i32 y) {
    for (i32 px = x - 1; px < x + 2; px++) {
        for (i32 py = y - 1; py < y + 2; py++) {

            CGiantRockLogic* r =
                static_cast<CGiantRockLogic*>(FindInLists12(py + (px << 8), TRIGID_GIANT_ROCK_22));
            if (r != NULL) {
                return r;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x00117f60, 0xa1)
i32 CTileTriggerContainer::SetCell(i32 tileX, i32 tileY, i32 playerSlot) {
    i32 key = (tileX << 8) + tileY;
    CTileActionEvent* elem = FindActionByCellKey(key);
    if (elem != NULL) {
        if (playerSlot == IDX(PLAYER_SLOT_ALL)) {
            elem->m_playerFlags[0] = elem->m_playerFlags[1] = elem->m_playerFlags[2] =
                elem->m_playerFlags[3] = 1;
        } else {
            elem->m_playerFlags[playerSlot] = 1;
        }
        elem->SetActionCode(elem->m_actionCode);
        return 1;
    }

    if (FindInLists12(key, TRIGID_COVERED_POWERUP_26) != NULL) {
        return 1;
    }
    return ScanNeighborhood(tileX, tileY) != NULL;
}

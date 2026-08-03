#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Play.h>
#include <Gruntz/Random.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Gruntz/SbiSideTabBuildViews.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTabWidgets.h>
#include <Gruntz/StatusBarUpdatersViews.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/WarpStoneFly.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/RezList.h>
#include <Rez/RezMgr.h>
#include <Utils/MapTyped.h>
#include <Utils/RegistryHelper.h>

#include <math.h>
#include <string.h>

DATA(0x00244c54)
i32 g_curPlayer = 0;

static CStatusBarMgr* volatile g_forceStatusBarMgrDtor;
#pragma inline_depth(0)
void ForceEmitStatusBarMgrDtor() {
    g_forceStatusBarMgrDtor->~CStatusBarMgr();
}
#pragma inline_depth()

RVA_COMPGEN(0x000c8980, 0x64, ??1CStatusBarMgr@@QAE@XZ)

RVA(0x00100530, 0x5)
i32 CStatusBarItem::OnPointerMove(i32, i32, i32) {
    return 0;
}
RVA(0x00100550, 0x5)
i32 CStatusBarItem::Click1c(i32, i32, i32) {
    return 0;
}
RVA(0x00100570, 0x5)
i32 CStatusBarItem::UnusedPointerAction(i32, i32, i32) {
    return 0;
}
RVA(0x00100590, 0x5)
i32 CStatusBarItem::Click24(i32, i32, i32) {
    return 0;
}

RVA_COMPGEN(0x00100620, 0x24, ??_GCStatusBarItem@@UAEPAXI@Z)
RVA_COMPGEN(0x001006d0, 0x1e, ??_GCSBI_RectOnly@@UAEPAXI@Z)
RVA_COMPGEN(0x00100700, 0x55, ??1CSBI_RectOnly@@UAE@XZ)
RVA_COMPGEN(0x00100780, 0xb, ??1CStatusBarItem@@UAE@XZ)
RVA_COMPGEN(0x001007a0, 0x1e, ??_GCSBI_MenuItem@@UAEPAXI@Z)
RVA_COMPGEN(0x001007d0, 0x7f, ??1CSBI_MenuItem@@UAE@XZ)
RVA_COMPGEN(0x00100870, 0x6a, ??1CSBI_Image@@UAE@XZ)
RVA_COMPGEN(0x00100900, 0x1e, ??_GCSBI_Image@@UAEPAXI@Z)
RVA(0x00105520, 0x21)
void CStatusBarMgr::ResetSlots() {
    for (i32 i = 0; i < 5; i++) {
        ArmSlot(i);
    }
    m_activeSlot = -1;
}

RVA(0x00105560, 0x33)
void CStatusBarMgr::ArmSlot(i32 idx) {
    m_slots[idx].m_state = kSlotArmed;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
}

RVA(0x00105710, 0x23)
i32 CStatusBarMgr::AnySlotActive() {
    for (i32 i = 0; i < 5; i++) {
        if (LoadGooCookingSprite(i)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00105750, 0x1f)
void CStatusBarMgr::AdvanceGauge(i32 delta) {
    i32 v = m_gauge + delta;
    if (v >= 100) {
        v = 100;
    }
    m_gaugeTarget = v;
}

RVA(0x001057d0, 0x13)
void CStatusBarMgr::SetGauge(i32 value) {
    m_gaugeTarget = value;
    m_gauge = value;
}

// @early-stop
RVA(0x00105800, 0x9e)
i32 CStatusBarMgr::PlaceCursorTarget(i32 row, i32 commit) {
    i32 col = g_curPlayer;
    if (g_gameReg->m_cmdGrid->ResetCell(col, row, 0, 0) != 0) {

        CGrunt* entry = g_gameReg->m_cmdGrid->m_grid[row + col * TM_GRID_COLS];
        if (entry != 0) {
            (static_cast<CPlay*>(g_gameReg->m_curState))
                ->ResetGoals(entry->m_object->m_screenX, entry->m_object->m_screenY);
            if (commit != 0) {
                CTriggerMgr* obj = g_gameReg->m_cmdGrid;
                if (obj->RecordListHas(col, row)) {
                    obj->m_recordPosition.m_x = col;
                    obj->m_recordPosition.m_y = row;
                    obj->m_armed = 1;
                    obj->LoadCameraSprite();
                }
            }
            return 1;
        }
    }
    return 0;
}

RVA(0x001058d0, 0x34)
void CStatusBarMgr::RefreshAll() {
    UpdateGruntOvenStatusBar();
    TickGauge();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructButtonStatusBar();
}

RVA(0x00105920, 0x47)
void CStatusBarMgr::Reset() {
    ResetSlots();
    m_gaugeTarget = 0;
    m_gauge = 0;
    ResetGroupA();
    UpdateRezMachineSnoozeStatusBar();
    InitTabRects();
    m_modeState = 1;
    m_destructWarnActive = 0;
}

RVA(0x00107aa0, 0x23)
void CStatusBarMgr::ToggleStat(i32 idx) {
    if (m_statFlags[idx]) {
        ClearStat(idx);
    } else {
        LoadStatzTabToggleSprite(idx, 1);
    }
}

// @early-stop
RVA(0x00105280, 0x61)
i32 CStatusBarMgr::HitTest(i32 x, i32 y) {
    if (m_hitTestDisabled == 0) {
        for (i32 i = 0; i < 15; i++) {
            CSBI_SideTab* p = m_hitRects[i];
            if (p && p->m_enabled) {
                i32 hit = p->m_enabled && x < p->m_rect14.right && x >= p->m_rect14.left
                          && y < p->m_rect14.bottom && y >= p->m_rect14.top;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

RVA(0x00106610, 0x3b)
void CStatusBarMgr::ResetGroupA() {
    for (i32 i = 0; i < 3; i++) {
        m_groupSlots[i].m_state = kSlotArmed;
        m_groupSlots[i].m_value = 1;
        if (m_groupNotify[i]) {
            m_groupNotify[i]->Notify(-1);
        }
    }
}

// @early-stop
RVA(0x001066f0, 0x3b)
void CStatusBarMgr::SetHudRectA(i32 y0, i32 x0, i32 z) {
    m_machineA.m_counter = y0;
    m_machineA.m_state = x0;
    m_machineA.m_interval = static_cast<u32>(z);
    m_machineA.m_last = g_frameTime;
}

// @early-stop
RVA(0x00106740, 0x3b)
void CStatusBarMgr::SetHudRectB(i32 y0, i32 x0, i32 z) {
    m_machineB.m_counter = y0;
    m_machineB.m_state = x0;
    m_machineB.m_interval = static_cast<u32>(z);
    m_machineB.m_last = g_frameTime;
}

RVA(0x00106790, 0x62)
void CStatusBarMgr::CommitSlot(i32 active) {
    if (active) {
        ArmSlot(m_activeSlot);
        m_activeSlot = -1;
    } else {
        m_slots[m_activeSlot].m_value = kSlotCommitLevel;
        if (m_slotNotify[m_activeSlot]) {
            m_slotNotify[m_activeSlot]->Notify(m_slots[m_activeSlot].m_value);
        }
        m_activeSlot = -1;
    }
}

RVA(0x001069c0, 0x2e)
void CStatusBarMgr::ClearHlCell(i32 row, i32 group) {
    i32 idx = group + row * 4;
    m_hlGrid[idx].m_state = 0;
    m_hlGrid[idx].m_value = 0;
    NotifyAllSlots();
}

RVA(0x00106af0, 0x37)
i32 CStatusBarMgr::SetHlCellByTier(i32 handle, i32 group) {
    i32 row;
    if (handle >= 0x22) {
        row = 2;
    } else {
        row = (handle >= 0x17);
    }
    return SetHlCell(row, handle, group);
}

RVA(0x00106b40, 0x44)
i32 CStatusBarMgr::SetHlCell(i32 row, i32 handle, i32 group) {
    i32 idx = group + row * 4;
    if (m_hlGrid[idx].m_state) {
        return 0;
    }
    m_hlGrid[idx].m_value = handle;
    m_hlGrid[idx].m_state = 1;
    NotifyAllSlots();
    return 1;
}

RVA(0x00106a00, 0xbf)
void CStatusBarMgr::NotifyAllSlots() {
    if (m_notify0) {
        m_notify0->SetSubtype();
    }
    if (m_notify2) {
        m_notify2->SetSubtype();
    }
    if (m_notify3) {
        m_notify3->SetSubtype();
    }
    if (m_extraNotify0 && m_extraNotifyArg0) {
        m_extraNotify0->Notify(m_extraNotifyArg0);
    }

    CSBI_ImageSet** p = &m_hlNotify[4];
    i32* h = &m_hlGrid[4].m_value;
    for (i32 n = 0; n < 4; n++) {
        if (p[-4]) {
            p[-4]->Notify(h[-24]);
        }
        if (p[0]) {
            p[0]->Notify(h[0]);
        }
        if (p[4]) {
            p[4]->Notify(h[24]);
        }
        p++;
        h += 6;
    }

    if (m_notify1) {
        m_notify1->SetSubtype();
    }
    if (m_extraNotify1) {
        m_extraNotify1->Notify(m_extraNotifyArg1);
    }
}

// @early-stop

RVA(0x001084d0, 0x96c)
i32 CStatusBarMgr::Sync(CFileMemBase* s, i32 op, i32 p4, i32 p5) {
    if (s == 0) {
        return 0;
    }
    switch (op) {
        case 4:
            if (Serialize(s) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Deserialize(s) == 0) {
                return 0;
            }
            break;
        case 8:
            (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
            if (m_position == 0) {
                RefreshA();
                DockStatusBarRight();
            }
            break;
    }

    if (m_retabNotify == 0) {
        i32 tmp = 0;
        if (op == 4) {
            s->Write(&tmp, 4);
        } else if (op == 7) {
            s->Read(&tmp, 4);
            if (tmp != 0) {
                CWarpStoneFly* c = new CWarpStoneFly();
                m_retabNotify = c;
                c->m_owner = this;
            }
        }
    } else {
        i32 tmp = 1;
        if (op == 4) {
            s->Write(&tmp, 4);
        }
    }

    if (m_retabNotify != 0) {
        if (m_retabNotify->Sync(s, op, p4, p5) == 0) {
            return 0;
        }
    }

    if (op == 4) {
        s->Write(&m_beltLast, 8);
        s->Write(&m_beltInterval, 8);
    } else if (op == 7) {
        s->Read(&m_beltLast, 8);
        s->Read(&m_beltInterval, 8);
    }
    if (op == 4) {
        s->Write(&m_fallLast, 8);
        s->Write(&m_fallDelay, 8);
    } else if (op == 7) {
        s->Read(&m_fallLast, 8);
        s->Read(&m_fallDelay, 8);
    }
    if (op == 4) {
        s->Write(&m_machineB.m_last, 8);
        s->Write(&m_machineB.m_interval, 8);
    } else if (op == 7) {
        s->Read(&m_machineB.m_last, 8);
        s->Read(&m_machineB.m_interval, 8);
    }
    if (op == 4) {
        s->Write(&m_machineA.m_last, 8);
        s->Write(&m_machineA.m_interval, 8);
    } else if (op == 7) {
        s->Read(&m_machineA.m_last, 8);
        s->Read(&m_machineA.m_interval, 8);
    }
    if (op == 4) {
        s->Write(&m_destructWarnLast, 8);
        s->Write(&m_destructWarnDelay, 8);
    } else if (op == 7) {
        s->Read(&m_destructWarnLast, 8);
        s->Read(&m_destructWarnDelay, 8);
    }

    CSbiSlot* p = m_slots;
    i32 n = 5;
    do {
        if (op == 4) {
            s->Write(&p->m_startTime, 8);
            s->Write(&p->m_interval, 8);
        } else if (op == 7) {
            s->Read(&p->m_startTime, 8);
            s->Read(&p->m_interval, 8);
        }
        p++;
        n--;
    } while (n != 0);

    n = 3;
    CSbiHlRow* r = m_groupSlots;
    do {
        if (op == 4) {
            s->Write(&r->m_last, 8);
            s->Write(&r->m_interval, 8);
        } else if (op == 7) {
            s->Read(&r->m_last, 8);
            s->Read(&r->m_interval, 8);
        }
        r++;
        n--;
    } while (n != 0);

    i32 outer = 3;
    r = m_hlGrid;
    do {
        n = 4;
        do {
            if (op == 4) {
                s->Write(&r->m_last, 8);
                s->Write(&r->m_interval, 8);
            } else if (op == 7) {
                s->Read(&r->m_last, 8);
                s->Read(&r->m_interval, 8);
            }
            r++;
            n--;
        } while (n != 0);
        outer--;
    } while (outer != 0);

    if (op == 4) {
        s->Write(&m_reserved2a0, 8);
        s->Write(&m_reserved2a8, 8);
    } else if (op == 7) {
        s->Read(&m_reserved2a0, 8);
        s->Read(&m_reserved2a8, 8);
    }
    if (op == 7 && m_position != 2) {
        BuildStatusBarTabs();
    }

#define SER(field)                                                                                 \
    if (field) {                                                                                   \
        if ((field)->SerializeFields(s, op, p4, p5) == 0)                                          \
            return 0;                                                                              \
    }

    {
        i32 i = 0;
        CSBI_StatzTabArrow** q = m_statObj;
        do {
            SER(m_hitRects[i])
            SER(*q)
            i++;
            q++;
        } while (i < 0xf);
    }
    {
        i32 i = 0;
        CSBI_ImageSet** q = m_slotNotify;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 5);
    }
    {
        i32 i = 0;
        CSBI_ImageSet** q = m_groupNotify;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 3);
    }
    {
        i32 row = 0;
        CSBI_ImageSet** base = m_hlNotify;
        do {
            i32 i = 0;
            CSBI_ImageSet** q = base;
            do {
                SER(*q)
                i++;
                q++;
            } while (i < 4);
            row++;
            base += 4;
        } while (row < 3);
    }
    {
        i32 i = 0;
        CSBI_WarlordHead** q = m_warlordHead;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 4);
    }

    SER(m_tabSprite0)
    SER(m_tabSprite1)
    SER(m_tabSprite2)
    SER(m_tabSprite3)
    SER(m_tabSprite4)
    SER(m_tabSprite5)
    SER(m_tabSprite6)
    SER(m_tabSprite7)
    SER(m_tabSprite8)
    SER(m_tabSprite9)
    SER(m_tabSprite10)
    SER(m_tabSprite10)
    SER(m_tabSprite11)
    SER(m_tabSprite12)
    SER(m_tabSprite13)
    SER(m_tabSprite14)
    SER(m_gaugeNotify)
    SER(m_gaugeSink)
    SER(m_machineDisplay)
    SER(m_notify0)
    SER(m_notify1)
    SER(m_notify2)
    SER(m_notify3)
    SER(m_extraNotify0)
    SER(m_extraNotify1)
    SER(m_modeNotify)
#undef SER

    Deactivate();
    return 1;
}

// @early-stop
RVA(0x001090a0, 0x38f)
i32 CStatusBarMgr::Serialize(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }

    s->Write(this, 4);
    s->Write(&m_restorePosition, 4);

    g_serialCounter++;

    i32 tmp = 0;

    if (m_barSprite) {
        tmp = m_barSprite->m_objectId;
    }
    s->Write(&tmp, 4);

    s->Write(&m_rect10.left, 0x10);
    s->Write(&m_redrawFrames, 4);
    s->Write(&m_barX, 4);
    s->Write(&m_barY, 4);
    s->Write(&m_itemKind, 4);
    s->Write(&m_tabCycle, 4);

    i32* p = m_statFlags;
    for (i32 i = 0; i < 15; i++) {
        s->Write(p, 4);
        p += 1;
    }

    s->Write(&m_reserved34c, 4);
    s->Write(&m_reserved350, 4);
    s->Write(&m_hitTestDisabled, 4);
    s->Write(&m_activeSlot, 4);
    s->Write(&m_pendingHlRow, 4);
    s->Write(&m_activeTab, 4);
    s->Write(&m_gauge, 4);
    s->Write(&m_gaugeTarget, 4);
    s->Write(&m_itemBaseX, 4);
    s->Write(&m_rezTick, 4);
    s->Write(&m_rezActive, 4);
    s->Write(&m_reserved544, 4);
    s->Write(&m_fallRect, 0x10);
    s->Write(&m_itemRect, 0x10);
    s->Write(&m_hlBusy, 4);
    s->Write(&m_toggleActive, 4);
    s->Write(&m_toggleHandle, 4);
    s->Write(&m_machinePhase, 4);
    s->Write(&m_extraNotifyArg0, 4);
    s->Write(&m_fallActive, 4);
    s->Write(&m_extraNotifyArg1, 4);
    s->Write(&m_machineB, 4);
    s->Write(&m_machineB.m_value, 4);
    s->Write(&m_machineA, 4);
    s->Write(&m_machineA.m_value, 4);
    s->Write(&m_destructWarnActive, 4);
    s->Write(&m_modeState, 4);
    s->Write(&m_modeArmed, 4);
    s->Write(&m_observerTabAvailable, 4);

    for (i32 j = 0; j < 5; j++) {
        s->Write(&m_slots[j].m_state, 4);
        s->Write(&m_slots[j].m_value, 4);
    }
    for (i32 k = 0; k < 3; k++) {
        s->Write(&m_groupSlots[k].m_state, 4);
        s->Write(&m_groupSlots[k].m_value, 4);
    }
    CSbiHlRow* nb = m_hlGrid;

    i32 cnt = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Write(&nb[m].m_state, 4);
            s->Write(&nb[m].m_value, 4);
        }
        nb += 4;
    } while (--cnt);

    cnt = m_ptrPool.GetSize();
    s->Write(&cnt, 4);
    for (u32 n = 0; n < static_cast<u32>(cnt); n++) {
        s->Write(m_ptrPool.GetData()[n], 8);
    }
    return 1;
}

// @early-stop
RVA(0x00109520, 0x44c)
i32 CStatusBarMgr::Deserialize(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* gm = g_gameReg->m_world;
    if (gm == 0) {
        return 0;
    }
    m_destructButton = 0;
    ResetWidgets(0);

    s->Read(this, 4);
    s->Read(&m_restorePosition, 4);

    g_serialCounter++;

    CGameObject* obj = 0;

    i32 seq;
    s->Read(&seq, 4);

    i32 kind = 0;
    if (MapLookupById(gm->m_childGroup->m_map48, seq, obj)) {

        if (obj == 0) {
            kind = 0;
        } else {
            kind = obj->GetClassId();
        }
    }
    CWwdGameObjectA* m8 = (kind == CLASSID_SERIALREF) ? static_cast<CWwdGameObjectA*>(obj) : 0;
    m_barSprite = m8;
    if (m8 == 0 && seq != 0) {
        return 0;
    }

    s->Read(&m_rect10.left, 0x10);
    s->Read(&m_redrawFrames, 4);
    s->Read(&m_barX, 4);
    s->Read(&m_barY, 4);
    s->Read(&m_itemKind, 4);
    s->Read(&m_tabCycle, 4);

    i32* p = m_statFlags;
    for (i32 i = 0; i < 15; i++) {
        s->Read(p, 4);
        p += 1;
    }

    s->Read(&m_reserved34c, 4);
    s->Read(&m_reserved350, 4);
    s->Read(&m_hitTestDisabled, 4);
    s->Read(&m_activeSlot, 4);
    s->Read(&m_pendingHlRow, 4);
    s->Read(&m_activeTab, 4);
    s->Read(&m_gauge, 4);
    s->Read(&m_gaugeTarget, 4);
    s->Read(&m_itemBaseX, 4);
    s->Read(&m_rezTick, 4);
    s->Read(&m_rezActive, 4);
    s->Read(&m_reserved544, 4);
    s->Read(&m_fallRect, 0x10);
    s->Read(&m_itemRect, 0x10);
    s->Read(&m_hlBusy, 4);
    s->Read(&m_toggleActive, 4);
    s->Read(&m_toggleHandle, 4);
    s->Read(&m_machinePhase, 4);
    s->Read(&m_extraNotifyArg0, 4);
    s->Read(&m_fallActive, 4);
    s->Read(&m_extraNotifyArg1, 4);
    s->Read(&m_machineB, 4);
    s->Read(&m_machineB.m_value, 4);
    s->Read(&m_machineA, 4);
    s->Read(&m_machineA.m_value, 4);
    s->Read(&m_destructWarnActive, 4);
    s->Read(&m_modeState, 4);
    s->Read(&m_modeArmed, 4);
    s->Read(&m_observerTabAvailable, 4);

    for (i32 j = 0; j < 5; j++) {
        s->Read(&m_slots[j].m_state, 4);
        s->Read(&m_slots[j].m_value, 4);
    }
    for (i32 k = 0; k < 3; k++) {
        s->Read(&m_groupSlots[k].m_state, 4);
        s->Read(&m_groupSlots[k].m_value, 4);
    }
    CSbiHlRow* nb = m_hlGrid;
    seq = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Read(&nb[m].m_state, 4);
            s->Read(&nb[m].m_value, 4);
        }
        nb += 4;
    } while (--seq);

    for (i32 t = 0; t < m_ptrPool.GetSize(); t++) {
        void* pp = m_ptrPool.GetData()[t];
        if (pp) {
            CoordPoolNode* node = g_coordPool.NodeOf(pp);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_ptrPool.SetSize(0, -1);

    i32 cnt;
    s->Read(&cnt, 4);
    m_ptrPool.SetSize(cnt, -1);
    for (u32 n = 0; n < static_cast<u32>(cnt); n++) {
        CoordPoolNode* head = g_coordPool.m_freeHead;
        void* node = 0;
        if (head->m_next != 0) {
            node = &head->m_coord;
            g_coordPool.m_freeHead = head->m_next;
        }
        s->Read(node, 8);
        m_ptrPool.GetData()[n] = node;
    }
    return 1;
}

RVA(0x0010b4f0, 0xaa)
void CStatusBarMgr::AdvanceTab(i32 reverse) {
    if (m_hlBusy != 0) {
        return;
    }
    if (g_gameReg->m_gameMode == 1) {
        return;
    }
    if (m_position == kSubtypeTag) {
        RefreshState();
    }
    if (m_activeTab != 4) {
        SetTabState(4, 3);
        Deactivate();
        return;
    }
    if (reverse != 0) {
        if (++m_tabCycle < 0) {
            m_tabCycle = 3;
        }
    } else {
        if (++m_tabCycle >= 4) {
            m_tabCycle = 0;
        }
    }
    ResetWidgets(0);
    TryActivate();
    Deactivate();
}

RVA(0x0010b5d0, 0xdd)
i32 CStatusBarMgr::HlClickGroup0(i32 row) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_hlGrid[row].m_state == 1) {
        i32 handle = m_hlGrid[row].m_value;
        i32* slot = &m_hlGrid[row].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b6f0, 0xdd)
i32 CStatusBarMgr::HlClickGroup1(i32 row) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_hlGrid[row + 4].m_state == 1) {
        i32 handle = m_hlGrid[row + 4].m_value;
        i32* slot = &m_hlGrid[row + 4].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b810, 0xdd)
i32 CStatusBarMgr::HlClickGroup2(i32 row) {
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_hlGrid[row + 8].m_state == 1) {
        i32 handle = m_hlGrid[row + 8].m_value;
        i32* slot = &m_hlGrid[row + 8].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

RVA(0x00105480, 0x7d)
void CStatusBarMgr::TickGauge() {
    i32 changed = 0;
    i32 g = m_gauge;
    i32 t = m_gaugeTarget;
    if (g < t) {
        g++;
    } else if (g <= t) {
        goto noChange;
    } else {
        g--;
    }
    m_gauge = g;
    changed = 1;
noChange:;
    if (m_gauge == 100) {
        if (AnySlotActive()) {
            changed = 1;
            SetGauge(0);
        }
    }
    if (changed) {
        if (m_gaugeSink && m_gaugeNotify) {
            m_gaugeNotify->SetSubtype();
            i32 fill = m_gauge;
            CSBI_WellGoo* sink = m_gaugeSink;
            sink->m_fillScale = fill;
            sink->SetSubtype();
        }
    }
}

RVA(0x00109a90, 0x25)
i32 CStatusBarMgr::FindReadySlot() {
    for (i32 i = 0; i < 5; i++) {
        if (m_slots[i].m_state == kSlotReady) {
            ArmSlot(i);
            return 1;
        }
    }
    return 0;
}

RVA(0x00109ad0, 0xa9)
i32 CStatusBarMgr::EnsureSub(i32 a, i32 b, i32 c) {
    if (m_retabNotify) {
        return 0;
    }
    CWarpStoneFly* o = new CWarpStoneFly();
    m_retabNotify = o;
    if (o == 0) {
        return 0;
    }
    return o->Init(this, a, b, c);
}

RVA(0x00101420, 0x110)
i32 CStatusBarMgr::ClearTabSprites(i32 idx) {
    if (idx == -1 || idx == 0) {
        if (m_tabSprite0) {
            m_tabSprite0->Blit();
        }
        if (m_tabSprite2) {
            m_tabSprite2->Blit();
        }
        if (m_tabSprite1) {
            m_tabSprite1->Blit();
        }
        if (m_tabSprite3) {
            m_tabSprite3->Blit();
        }
        if (m_tabSprite4) {
            m_tabSprite4->Blit();
        }
    }
    if (idx == 5 || idx == -1) {
        if (m_tabSprite5) {
            m_tabSprite5->Blit();
        }
        if (m_tabSprite6) {
            m_tabSprite6->Blit();
        }
        if (m_tabSprite7) {
            m_tabSprite7->Blit();
        }
        if (m_tabSprite8) {
            m_tabSprite8->Blit();
        }
        if (m_tabSprite9) {
            m_tabSprite9->Blit();
        }
        if (m_tabSprite10) {
            m_tabSprite10->Blit();
        }
    }
    if (idx == 6 || idx == -1) {
        if (m_tabSprite11) {
            m_tabSprite11->Blit();
        }
        if (m_tabSprite12) {
            m_tabSprite12->Blit();
        }
        if (m_tabSprite13) {
            m_tabSprite13->Blit();
        }
        if (m_tabSprite14) {
            m_tabSprite14->Blit();
        }
    }
    return 1;
}

RVA(0x00100cb0, 0x8b)
i32 CStatusBarMgr::Deactivate() {
    if (m_position == kSubtypeTag) {

        i32 w = g_gameReg->m_modeW;
        i32 h = g_gameReg->m_modeH;
        m_barX = w - 0x45;
        m_barY = h - 0x30;
        SetSpritePos(w - 0x45, h - 0x30);
    }

    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CSBI_ImageSet* cur = static_cast<CSBI_ImageSet*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->SetSubtype();
        }
    }

    CPtrList& tab = m_tabLists[m_activeTab];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CSBI_ImageSet* cur = static_cast<CSBI_ImageSet*>(tab.GetNext(m));
        if (cur) {
            cur->SetSubtype();
        }
    }

    ClearTabSprites(-1);
    m_redrawFrames = 2;
    return 1;
}

RVA(0x001020a0, 0xae)
i32 CStatusBarMgr::SetTab(i32 tab, i32 flag) {
    if (tab == m_itemKind && flag == 0) {
        return 1;
    }
    POSITION n = m_tabLists[5].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[5].GetNext(n));
        if (cur) {
            delete cur;
        }
    }
    m_tabLists[5].RemoveAll();
    m_tabSprite5 = 0;
    m_tabSprite6 = 0;
    m_tabSprite7 = 0;
    m_tabSprite8 = 0;
    m_tabSprite9 = 0;
    m_tabSprite10 = 0;
    m_itemKind = tab;

    if (!LoadTabSprites()) {
        g_gameReg->ReportError(kActivateErrId, kSetTabErrTag);
        return 0;
    }
    Deactivate();
    return 1;
}

RVA(0x000fe350, 0x6d)
void CStatusBarMgr::Teardown() {
    (static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))
        ->SetValueDword("StatusBar Position", m_position);
    ResetWidgets(0);
    for (i32 i = 0; i < m_ptrPool.GetSize(); i++) {
        void* p = m_ptrPool.GetData()[i];
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }

    m_ptrPool.SetSize(0, -1);
}

RVA(0x00104d60, 0x48)
i32 CStatusBarMgr::TryActivate() {

    if (m_position == kSubtypeTag) {
        return Activate();
    }
    if (!BuildStatusBarTabs()) {
        g_gameReg->ReportError(kActivateErrId, kActivateErrTag);
        return 0;
    }
    SetTabState(m_activeTab, 3);
    return 1;
}

RVA(0x00104dd0, 0x6b)
i32 CStatusBarMgr::Activate() {
    if (m_barSprite != 0) {
        return 0;
    }
    i32 w = g_gameReg->m_modeW;
    i32 d = g_gameReg->m_modeH;
    if (m_barX > w - 0x22) {
        m_barX = w - 0x22;
    }
    if (m_barY > d - 9) {
        m_barY = d - 0x22;
    }
    m_barSprite =
        (m_world)->m_childGroup->CreateSprite(0, m_barX, m_barY, 0xf4240, "StatusBarSprite", 1);
    return m_barSprite != 0;
}

// @early-stop
RVA(0x00104e60, 0xed)
i32 CStatusBarMgr::LoadStatzTabToggleSprite(i32 value, i32 idx) {
    if (m_statFlags[idx] == value) {
        return 1;
    }

    i32 slot = idx + 15 * g_curPlayer;
    if (g_gameReg->m_cmdGrid->m_grid[slot] == 0) {
        return 0;
    }

    CSBI_SideTab* item = m_hitRects[idx];
    i32 one = 1;
    if (item) {
        item->m_sampleMode = value;
        item->m_enabled = one;
        if (m_activeTab == one) {
            m_statObj[idx]->SetDirectionAlt(m_position, one);
            CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
            if (h->m_emitGate == 0) {
                void* spr_ob = 0;
                h->m_cues.Lookup("GAME_STATZTABTOGGLE", spr_ob);
                LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                if (spr) {

                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0 && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                        spr->m_lastPlayTime = g_killCueClock;
                        spr->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    m_statFlags[idx] = value;
    return 1;
}

// @early-stop
RVA(0x00105310, 0x11a)
void CStatusBarMgr::UpdateGruntOvenStatusBar() {

    CSBI_ImageSet** slot = m_slotNotify;
    CSbiSlot* tab = m_slots;
    i32 n = 5;
    do {
        if (tab->m_state == 1) {
            i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - tab->m_startTime;

            i32 elapsed = (d < 0) ? 0 : static_cast<i32>(d);
            u32 delay = g_buteMgr.GetDwordDef("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = static_cast<i32>((static_cast<u32>(elapsed) / delay)) + 1;
            if (frame >= 0x1a) {
                tab->m_state = 2;
                frame = 0x1a;
                CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
                if (h->m_emitGate == 0) {
                    void* spr_ob = 0;
                    h->m_cues.Lookup("GAME_COOKINGCOMPLETE", spr_ob);
                    LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                    if (spr) {

                        i32 gate = g_sndEnabled;
                        i32 item = g_sndCueTag;
                        if (gate != 0
                            && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                            spr->m_lastPlayTime = g_killCueClock;
                            spr->m_sound->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
            if (frame != tab->m_value) {
                tab->m_value = frame;
                CSBI_ImageSet* w = *slot;
                if (w) {
                    w->Notify(frame);
                }
            }
        }
        ++slot;
        ++tab;
    } while (--n != 0);
}

// @early-stop
RVA(0x001076a0, 0x1f3)
void CStatusBarMgr::UpdateChipGrinderStatusBar() {

    if (m_fallActive == 0) {
        return;
    }

    i32 stepped = 0;
    if (m_fallActive == 1 || m_fallActive == 2) {
        u32 delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
        i32 speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 4);

        if (m_fallRect.top >= 0x1c7) {
            m_fallActive = 0;
            m_extraNotifyArg1 = 0;
        } else if (m_fallRect.bottom >= 0x1bf) {
            if (m_fallActive != 2) {
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
                    if (h->m_emitGate == 0) {
                        void* spr_ob = 0;
                        h->m_cues.Lookup("GAME_REZGRINDING", spr_ob);
                        LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                        if (spr) {

                            i32 gate = g_sndEnabled;
                            i32 item = g_sndCueTag;
                            if (gate != 0
                                && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                                spr->m_lastPlayTime = g_killCueClock;
                                spr->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_fallActive = 2;
            }
            delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemShredderSpeed", 2);
        }

        i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_fallLast;
        if (d >= m_fallDelay) {
            i32 newLo = m_fallRect.top + speed;
            m_fallRect.top = newLo;
            i32 newHi = m_fallRect.bottom + speed;
            m_fallRect.bottom = newHi;
            CSBI_ImageSet* w = m_extraNotify1;
            if (w) {
                i32 sx = m_rect10.left;
                i32 sy = m_rect10.top;
                w->m_rect14.left = m_fallRect.left + sx;
                w->m_rect14.top = sy + newLo;
                w->m_rect14.right = m_fallRect.right + sx;
                w->m_rect14.bottom = sy + newHi;
            }
            m_fallDelay = static_cast<u32>(static_cast<i32>(delay));
            m_fallLast = static_cast<u32>(static_cast<i32>(g_frameTime));
            stepped = 1;
        }
    }

    if (m_extraNotify1 != 0 && stepped) {
        NotifyAllSlots();
    }
}

// @early-stop
RVA(0x00109bd0, 0x1b5)
i32 CWarpStoneFly::Init(void* owner, i32 srcX, i32 srcY, i32 phase) {
    m_owner = static_cast<CStatusBarMgr*>(owner);

    CObject* spr_ob = 0;
    i32 n = phase + 1;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_STATUSBAR_TABZ_GAMETAB_WARP", spr_ob);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(spr_ob);
    CImage* frame = (spr && n >= spr->m_minIndex && n <= spr->m_maxIndex)
                        ? static_cast<CImage*>(spr->m_items.GetAt(n))
                        : 0;
    m_sprite = frame;
    if (frame == 0) {

        return 0;
    }

    m_arrivalMode = phase;
    i32 cx, dy;
    switch (phase) {
        case 2:
            cx = 0x69;
            dy = 0x26;
            break;
        case 3:
            cx = 0x65;
            dy = 0x50;
            break;
        case 4:
            cx = 0x69;
            dy = 0x54;
            break;
        default:
            cx = 0x34;
            dy = 0x29;
            break;
    }

    CStatusBarMgr* base = m_owner;
    i32 tx = base->m_rect10.left + cx;
    m_targetX = tx;
    i32 ty = base->m_rect10.top + dy;
    m_targetY = ty;

    i32 dxv = tx - srcX;
    i32 dyv = ty - srcY;
    i32 dist2 = dxv * dxv + dyv * dyv;
    double dist = sqrt(static_cast<double>(dist2));
    u32 flyTime = g_buteMgr.GetDwordDef("WarpStone", "FlyTime", 0x5dc);

    m_velocityScale = dist / static_cast<double>(flyTime);
    m_xDirection = static_cast<double>(dxv) / dist;
    m_yDirection = static_cast<double>(dyv) / dist;

    CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
    if (h->m_emitGate == 0) {
        void* fly_ob = 0;
        h->m_cues.Lookup("GAME_WARPSTONEFLY", fly_ob);
        LeafCue* fly = static_cast<LeafCue*>(fly_ob);
        if (fly) {

            i32 gate = g_sndEnabled;
            i32 item = g_sndCueTag;
            if (gate != 0 && g_killCueClock - fly->m_lastPlayTime >= fly->m_replayDelay) {
                fly->m_lastPlayTime = g_killCueClock;
                fly->m_sound->ConfigureItem(item, 0, 0, 0);
            }
        }
    }

    m_currentX = static_cast<double>(srcX);
    m_currentY = static_cast<double>(srcY);
    return 1;
}

// @early-stop
RVA(0x0010b320, 0x167)
void CStatusBarMgr::UpdateDestructButtonStatusBar() {

    switch (m_destructWarnActive) {
        case 1: {
            i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_destructWarnLast;
            if (d >= m_destructWarnDelay) {
                if (++m_modeState >= 6) {
                    m_modeState = 6;
                    m_destructWarnActive = 2;
                }
                m_destructWarnDelay = static_cast<u32>(
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32)
                );
                m_destructWarnLast = static_cast<u32>(g_frameTime);
                CSBI_ImageSet* w = m_modeNotify;
                if (w) {
                    w->Notify(m_modeState);
                }
            }
            break;
        }
        case 2: {
            i64 d = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_destructWarnLast;
            if (d >= m_destructWarnDelay) {
                if (--m_modeState <= 2) {
                    m_modeState = 2;
                    m_destructWarnActive = 1;
                }
                m_destructWarnDelay = static_cast<u32>(
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32)
                );
                m_destructWarnLast = static_cast<u32>(g_frameTime);
                CSBI_ImageSet* w = m_modeNotify;
                if (w) {
                    w->Notify(m_modeState);
                }
            }
            break;
        }
    }
}

RVA(0x0010bb90, 0x3f)
void CStatusBarMgr::SetMode(i32 mode) {
    m_modeArmed = 1;
    if (mode && m_modeState != 7) {
        m_destructWarnActive = 0;
        m_modeState = 1;
        if (m_modeNotify) {
            m_modeNotify->Notify(1);
        }
    }
}

RVA(0x0010bbe0, 0x34)
i32 CStatusBarMgr::GetActiveValue() {
    if (m_rezActive == 0) {
        return m_extraNotifyArg0;
    }
    if (m_ptrPool.GetSize() > 0 && m_ptrPool.GetSize() > m_rezTick) {
        return *static_cast<i32*>(m_ptrPool.GetAt(m_rezTick));
    }
    return 0;
}

// @early-stop
RVA(0x000ffcb0, 0xe2)
CStatusBarItem* CStatusBarMgr::HitTestRects(i32 x, i32 y) {
    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = x < r->m_rect14.right && x >= r->m_rect14.left && y < r->m_rect14.bottom
                      && y >= r->m_rect14.top;
            if (hit) {
                return r;
            }
        }
    }
    CPtrList& tab = m_tabLists[m_activeTab];
    n = tab.GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(tab.GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = x < r->m_rect14.right && x >= r->m_rect14.left && y < r->m_rect14.bottom
                      && y >= r->m_rect14.top;
            if (hit) {
                return r;
            }
        }
    }
    n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = x < r->m_rect14.right && x >= r->m_rect14.left && y < r->m_rect14.bottom
                      && y >= r->m_rect14.top;
            if (hit) {
                return r;
            }
        }
    }
    return 0;
}

RVA(0x00106900, 0x8d)
void CStatusBarMgr::InitTabRects() {
    for (i32 i = 0; i < 4; i++) {
        ClearHlCell(0, i);
        ClearHlCell(1, i);
        ClearHlCell(2, i);
    }
    m_machinePhase = 1;
    m_extraNotifyArg0 = 0;
    m_fallActive = 0;
    m_extraNotifyArg1 = 0;
    SetRect(&m_fallRect, 0, 0, 1, 1);
    SetRect(&m_itemRect, 0x49, 0xd7, 0x61, 0xef);
    m_pendingHlRow = -1;
}

RVA(0x000ff9d0, 0x8)
i32 CStatusBarMgr::OnPointerRelease(i32, i32, i32) {
    return 1;
}

RVA(0x000ff9f0, 0xe4)
i32 CStatusBarMgr::ClickToggle(i32 btn, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == 0) {
        ClearTabSprites(-1);
        return 1;
    }
    r->Click24(btn, x, y);
    if (r->m_kind != 2) {
        ClearTabSprites(-1);
        return 1;
    }
    i32 cmd = r->m_cmd;
    if (m_hitTestDisabled == 0) {
        if (cmd >= 1 && cmd <= 5) {
            SetTabState(cmd, 2);
        } else {
            ClearTabSprites(0);
        }
    }
    if (m_activeTab == 5) {
        if (r->m_tab == 5) {
            SetTabState(cmd, 2);
        } else {
            ClearTabSprites(5);
        }
    }
    if (m_toggleActive) {
        if (r->m_tab == 6) {
            SetTabState(cmd, 2);
            return 1;
        }
        ClearTabSprites(5);
    }
    return 1;
}

// @early-stop
RVA(0x00100930, 0x16c)
void CStatusBarMgr::ResetWidgets(i32 keepHost) {
    for (i32 t = 0; t < 8; t++) {
        POSITION n = m_tabLists[t].GetHeadPosition();
        while (n) {
            CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[t].GetNext(n));
            if (cur) {
                delete cur;
            }
        }
        m_tabLists[t].RemoveAll();
    }
    if (keepHost) {
        if (m_barSprite) {

            m_barSprite->m_stateFlags |= 1;
            m_barSprite->m_flags |= 0x10000;
        }
    }
    m_tabSprite0 = 0;
    m_tabSprite1 = 0;
    m_tabSprite2 = 0;
    m_tabSprite3 = 0;
    m_tabSprite4 = 0;
    m_tabSprite5 = 0;
    m_tabSprite6 = 0;
    m_tabSprite7 = 0;
    m_tabSprite8 = 0;
    m_tabSprite9 = 0;
    m_tabSprite10 = 0;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_barSprite = 0;
    i32 i;
    for (i = 0; i < 15; i++) {
        m_hitRects[i] = 0;
    }
    for (i = 0; i < 15; i++) {
        m_statObj[i] = 0;
    }
    m_slotNotify[0] = 0;
    m_slotNotify[1] = 0;
    m_slotNotify[2] = 0;
    m_slotNotify[3] = 0;
    m_slotNotify[4] = 0;
    m_groupNotify[0] = 0;
    m_groupNotify[1] = 0;
    m_groupNotify[2] = 0;
    for (i = 0; i < 12; i++) {
        m_hlNotify[i] = 0;
    }
    CSBI_WarlordHead** tp = m_warlordHead;
    tp[0] = 0;
    tp[1] = 0;
    tp[2] = 0;
    tp[3] = 0;
    m_extraNotify0 = 0;
    m_extraNotify1 = 0;
    m_modeNotify = 0;
    m_notify0 = 0;
    m_notify2 = 0;
    m_notify3 = 0;
    m_notify1 = 0;
    m_machineDisplay = 0;
    m_gaugeNotify = 0;
    m_gaugeSink = 0;
    m_tabsBuilt = 0;
}

RVA(0x0010b210, 0xc5)
void CStatusBarMgr::ExitMode() {
    if (m_toggleActive == 0) {
        return;
    }
    POSITION n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(n));
        if (cur) {
            delete cur;
        }
    }
    m_tabLists[6].RemoveAll();
    i32 handle = m_toggleHandle;
    m_tabSprite11 = 0;
    m_tabSprite12 = 0;
    m_tabSprite13 = 0;
    m_tabSprite14 = 0;
    m_hlBusy = 0;
    if (handle == 0 && g_gameReg->m_gameMode != 1) {
        if (m_position == 2) {
            RefreshState();
        }
        if (m_activeTab != 5) {
            SetTabState(5, 3);
        }
        SetTab(5, 1);
        Deactivate();
    } else {
        m_hitTestDisabled = 0;
    }
    m_toggleActive = 0;
    m_toggleHandle = 0;
    Deactivate();
}

// @early-stop
RVA(0x00100b00, 0x150)
void CStatusBarMgr::ClearTabGroup() {
    if (m_activeTab == 0) {
        return;
    }
    CPtrList& tab = m_tabLists[m_activeTab];
    POSITION n = tab.GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(n));
        if (cur) {
            delete cur;
        }
    }
    m_tabLists[m_activeTab].RemoveAll();
    switch (m_activeTab) {
        case 1:
            m_tabSprite5 = 0;
            m_tabSprite6 = 0;
            m_tabSprite7 = 0;
            m_tabSprite8 = 0;
            m_tabSprite9 = 0;
            m_tabSprite10 = 0;
            m_modeNotify = 0;
            break;
        case 2:

            memset(m_statObj, 0, sizeof(m_statObj));
            break;
        case 3: {
            CSBI_WarlordHead** p = m_warlordHead;
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p[3] = 0;
            break;
        }
        case 4: {

            CSBI_ImageSet** q = m_slotNotify;
            q[0] = 0;
            q[1] = 0;
            q[2] = 0;
            q[3] = 0;
            q[4] = 0;
            m_gaugeNotify = 0;
            m_gaugeSink = 0;
            break;
        }
        case 5: {

            CSBI_ImageSet** g = m_groupNotify;
            g[0] = 0;
            g[1] = 0;
            g[2] = 0;
            m_machineDisplay = 0;

            memset(m_hlNotify, 0, sizeof(m_hlNotify));
            m_notify0 = 0;
            m_notify2 = 0;
            m_notify3 = 0;
            m_notify1 = 0;
            m_extraNotify0 = 0;
            m_extraNotify1 = 0;
            break;
        }
    }
}

// @early-stop
RVA(0x00100d70, 0x548)
i32 CStatusBarMgr::SetTabState(i32 tab, i32 state) {
    if (m_tabSprite0 == 0 || m_tabSprite1 == 0 || m_tabSprite2 == 0 || m_tabSprite3 == 0
        || m_tabSprite4 == 0) {
        return 0;
    }
    switch (tab) {
        case 1:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->SetState(state, 1);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 2:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->SetState(state, 1);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 3:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->SetState(state, 1);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 4:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->SetState(state, 1);
            m_tabSprite4->ProbeState(state);
            return 1;
        case 5:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->SetState(state, 1);
            return 1;
        case 0x1f4:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->SetState(state, 1);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f5:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->SetState(state, 1);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f6:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->SetState(state, 1);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f7:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->SetState(state, 1);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f8:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->SetState(state, 1);
            m_tabSprite10->ProbeState(state);
            return 1;
        case 0x1f9:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->SetState(state, 1);
            return 1;
        case 0x1fa:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite10->SetState(state, 1);
            return 1;
        case 0x324:
            if (m_tabSprite11) {
                m_tabSprite11->SetState(state, 1);
            }
            m_tabSprite12->ProbeState(state);
            return 1;
        case 0x325:
            if (m_tabSprite11) {
                m_tabSprite11->ProbeState(state);
            }
            m_tabSprite12->SetState(state, 1);
            return 1;
        case 0x327:
            m_tabSprite13->SetState(state, 1);
            m_tabSprite14->ProbeState(state);
            return 1;
        case 0x328:
            m_tabSprite13->ProbeState(state);
            m_tabSprite14->SetState(state, 1);
            return 1;
    }
    return 1;
}

RVA(0x00106820, 0xa8)
void CStatusBarMgr::EnterHlRow(i32 shift, i32 key) {
    if (m_pendingHlRow == -1) {
        return;
    }
    i32 group;
    if (key >= 0x22) {
        group = 2;
    } else {
        group = (key >= 0x17);
    }
    if (shift != 0) {
        ClearHlCell(group, m_pendingHlRow);
        for (i32 row = m_pendingHlRow - 1; row >= 0; row--) {
            CSbiHlRow* cell = &m_hlGrid[row + group * 4];
            if (cell->m_state == 1) {
                m_hlGrid[row + group * 4 + 1].m_state = 1;
                cell[1].m_value = cell->m_value;
                cell->m_state = 0;
                cell->m_value = 0;
            }
        }
    } else {
        m_hlGrid[m_pendingHlRow + group * 4].m_value = key;
    }
    NotifyAllSlots();
    m_pendingHlRow = -1;
}

// @early-stop
RVA(0x000ff850, 0x121)
i32 CStatusBarMgr::ClickHilite(i32 a, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == 0) {
        return 1;
    }
    r->Click1c(a, x, y);
    i32 cmd = r->m_cmd;
    if (r->m_tab == 1 && m_hitTestDisabled == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0
        && cmd >= 0x13b && cmd <= 0x149) {
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        PlaceCursorTarget(cmd - 0x13b, 1);
        return 1;
    }

    return UpdateStatusBarTabHighlight(a, x, y);
}

RVA(0x00104f90, 0xa8)
i32 CStatusBarMgr::ClearStat(i32 idx) {
    CSBI_SideTab* r = m_hitRects[idx];
    if (r != 0) {
        r->m_sampleMode = 0;
        r->m_enabled = 0;
        if (m_activeTab == 1) {

            m_statObj[idx]->SetDirection(m_position, 1);
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_STATZTABTOGGLE", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    m_statFlags[idx] = 0;
    return 1;
}

RVA(0x00107920, 0xb7)
i32 CStatusBarMgr::SetFallRect(i32 x, i32 y, i32 item) {
    if (m_pendingHlRow == -1) {
        return 0;
    }
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == 0) {
        return 0;
    }
    if (r->m_cmd != 0xce && r->m_cmd != 0xd0) {
        return 0;
    }

    i32 cx = x;
    RECT rc = r->m_rect14;
    i32 lo = rc.left + 0x1b;
    i32 xHi = rc.right;
    if (x < lo) {
        cx = lo;
    } else if (x > xHi - 0x1a) {
        cx = xHi - 0x1a;
    }
    i32 localX = cx - m_rect10.left;
    i32 localY = 0x1b3 - m_rect10.top;
    UpdateFallingItemStatusBar(item, localX, localY);
    EnterHlRow(1, item);
    return 1;
}

// @early-stop
RVA(0x0010b930, 0x1a7)
i32 CStatusBarMgr::ActivateSlot(i32 idx) {

    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending != 0) {
        goto notActivated;
    }
    if (idx == -1) {
        i32 slot = 0;
        for (;;) {
            if (m_slots[slot].m_state == kSlotReady) {
                break;
            }
            slot++;
            if (slot >= 5) {
                return 0;
            }
        }
        if (!(static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(0x66)) {
            goto notActivated;
        }
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        m_activeSlot = slot;
        m_slots[slot].m_value = 1;
        if (m_slotNotify[slot]) {
            m_slotNotify[slot]->Notify(1);
        }
        return 1;
    }
    {
        if (m_slots[idx].m_state != kSlotReady) {
            goto notActivated;
        }
        if (!(static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(0x66)) {
            goto notActivated;
        }
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        m_activeSlot = idx;
        m_slots[idx].m_value = 1;
        if (m_slotNotify[idx]) {
            m_slotNotify[idx]->Notify(1);
        }
        return 1;
    }
notActivated:
    return 0;
}

RVA(0x000fe3e0, 0x55)
i32 CStatusBarMgr::SetState(i32 state) {
    if (m_hlBusy != 0) {
        return 1;
    }
    i32 old = m_position;
    if (old == state) {
        return 1;
    }
    if (state == 2) {
        if (Activate() == 0) {
            return 0;
        }
        m_restorePosition = m_position;
    } else {
        Deactivate();
    }
    old = m_position;
    m_position = state;
    (static_cast<CPlay*>(g_gameReg->m_curState))->PositionBridgeToggle(state, old);
    return 1;
}

RVA(0x000fe460, 0x83)
i32 CStatusBarMgr::RefreshA() {
    if (m_hlBusy == 0 && m_position != 1) {
        ResetWidgets(1);
        SetRect(&m_rect10, 0, 0, 0xa0, 0x1e0);
        SetState(1);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
        if (BuildStatusBarTabs() == 0) {
            g_gameReg->ReportError(kActivateErrId, 0x448);
            return 0;
        }
        SetTabState(m_activeTab, 3);
    }
    return 1;
}

RVA(0x000fe520, 0xa9)
i32 CStatusBarMgr::DockStatusBarRight() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position == 0) {
        return 1;
    }
    ResetWidgets(1);

    i32 w = g_gameReg->m_modeW;
    volatile POINT pt;
    pt.y = g_gameReg->m_modeH;
    SetRect(&m_rect10, w - 0xa0, 0, w, 0x1e0);
    SetState(0);
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    if (BuildStatusBarTabs() == 0) {
        g_gameReg->ReportError(kActivateErrId, 0x449);
        return 0;
    }
    SetTabState(m_activeTab, 3);
    return 1;
}

RVA(0x000fe600, 0x49)
i32 CStatusBarMgr::HideRect() {
    if (m_hlBusy == 0 && m_position != 2) {
        ResetWidgets(1);
        SetRect(&m_rect10, -1, -1, -1, -1);
        SetState(2);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    }
    return 1;
}

RVA(0x000fe670, 0x2b)
i32 CStatusBarMgr::RefreshState() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position != 2) {
        return 1;
    }
    if (m_restorePosition == 1) {
        return RefreshA();
    }
    return DockStatusBarRight();
}

// @early-stop
RVA(0x000fe860, 0x2d)
i32 CStatusBarMgr::SetSpritePos(i32 x, i32 y) {
    if (m_barSprite == 0) {
        return 0;
    }
    m_barSprite->m_screenX = x;
    m_barSprite->m_screenY = y;
    m_barX = x;
    m_barY = y;
    return 1;
}

RVA(0x000fe8a0, 0x4e)
i32 CStatusBarMgr::HitTestLayer(i32 x, i32 y) {
    CWwdGameObjectA* r = m_barSprite;
    CImage* L = r->m_layer;
    i32 xlo = r->m_screenX - L->m_anchorX;
    i32 ylo = r->m_screenY - L->m_anchorY;
    i32 xhi = L->m_width + xlo;
    i32 yhi = L->m_height + ylo;
    if (x >= xhi || x < xlo || y >= yhi || y < ylo) {
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x00108410, 0x8e)
i32 CStatusBarMgr::InsertPtr(i32 a, i32 b) {
    CoordPoolNode* head = g_coordPool.m_freeHead;
    Coord* node = 0;
    if (head->m_next != 0) {
        node = &head->m_coord;
        node->m_x = a;
        node->m_y = b;
        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
    }
    i32 n = m_ptrPool.GetSize();
    i32 i = 0;
    if (i < n) {
        void** t = m_ptrPool.GetData();
        while (i < n) {
            Coord* e = static_cast<Coord*>(*t);
            if (e != 0 && b < e->m_y) {
                goto insert;
            }
            i++;
            t++;
        }
    }
    m_ptrPool.Add(node);
    return 1;
insert:
    m_ptrPool.InsertAt(i, node, 1);
    return 1;
}

RVA(0x0010bb50, 0x24)
void CStatusBarMgr::ReportTab(i32 tab) {
    UpdateFallingItemStatusBar(tab, 0x4f, 0x1b3);
    EnterHlRow(1, tab);
}

// @early-stop
RVA(0x000ffde0, 0x5b1)
i32 CStatusBarMgr::BuildStatusBarTabs() {
    if (m_tabsBuilt != 0) {
        return 1;
    }
    if (m_world == 0) {
        return 0;
    }
    i32 bx = m_rect10.left;
    i32 by = m_rect10.top;
    CDDrawSurfaceMgr* code = m_world;
    CStatusBarItem* it;

    it = new CSBI_RectOnly;
    if (!it->Setup(
            this,
            code,
            0x259,
            0,
            SbGeom(bx + 0x7c, by + 0xad, bx + 0x88, by + 0xb9),
            0,
            -1
        )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);

    it = new CSBI_RectOnly;
    if (!it->Setup(
            this,
            code,
            0x25a,
            0,
            SbGeom(bx + 0x8a, by + 0xb9, bx + 0x96, by + 0xc7),
            0,
            -1
        )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);

    it = new CSBI_RectOnly;
    if (!it->Setup(
            this,
            code,
            0x25b,
            0,
            SbGeom(bx + 0x83, by + 0xbb, bx + 0x8f, by + 0xc7),
            0,
            -1
        )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);

    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 1,
                 0,
                 SbGeom(bx + 0x42, by + 0x82, bx + 0x62, by + 0x99),
                 "GAME_STATUSBAR_TABZ_STATZTAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite0 = static_cast<CSBI_MenuItem*>(it);

    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 2,
                 0,
                 SbGeom(bx + 0x04, by + 0x82, bx + 0x24, by + 0x99),
                 "GAME_STATUSBAR_TABZ_GRUNTZTAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite2 = static_cast<CSBI_MenuItem*>(it);

    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 3,
                 0,
                 SbGeom(bx + 0x24, by + 0x82, bx + 0x44, by + 0x99),
                 "GAME_STATUSBAR_TABZ_RESOURCETAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite1 = static_cast<CSBI_MenuItem*>(it);

    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 4,
                 0,
                 SbGeom(bx + 0x60, by + 0x82, bx + 0x80, by + 0x99),
                 "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite3 = static_cast<CSBI_MenuItem*>(it);
    if (g_gameReg->m_gameMode == 1) {
        CSBI_MenuItem* mp = static_cast<CSBI_MenuItem*>(it);
        mp->m_state = 4;
        CDDrawWorker* f = mp->m_record;
        CImage* v;
        if (f != 0 && f->m_minIndex <= 4 && f->m_maxIndex >= 4) {
            v = static_cast<CImage*>(f->m_items.GetAt(4));
        } else {
            v = 0;
        }
        mp->m_frame = v;
        mp->m_enabled = 0;
        mp->SetSubtype();
    }

    it = new CSBI_MenuItem;
    if (!(static_cast<CSBI_MenuItem*>(it))
             ->SetupImage(
                 this,
                 static_cast<CDDrawSurfaceMgr*>(code),
                 5,
                 0,
                 SbGeom(bx + 0x7e, by + 0x82, bx + 0x9e, by + 0x99),
                 "GAME_STATUSBAR_TABZ_GAMETAB",
                 -1,
                 0
             )) {
        if (it) {
            delete it;
        }
        return 0;
    }
    m_tabLists[0].AddTail(it);
    m_tabSprite4 = static_cast<CSBI_MenuItem*>(it);

    if (BuildSideTabs() == 0) {
        return 0;
    }
    if (RefreshState() == 0) {
        return 0;
    }
    if (BuildTabzDialog() == 0) {
        return 0;
    }
    m_tabsBuilt = 1;
    return 1;
}

static __inline i32 WapRand(i32 range) {
    u32 x;
    if (range == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        return (static_cast<u32>(g_randSeed) >> 16) & 1;
    }
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        x = timeGetTime();
    } else {
        x = g_randSeed;
    }
    g_randSeed = x * 214013 + 2531011;
    return ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % range + 1;
}

RVA(0x00100510, 0x6)
i32 CStatusBarItem::Render() {
    return 1;
}

RVA(0x00107d00, 0x591)
i32 CStatusBarMgr::StartChipMachineCycle() {
    i32 result;
    if (g_gameReg->m_gameMode == 1) {
        if (m_ptrPool.GetSize() > 0) {
            void* p = m_ptrPool.GetData()[0];
            result = *static_cast<i32*>(p);
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_ptrPool.RemoveAt(0, 1);
        } else {
            result = 0;
            if (m_extraNotify0) {
                m_extraNotify0->Notify(0);
            }
        }
    } else {
        i32 r1 = WapRand(m_battlezPct[2]);
        if (r1 <= m_battlezPct[0]) {
            i32 r = WapRand(m_battlezPct[37]);
            if (r <= m_battlezPct[17]) {
                result = 1;
            } else if (r <= m_battlezPct[18]) {
                result = 2;
            } else if (r <= m_battlezPct[19]) {
                result = 3;
            } else if (r <= m_battlezPct[20]) {
                result = 4;
            } else if (r <= m_battlezPct[21]) {
                result = 5;
            } else if (r <= m_battlezPct[22]) {
                result = 6;
            } else if (r <= m_battlezPct[23]) {
                result = 7;
            } else if (r <= m_battlezPct[24]) {
                result = 8;
            } else if (r <= m_battlezPct[25]) {
                result = 9;
            } else if (r <= m_battlezPct[26]) {
                result = 10;
            } else if (r <= m_battlezPct[27]) {
                result = 11;
            } else if (r <= m_battlezPct[28]) {
                result = 12;
            } else if (r <= m_battlezPct[29]) {
                result = 13;
            } else if (r <= m_battlezPct[30]) {
                result = 14;
            } else if (r <= m_battlezPct[31]) {
                result = 15;
            } else if (r <= m_battlezPct[32]) {
                result = 16;
            } else if (r <= m_battlezPct[33]) {
                result = 17;
            } else if (r <= m_battlezPct[34]) {
                result = 18;
            } else if (r <= m_battlezPct[35]) {
                result = 19;
            } else {
                result = 0x15 + (r > m_battlezPct[36]);
            }
        } else if (r1 <= m_battlezPct[1]) {
            i32 r = WapRand(m_battlezPct[16]);
            if (r <= m_battlezPct[7]) {
                result = 0x17;
            } else if (r <= m_battlezPct[8]) {
                result = 0x18;
            } else if (r <= m_battlezPct[9]) {
                result = 0x19;
            } else if (r <= m_battlezPct[10]) {
                result = 0x1a;
            } else if (r <= m_battlezPct[11]) {
                result = 0x1b;
            } else if (r <= m_battlezPct[12]) {
                result = 0x1c;
            } else if (r <= m_battlezPct[13]) {
                result = 0x1d;
            } else if (r <= m_battlezPct[14]) {
                result = 0x1e;
            } else {
                result = 0x1f + (r > m_battlezPct[15]);
            }
        } else {
            i32 r = WapRand(m_battlezPct[6]);
            if (r <= m_battlezPct[3]) {
                result = 0x23;
            } else if (r <= m_battlezPct[4]) {
                result = 0x24;
            } else {
                result = 0x25 + (r > m_battlezPct[5]);
            }
        }
        if (result == 0x14) {
            result = 5;
        }
    }
    m_extraNotifyArg1 = result;
    m_machinePhase = 1;
    SetRect(&m_itemRect, 0x49, 0xd7, 0x61, 0xef);
    if (m_extraNotify0) {
        i32 x = m_rect10.left;
        i32 y = m_rect10.top;
        m_extraNotify0->m_rect14.left = m_itemRect.left + x;
        m_extraNotify0->m_rect14.top = m_itemRect.top + y;
        m_extraNotify0->m_rect14.right = m_itemRect.right + x;
        m_extraNotify0->m_rect14.bottom = m_itemRect.bottom + y;
    }
    NotifyAllSlots();
    i32 c = m_rezTick;
    m_rezActive = 0;
    if (c > 0) {
        m_rezTick = c - 1;
        UpdateRezMachineWakeStatusBar();
    }
    return 1;
}

// @early-stop
RVA(0x000fdc00, 0x5c2)
i32 CStatusBarMgr::LoadBattlezItemConfig(CDDrawSurfaceMgr* world) {
    m_world = world;
    m_restorePosition = 0;
    m_position = 0;
    i32 vx = g_gameReg->m_modeW;
    i32 vy = g_gameReg->m_modeH;
    SetRect(&m_rect10, vx - 0xa0, 0, vx, 0x1e0);
    m_redrawFrames = 0;
    m_barX = vx - 0x45;
    m_barY = vy - 0x30;
    m_itemKind = 5;
    m_tabCycle = g_curPlayer;
    Reset();
    if (BuildStatusBarTabs() == 0) {
        return 0;
    }
    m_activeSlot = -1;
    m_pendingHlRow = -1;
    m_rezActive = 0;
    m_rezTick = 0;
    m_toggleActive = 0;
    m_toggleHandle = 0;
    m_battlezPct[0] = g_buteMgr.GetInt("Multiplayer", "ToolzPercent");
    m_battlezPct[1] = m_battlezPct[0] + g_buteMgr.GetInt("Multiplayer", "ToyzPercent");
    m_battlezPct[2] = m_battlezPct[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
    m_battlezPct[3] = m_battlezPct[2] + g_buteMgr.GetInt("Multiplayer", "RedBrick");
    m_battlezPct[4] = m_battlezPct[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
    m_battlezPct[5] = m_battlezPct[4] + g_buteMgr.GetInt("Multiplayer", "GoldBrick");
    m_battlezPct[6] = m_battlezPct[5] + g_buteMgr.GetInt("Multiplayer", "BlackBrick");
    m_battlezPct[7] = m_battlezPct[6] + g_buteMgr.GetInt("Multiplayer", "BabyWalkerz");
    m_battlezPct[8] = m_battlezPct[7] + g_buteMgr.GetInt("Multiplayer", "BeachBallz");
    m_battlezPct[9] = m_battlezPct[8] + g_buteMgr.GetInt("Multiplayer", "BigWheelz");
    m_battlezPct[10] = m_battlezPct[9] + g_buteMgr.GetInt("Multiplayer", "GoKartz");
    m_battlezPct[11] = m_battlezPct[10] + g_buteMgr.GetInt("Multiplayer", "JackInTheBoxz");
    m_battlezPct[12] = m_battlezPct[11] + g_buteMgr.GetInt("Multiplayer", "JumpRopez");
    m_battlezPct[13] = m_battlezPct[12] + g_buteMgr.GetInt("Multiplayer", "PogoStickz");
    m_battlezPct[14] = m_battlezPct[13] + g_buteMgr.GetInt("Multiplayer", "Scrollz");
    m_battlezPct[15] = m_battlezPct[14] + g_buteMgr.GetInt("Multiplayer", "SqueakToyz");
    m_battlezPct[16] = m_battlezPct[15] + g_buteMgr.GetInt("Multiplayer", "Yoyoz");
    m_battlezPct[17] = m_battlezPct[16] + g_buteMgr.GetInt("Multiplayer", "Bombz");
    m_battlezPct[18] = m_battlezPct[17] + g_buteMgr.GetInt("Multiplayer", "Boomerangz");
    m_battlezPct[19] = m_battlezPct[18] + g_buteMgr.GetInt("Multiplayer", "Brickz");
    m_battlezPct[20] = m_battlezPct[19] + g_buteMgr.GetInt("Multiplayer", "Clubz");
    m_battlezPct[21] = m_battlezPct[20] + g_buteMgr.GetInt("Multiplayer", "Gauntletz");
    m_battlezPct[22] = m_battlezPct[21] + g_buteMgr.GetInt("Multiplayer", "Glovez");
    m_battlezPct[23] = m_battlezPct[22] + g_buteMgr.GetInt("Multiplayer", "Gooberz");
    m_battlezPct[24] = m_battlezPct[23] + g_buteMgr.GetInt("Multiplayer", "GravityBootz");
    m_battlezPct[25] = m_battlezPct[24] + g_buteMgr.GetInt("Multiplayer", "GunHatz");
    m_battlezPct[26] = m_battlezPct[25] + g_buteMgr.GetInt("Multiplayer", "NerfGunz");
    m_battlezPct[27] = m_battlezPct[26] + g_buteMgr.GetInt("Multiplayer", "Rockz");
    m_battlezPct[28] = m_battlezPct[27] + g_buteMgr.GetInt("Multiplayer", "Shieldz");
    m_battlezPct[29] = m_battlezPct[28] + g_buteMgr.GetInt("Multiplayer", "Shovelz");
    m_battlezPct[30] = m_battlezPct[29] + g_buteMgr.GetInt("Multiplayer", "Springz");
    m_battlezPct[31] = m_battlezPct[30] + g_buteMgr.GetInt("Multiplayer", "Spyz");
    m_battlezPct[32] = m_battlezPct[31] + g_buteMgr.GetInt("Multiplayer", "Swordz");
    m_battlezPct[33] = m_battlezPct[32] + g_buteMgr.GetInt("Multiplayer", "TimeBombz");
    m_battlezPct[34] = m_battlezPct[33] + g_buteMgr.GetInt("Multiplayer", "Toobz");
    m_battlezPct[35] = m_battlezPct[34] + g_buteMgr.GetInt("Multiplayer", "Wandz");
    m_battlezPct[36] = m_battlezPct[35] + g_buteMgr.GetInt("Multiplayer", "Welderz");
    m_battlezPct[37] = m_battlezPct[36] + g_buteMgr.GetInt("Multiplayer", "Wingz");
    SetTabState(5, 3);
    if ((static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))
            ->GetValueDword("StatusBar Position", 0)
        == 1) {
        RefreshA();
    }
    return 1;
}

// @early-stop
RVA(0x000fe6b0, 0x145)
i32 CStatusBarMgr::LoadMainStatusBarSprite() {
    if (m_position != kSubtypeTag) {
        if (m_redrawFrames > 0) {
            m_redrawFrames--;
            i32 v = m_barFrameGate;
            if (v > 0x1e0) {
                CDDSurface* tgt = (g_gameReg->m_world->m_drawTarget)->m_backPair->m_surface;

                RECT below;
                below.left = m_rect10.left;
                below.top = m_rect10.bottom;
                below.right = m_rect10.right;
                below.bottom = v;
                tgt->Restore(&below, 0);
            }
            CMapStringToOb* map = &m_world->m_imageRegistry->m_10map;
            CObject* found = 0;

            map->Lookup("GAME_STATUSBAR_MAINBAR", found);
            if (found) {

                CDDrawWorker* cfg = static_cast<CDDrawWorker*>(found);
                CImage* entry = static_cast<CImage*>(cfg->m_items.GetAt(cfg->m_minIndex));
                if (entry) {
                    CDDrawSubMgrPages* l1 = g_gameReg->m_world->m_drawTarget;
                    entry->RenderFrame(
                        l1->m_backPair,
                        entry->m_anchorX + m_rect10.left,
                        entry->m_anchorY + m_rect10.top,
                        0
                    );
                }
            }
        }

        POSITION n = m_tabLists[0].GetHeadPosition();
        while (n) {
            CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
            if (cur) {
                cur->Render();
            }
        }
        CPtrList& tab = m_tabLists[m_activeTab];
        POSITION m = tab.GetHeadPosition();
        while (m) {
            CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(m));
            if (cur) {
                cur->Render();
            }
        }
        if (m_retabNotify) {
            m_retabNotify->Draw();
        }
    }

    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CStatusBarItem* p = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(k));
        if (p) {
            p->SetSubtype();
            p->Render();
        }
    }
    return 1;
}

static __inline void HiCueFind() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
    if (host->m_emitGate == 0) {
        void* obj = ((host))->Lookup("GAME_TABHIGHLIGHT1");
        if (obj) {
            (static_cast<LeafCue*>(obj))->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }
}

static __inline void HiCueLookup() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
    if (host->m_emitGate == 0) {
        void* out = 0;
        host->m_cues.Lookup("GAME_TABHIGHLIGHT1", out);
        if (out) {
            (static_cast<LeafCue*>(out))->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }
}

static __inline void HiCueTimed() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
    if (host->m_emitGate == 0) {
        void* found = 0;
        host->m_cues.Lookup("GAME_TABHIGHLIGHT1", found);
        if (found && g_sndEnabled != 0) {
            i32 item = g_sndCueTag;
            LeafCue* p = static_cast<LeafCue*>(found);
            if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                >= static_cast<u32>(p->m_replayDelay)) {
                p->m_lastPlayTime = g_killCueClock;
                p->m_sound->ConfigureItem(item, 0, 0, 0);
            }
        }
    }
}

static __inline void HiPost(i32 cmdId) {
    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, cmdId, 0);
}

RVA(0x000fe910, 0xc2c)
i32 CStatusBarMgr::UpdateStatusBarTabHighlight(i32 a1, i32 a2, i32 a3) {
    CStatusBarItem* w = HitTestRects(a2, a3);
    if (w == 0) {
        return 1;
    }
    w->OnPointerMove(a1, a2, a3);
    i32 cmd = w->m_cmd;
    switch (w->m_tab) {
        case 0:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd > 0x259) {
                if (cmd == 0x25a) {
                    HiCueFind();
                    DockStatusBarRight();
                    return 1;
                }
                if (cmd == 0x25b) {
                    HiCueFind();
                    HideRect();
                    return 1;
                }
                return 0;
            }
            if (cmd == 0x259) {
                HiCueFind();
                RefreshA();
                return 1;
            }
            if (cmd <= 0 || cmd > 5) {
                return 0;
            }
            HiCueFind();
            SetTabState(cmd, 3);
            return 1;

        case 1:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0x12c || cmd > 0x149) {
                return 0;
            }
            if (cmd <= 0x13a) {
                HiCueLookup();
                ToggleStat(cmd - 0x12c);
            } else {
                HiCueLookup();
                PlaceCursorTarget(cmd - 0x13b, 0);
            }
            return 1;

        case 2:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0x64 || cmd > 0x68) {
                return 0;
            }
            ActivateSlot(cmd - 0x64);
            return 1;

        case 3:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0xd3 || cmd > 0xde) {
                return 1;
            }
            if (cmd <= 0xd6) {
                HlClickGroup0(cmd - 0xd3);
            } else if (cmd <= 0xda) {
                HlClickGroup1(cmd - 0xd7);
            } else {
                HlClickGroup2(cmd - 0xdb);
            }
            return 1;

        case 4:
            if (m_hitTestDisabled != 0) {
                return 1;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                return 1;
            }
            if (cmd < 0x190 || cmd > 0x193) {
                return 0;
            }
            HiCueLookup();
            m_tabCycle = cmd - 0x190;
            ResetWidgets(0);
            TryActivate();
            Deactivate();
            return 1;

        case 5:
            if (m_toggleActive != 0) {
                return 1;
            }
            switch (cmd) {
                case 0x1f4:
                    HiCueFind();
                    HiPost(0x8007);
                    return 1;
                case 0x1f5:
                    HiCueFind();
                    HiPost(0x80ce);
                    return 1;
                case 0x1f6:
                    HiCueFind();
                    HiPost(0x80cf);
                    return 1;
                case 0x1f8:
                    HiCueFind();
                    HiPost(0x8035);
                    return 1;
                case 0x1f7:
                    HiCueLookup();
                    HiPost(0x80e2);
                    return 1;
                case 0x1f9:
                    HiCueLookup();
                    if (g_gameReg->m_frameGate != 0) {
                        g_gameReg->m_frameGate ^= 1;
                        g_gameReg->FinishLevel(g_gameReg->m_frameGate, 1);
                    }
                    (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(1);
                    return 1;
                case 0x1fa:
                    HiCueLookup();
                    SetTab(5, 0);
                    return 1;
                case 0x1fc:
                    if (g_gameReg->m_gameMode != 1) {
                        return 1;
                    }
                    if (m_modeArmed != 0) {
                        return 1;
                    }
                    if (m_hitTestDisabled != 0) {
                        return 1;
                    }
                    HiCueLookup();
                    {
                        CPlay* sm = static_cast<CPlay*>(g_gameReg->m_curState);
                        if (m_destructWarnActive == 0) {
                            m_destructWarnActive = 1;
                            m_modeState = 2;
                            m_destructWarnDelay = g_buteMgr.GetDwordDef(
                                "StatusBar",
                                "DestructButtonWarningDelay",
                                0x32
                            );
                            m_destructWarnLast = static_cast<u32>(g_frameTime);
                            sm->ArmSnapshot(1, 0xbb7);
                        } else {
                            CSBI_ImageSet* n = m_modeNotify;
                            m_destructWarnActive = 0;
                            m_modeState = 1;
                            if (n) {
                                n->Notify(1);
                            }
                            sm->ArmSnapshot(0, 0xbb7);
                        }
                    }
                    return 1;
                default:
                    return 0;
            }

        case 6:
            switch (cmd) {
                case 0x324:
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        HiCueLookup();
                        g_gameReg->AccrueScoreTime();
                    } else if (g_gameReg->m_gameMode == 1) {
                        HiCueLookup();
                        HiPost(0x806b);
                    } else {
                        HiCueLookup();
                    }
                    return 1;
                case 0x325:
                    if (g_gameReg->m_gameMode == 1) {
                        if (g_gameReg->m_cmdGrid->m_phase == 1) {
                            g_gameReg->UpdateScoreHud();
                        }
                        HiCueLookup();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->AccrueScoreTime();
                    }
                    return 1;
                case 0x327:
                    if (g_gameReg->m_gameMode == 1) {
                        if (g_gameReg->m_cmdGrid->m_phase == 1) {
                            g_gameReg->UpdateScoreHud();
                        }
                        HiCueTimed();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->AccrueScoreTime();
                    }
                    return 1;
                case 0x328:
                    HiCueTimed();
                    return 1;
                default:
                    return 0;
            }

        default:
            return 0;
    }
}

// @early-stop
RVA(0x000ffb20, 0x13a)
i32 CStatusBarMgr::LoadDestructButtonSprite(i32 arg) {
    if (g_gameReg->m_soundEnabled != 0) {
        if (m_destructWarnActive != 0 && m_modeArmed == 0) {
            if (m_destructButton == 0) {

                CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                CMapStringToPtr* map = &host->m_cues;
                void* found = 0;
                map->Lookup("GAME_DESTRUCT", found);
                if (found) {
                    DSoundCloneInst* f = (static_cast<LeafCue*>(found))->m_sound;
                    if (f) {
                        DirectSoundMgr* obj = f->GetItem();
                        m_destructButton = obj;
                        if (obj) {
                            obj->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                        }
                    }
                }
            }
        } else {
            if (m_destructButton) {
                m_destructButton->StopAndRewind();
                m_destructButton = 0;
            }
        }
    }
    RefreshAll();

    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->Refresh(arg);
        }
    }
    CPtrList& tab = m_tabLists[m_activeTab];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(m));
        if (cur) {
            cur->Refresh(arg);
        }
    }
    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(k));
        if (cur) {
            cur->Refresh(arg);
        }
    }
    if (m_retabNotify) {
        m_retabNotify->Tick(arg);
        Deactivate();
    }
    return 1;
}

RVA(0x00102180, 0x5f)
void CStatusBarMgr::BuildGameTabResumeButton(i32 show) {
    if (m_position == kSubtypeTag) {
        RefreshState();
    }
    if (show && m_activeTab != 5) {
        SetTabState(5, 3);
    }
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_RESUME", 1);
        Deactivate();
        m_tabSprite5->SetSubtype();
    }
    m_hitTestDisabled = 1;
}

RVA(0x00102200, 0x37)
void CStatusBarMgr::BuildGameTabPauseButton() {
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", 1);
        Deactivate();
        m_tabSprite5->SetSubtype();
    }
    m_hitTestDisabled = 0;
}

// @early-stop
RVA(0x001055b0, 0x109)
i32 CStatusBarMgr::LoadGooCookingSprite(i32 idx) {
    CSbiSlot* sp = &m_slots[idx];
    if (sp->m_state != 0) {
        return 0;
    }
    if (g_gameReg->m_gameMode == 1 && m_hlBusy == 0) {
        if (m_position == kSubtypeTag) {
            RefreshState();
        }
        if (m_activeTab != 2) {
            SetTabState(2, 3);
        }
        Deactivate();
    }
    sp->m_state = 1;

    CSbiSlot* s = &m_slots[idx];
    s->m_interval = 0x7fffffff;
    s->m_startTimeLo = g_frameTime;
    s->m_startTimeHi = 0;
    if (m_activeTab == 2 && m_position != 2) {
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_GOOCOOKING1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x00105990, 0x3b4)
void CStatusBarMgr::UpdateRezConveyorStatusBar() {
    i32 count = 3;
    CSBI_ImageSet** notify = m_groupNotify;
    CSbiHlRow* ph = m_groupSlots;
    do {
        switch (ph->m_state) {
            case 1:
                if (++ph->m_counter > 9) {
                    ph->m_counter = 1;
                }
                break;
            case 2:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last
                    >= ph->m_interval) {
                    if (++ph->m_counter >= 0x12) {
                        ph->m_counter = 0x12;
                        ph->m_state = 7;
                        ph->m_interval =
                            g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltHoldDelay", 0x1f4);
                        ph->m_last = static_cast<u32>(g_frameTime);
                        UpdateFallingItemStatusBar(
                            m_extraNotifyArg0,
                            m_itemRect.left + 0xc,
                            m_itemRect.top + 0xc
                        );
                    }
                }
                break;
            case 3:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last
                    >= ph->m_interval) {
                    if (--ph->m_counter < 0xa) {
                        ph->m_state = 0;
                        ph->m_counter = 1;
                    }
                }
                break;
            case 4:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last
                    >= ph->m_interval) {
                    if (++ph->m_counter >= 0x18) {
                        ph->m_counter = 0x18;
                        ph->m_state = 6;
                        ph->m_interval =
                            g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltHoldInDelay", 0x1f4);
                        ph->m_last = static_cast<u32>(g_frameTime);
                        m_machinePhase = 8;
                        m_beltInterval =
                            g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                        m_beltLast = static_cast<u32>(g_frameTime);
                    }
                }
                break;
            case 5:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last
                    >= ph->m_interval) {
                    if (--ph->m_counter < 0x13) {
                        ph->m_state = 0;
                        ph->m_counter = 1;
                    }
                }
                break;
            case 6:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last
                    >= ph->m_interval) {
                    if (m_activeTab == 3 && m_position != 2) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                        if (host->m_emitGate == 0) {
                            void* found = 0;
                            host->m_cues.Lookup("GAME_REZBELTRETURN", found);
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                LeafCue* p = static_cast<LeafCue*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                    >= static_cast<u32>(p->m_replayDelay)) {
                                    p->m_lastPlayTime = g_killCueClock;
                                    p->m_sound->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = 5;
                }
                break;
            case 7:
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - ph->m_last
                    >= ph->m_interval) {
                    if (m_activeTab == 3 && m_position != 2) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                        if (host->m_emitGate == 0) {
                            void* found = 0;
                            host->m_cues.Lookup("GAME_REZBELTBACKUP", found);
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                LeafCue* p = static_cast<LeafCue*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                    >= static_cast<u32>(p->m_replayDelay)) {
                                    p->m_lastPlayTime = g_killCueClock;
                                    p->m_sound->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = 3;
                }
                break;
        }
        if (*notify) {
            (*notify)->Notify(ph->m_counter);
        }
        notify++;
        ph++;
    } while (--count);
}

// @early-stop
RVA(0x00105e40, 0x63c)
void CStatusBarMgr::LoadRezMachineConfig() {
    CSbiHlRow* pA = &m_machineB;
    CSbiHlRow* pB = &m_machineA;
    CSbiHlRow* g = m_groupSlots;
    if (pA->m_state == 5) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pA->m_last >= pA->m_interval) {
            if (++pA->m_counter > 0x34) {
                SetHudRectB(
                    0x2b,
                    5,
                    g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                );
            } else {
                pA->m_interval =
                    g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d);
                pA->m_last = static_cast<u32>(g_frameTime);
            }
        }
    } else if (pA->m_state == 6) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pA->m_last >= pA->m_interval) {
            if (++pA->m_counter > 0x44) {
                SetHudRectB(0x2b, 0, 0x7fffffff);
            } else {
                pA->m_interval =
                    g_buteMgr.GetDwordDef("StatusBar", "RightMachineSpewingDelay", 0x7d);
                pA->m_last = static_cast<u32>(g_frameTime);
            }
        }
    }

    switch (pB->m_state) {
        case 1:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 8) {
                    SetHudRectA(
                        1,
                        1,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case 2:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x13) {
                    SetHudRectA(
                        0x14,
                        3,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                    SetHudRectB(
                        0x2b,
                        5,
                        g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                    CSbiHlRow* s = m_groupSlots;
                    for (i32 i = 0; i < 3; i++) {
                        s->m_state = 1;
                        s->m_value = 1;
                        s++;
                    }
                    m_machinePhase = 2;
                    m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                    m_beltLast = static_cast<u32>(g_frameTime);
                    if (m_activeTab == 3 && m_position != 2) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                        if (host->m_emitGate == 0) {
                            void* found = 0;
                            host->m_cues.Lookup("GAME_REZMACHINE", found);
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                LeafCue* p = static_cast<LeafCue*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                    >= static_cast<u32>(p->m_replayDelay)) {
                                    p->m_lastPlayTime = g_killCueClock;
                                    p->m_sound->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineWakingDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case 3:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x1d) {
                    SetHudRectA(
                        0x14,
                        3,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case 4:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter == 0x26) {
                    i32 col;
                    i32 which = m_extraNotifyArg0;
                    if (which >= 0x22) {
                        col = 2;
                    } else {
                        col = (which >= 0x17) ? 1 : 0;
                    }
                    i32 found = 0;
                    for (i32 r = 3; r >= 0; r--) {
                        if (m_hlGrid[col * 4 + r].m_state == 0) {
                            found = 1;
                            break;
                        }
                    }
                    if (found) {
                        g[col].m_state = 4;
                        g[col].m_counter = 0x13;
                        if (m_activeTab == 3 && m_position != 2) {
                            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                            if (host->m_emitGate == 0) {
                                void* fnd = 0;
                                host->m_cues.Lookup("GAME_REZBELTRETRACT", fnd);
                                if (fnd && g_sndEnabled != 0) {
                                    i32 item = g_sndCueTag;
                                    LeafCue* p = static_cast<LeafCue*>(fnd);
                                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                        >= static_cast<u32>(p->m_replayDelay)) {
                                        p->m_lastPlayTime = g_killCueClock;
                                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    } else {
                        g[col].m_state = 2;
                        g[col].m_counter = 0xa;
                        if (m_activeTab == 3 && m_position != 2) {
                            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                            if (host->m_emitGate == 0) {
                                void* fnd = 0;
                                host->m_cues.Lookup("GAME_REZBELTDROP", fnd);
                                if (fnd && g_sndEnabled != 0) {
                                    i32 item = g_sndCueTag;
                                    LeafCue* p = static_cast<LeafCue*>(fnd);
                                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                        >= static_cast<u32>(p->m_replayDelay)) {
                                        p->m_lastPlayTime = g_killCueClock;
                                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    g[0].m_interval = g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltDelay", 0x64);
                    g[0].m_last = static_cast<u32>(g_frameTime);
                    if (pB->m_counter > 0x2a) {
                        SetHudRectA(
                            1,
                            1,
                            g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                        );
                    } else {
                        pB->m_interval =
                            g_buteMgr.GetDwordDef("StatusBar", "LeftMachineLeverDelay", 0x64);
                        pB->m_last = static_cast<u32>(g_frameTime);
                    }
                }
            }
            break;
    }

    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(pB->m_counter, pA->m_counter);
    }
}

RVA(0x00106660, 0x68)
void CStatusBarMgr::UpdateRezMachineSnoozeStatusBar() {
    SetHudRectA(1, 1, g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 100));
    SetHudRectB(0x2b, 0, 0x7fffffff);
    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(m_machineA.m_counter, m_machineB.m_counter);
    }
    m_rezActive = 0;
    m_rezTick = 0;
}

// @early-stop
RVA(0x00106bb0, 0x7d8)
void CStatusBarMgr::LoadChipMachineConfig() {
    i32 refreshFlag = 0;
    i32 rectFlag = 0;
    switch (m_machinePhase) {
        case 2:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRect.left += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRect.right += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.left >= 0x6d) {
                m_itemRect.left = 0x6d;
                m_itemRect.right = 0x84;
                m_machinePhase = 3;
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemInMachineTime", 0x7d0);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            refreshFlag = 1;
            break;
        case 3:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                SetHudRectB(
                    0x35,
                    6,
                    g_buteMgr.GetDwordDef("StatusBar", "RightMachineSpewingDelay", 0x7d)
                );
                m_machinePhase = 4;
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemWaitTime", 0x1f4);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            break;
        case 4:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_machinePhase = 5;
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* found = 0;
                        host->m_cues.Lookup("GAME_CHIPFALLOUT", found);
                        if (found && g_sndEnabled != 0) {
                            i32 item = g_sndCueTag;
                            LeafCue* p = static_cast<LeafCue*>(found);
                            if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                >= static_cast<u32>(p->m_replayDelay)) {
                                p->m_lastPlayTime = g_killCueClock;
                                p->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            break;
        case 5:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRect.top += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRect.bottom += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.bottom >= 0x11c) {
                m_itemRect.bottom = 0x11c;
                m_itemRect.top = 0x104;
                rectFlag = 1;
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* found = 0;
                        host->m_cues.Lookup("GAME_CHIPLAND", found);
                        if (found && g_sndEnabled != 0) {
                            i32 item = g_sndCueTag;
                            LeafCue* p = static_cast<LeafCue*>(found);
                            if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                >= static_cast<u32>(p->m_replayDelay)) {
                                p->m_lastPlayTime = g_killCueClock;
                                p->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_machinePhase = 7;
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
                if (m_extraNotifyArg0 >= 0x22) {
                    m_itemBaseX = 0x6d;
                } else if (m_extraNotifyArg0 >= 0x17) {
                    m_itemBaseX = 0x45;
                } else {
                    m_itemBaseX = 0x1d;
                }
            }
            refreshFlag = 1;
            break;
        case 7:
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRect.left -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRect.right -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.left <= m_itemBaseX) {
                m_itemRect.left = m_itemBaseX;
                m_itemRect.right = m_itemBaseX + 0x17;
                rectFlag = 1;
                SetHudRectA(
                    0x1e,
                    4,
                    g_buteMgr.GetDwordDef("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_machinePhase = 1;
            }
            refreshFlag = 1;
            break;
        case 8: {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_beltLast >= m_beltInterval) {
                m_itemRect.top += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRect.bottom += g_buteMgr.GetIntDef("StatusBar", "(FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            i32 col;
            if (m_extraNotifyArg0 >= 0x22) {
                col = 2;
            } else {
                col = (m_extraNotifyArg0 >= 0x17) ? 1 : 0;
            }
            i32 row = 3;
            while (m_hlGrid[col * 4 + row].m_state == 1) {
                row--;
                if (row < 0) {
                    break;
                }
            }
            if (m_itemRect.top >= row * 0x20 + 0x13e) {
                if (m_activeTab == 3 && m_position != 2) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* found = 0;
                        host->m_cues.Lookup("GAME_CHIPLAND", found);
                        if (found && g_sndEnabled != 0) {
                            i32 item = g_sndCueTag;
                            LeafCue* p = static_cast<LeafCue*>(found);
                            if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                >= static_cast<u32>(p->m_replayDelay)) {
                                p->m_lastPlayTime = g_killCueClock;
                                p->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                SetHlCell(col, m_extraNotifyArg0, row);
            }
            refreshFlag = 1;
            break;
        }
    }

    if (m_extraNotify0) {
        if (rectFlag) {
            m_extraNotify0->m_rect14.left = m_itemRect.left + m_rect10.left;
            m_extraNotify0->m_rect14.top = m_itemRect.top + m_rect10.top;
            m_extraNotify0->m_rect14.right = m_itemRect.right + m_rect10.left;
            m_extraNotify0->m_rect14.bottom = m_itemRect.bottom + m_rect10.top;
        }
        if (refreshFlag) {
            NotifyAllSlots();
        }
    }
}

// @early-stop
RVA(0x00107590, 0xc4)
i32 CStatusBarMgr::UpdateFallingItemStatusBar(i32 a1, i32 a2, i32 a3) {
    m_extraNotifyArg1 = a1;
    m_fallActive = 1;
    m_fallDelay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
    m_fallLast = static_cast<u32>(g_frameTime);
    CSBI_ImageSet* n = m_extraNotify1;
    i32 l = a2 - 0xc;
    i32 t = a3 - 0xc;
    i32 rr = a2 + 0xc;
    i32 b = a3 + 0xc;
    m_fallRect.left = l;
    m_fallRect.top = t;
    m_fallRect.right = rr;
    m_fallRect.bottom = b;
    if (n) {

        RECT rc;
        i32 x = m_rect10.left;
        rc.left = l + x;
        rc.right = x + rr;
        i32 y = m_rect10.top;
        rc.top = t + y;
        rc.bottom = y + b;
        n->m_rect14 = rc;
    }
    NotifyAllSlots();
    return 1;
}

RVA(0x00107a10, 0x62)
i32 CStatusBarMgr::UpdateRezMachineWakeStatusBar() {
    if (m_rezActive == 0) {
        if (m_extraNotifyArg0 == 0) {
            return 0;
        }
        SetHudRectA(9, 2, g_buteMgr.GetDwordDef("StatusBar", "LeftMachineWakingDelay", 100));
        m_rezActive = 1;
    } else {
        m_rezTick++;
    }
    return 1;
}

// @early-stop
RVA(0x00107ae0, 0x1aa)
void CStatusBarMgr::LoadMultiplayerBattlezConfig(i32) {
    BuildGameTabPauseButton();
    if (m_position == kSubtypeTag) {
        RefreshState();
    }
    if (m_activeTab != 5) {
        ClearTabGroup();
        m_activeTab = 5;
    }
    SetTab(5, 1);
    memset(m_statFlags, 0, sizeof(m_statFlags));
    Reset();

    i32 mode = g_gameReg->m_gameMode;
    if (mode == 2) {
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Multiplayer", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = kSlotCommitLevel;
            m_slots[i].m_state = kSlotReady;
        }
    } else if (mode == 3) {
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Battlez", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = kSlotCommitLevel;
            m_slots[i].m_state = kSlotReady;
        }
    }

    for (i32 j = 0; j < m_ptrPool.GetSize(); j++) {
        void* p = m_ptrPool.GetData()[j];
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_ptrPool.SetSize(0, -1);
    m_reserved2b0 = 0;
    m_reserved2b8 = 0;
    m_reserved2b4 = 0;
    m_reserved2bc = 0;
    m_hlBusy = 0;
    if (m_retabNotify) {
        free(m_retabNotify);
        m_retabNotify = 0;
    }
    ExitMode();
    m_observerTabAvailable = 0;
    m_modeArmed = 0;
    TryActivate();
}

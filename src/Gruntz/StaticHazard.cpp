#include <rva.h>

#include <Gruntz/StaticHazard.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Enums.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000fbb50, 0xa, int)
RVA_DYNINIT(0x000fbb70, 0x15, int)
RVA_DYNINIT(0x000fbba0, 0xe, int)
RVA_DYNINIT(0x000fbbc0, 0x1f, int)
template<> DATA(0x0024e3d0)
CActReg CActRegPool<CStaticHazard>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00012b00, 0x1e, ??_GCStaticHazard@@UAEPAXI@Z)
RVA_COMPGEN(0x00012b30, 0x44, ??1CStaticHazard@@UAE@XZ)

struct CString;

static inline CActHandler* HaznLookup(i32 coord) {
    return (CActRegPool<CStaticHazard>::s_table.ResolveEntry(coord));
}

// @early-stop
RVA(0x000fb7a0, 0x2f0)
CStaticHazard::CStaticHazard(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
    {
        CAniElement* d = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* e =
            d->m_records.GetSize() > 0 ? static_cast<CAniRecordView*>(d->m_records.GetAt(0)) : 0;
        m_wwdObject->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_param);
    }

    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0) {
        o->m_sortKey = 0;
        o->m_flags |= 0x20000;
    }
    m_tileCol = m_object->m_screenX >> TILE_SHIFT_PX;
    m_tileRow = m_object->m_screenY >> TILE_SHIFT_PX;
    m_object->m_health = 0;
    switch (g_gameReg->m_curState->m_levelType) {
        case AREA_TROUBLE_IN_THE_TROPICZ:
        case AREA_HIGH_ON_SWEETZ:
        case AREA_MINIATURE_MASTERZ:
        case AREA_GRUNTZ_IN_SPACE:
            m_object->m_health = m_object->m_screenY + 0x186b0;
            break;
        default:
            break;
    }
    m_object->m_area.left = m_object->m_screenX - 7;
    m_object->m_area.right = m_object->m_area.left + 14;
    m_object->m_area.top = m_object->m_screenY - 7;
    m_object->m_area.bottom = m_object->m_area.top + 14;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 0x2000002;
    m_object->m_animCursor.m_consumeDraw = 0;
    m_object->m_smarts = IDX(g_areaHazardDeath);
    m_activeWindow = 0;
    m_idleWindow = m_object->m_damage;
    m_pulseEpoch = g_frameTime;
    void* entry_ob = 0;
    g_gameReg->m_world->m_animRegistry->m_animations.Lookup("LEVEL_STATICHAZARDGO", entry_ob);
    CAniElement* entry = static_cast<CAniElement*>(entry_ob);
    if (entry != NULL) {

        m_activeWindow = g_buteMgr.GetIntDef("Hazardz", "AniPad", 0x64) + entry->m_total;
    } else {
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x461);
    }
    if (m_object->m_damage == 0) {
        m_idleWindow = m_activeWindow;
    }
}

RVA(0x000fbbf0, 0x102)
void CStaticHazard::FireActivation(i32 coord) {
    CActHandler* e = HaznLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = HaznLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000fbd50, 0x2ac)
void CStaticHazard::RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    (*CActRegPool<CStaticHazard>::s_table.ResolveEntryCallReport(id)) =
        static_cast<CActHandler>(&CStaticHazard::LoadAttributes2);

    ACT_NAME_ID(id2, "B")
    (*CActRegPool<CStaticHazard>::s_table.ResolveEntryCallReport(id2)) =
        static_cast<CActHandler>(&CStaticHazard::LoadAttributes);
}

// @early-stop
RVA(0x000fc0b0, 0xb2)
i32 CStaticHazard::LoadAttributes2() {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_gameMode == GAMEMODE_SINGLE) {
        return 0;
    }
    u32 phase = g_frameTime - m_pulseEpoch;
    u32 base = static_cast<u32>(m_object->m_points);
    if (phase <= base) {
        return 0;
    }
    phase -= base;
    u32 span = m_idleWindow + m_activeWindow;
    if (phase % span > static_cast<u32>(m_activeWindow)) {
        return 0;
    }
    m_fired = 1;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
    {
        CAniElement* d = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* e =
            d->m_records.GetSize() > 0 ? static_cast<CAniRecordView*>(d->m_records.GetAt(0)) : 0;
        m_wwdObject->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_param);
    }
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("B");
    return 0;
}

// @early-stop
// two instruction-selection residues: retail compares the HitTestCell pointer with
// its cached zero register (`cmp eax,ebx`) where cl emits `test eax,eax`, and it
// forms the m_animCursor sub-object address once (`add eax,0x1a0`) where cl folds
// 0x1a0 into both field displacements.
RVA(0x000fc1a0, 0x33b)
i32 CStaticHazard::LoadAttributes() {
    u32 phase = (g_frameTime - m_pulseEpoch) - static_cast<u32>(m_object->m_points);
    u32 rem = phase % static_cast<u32>((m_idleWindow + m_activeWindow));
    if (rem > static_cast<u32>(m_activeWindow)) {

        if (m_fired != 0) {

            if (m_object->m_damage == 0) {

                m_value = m_wwdObject->m_animCursor.m_animation;
                m_wwdObject->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
                {
                    CAniElement* d = m_wwdObject->m_animCursor.m_animation;
                    CAniRecordView* e = d->m_records.GetSize() > 0
                                            ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                            : 0;
                    m_wwdObject->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_param);
                }
                CWwdGameObjectA* o = m_object;
                if (o->m_sortKey != 0) {
                    o->m_sortKey = 0;
                    o->m_flags |= 0x20000;
                }
                m_fired = 0;
                return 0;
            }

            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId("A");
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                CAniElement* d = m_wwdObject->m_animCursor.m_animation;
                CAniRecordView* e = d->m_records.GetSize() > 0
                                        ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                        : 0;
                m_wwdObject->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_param);
            }
            CWwdGameObjectA* o = m_object;
            if (o->m_sortKey != 0) {
                o->m_sortKey = 0;
                o->m_flags |= 0x20000;
            }

            CMapMgr* grid = g_gameReg->m_tileGrid;
            i32 row = m_tileRow;
            i32 col = m_tileCol;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
                grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
            }
            return 0;
        }
    } else if (m_fired == 0 && m_object->m_damage == 0) {

        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("LEVEL_STATICHAZARDGO", 0);
        {
            CAniElement* d = m_wwdObject->m_animCursor.m_animation;
            CAniRecordView* e = d->m_records.GetSize() > 0
                                    ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                    : 0;
            m_wwdObject->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_param);
        }
        CWwdGameObjectA* o = m_object;
        if (o->m_sortKey != 0) {
            o->m_sortKey = 0;
            o->m_flags |= 0x20000;
        }
        m_fired = 1;
        return 0;
    }

    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) == WWDDRAW_EFFECT_FRAME) {
        i32 a, b;
        if (g_gameReg->m_cmdGrid->HitTestCell(m_object->m_screenX, m_object->m_screenY, &a, &b, 0)
            != NULL) {
            g_gameReg->m_cmdGrid
                ->CellDispatch(a, b, static_cast<GruntDeathType>(m_object->m_smarts), -1);
        }
        CWwdGameObjectA* o = m_object;
        if (o->m_sortKey != o->m_health) {
            o->m_sortKey = o->m_health;
            o->m_flags |= 0x20000;
        }
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 row = m_tileRow;
        i32 col = m_tileCol;
        if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[row][col * 7] |= 0x8000000;
        }
    } else {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 row = m_tileRow;
        i32 col = m_tileCol;
        if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
        }
        CWwdGameObjectA* o = m_object;
        if (o->m_sortKey != 0) {
            o->m_sortKey = 0;
            o->m_flags |= 0x20000;
        }
    }
    {
        CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
        if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyLookupGeometry("LEVEL_STATICHAZARDIDLE", 0);
            {
                CAniElement* d = m_wwdObject->m_animCursor.m_animation;
                CAniRecordView* e = d->m_records.GetSize() > 0
                                        ? static_cast<CAniRecordView*>(d->m_records.GetAt(0))
                                        : 0;
                m_wwdObject->ApplyLookupSprite("LEVEL_STATICHAZARD", e->m_param);
            }
            CMapMgr* grid = g_gameReg->m_tileGrid;
            i32 row = m_tileRow;
            i32 col = m_tileCol;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
                grid->m_rowInts[row][col * 7] &= 0xf7ffffff;
            }
        }
    }
    return 0;
}

RVA(0x000fc5b0, 0xf5)
i32 CStaticHazard::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    CFileMemBase* arc = ar;
    switch (mode) {
        case SERIAL_SAVE:
            arc->Write(&m_pulseEpoch, sizeof(m_pulseEpoch));
            arc->Write(&m_activeWindow, sizeof(m_activeWindow));
            arc->Write(&m_idleWindow, sizeof(m_idleWindow));
            arc->Write(&m_fired, sizeof(m_fired));
            arc->Write(&m_tileCol, sizeof(m_tileCol));
            arc->Write(&m_tileRow, sizeof(m_tileRow));
            break;
        case SERIAL_LOAD:
            arc->Read(&m_pulseEpoch, sizeof(m_pulseEpoch));
            arc->Read(&m_activeWindow, sizeof(m_activeWindow));
            arc->Read(&m_idleWindow, sizeof(m_idleWindow));
            arc->Read(&m_fired, sizeof(m_fired));
            arc->Read(&m_tileCol, sizeof(m_tileCol));
            arc->Read(&m_tileRow, sizeof(m_tileRow));
            break;
    }
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(arc, mode, typeId, pObj) != 0;
}

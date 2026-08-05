#include <rva.h>

#include <Gruntz/GruntEntranceMove.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

DATA(0x0020d7f4)
char s_codeM[] = "M";

DATA(0x0020cca4)
char s_codeD[] = "D";

DATA(0x001e9a48)
double g_wingzScale = 100.0;
DATA(0x001e9a50)
double g_wingzBias = -0.5;
static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";
static char s_EntranceSafeTime[] = "EntranceSafeTime";
static char s_IdleDelay[] = "IdleDelay";
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius";
static char s_CombatTimeout[] = "CombatTimeout";

static const char s_GRUNTZ_DEATHZ_FREEZE[] = "GRUNTZ_DEATHZ_FREEZE";

DATA(0x0020ee48)
static const char s_GRUNTZ_DEATHZ_SPARKLE[] = "GRUNTZ_DEATHZ_SPARKLE";
DATA(0x0020ee1c)
static const char s_GRUNTZ_DEATHZ_UNFREEZE[] = "GRUNTZ_DEATHZ_UNFREEZE";
DATA(0x0020cca8)
static char s_Spellz[] = "Spellz";
DATA(0x0020ee38)
static char s_FreezeDelay[] = "FreezeDelay";

static char s_BOMBGRUNT[] = "BOMBGRUNT";
DATA(0x0020e264)
static char s_RunningTimePerTile[] = "RunningTimePerTile";

static const char s_animKeyK[] = "K";

DATA(0x0020edf4)
static const char s_NW_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_ITEM";
DATA(0x0020edd0)
static const char s_N_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTH_ITEM";
DATA(0x0020eda8)
static const char s_NE_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_ITEM";
DATA(0x0020ed84)
static const char s_W_ITEM[] = "GRUNTZ_WINGZGRUNT_WEST_ITEM";
DATA(0x0020ed60)
static const char s_E_ITEM[] = "GRUNTZ_WINGZGRUNT_EAST_ITEM";
DATA(0x0020ed38)
static const char s_SW_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_ITEM";
DATA(0x0020ed14)
static const char s_S_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTH_ITEM";
DATA(0x0020ecec)
static const char s_SE_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_ITEM";
DATA(0x0020eca8)
static const char s_NW_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_WALK";
DATA(0x0020ec84)
static const char s_N_WALK[] = "GRUNTZ_WINGZGRUNT_NORTH_WALK";
DATA(0x0020ec5c)
static const char s_NE_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_WALK";
DATA(0x0020ec38)
static const char s_W_WALK[] = "GRUNTZ_WINGZGRUNT_WEST_WALK";
DATA(0x0020ec14)
static const char s_E_WALK[] = "GRUNTZ_WINGZGRUNT_EAST_WALK";
DATA(0x0020ebec)
static const char s_SW_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_WALK";
DATA(0x0020ebc8)
static const char s_S_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTH_WALK";
DATA(0x0020eba0)
static const char s_SE_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_WALK";
DATA(0x0020eb78)
static const char s_NW_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_IDLE";
DATA(0x0020eb54)
static const char s_N_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTH_IDLE";
DATA(0x0020eb2c)
static const char s_NE_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_IDLE";
DATA(0x0020eb08)
static const char s_W_IDLE[] = "GRUNTZ_WINGZGRUNT_WEST_IDLE";
DATA(0x0020eae4)
static const char s_E_IDLE[] = "GRUNTZ_WINGZGRUNT_EAST_IDLE";
DATA(0x0020eabc)
static const char s_SW_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_IDLE";
DATA(0x0020ea98)
static const char s_S_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTH_IDLE";
DATA(0x0020ea70)
static const char s_SE_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_IDLE";
DATA(0x0020ecd0)
static const char s_WG_ITEM[] = "GRUNTZ_WINGZGRUNT_ITEM";
DATA(0x0020ea54)
static const char s_WG_WALK[] = "GRUNTZ_WINGZGRUNT_WALK";
DATA(0x0020ea38)
static const char s_WG_IDLE1[] = "GRUNTZ_WINGZGRUNT_IDLE1";
DATA(0x0020ea1c)
static const char s_WG_IDLE2[] = "GRUNTZ_WINGZGRUNT_IDLE2";
DATA(0x0020ea00)
static const char s_WG_IDLE3[] = "GRUNTZ_WINGZGRUNT_IDLE3";
DATA(0x0020e9e4)
static const char s_WG_IDLE4[] = "GRUNTZ_WINGZGRUNT_IDLE4";
DATA(0x0020e9c8)
static const char s_WG_IDLE5[] = "GRUNTZ_WINGZGRUNT_IDLE5";

DATA(0x0020e998)
static const char s_GRUNTZ_ENTRANCEZ[] = "GRUNTZ_ENTRANCEZ";
DATA(0x0020e9ac)
static const char s_GRUNTZ_ENTRANCEZ_ONE[] = "GRUNTZ_ENTRANCEZ_ONE";
DATA(0x0020e97c)
static const char s_GRUNTZ_ENTRANCEZ_TWO[] = "GRUNTZ_ENTRANCEZ_TWO";
DATA(0x0020e960)
static const char s_GRUNTZ_ENTRANCEZ_THREE[] = "GRUNTZ_ENTRANCEZ_THREE";
DATA(0x0020e944)
static const char s_GRUNTZ_ENTRANCEZ_DROP[] = "GRUNTZ_ENTRANCEZ_DROP";
DATA(0x0020e924)
static const char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
static const char s_GRUNTZ_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";

DATA(0x0020ee64)
static char s_MovingDeathTime[] = "MovingDeathTime";
static const char s_animKeyS[] = "S";

static __inline i32 s_TileFlags(CGruntzMapMgr* b, i32 tx, i32 ty) {
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        return 1;
    }
    return b->m_rowInts[ty][tx * 7];
}

void CGrunt::ApplyMoveKind(i32 v) {}

static void GruntScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt--) {
        if (slot != NULL) {
            slot->~CString();
        }
        slot++;
    }
}

// @early-stop
RVA(0x00067850, 0x214)
i32 CGrunt::RunEntranceMove() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    CAniAdvanceCursor* cur = &m_wwdObject->m_animCursor;
    if (!((cur->m_finished != 0 && cur->m_frameTicksLeft == 0)
          || m_entrancePickup == PICKUP_NONE)) {
        return 0;
    }

    m_entranceActive = 0;
    const char* nm0 = *g_typeColl.ScratchResolve(m_prevAnimSetNode);
    GruntScratchTeardown();
    bool eq;
    eq = (strcmp(nm0, s_codeD) == 0);
    if (eq) {
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
        }
        m_tileMoveCommitted = 0;
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId(s_codeD);
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
        GruntDirectionCell cell = m_entranceCell;
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        char* nm = m_cells[base].WalkName().GetBuffer(0);
        m_wwdObject->ApplyName(nm);
    } else {
        ResetEntranceAnimation(1, 0, 0);
    }

    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    PickupType mode = m_entrancePickup;
    if (mode == PICKUP_INVALID) {
        return 0;
    }
    if (mode >= PICKUP_POWERUPZ_FIRST) {
        return LoadVehicleGruntSprites(mode);
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        m_brickPickupType = mode;
        m_entrancePickup = PICKUP_INVALID;
        return 1;
    }
    if (mode >= PICKUP_TOYZ_FIRST) {
        LoadVehicleGruntSprites(mode);
        return 0;
    }
    return LoadTypeTableClearMove(mode);
}

// @early-stop
RVA(0x00067b00, 0x92)
i32 CGrunt::GruntInRadius(i32 col, i32 row) {
    CGrunt* other = m_tileMgr->m_grid[col * TM_GRID_COLS + row];
    if (other != NULL && other->m_entranceCommitted != 0 && other->m_gruntKind != GRUNT_GHOST) {
        i32 ox = other->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oy = other->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 tx = m_defenderPx.m_x >> TILE_SHIFT_PX;
        i32 ty = m_defenderPx.m_y >> TILE_SHIFT_PX;
        i32 dx = oy - ty;
        i32 dy = ox - tx;
        i32 sum = m_defenderRadius + m_reachRect.right;
        i32 dist2 = abs(dx * dx + dy * dy);
        return dist2 < sum * sum ? 1 : 0;
    }
    return 0;
}

// @early-stop
// Regalloc: retail spills `found` into the dead `mode` param home (one fewer
// frame dword) and keeps `base` on the stack instead of a register.
RVA(0x00067bd0, 0x2ef)
void CGrunt::BuildEntranceAnimation(i32 mode) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_animKeyK);

    m_entranceArmed = 1;
    m_entranceCommitted = 0;
    m_entranceActive = 1;
    CWwdGameObjectA* h = m_object;
    if (h->m_sortKey != SORTKEY_ACTOR) {
        h->m_sortKey = SORTKEY_ACTOR;
        h->m_flags |= 0x20000;
    }

    ClearAllSprites();

    CString key;

    CAniElement* found;
    const char* base;

    if (mode == 1) {
        i32 onScreen = 0;
        CGruntzMgr* g = g_gameReg;
        {
            i32 x = m_object->m_screenX;
            i32 y = m_object->m_screenY;
            if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
                && y >= g->m_viewBounds.top) {
                onScreen = 1;
            } else {

                CGrunt* focus;
                CTriggerMgr* tm = g->m_cmdGrid;
                if (tm->m_recList.GetCount() == 1) {
                    i32* vec = static_cast<i32*>(tm->m_recList.GetHead());
                    i32 a = vec[0];
                    i32 b = vec[1];
                    focus = tm->m_grid[a * TM_GRID_COLS + b];
                } else {
                    focus = 0;
                }
                if (this == focus && m_tileOwnerHi == g_curPlayer) {
                    onScreen = 1;
                }
            }
        }

        i32 r = rand() % 0x1e1;
        if (r > 0x140) {
            found = 0;
            MapLookup(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_GRUNTZ_ENTRANCEZ_ONE,
                found
            );
            if (onScreen) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x37a, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else if (r > 0xa0) {
            found = 0;
            MapLookup(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_GRUNTZ_ENTRANCEZ_TWO,
                found
            );
            if (onScreen) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x37b, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else {
            found = 0;
            MapLookup(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_GRUNTZ_ENTRANCEZ_THREE,
                found
            );
            if (onScreen) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x37c, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        }
    } else if (mode == 2) {
        found = 0;
        MapLookup(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_GRUNTZ_ENTRANCEZ_DROP,
            found
        );
        base = s_GRUNTZ_ENTRANCEZ_DROP;
    } else {
        found = 0;
        MapLookup(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_GRUNTZ_ENTRANCEZ_RESSURECT,
            found
        );
        base = s_GRUNTZ_DEATHZ_MELT;
    }

    key = base;

    if (!found) {
        ResetEntranceAnimation(1, 0, 0);
    } else {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(found);
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        m_wwdObject->ApplyLookupSprite(key, elem->m_param);
    }
}

// @early-stop
// Regalloc colour only: retail pins the tile flags in edi where cl uses ebp,
// so retail cannot CSE grid->m_width across the two range checks.
RVA(0x00067f80, 0x313)
i32 CGrunt::LoadEntranceConfig() {
    if (m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta)) == 1) {
        CGruntzMgr* g = g_gameReg;
        CWwdGameObjectA* h = m_object;
        CMapMgr* grid = g->m_tileGrid;
        i32 tx = h->m_screenX >> TILE_SHIFT_PX;
        i32 ty = h->m_screenY >> TILE_SHIFT_PX;

        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            flags = 1;
        } else {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        }

        if (flags & 0x20000000) {
            i32 owner;
            if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
                || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
                owner = -1;
            } else {
                owner = ((grid->m_rowInts[ty]))[tx * 7 + 1];
            }
            i32 b = (owner >> 8) & 0xff;
            i32 a = owner & 0xff;
            if (m_tileOwnerHi != b || m_tileOwnerLo != a) {
                m_tileMgr->CellDispatch(b, a, DEATH_SQUASH, m_tileOwnerHi);
            }
        }

        h = m_object;
        i32 oldX = m_lastTilePx.m_x;
        m_entranceArmed = 0;
        i32 newPxX = h->m_screenX;
        i32 newPxY = h->m_screenY;
        i32 oldTileX = oldX >> TILE_SHIFT_PX;
        i32 oldTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 newTileX = newPxX >> TILE_SHIFT_PX;
        i32 newTileY = newPxY >> TILE_SHIFT_PX;

        if (oldX != -1 && m_lastTilePx.m_y != -1) {
            CMapMgr* og = g_gameReg->m_tileGrid;

            og->m_rows[oldTileY][oldTileX].m_flagBytes[3] &= ~0x20;
            og->m_rowInts[oldTileY][oldTileX * 7 + 1] = -1;
        }
        {
            CMapMgr* ng = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);

            ng->m_rows[newTileY][newTileX].m_flagBytes[3] |= 0x20;
            ng->m_rowInts[newTileY][newTileX * 7 + 1] = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        }
        m_lastTilePx.m_x = newPxX;
        m_lastTilePx.m_y = newPxY;
        m_tileMgr->WireTileSwitchLogic(this, newPxX, newPxY);

        h = m_object;
        m_entranceCommitted = 1;
        if (h->m_sortKey != h->m_screenY + 0x186a0) {
            h->m_sortKey = h->m_screenY + 0x186a0;
            h->m_flags |= 0x20000;
        }

        CWwdGameObjectA* p = m_wwdObject;
        CAniElement* found = 0;
        CAniElement* cached = p->m_animCursor.m_animation;
        MapLookup(p->OwnerMgr()->m_animRegistry->m_animations, s_GRUNTZ_ENTRANCEZ_DROP, found);
        if (cached == found) {
            if (m_tileOwnerHi == g_curPlayer) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x33f, -1, 0, -1, -1);
            }
            m_tileMgr->ResetCell(m_tileOwnerHi, m_tileOwnerLo, 0, 0);
            m_entranceDropActive = 1;
            m_entranceSafeTimeLo = g_buteMgr.GetDwordDef(s_Grunt, s_EntranceSafeTime, 5000);
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = g_frameTime;
            m_entranceClockHi = 0;
            m_flashWindowLo = 0;
            m_flashWindowHi = 0;
        } else {
            if (m_tileMgr->RecordListHas(m_tileOwnerHi, m_tileOwnerLo)) {
                CommitArrival();
            }
        }
        m_entranceActive = 0;
        ReadConfigFromButeMgr();
        LoadCellAnimNames(0, 0);
        LoadAnimNameTable(0, 0);
    }

    if (m_wwdObject->m_animCursor.m_finished == 0
        || m_wwdObject->m_animCursor.m_frameTicksLeft != 0) {
        return 0;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 0;
}

// @early-stop
RVA(0x00068370, 0x14c)
i32 CGrunt::RearmEntranceDrop() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_wwdObject->m_animCursor.m_finished != 0
        && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
        m_bombRunActive = 0;
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(AT(m_poseItem, GRUNT_ITEM2));

        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        i32 frame = elem->m_param;

        GruntDirectionCell cell = m_entranceCell;
        i32 row = cell.row;
        i32 column = cell.column;

        const char* name = m_cells[3 * row + column].ItemName().GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(name, frame);
    }

    if (m_bombRunActive == 0) {
        i32 a;
        i32 b;
        m_entranceCommitted = 0;
        if (m_tileMgr->HitTestCell(m_object->m_screenX, m_object->m_screenY, &a, &b, 0) != NULL) {
            m_tileMgr->CellDispatch(a, b, DEATH_EXPLODE, -1);
            m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
        } else {
            m_entranceCommitted = 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00068520, 0x2a2)
i32 CGrunt::StartBombGruntRun() {
    FinishActiveAction();
    if (m_healthSprite != NULL) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = NULL;
    }
    if (m_staminaSprite != NULL) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = NULL;
    }
    if (m_toySprite != NULL) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = NULL;
    }
    if (m_toyTimeSprite != NULL) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = NULL;
    }
    if (m_wingzTimeSprite != NULL) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = NULL;
    }
    if (m_powerupSprite != NULL) {
        m_powerupSprite->m_flags |= 0x10000;
        m_powerupSprite = NULL;
    }
    if (m_selectedSprite != NULL) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = NULL;
    }
    m_gruntKind = GRUNT_NORMAL;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    m_entranceActive = 1;
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);
    SnapToLastTile(1);
    SetEntrancePos(1, 1);
    if (LoadGruntTypeTable(PICKUP_BOMB, 1, 0, 1) == 0) {
        CWwdGameObjectA* h = m_object;
        m_tileMgr->LoadExplosionSprites(h->m_screenX, h->m_screenY, -1, 0);
        return 0;
    }
    i32 dx = rand() % 3 - 1;
    i32 dy = rand() % 3 - 1;
    if (dx == 0 && dy == 0) {
        dx = 1;
    }
    {
        CWwdGameObjectA* h = m_object;
        dy += h->m_screenY >> TILE_SHIFT_PX;
        dx += h->m_screenX >> TILE_SHIFT_PX;
    }
    PlayMoveSoundAtTile(dx, dy);
    m_moveTile.m_x = dx;
    m_moveTile.m_y = dy;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_codeM);
    m_timePerTile = g_buteMgr.GetIntDef(s_BOMBGRUNT, s_RunningTimePerTile, 0x64);
    m_bombRunActive = 1;
    {
        CWwdGameObjectA* h = m_object;
        i32 vx = h->m_screenX;
        i32 vy = h->m_screenY;
        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
        if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
            g_gameReg->m_cueSink->LoadGruntSpawnConfig(this, 8, -1, -1, -1);
        }
    }
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(AT(m_poseItem, GRUNT_ITEM1));
    GruntDirectionCell cell = m_entranceCell;
    i32 col = cell.column + cell.row * 2;
    i32 base = cell.row + col;
    char* cn = m_cells[base].ItemName().GetBuffer(0);
    m_wwdObject->ApplyName(cn);
    return 0;
}

// @early-stop
RVA(0x00068880, 0x67c)
i32 CGrunt::LoadWingzGruntSprites(i32 enable) {
    CAniElement* _out;
    if (enable != 0) {
        m_wingzEnabled = 1;
        m_wingzDurationLo =
            static_cast<i32>((static_cast<double>(m_wingzTime) * g_wingzScale - g_wingzBias));
        m_wingzDurationHi = 0;
        m_wingzClockLo = static_cast<i32>(g_frameTime);
        m_wingzClockHi = 0;
        CreateWingzTimeSprite();

        m_cells[0].IdleName() = s_NW_ITEM;
        m_cells[1].IdleName() = s_N_ITEM;
        m_cells[2].IdleName() = s_NE_ITEM;
        m_cells[3].IdleName() = s_W_ITEM;
        m_cells[4].IdleName() = s_N_ITEM;
        m_cells[5].IdleName() = s_E_ITEM;
        m_cells[6].IdleName() = s_SW_ITEM;
        m_cells[7].IdleName() = s_S_ITEM;
        m_cells[8].IdleName() = s_SE_ITEM;
        m_cells[0].WalkName() = s_NW_ITEM;
        m_cells[1].WalkName() = s_N_ITEM;
        m_cells[2].WalkName() = s_NE_ITEM;
        m_cells[3].WalkName() = s_W_ITEM;
        m_cells[4].WalkName() = s_N_ITEM;
        m_cells[5].WalkName() = s_E_ITEM;
        m_cells[6].WalkName() = s_SW_ITEM;
        m_cells[7].WalkName() = s_S_ITEM;
        m_cells[8].WalkName() = s_SE_ITEM;

        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_ITEM, _out);
        m_poseWalk = _out;
        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_ITEM, _out);
        AT(m_poseIdle, GRUNT_IDLE3) = 0;
        AT(m_poseIdle, GRUNT_IDLE1) = _out;
        AT(m_poseIdle, GRUNT_IDLE2) = _out;
        AT(m_poseIdle, GRUNT_IDLE4) = 0;
        AT(m_poseIdle, GRUNT_IDLE5) = 0;

        CGruntzMgr* g = g_gameReg;
        i32 y = m_object->m_screenY;
        i32 x = m_object->m_screenX;
        CCueRect* r = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 8, -1, -1, -1);
        }
    } else {
        m_wingzEnabled = 0;
        m_wingzDurationLo = 0;
        m_wingzDurationHi = 0;
        if (m_wingzTimeSprite != NULL) {
            m_wingzTimeSprite->m_flags |= 0x10000;
            m_wingzTimeSprite = NULL;
        }

        m_cells[0].WalkName() = s_NW_WALK;
        m_cells[1].WalkName() = s_N_WALK;
        m_cells[2].WalkName() = s_NE_WALK;
        m_cells[3].WalkName() = s_W_WALK;
        m_cells[4].WalkName() = s_N_WALK;
        m_cells[5].WalkName() = s_E_WALK;
        m_cells[6].WalkName() = s_SW_WALK;
        m_cells[7].WalkName() = s_S_WALK;
        m_cells[8].WalkName() = s_SE_WALK;
        m_cells[0].IdleName() = s_NW_IDLE;
        m_cells[1].IdleName() = s_N_IDLE;
        m_cells[2].IdleName() = s_NE_IDLE;
        m_cells[3].IdleName() = s_W_IDLE;
        m_cells[4].IdleName() = s_N_IDLE;
        m_cells[5].IdleName() = s_E_IDLE;
        m_cells[6].IdleName() = s_SW_IDLE;
        m_cells[7].IdleName() = s_S_IDLE;
        m_cells[8].IdleName() = s_SE_IDLE;

        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_WALK, _out);
        m_poseWalk = _out;
        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE1, _out);
        AT(m_poseIdle, GRUNT_IDLE1) = _out;
        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE2, _out);
        AT(m_poseIdle, GRUNT_IDLE2) = _out;
        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE3, _out);
        AT(m_poseIdle, GRUNT_IDLE3) = _out;
        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE4, _out);
        AT(m_poseIdle, GRUNT_IDLE4) = _out;
        _out = NULL;
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE5, _out);
        AT(m_poseIdle, GRUNT_IDLE5) = _out;
    }

    CString* rec = g_typeColl.ScratchResolve(m_objAux->m_actKey);
    GruntScratchTeardown();
    if (strcmp(*rec, s_codeD) == 0) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(m_poseWalk);
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        i32 frame = elem->m_param;
        i32 idx = 3 * m_entranceCell.row + m_entranceCell.column;
        char* buf = m_cells[idx].WalkName().GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(buf, frame);
        return 1;
    }

    CString* rec2 = g_typeColl.ScratchResolve(m_objAux->m_actKey);
    GruntScratchTeardown();
    if (strcmp(*rec2, "A") == 0) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(AT(m_poseIdle, GRUNT_IDLE1));
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        i32 frame = elem->m_param;
        i32 idx = 3 * m_entranceCell.row + m_entranceCell.column;
        char* buf = m_cells[idx].IdleName().GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(buf, frame);
    }
    return 1;
}

// @early-stop
RVA(0x000690a0, 0x1c5)
i32 CGrunt::UpdateEntranceAnim() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    if (m_wwdObject->m_animCursor.m_finished == 0
        || m_wwdObject->m_animCursor.m_frameTicksLeft != 0) {
        return 0;
    }

    if (m_entranceStamped == 0) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(AT(m_poseToy, GRUNT_TOY_BREAK));

        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        i32 frame = elem->m_param;

        char* buf = (&m_frameSetName)->GetBuffer(0);
        m_wwdObject->ApplyLookupSprite(buf, frame);

        m_entranceStamped = 1;
        i32 v = m_moveVariant;
        if (v != 0) {
            ApplyMoveKind(v);
        } else {
            ApplyMoveKind(m_moveKind);
        }
        return 0;
    }

    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    LoadGruntTypeTable(m_toolId, 1, 0, 0);
    m_entranceActive = 0;

    CGruntzMgr* g = g_gameReg;
    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    CGruntzMapMgr* board = g->m_tileGrid;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(board->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(board->m_height)) {
        flags = 1;
    } else {
        flags = board->m_rowInts[ty][tx * 7];
    }

    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        return 0;
    }

    CWwdGameObjectA* h = m_object;
    i32 z = h->m_screenY + 0x186a0;
    if (h->m_sortKey != z) {
        h->m_sortKey = z;
        h->m_flags |= 0x20000;
    }
    return 0;
}

// @early-stop
RVA(0x000692f0, 0x850)
i32 CGrunt::StepArrivalCommit() {
    if (m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "A") != 0);
    if (!eq) {
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeD) != 0);
    if (!eq) {
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_cueSink->StopVoice(m_object->m_objectId);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            -1
        );
        if (m_entranceReason != PICKUP_BOMB) {
            goto finalize;
        }
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "G") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "L") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "P") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeO) == 0);
    if (eq) {
        SnapToLastTile(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "J") == 0);
    if (eq) {

        m_entranceActive = 0;
        eq = (strcmp(*g_typeColl.GetNameRecord(m_prevAnimSetNode), s_codeD) == 0);
        if (eq) {
            if (m_poweredUp == 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
            m_tileMoveCommitted = 0;
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId(s_codeD);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_poseWalk);
            GruntDirectionCell cell = m_entranceCell;
            i32 colv = cell.column + cell.row * 2;
            i32 base = cell.row + colv;
            char* nm = m_cells[base].WalkName().GetBuffer(0);
            m_wwdObject->ApplyName(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeN) == 0);
    if (eq) {
        i32 px = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 py = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 redo = 1;
        if (px != m_lastTilePx.m_x || py != m_lastTilePx.m_y) {
            if (IsDropReady(1)) {
                m_coordToggle = (m_coordToggle == 0);
                redo = 0;
            }
        }
        SnapToLastTile(1);
        if (redo) {
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId(s_codeD);
        }
        goto finalize;
    }
    {
        const char* prev = *g_typeColl.ScratchResolve(m_objAux->m_actKey);
        GruntScratchTeardown();
        eq = (strcmp(prev, s_codeM) == 0);
        if (eq) {
            m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, DEATH_NORMAL, -1);
            return 0;
        }
        goto finalize;
    }

idleReseed:
    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 0);
    {
        i32 z = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != z) {
            m_object->m_sortKey = z;
            m_object->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != NULL) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = NULL;
    }
    m_toyTime = 0;
    StopStruckSlotSound();
    goto finalize;

modeDispatch: {
    PickupType mode = m_entrancePickup;
    if (mode >= PICKUP_POWERUPZ_FIRST) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_entrancePickup = PICKUP_INVALID;
        m_helpCueId = 0;
        goto finalize;
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        m_brickPickupType = mode;
        m_entrancePickup = PICKUP_INVALID;
        goto finalize;
    }
    if (mode >= PICKUP_TOYZ_FIRST) {
        LoadVehicleGruntSprites(mode);
        goto finalize;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_entrancePickup = PICKUP_INVALID;
    goto finalize;
}

finalize:
    ConsiderArrival(1);
    if (m_healthSprite != NULL) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = NULL;
    }
    if (m_staminaSprite != NULL) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = NULL;
    }
    if (m_toySprite != NULL) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = NULL;
    }
    if (m_toyTimeSprite != NULL) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = NULL;
    }
    if (m_wingzTimeSprite != NULL) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = NULL;
    }
    if (m_poweredUp == 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    m_entranceActive = 1;
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_codeQ);
    {
        i32 z = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != z) {
            m_object->m_sortKey = z;
            m_object->m_flags |= 0x20000;
        }
    }
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_FREEZE, 0);
    {
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = desc->m_records.GetSize() > 0
                                   ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0))
                                   : 0;
        i32 frame = elem->m_param;
        m_wwdObject->ApplyLookupSprite(s_GRUNTZ_DEATHZ_FREEZE, frame);
    }
    m_freezeUnfrozen = 0;
    m_freezeDelayDone = 1;
    return 0;
}

// @early-stop
RVA(0x00069d60, 0x1e1)
i32 CGrunt::LoadFreezeSpellAssets() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    if (m_wwdObject->m_animCursor.m_finished != 0
        && m_wwdObject->m_animCursor.m_frameTicksLeft == 0) {
        if (m_freezeUnfrozen != 0) {
            m_entranceActive = 0;
            ReadConfigFromButeMgr();
            LoadCellAnimNames(0, 0);
            LoadAnimNameTable(0, 0);
            ResetEntranceAnimation(1, 0, 0);
            if (s_TileFlags(
                    g_gameReg->m_tileGrid,
                    m_lastTilePx.m_x >> TILE_SHIFT_PX,
                    m_lastTilePx.m_y >> TILE_SHIFT_PX
                )
                & 0x80) {
                m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            }
            return 0;
        }
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_SPARKLE, 0);
        m_idleDelay = static_cast<u32>(g_buteMgr.GetIntDef(s_Spellz, s_FreezeDelay, 0x2710));
        m_idleAnchor = static_cast<u32>(static_cast<i32>(g_frameTime));
        m_freezeDelayDone = 0;
    }
    if (m_freezeDelayDone == 0) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleAnchor >= m_idleDelay) {
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_UNFREEZE, 0);
            CWwdGameObjectA* h = m_object;
            i32 vx = h->m_screenX;
            i32 vy = h->m_screenY;
            const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
            if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x35c, -1, 0, -1, -1);
            }
            m_freezeUnfrozen = 1;
            m_freezeDelayDone = 1;
        }
    }
    return 0;
}

RVA(0x00069fd0, 0x69)
i32 CGrunt::FinishEntranceMove() {

    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* cur = &m_wwdObject->m_animCursor;
    if (cur->m_finished == 0 || cur->m_frameTicksLeft != 0) {
        return 0;
    }
    if (m_cellRemovalNotified == 0) {

        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
    }
    m_wwdObject->m_flags |= 0x10000;
    return 0;
}

RVA(0x0006a060, 0x520)
i32 CGrunt::LoadGruntMovingDeathConfig() {
    m_moveSpeed =
        16.0 / static_cast<double>(g_buteMgr.GetDwordDef(s_Grunt, s_MovingDeathTime, 0x3e8));

    CGruntzMgr* g = g_gameReg;
    CState* state = g->m_curState;
    CGruntzMapMgr* b = g->m_tileGrid;
    CWwdGameObjectA* h = m_object;
    i32 xbound = b->m_width;
    i32 tileY = h->m_screenY >> TILE_SHIFT_PX;
    i32 tileX = h->m_screenX >> TILE_SHIFT_PX;
    // NOT a direction and NOT a TileCollisionKind: BrickzCell int 3 is m_tileId,
    // the raw WWD tile-image index. That is why the same numbers are dispatched
    // twice below - CState::m_levelType is the AREA number (levelIndex / 4 + 1,
    // "AREA%i"), so areas 1-4 and 5-8 ship different tilesets and the id space is
    // per-tileset with no engine-side names. The DIRECTION lives in the arms: each
    // shoreline image nudges the dying grunt half a tile toward its own edge. The
    // area-1-4 half agrees value-for-value with CRollingBall::Update's sink table.
    i32 tileId;
    if (static_cast<u32>(tileX) >= static_cast<u32>(xbound)
        || static_cast<u32>(tileY) >= static_cast<u32>(b->m_height)) {
        tileId = 0;
    } else {
        tileId = b->m_rowInts[tileY][tileX * 7 + 3];
    }

    i32 area = state->m_levelType;

#define MV_VEC(V) m_entranceCell = g_gruntDir##V
#define MV_N                                                                                       \
    MV_VEC(North);                                                                                 \
    m_lastTilePx.m_y -= 0x10
#define MV_S                                                                                       \
    MV_VEC(South);                                                                                 \
    m_lastTilePx.m_y += 0x10
#define MV_E                                                                                       \
    MV_VEC(East);                                                                                  \
    m_lastTilePx.m_x += 0x10
#define MV_W                                                                                       \
    MV_VEC(West);                                                                                  \
    m_lastTilePx.m_x -= 0x10
#define MV_NE                                                                                      \
    MV_VEC(NorthEast);                                                                             \
    m_lastTilePx.m_x += 0x10;                                                                      \
    m_lastTilePx.m_y -= 0x10
#define MV_NW                                                                                      \
    MV_VEC(NorthWest);                                                                             \
    m_lastTilePx.m_x -= 0x10;                                                                      \
    m_lastTilePx.m_y -= 0x10
#define MV_SE                                                                                      \
    MV_VEC(SouthEast);                                                                             \
    m_lastTilePx.m_x += 0x10;                                                                      \
    m_lastTilePx.m_y += 0x10
#define MV_SW                                                                                      \
    MV_VEC(SouthWest);                                                                             \
    m_lastTilePx.m_x -= 0x10;                                                                      \
    m_lastTilePx.m_y += 0x10

    if (area < AREA_TILESET_B_FIRST) {
        switch (tileId) {
            case 0x69:
            case 0x6a:
                MV_S;
                break;
            case 0x6b:
            case 0x70:
            case 0x71:
                MV_SW;
                break;
            case 0x78:
            case 0x80:
                MV_W;
                break;
            case 0x86:
            case 0x87:
            case 0x8b:
                MV_NW;
                break;
            case 0x89:
            case 0x8a:
                MV_N;
                break;
            case 0x82:
            case 0x83:
            case 0x88:
                MV_NE;
                break;
            case 0x73:
            case 0x7b:
                MV_E;
                break;
            case 0x68:
            case 0x6c:
            case 0x6d:
                MV_SE;
                break;
            default:
                return 0;
        }
    } else {
        switch (tileId) {
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
                MV_N;
                break;
            case 0x79:
            case 0x7f:
            case 0x80:
            case 0x81:
            case 0x85:
                MV_NE;
                break;
            case 0x6f:
            case 0x70:
            case 0x77:
            case 0x78:
                MV_E;
                break;
            case 0x63:
            case 0x64:
            case 0x69:
            case 0x6a:
            case 0x6b:
            case 0x71:
                MV_SE;
                break;
            case 0x65:
            case 0x66:
                MV_S;
                break;
            case 0x67:
            case 0x68:
            case 0x6c:
            case 0x6d:
            case 0x6e:
            case 0x74:
                MV_SW;
                break;
            case 0x75:
            case 0x76:
            case 0x7d:
            case 0x7e:
                MV_W;
                break;
            case 0x7c:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x8a:
                MV_NW;
                break;
            default:
                return 0;
        }
    }
#undef MV_VEC
#undef MV_N
#undef MV_S
#undef MV_E
#undef MV_W
#undef MV_NE
#undef MV_NW
#undef MV_SE
#undef MV_SW

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_animKeyS);
    return 1;
}

// @early-stop
// Same 144 blocks as retail; cl hoists the shared `idleReseed` goto target to
// ~35% of the function where retail leaves it next to the epilogue
// (docs/patterns/forward-goto-hoists-target-block.md).
RVA(0x0006a6d0, 0x936)
i32 CGrunt::FinishActiveAction() {
    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "A") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeD) == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_cueSink->StopVoice(m_object->m_objectId);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            -1
        );
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "G") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "L") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "P") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeO) == 0);
    if (eq) {
        SnapToLastTile(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_y, m_lastTilePx.m_x);
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "J") == 0);
    if (eq) {
        m_entranceActive = 0;
        eq = (strcmp(*g_typeColl.GetNameRecord(m_prevAnimSetNode), s_codeD) == 0);
        if (eq) {
            if (m_poweredUp != 0 && m_neighborValid == 0) {
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
            m_tileMoveCommitted = 0;
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId(s_codeD);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_poseWalk);

            GruntDirectionCell cell = m_entranceCell;
            i32 col = cell.column + cell.row * 2;
            i32 base = cell.row + col;
            char* nm = m_cells[base].WalkName().GetBuffer(0);
            m_wwdObject->ApplyName(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeN) == 0);
    if (eq) {
        i32 px = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 py = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 redo = 1;
        if ((px != m_lastTilePx.m_x || py != m_lastTilePx.m_y) && IsDropReady(1)) {
            m_coordToggle = (m_coordToggle == 0);
            redo = 0;
        }
        SnapToLastTile(1);
        if (redo) {
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId(s_codeD);
            SetupTubeAnim(m_coordToggle);
        }
        return 1;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeK) == 0);
    if (!eq || m_entranceArmed == 0) {
        return 0;
    }

    {
        CGruntzMgr* game = g_gameReg;
        CWwdGameObjectA* object = m_object;
        CMapMgr* grid = game->m_tileGrid;
        i32 tx = object->m_screenX >> TILE_SHIFT_PX;
        i32 ty = object->m_screenY >> TILE_SHIFT_PX;
        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
            || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            flags = 1;
        } else {
            flags = grid->m_rowInts[ty][tx * 7];
        }

        if (flags & 0x20000000) {
            i32 owner;
            if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
                || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
                owner = -1;
            } else {
                owner = grid->m_rowInts[ty][tx * 7 + 1];
            }
            i32 ownerHi = (owner >> 8) & 0xff;
            i32 ownerLo = owner & 0xff;
            if (m_tileOwnerHi != ownerHi || m_tileOwnerLo != ownerLo) {
                m_tileMgr->CellDispatch(ownerHi, ownerLo, DEATH_SQUASH, m_tileOwnerHi);
            }
        }

        i32 oldX = m_lastTilePx.m_x;
        m_entranceArmed = 0;
        i32 newX = object->m_screenX;
        i32 newY = object->m_screenY;
        i32 oldTx = oldX >> TILE_SHIFT_PX;
        i32 oldTy = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 newTx = newX >> TILE_SHIFT_PX;
        i32 newTy = newY >> TILE_SHIFT_PX;
        if (oldX != -1 && m_lastTilePx.m_y != -1) {
            CMapMgr* oldGrid = game->m_tileGrid;
            oldGrid->m_rows[oldTy][oldTx].m_flagBytes[3] &= ~0x20;
            oldGrid->m_rowInts[oldTy][oldTx * 7 + 1] = -1;
        }
        CMapMgr* newGrid = game->m_tileGrid;
        newGrid->m_rows[newTy][newTx].m_flagBytes[3] |= 0x20;
        newGrid->m_rowInts[newTy][newTx * 7 + 1] = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        m_lastTilePx.m_x = newX;
        m_lastTilePx.m_y = newY;
        m_tileMgr->WireTileSwitchLogic(this, newX, newY);

        m_entranceCommitted = 1;
        i32 sortKey = object->m_screenY + 0x186a0;
        if (object->m_sortKey != sortKey) {
            object->m_sortKey = sortKey;
            object->m_flags |= 0x20000;
        }

        CAniElement* found = 0;
        CAniElement* cached = m_wwdObject->m_animCursor.m_animation;
        MapLookup(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_GRUNTZ_ENTRANCEZ_DROP,
            found
        );
        if (found == cached) {
            if (m_tileOwnerHi == g_curPlayer) {
                game->m_cueSink->SpawnVoiceDriver(this, 0x33f, -1, 0, -1, -1);
                m_tileMgr->ResetCell(m_tileOwnerHi, m_tileOwnerLo, 0, 0);
            }
            m_entranceDropActive = 1;
            m_entranceSafeTimeLo = g_buteMgr.GetDwordDef(s_Grunt, s_EntranceSafeTime, 5000);
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = g_frameTime;
            m_entranceClockHi = 0;
            m_flashWindowLo = 0;
            m_flashWindowHi = 0;
        } else if (m_tileMgr->RecordListHas(m_tileOwnerHi, m_tileOwnerLo)) {
            CommitArrival();
        }

        m_entranceActive = 0;
        ReadConfigFromButeMgr();
        LoadCellAnimNames(0, 0);
        LoadAnimNameTable(0, 0);
        return 1;
    }

idleReseed:
    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        i32 sortKey = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != sortKey) {
            m_object->m_sortKey = sortKey;
            m_object->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != NULL) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = NULL;
    }
    m_toyTime = 0;
    StopStruckSlotSound();
    return 1;

modeDispatch: {
    PickupType mode = m_entrancePickup;
    if (mode >= PICKUP_POWERUPZ_FIRST) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_entrancePickup = PICKUP_INVALID;
        m_helpCueId = 0;
        return 1;
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        m_brickPickupType = mode;
        m_entrancePickup = PICKUP_INVALID;
        return 1;
    }
    if (mode >= PICKUP_TOYZ_FIRST) {
        LoadVehicleGruntSprites(mode);
        return 1;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_entrancePickup = PICKUP_INVALID;
    return 1;
}
}

RVA(0x0006b260, 0x5)
i32 CGrunt::StepAttackAction() {

    return StepAttackFire();
}

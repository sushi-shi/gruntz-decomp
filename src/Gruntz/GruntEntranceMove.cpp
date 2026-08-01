

#include <Bute/ButeTree.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <DDrawMgr/AniAdvance.h>
#include <rva.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GruntEntranceMove.h>
#include <Utils/MapTyped.h>

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
static char s_Spellz[] = "Spellz";
static char s_FreezeDelay[] = "FreezeDelay";

static char s_BOMBGRUNT[] = "BOMBGRUNT";
static char s_RunningTimePerTile[] = "RunningTimePerTile";

static const char s_animKeyA[] = "A";
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

static const char s_exitKeyB[] = "B";
static const char s_GRUNTZ_EXITZ[] = "GRUNTZ_EXITZ";
static const char s_GRUNTZ_EXITZ_ONE[] = "GRUNTZ_EXITZ_ONE";
static const char s_GRUNTZ_EXITZ_TWO[] = "GRUNTZ_EXITZ_TWO";
static const char s_GRUNTZ_EXITZ_THREE[] = "GRUNTZ_EXITZ_THREE";

static const char s_GRUNTZ_GOKARTGRUNT[] = "GRUNTZ_GOKARTGRUNT_GOKARTGRUNTLOOP";
static const char s_GRUNTZ_BIGWHEELGRUNT[] = "GRUNTZ_BIGWHEELGRUNT_BIGWHEELGRUNTLOOP";
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
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
}

// @early-stop
RVA(0x00067850, 0x214)
i32 CGrunt::RunEntranceMove() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));

    CAniAdvanceCursor* cur = &m_38->m_1a0;
    if (!((cur->m_finished != 0 && cur->m_frameTicksLeft == 0) || m_moveMode == 0)) {
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
        m_35c = 0;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId(s_codeD);
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseWalk);
        GruntDirectionCell cell = m_entranceCell;
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        char* nm = m_cells[base].WalkName().GetBuffer(0);
        m_38->ApplyName(nm);
    } else {
        ResetEntranceAnimation(1, 0, 0);
    }

    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    i32 mode = m_moveMode;
    if (mode == -1) {
        return 0;
    }
    if (mode >= 0x32) {
        return LoadVehicleGruntSprites(mode);
    }
    if (mode >= 0x22) {
        m_194 = mode;
        m_moveMode = -1;
        return 1;
    }
    if (mode >= 0x17) {
        LoadVehicleGruntSprites(mode);
        return 0;
    }
    return LoadTypeTableClearMove(mode);
}

// @early-stop
RVA(0x00067b00, 0x92)
i32 CGrunt::GruntInRadius(i32 col, i32 row) {
    CGrunt* other = m_tileMgr->m_grid[col * TM_GRID_COLS + row];
    if (other != 0 && other->m_entranceCommitted != 0 && other->m_gruntKind != 0x36) {
        i32 ox = other->m_lastTilePxX >> 5;
        i32 oy = other->m_lastTilePxY >> 5;
        i32 tx = m_defenderX >> 5;
        i32 ty = m_defenderY >> 5;
        i32 dx = oy - ty;
        i32 dy = ox - tx;
        i32 sum = m_defenderRadius + m_reachRadius;
        i32 dist2 = abs(dx * dx + dy * dy);
        return dist2 < sum * sum ? 1 : 0;
    }
    return 0;
}

RVA(0x00067bd0, 0x2ef)
void CGrunt::BuildEntranceAnimation(i32 mode) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_animKeyK);

    m_entranceArmed = 1;
    m_entranceCommitted = 0;
    m_entranceActive = 1;
    if (m_object->m_sortKey != 0xcf850) {
        m_object->m_sortKey = 0xcf850;
        m_object->m_flags |= 0x20000;
    }

    ClearAllSprites();

    CString key;

    i32 onScreen = 0;
    CGruntzMgr* g = g_gameReg;
    {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            onScreen = 1;
        } else {

            CGrunt* focus = 0;
            CTriggerMgr* tm = g->m_cmdGrid;
            if (tm->m_recList.GetCount() == 1) {
                i32* vec = static_cast<i32*>(tm->m_recList.GetHead());
                i32 a = vec[0];
                i32 b = vec[1];
                focus = tm->m_grid[a * TM_GRID_COLS + b];
            }
            if (this == focus && m_tileOwnerHi == g_curPlayer) {
                onScreen = 1;
            }
        }
    }

    CAniElement* found = 0;
    const char* base;

    if (mode == 1) {
        i32 r = GruntRand() % 0x1e1;
        if (r > 0x140) {
            MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_ONE, found);
            if (onScreen) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x37a, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else if (r > 0xa0) {
            MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_TWO, found);
            if (onScreen) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x37b, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else {
            MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_THREE, found);
            if (onScreen) {
                g->m_cueSink->SpawnVoiceDriver(this, 0x37c, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        }
    } else if (mode == 2) {
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_DROP, found);
        base = s_GRUNTZ_ENTRANCEZ_DROP;
    } else {
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_RESSURECT, found);
        base = s_GRUNTZ_DEATHZ_MELT;
    }

    key = base;

    if (!found) {
        ResetEntranceAnimation(1, 0, 0);
    } else {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(found);
        CAniElement* desc = m_38->m_1a0.m_14;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        m_38->ApplyLookupSprite(key, elem->m_param);
    }
}

RVA(0x00067f80, 0x313)
i32 CGrunt::LoadEntranceConfig() {
    if (m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta)) == 1) {
        CGruntzMgr* g = g_gameReg;
        CWwdGameObjectA* h = m_object;
        CMapMgr* grid = g->m_tileGrid;
        i32 tx = h->m_screenX >> 5;
        i32 ty = h->m_screenY >> 5;

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
                m_tileMgr->CellDispatch(b, a, 2, m_tileOwnerHi);
            }
        }

        h = m_object;
        i32 oldX = m_lastTilePxX;
        m_entranceArmed = 0;
        i32 newPxX = h->m_screenX;
        i32 newPxY = h->m_screenY;
        i32 oldTileX = oldX >> 5;
        i32 oldTileY = m_lastTilePxY >> 5;
        i32 newTileX = newPxX >> 5;
        i32 newTileY = newPxY >> 5;

        if (oldX != -1 && m_lastTilePxY != -1) {
            CMapMgr* og = g_gameReg->m_tileGrid;

            og->m_rows[oldTileY][oldTileX].m_flagBytes[3] &= ~0x20;
            og->m_rowInts[oldTileY][oldTileX * 7 + 1] = -1;
        }
        {
            CMapMgr* ng = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);

            ng->m_rows[newTileY][newTileX].m_flagBytes[3] |= 0x20;
            ng->m_rowInts[newTileY][newTileX * 7 + 1] = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        }
        m_lastTilePxX = newPxX;
        m_lastTilePxY = newPxY;
        m_tileMgr->WireTileSwitchLogic(this, newPxX, newPxY);

        h = m_object;
        m_entranceCommitted = 1;
        if (h->m_sortKey != h->m_screenY + 0x186a0) {
            h->m_sortKey = h->m_screenY + 0x186a0;
            h->m_flags |= 0x20000;
        }

        CWwdGameObjectA* p = m_38;
        CAniElement* found = 0;
        CAniElement* cached = p->m_1a0.m_14;
        MapLookup(p->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_DROP, found);
        if (found == cached) {
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

    if (m_38->m_1a0.m_finished == 0 || m_38->m_1a0.m_frameTicksLeft != 0) {
        return 0;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 0;
}

// @early-stop
RVA(0x00068370, 0x14c)
i32 CGrunt::RearmEntranceDrop() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_38->m_1a0.m_finished != 0 && m_38->m_1a0.m_frameTicksLeft == 0) {
        m_22c = 0;
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseItem[GRUNT_ITEM2]);

        CAniElement* desc = m_38->m_1a0.m_14;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem->m_param;

        GruntDirectionCell cell = m_entranceCell;
        i32 row = cell.row;
        i32 column = cell.column;

        const char* name = m_cells[3 * row + column].ItemName().GetBuffer(0);
        m_38->ApplyLookupSprite(name, frame);
    }

    if (m_22c == 0) {
        i32 a;
        i32 b;
        m_entranceCommitted = 0;
        if (m_tileMgr->HitTestCell(m_object->m_screenX, m_object->m_screenY, &a, &b, 0) != 0) {
            m_tileMgr->CellDispatch(a, b, 0xb, -1);
            m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        } else {
            m_entranceCommitted = 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00068520, 0x2a2)
i32 CGrunt::StartBombGruntRun() {
    StepAnimDispatchB();
    if (m_healthSprite != 0) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite != 0) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite != 0) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite != 0) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite != 0) {
        m_powerupSprite->m_flags |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite != 0) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = 0;
    }
    m_gruntKind = 0;
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
    if (LoadGruntTypeTable(1, 1, 0, 1) == 0) {
        CWwdGameObjectA* h = m_object;
        m_tileMgr->LoadExplosionSprites(h->m_screenX, h->m_screenY, -1, 0);
        return 0;
    }
    i32 dx = GruntRand() % 3 - 1;
    i32 dy = GruntRand() % 3 - 1;
    if (dx == 0 && dy == 0) {
        dx = 1;
    }
    {
        CWwdGameObjectA* h = m_object;
        dy += h->m_screenY >> 5;
        dx += h->m_screenX >> 5;
    }
    PlayMoveSoundAtTile(dx, dy);
    m_moveTileX = dx;
    m_moveTileY = dy;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_codeM);
    m_timePerTile = g_buteMgr.GetIntDef(s_BOMBGRUNT, s_RunningTimePerTile, 0x64);
    m_22c = 1;
    {
        CWwdGameObjectA* h = m_object;
        i32 vx = h->m_screenX;
        i32 vy = h->m_screenY;
        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
        if (vx < rect->right && vx >= rect->left && vy < rect->bottom && vy >= rect->top) {
            g_gameReg->m_cueSink->LoadGruntSpawnConfig(this, 8, -1, -1, -1);
        }
    }
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_poseItem[GRUNT_ITEM1]);
    GruntDirectionCell cell = m_entranceCell;
    i32 col = cell.column + cell.row * 2;
    i32 base = cell.row + col;
    char* cn = m_cells[base].ItemName().GetBuffer(0);
    m_38->ApplyName(cn);
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

        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_ITEM, _out);
        m_poseWalk = _out;
        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_ITEM, _out);
        m_poseIdle[GRUNT_IDLE3] = 0;
        m_poseIdle[GRUNT_IDLE1] = _out;
        m_poseIdle[GRUNT_IDLE2] = _out;
        m_poseIdle[GRUNT_IDLE4] = 0;
        m_poseIdle[GRUNT_IDLE5] = 0;

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
        if (m_wingzTimeSprite != 0) {
            m_wingzTimeSprite->m_flags |= 0x10000;
            m_wingzTimeSprite = 0;
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

        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_WALK, _out);
        m_poseWalk = _out;
        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_IDLE1, _out);
        m_poseIdle[GRUNT_IDLE1] = _out;
        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_IDLE2, _out);
        m_poseIdle[GRUNT_IDLE2] = _out;
        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_IDLE3, _out);
        m_poseIdle[GRUNT_IDLE3] = _out;
        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_IDLE4, _out);
        m_poseIdle[GRUNT_IDLE4] = _out;
        _out = 0;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_WG_IDLE5, _out);
        m_poseIdle[GRUNT_IDLE5] = _out;
    }

    CString* rec = g_typeColl.ScratchResolve(m_objAux->m_1c);
    GruntScratchTeardown();
    if (strcmp(*rec, s_codeD) == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseWalk);
        CAniElement* desc = m_38->m_1a0.m_14;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem->m_param;
        i32 idx = 3 * m_entranceCell.row + m_entranceCell.column;
        char* buf = m_cells[idx].WalkName().GetBuffer(0);
        m_38->ApplyLookupSprite(buf, frame);
        return 1;
    }

    CString* rec2 = g_typeColl.ScratchResolve(m_objAux->m_1c);
    GruntScratchTeardown();
    if (strcmp(*rec2, "A") == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseIdle[GRUNT_IDLE1]);
        CAniElement* desc = m_38->m_1a0.m_14;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem->m_param;
        i32 idx = 3 * m_entranceCell.row + m_entranceCell.column;
        char* buf = m_cells[idx].IdleName().GetBuffer(0);
        m_38->ApplyLookupSprite(buf, frame);
    }
    return 1;
}

// @early-stop
RVA(0x000690a0, 0x1c5)
i32 CGrunt::UpdateEntranceAnim() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    if (m_38->m_1a0.m_finished == 0 || m_38->m_1a0.m_frameTicksLeft != 0) {
        return 0;
    }

    if (m_entranceStamped == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup(m_poseToy[GRUNT_TOY_BREAK]);

        CAniElement* desc = m_38->m_1a0.m_14;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem->m_param;

        char* buf = (&m_448)->GetBuffer(0);
        m_38->ApplyLookupSprite(buf, frame);

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

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    LoadGruntTypeTable(m_toolId, 1, 0, 0);
    m_entranceActive = 0;

    CGruntzMgr* g = g_gameReg;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
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
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
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

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") != 0);
    if (!eq) {
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) != 0);
    if (!eq) {
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_cueSink->StopVoice(m_object->m_188);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
        if (m_entranceReason != 1) {
            goto finalize;
        }
        m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "G") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "L") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "P") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeO) == 0);
    if (eq) {
        SnapToLastTile(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "J") == 0);
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
            m_35c = 0;
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup(m_poseWalk);
            GruntDirectionCell cell = m_entranceCell;
            i32 colv = cell.column + cell.row * 2;
            i32 base = cell.row + colv;
            char* nm = m_cells[base].WalkName().GetBuffer(0);
            m_38->ApplyName(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeN) == 0);
    if (eq) {
        i32 px = (m_object->m_screenX & ~0x1f) + 0x10;
        i32 py = (m_object->m_screenY & ~0x1f) + 0x10;
        i32 redo = 1;
        if (px != m_lastTilePxX || py != m_lastTilePxY) {
            if (IsDropReady(1)) {
                m_coordToggle = (m_coordToggle == 0);
                redo = 0;
            }
        }
        SnapToLastTile(1);
        if (redo) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
        }
        goto finalize;
    }
    {
        const char* prev = *g_typeColl.ScratchResolve(m_objAux->m_1c);
        GruntScratchTeardown();
        eq = (strcmp(prev, s_codeM) == 0);
        if (eq) {
            m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 1, -1);
            return 0;
        }
        goto finalize;
    }

idleReseed:
    if (m_entranceReason == 0x1e) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_188);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 0);
    {
        i32 z = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != z) {
            m_object->m_sortKey = z;
            m_object->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    ClearSubA();
    goto finalize;

modeDispatch: {
    i32 mode = m_moveMode;
    if (mode >= 0x32) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_moveMode = -1;
        m_1a4 = 0;
        goto finalize;
    }
    if (mode >= 0x22) {
        m_194 = mode;
        m_moveMode = -1;
        goto finalize;
    }
    if (mode >= 0x17) {
        LoadVehicleGruntSprites(mode);
        goto finalize;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_moveMode = -1;
    goto finalize;
}

finalize:
    ConsiderArrival(1);
    if (m_healthSprite != 0) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite != 0) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite != 0) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite != 0) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
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
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_codeQ);
    {
        i32 z = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != z) {
            m_object->m_sortKey = z;
            m_object->m_flags |= 0x20000;
        }
    }
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_FREEZE, 0);
    {
        CAniElement* desc = m_38->m_1a0.m_14;
        CAniDesc* elem =
            desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem->m_param;
        m_38->ApplyLookupSprite(s_GRUNTZ_DEATHZ_FREEZE, frame);
    }
    m_freezeUnfrozen = 0;
    m_freezeDelayDone = 1;
    return 0;
}

// @early-stop
RVA(0x00069d60, 0x1e1)
i32 CGrunt::LoadFreezeSpellAssets() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    if (m_38->m_1a0.m_finished != 0 && m_38->m_1a0.m_frameTicksLeft == 0) {
        if (m_freezeUnfrozen != 0) {
            m_entranceActive = 0;
            ReadConfigFromButeMgr();
            LoadCellAnimNames(0, 0);
            LoadAnimNameTable(0, 0);
            ResetEntranceAnimation(1, 0, 0);
            if (s_TileFlags(g_gameReg->m_tileGrid, m_lastTilePxX >> 5, m_lastTilePxY >> 5) & 0x80) {
                m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxX, m_lastTilePxY);
            }
            return 0;
        }
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_SPARKLE, 0);
        m_idleDelay = static_cast<u32>(g_buteMgr.GetIntDef(s_Spellz, s_FreezeDelay, 0x2710));
        m_idleAnchor = static_cast<u32>(static_cast<i32>(g_frameTime));
        m_freezeDelayDone = 0;
    }
    if (m_freezeDelayDone == 0) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleAnchor >= m_idleDelay) {
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyLookupGeometry(s_GRUNTZ_DEATHZ_UNFREEZE, 0);
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

    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* cur = &m_38->m_1a0;
    if (cur->m_finished == 0 || cur->m_frameTicksLeft != 0) {
        return 0;
    }
    if (m_36c == 0) {

        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
    }
    m_38->m_flags |= 0x10000;
    return 0;
}

RVA(0x0006a060, 0x43d)
i32 CGrunt::LoadGruntMovingDeathConfig() {
    m_moveSpeed =
        16.0 / static_cast<double>(g_buteMgr.GetDwordDef(s_Grunt, s_MovingDeathTime, 0x3e8));

    CGruntzMgr* g = g_gameReg;
    CState* state = g->m_curState;
    CGruntzMapMgr* b = g->m_tileGrid;
    CWwdGameObjectA* h = m_object;
    i32 xbound = b->m_width;
    i32 tileY = h->m_screenY >> 5;
    i32 tileX = h->m_screenX >> 5;
    i32 dir;
    if (static_cast<u32>(tileX) >= static_cast<u32>(xbound)
        || static_cast<u32>(tileY) >= static_cast<u32>(b->m_height)) {
        dir = 0;
    } else {
        dir = b->m_rowInts[tileY][tileX * 7 + 3];
    }

    i32 sel = state->m_levelType;

#define MV_VEC(V) m_entranceCell = g_gruntDir##V
#define MV_N                                                                                       \
    MV_VEC(North);                                                                                 \
    m_lastTilePxY -= 0x10
#define MV_S                                                                                       \
    MV_VEC(South);                                                                                 \
    m_lastTilePxY += 0x10
#define MV_E                                                                                       \
    MV_VEC(East);                                                                                  \
    m_lastTilePxX += 0x10
#define MV_W                                                                                       \
    MV_VEC(West);                                                                                  \
    m_lastTilePxX -= 0x10
#define MV_NE                                                                                      \
    MV_VEC(NorthEast);                                                                             \
    m_lastTilePxX += 0x10;                                                                         \
    m_lastTilePxY -= 0x10
#define MV_NW                                                                                      \
    MV_VEC(NorthWest);                                                                             \
    m_lastTilePxX -= 0x10;                                                                         \
    m_lastTilePxY -= 0x10
#define MV_SE                                                                                      \
    MV_VEC(SouthEast);                                                                             \
    m_lastTilePxX += 0x10;                                                                         \
    m_lastTilePxY += 0x10
#define MV_SW                                                                                      \
    MV_VEC(SouthWest);                                                                             \
    m_lastTilePxX -= 0x10;                                                                         \
    m_lastTilePxY += 0x10

    if (sel < 5) {
        switch (dir) {
            case 0x69:
            case 0x6a:
                MV_S;
                break;
            case 0x6b:
                MV_SW;
                break;
            case 0x78:
                MV_W;
                break;
            case 0x86:
            case 0x87:
                MV_NW;
                break;
            case 0x89:
            case 0x8a:
                MV_N;
                break;
            case 0x82:
            case 0x83:
                MV_NE;
                break;
            case 0x73:
                MV_E;
                break;
            case 0x68:
                MV_SE;
                break;
            case 0x6c:
            case 0x6d:
                MV_SE;
                break;
            case 0x70:
            case 0x71:
                MV_SW;
                break;
            case 0x7b:
                MV_E;
                break;
            case 0x80:
                MV_W;
                break;
            case 0x88:
                MV_NE;
                break;
            case 0x8b:
                MV_NW;
                break;
            default:
                return 0;
        }
    } else {
        switch (dir) {
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
                MV_N;
                break;
            case 0x79:
                MV_NE;
                break;
            case 0x6f:
            case 0x70:
                MV_E;
                break;
            case 0x63:
            case 0x64:
                MV_SE;
                break;
            case 0x65:
            case 0x66:
                MV_S;
                break;
            case 0x67:
            case 0x68:
                MV_SW;
                break;
            case 0x75:
            case 0x76:
                MV_W;
                break;
            case 0x7c:
                MV_NW;
                break;
            case 0x69:
            case 0x6a:
            case 0x6b:
                MV_SE;
                break;
            case 0x6c:
            case 0x6d:
            case 0x6e:
                MV_SW;
                break;
            case 0x71:
                MV_SE;
                break;
            case 0x74:
                MV_SW;
                break;
            case 0x77:
            case 0x78:
                MV_E;
                break;
            case 0x7d:
            case 0x7e:
                MV_W;
                break;
            case 0x7f:
            case 0x80:
            case 0x81:
                MV_NE;
                break;
            case 0x82:
            case 0x83:
            case 0x84:
                MV_NW;
                break;
            case 0x85:
                MV_NE;
                break;
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

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_animKeyS);
    return 1;
}

RVA(0x0006a6d0, 0x936)
i32 CGrunt::StepAnimDispatchB() {
    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_cueSink->StopVoice(m_object->m_188);
        }
        m_tileMgr->LoadTileArrivalFx(
            m_tileOwnerHi,
            m_tileOwnerLo,
            m_moveTileX,
            m_moveTileY,
            m_entranceReason,
            -1
        );
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "G") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "L") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "P") == 0);
    if (eq) {
        goto idleReseed;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeO) == 0);
    if (eq) {
        SnapToLastTile(1);
        m_tileMgr->WireTileSwitchLogic(this, m_lastTilePxY, m_lastTilePxX);
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "J") == 0);
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
            m_35c = 0;
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup(m_poseWalk);

            GruntDirectionCell cell = m_entranceCell;
            i32 col = cell.column + cell.row * 2;
            i32 base = cell.row + col;
            char* nm = m_cells[base].WalkName().GetBuffer(0);
            m_38->ApplyName(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeN) == 0);
    if (eq) {
        i32 px = (m_object->m_screenX & ~0x1f) + 0x10;
        i32 py = (m_object->m_screenY & ~0x1f) + 0x10;
        i32 redo = 1;
        if ((px != m_lastTilePxX || py != m_lastTilePxY) && IsDropReady(1)) {
            m_coordToggle = (m_coordToggle == 0);
            redo = 0;
        }
        SnapToLastTile(1);
        if (redo) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
            SetupTubeAnim(m_coordToggle);
        }
        return 1;
    }

    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeK) == 0);
    if (!eq || m_entranceArmed == 0) {
        return 0;
    }

    {
        CGruntzMgr* game = g_gameReg;
        CWwdGameObjectA* object = m_object;
        CMapMgr* grid = game->m_tileGrid;
        i32 tx = object->m_screenX >> 5;
        i32 ty = object->m_screenY >> 5;
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
                m_tileMgr->CellDispatch(ownerHi, ownerLo, 2, m_tileOwnerHi);
            }
        }

        i32 oldX = m_lastTilePxX;
        m_entranceArmed = 0;
        i32 newX = object->m_screenX;
        i32 newY = object->m_screenY;
        i32 oldTx = oldX >> 5;
        i32 oldTy = m_lastTilePxY >> 5;
        i32 newTx = newX >> 5;
        i32 newTy = newY >> 5;
        if (oldX != -1 && m_lastTilePxY != -1) {
            CMapMgr* oldGrid = game->m_tileGrid;
            oldGrid->m_rows[oldTy][oldTx].m_flagBytes[3] &= ~0x20;
            oldGrid->m_rowInts[oldTy][oldTx * 7 + 1] = -1;
        }
        CMapMgr* newGrid = game->m_tileGrid;
        newGrid->m_rows[newTy][newTx].m_flagBytes[3] |= 0x20;
        newGrid->m_rowInts[newTy][newTx * 7 + 1] = (m_tileOwnerHi << 8) | m_tileOwnerLo;
        m_lastTilePxX = newX;
        m_lastTilePxY = newY;
        m_tileMgr->WireTileSwitchLogic(this, newX, newY);

        m_entranceCommitted = 1;
        i32 sortKey = object->m_screenY + 0x186a0;
        if (object->m_sortKey != sortKey) {
            object->m_sortKey = sortKey;
            object->m_flags |= 0x20000;
        }

        CAniElement* found = 0;
        CAniElement* cached = m_38->m_1a0.m_14;
        MapLookup(m_38->OwnerMgr()->m_animRegistry->m_10, s_GRUNTZ_ENTRANCEZ_DROP, found);
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
    if (m_entranceReason == 0x1e) {
        g_gameReg->m_cueSink->StopVoice(m_object->m_188);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        i32 sortKey = m_object->m_screenY + 0x186a0;
        if (m_object->m_sortKey != sortKey) {
            m_object->m_sortKey = sortKey;
            m_object->m_flags |= 0x20000;
        }
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    m_toyTime = 0;
    ClearSubA();
    return 1;

modeDispatch: {
    i32 mode = m_moveMode;
    if (mode >= 0x32) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_moveMode = -1;
        m_1a4 = 0;
        return 1;
    }
    if (mode >= 0x22) {
        m_194 = mode;
        m_moveMode = -1;
        return 1;
    }
    if (mode >= 0x17) {
        LoadVehicleGruntSprites(mode);
        return 1;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_moveMode = -1;
    return 1;
}
}

RVA(0x0006b260, 0x5)
i32 CGrunt::DispatchVtbl24() {

    return StepAttackFire();
}

// GruntEntranceMove.cpp - the FIFTH original grunt TU (retail text
// 0x67850-0x6b265): the entrance-move / entrance-anim / wingz-freeze asset /
// arrival-commit / anim-dispatch-B family, carved out of the conflated
// Grunt.cpp (wave3-I grunt-region partition).
//
// original TU: filename unknown (@identity-TODO; named for the dominant
// entrance-move family). ONE-obj evidence:
//   * private .data extents in TU link order: BuildEntranceAnimation @0x67bd0's
//     cells (0x20e924-0x20e9ac), LoadWingzGruntSprites @0x68880's 31 cells
//     (0x20e9c8-0x20edf4), LoadFreezeSpellAssets @0x69d60's (0x20ee1c-0x20ee48),
//     LoadGruntMovingDeathConfig @0x6a060's (0x20ee64) - one contiguous band
//     after the gruntpickupload extent; the shared-private cell 0x20e944
//     ({0x67bd0, 0x67f80, 0x6a6d0}) binds the family.
//   * 1 EH site in the interval -> /GX (flags "eh").
// In-interval fold: LoadWingzGruntSprites @0x68880 (ex GruntAssetLoaders.cpp -
// its 31 private cells sit inside this TU's band).
#include <Bute/ButeTree.h> // CButeTree::Find - g_buteTree @0x6bf620
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <Gruntz/GruntzMapMgr.h> // the real +0x70 board class (ex GruntBoard view)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Gruntz/GameLevel.h>   // canonical CGameLevel/CLevelPlane (m_world->m_level visible rect)
#include <Gruntz/TypeKeyColl.h> // g_typeColl (folded CAnimNameResolver anim registry)
#include <Gruntz/ActReg.h>      // CLookupColl/CActReg::ResolveEntry
#include <Gruntz/AniElement.h>
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (0x15c360)
#include <Gruntz/TriggerMgr.h>       // CTriggerMgr::NotifyCell (0x79fb0) + CellDispatch (0x6bcb0)
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/BoundaryLowerMethodsViews.h>
#include <Gruntz/Effect6b.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Bute/ButeMgr.h>
#include <Globals.h>

DATA(0x001e9a48)
double g_wingzScale = 100.0; // 0x5e9a48
DATA(0x001e9a50)
double g_wingzBias = -0.5; // 0x5e9a50
static char s_TimePerTile[] = "TimePerTile";
static char s_Grunt[] = "Grunt";                               // s_Grunt_0060a9ec
static char s_EntranceSafeTime[] = "EntranceSafeTime";         // s_EntranceSafeTime_0060df98
static char s_IdleDelay[] = "IdleDelay";                       // s_IdleDelay_0060e1a0
static char s_PlayerDefenderRadius[] = "PlayerDefenderRadius"; // s_PlayerDefenderRadius_0060e1ac
static char s_CombatTimeout[] = "CombatTimeout";               // s_CombatTimeout_0060df84

static const char s_GRUNTZ_DEATHZ_FREEZE[] = "GRUNTZ_DEATHZ_FREEZE";

static const char s_GRUNTZ_DEATHZ_SPARKLE[] = "GRUNTZ_DEATHZ_SPARKLE";   // 0x60ee48
static const char s_GRUNTZ_DEATHZ_UNFREEZE[] = "GRUNTZ_DEATHZ_UNFREEZE"; // 0x60ee1c
static char s_Spellz[] = "Spellz";                                       // 0x60cca8
static char s_FreezeDelay[] = "FreezeDelay";                             // 0x60ee38

static char s_BOMBGRUNT[] = "BOMBGRUNT";                   // 0x60dbd0
static char s_RunningTimePerTile[] = "RunningTimePerTile"; // 0x60e264

static const char s_animKeyA[] = "A";
static const char s_animKeyK[] = "K";


static void GruntScratchTeardown();

static __inline i32 s_TileFlags(CGruntzMapMgr* b, i32 tx, i32 ty) {
    if (static_cast<u32>(tx) >= static_cast<u32>(b->m_width) || static_cast<u32>(ty) >= static_cast<u32>(b->m_height)) {
        return 1;
    }
    return (reinterpret_cast<i32*>(b->m_rowBytes[ty]))[tx * 7];
}

void CGrunt::ApplyMoveKind(i32 v) {} // thunk_0x3c29 (0x57100); external/reloc-masked

// ===========================================================================
// The 5 grunt movement / anim-name dispatch state machines (formerly the
// CUserLogic_* stubs @0x4b370 / 0x4c170 / 0x52fb0 / 0x5f310 / 0x6a6d0). Each
// resolves the grunt's current anim-set node name
// (g_typeColl.GetNameRecord(m_objAux->m_1c), or the scratch-teardown
// GetNameRecords form) and dispatches on its single-letter type code
// (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's movement/arrival state, recycling
// its occupied-coord nodes onto the shared freelist, and re-latching m_objAux->m_1c to
// a new anim set via g_entranceAnimSrc.LookupAnimSet. The inline-strcmp `== bool` setcc
// reject form is per docs/patterns/strcmp-eq-bool-local-setcc.md.
//
// These are the CGrunt analogues of CBattlezMapConfig::Method_025d90 /
// Method_02f620 (the documented large-state-machine + grid-regalloc walls). Each is
// reconstructed complete in shape/order; all carry @early-stop on those walls.
// Raw-offset member access (the campaign style used by the cluster above) keeps the
// giant ~0x46c layout tractable.

static void GruntScratchTeardown() {
    CAnimScratchString* slot = (reinterpret_cast<CAnimScratchString*>(g_typeColl.m_alloc));
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            (reinterpret_cast<CString*>(slot))->~CString();
        }
        slot++;
        cnt--;
    }
}

static const char s_NW_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_ITEM";
static const char s_N_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTH_ITEM";
static const char s_NE_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_ITEM";
static const char s_W_ITEM[] = "GRUNTZ_WINGZGRUNT_WEST_ITEM";
static const char s_E_ITEM[] = "GRUNTZ_WINGZGRUNT_EAST_ITEM";
static const char s_SW_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_ITEM";
static const char s_S_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTH_ITEM";
static const char s_SE_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_ITEM";
static const char s_NW_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_WALK";
static const char s_N_WALK[] = "GRUNTZ_WINGZGRUNT_NORTH_WALK";
static const char s_NE_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_WALK";
static const char s_W_WALK[] = "GRUNTZ_WINGZGRUNT_WEST_WALK";
static const char s_E_WALK[] = "GRUNTZ_WINGZGRUNT_EAST_WALK";
static const char s_SW_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_WALK";
static const char s_S_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTH_WALK";
static const char s_SE_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_WALK";
static const char s_NW_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_IDLE";
static const char s_N_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTH_IDLE";
static const char s_NE_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_IDLE";
static const char s_W_IDLE[] = "GRUNTZ_WINGZGRUNT_WEST_IDLE";
static const char s_E_IDLE[] = "GRUNTZ_WINGZGRUNT_EAST_IDLE";
static const char s_SW_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_IDLE";
static const char s_S_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTH_IDLE";
static const char s_SE_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_IDLE";
static const char s_WG_ITEM[] = "GRUNTZ_WINGZGRUNT_ITEM";
static const char s_WG_WALK[] = "GRUNTZ_WINGZGRUNT_WALK";
static const char s_WG_IDLE1[] = "GRUNTZ_WINGZGRUNT_IDLE1";
static const char s_WG_IDLE2[] = "GRUNTZ_WINGZGRUNT_IDLE2";
static const char s_WG_IDLE3[] = "GRUNTZ_WINGZGRUNT_IDLE3";
static const char s_WG_IDLE4[] = "GRUNTZ_WINGZGRUNT_IDLE4";
static const char s_WG_IDLE5[] = "GRUNTZ_WINGZGRUNT_IDLE5";

// ---------------------------------------------------------------------------
// CGrunt::RunEntranceMove()   @0x67850   (ret 0)
// @early-stop
// large-state-machine plateau: the armed-but-not-running sub-player gate, the
// scratch-resolver "D" re-latch (GetNameRecords + the scratch CString teardown),
// the on-arrival HUD-stat-sprite creation, the entrance-cell frame re-stamp, and the
// +0x1a0 move-mode dispatch are reconstructed in shape/order. Residue is the
// scratch loop-strength-reduction (shared, no source spelling), the short-circuit
// gate branch ordering, and the cross-arm regalloc. Deferred to the final sweep.
RVA(0x00067850, 0x214)
i32 CGrunt::RunEntranceMove() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    // The +0x1a0 cursor's done-gates (m_28 paused-done, m_20 per-frame timer).
    CAniAdvanceCursor* cur = &m_38->m_1a0;
    if (!((cur->m_28 != 0 && cur->m_20 == 0) || m_moveMode == 0)) {
        return 0;
    }

    m_entranceActive = 0;
    char* nm0 = g_typeColl.GetNameRecords(m_prevAnimSetNode)->m_name;
    GruntScratchTeardown();
    bool eq;
    eq = (strcmp(nm0, s_codeD) == 0);
    if (eq) {
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_35c = 0;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeD));
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(m_poseWalk);
        GruntEntranceCell cell = m_entranceCell;
        i32 col = cell.row + cell.col * 2;
        i32 base = cell.col + col;
        char* nm = m_cells[base].m_walk.GetBuffer(0);
        m_38->ApplyName(nm);
    } else {
        ReseedIdleReset(1, 0, 0);
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
        return ApplyMoveMode(mode);
    }
    if (mode >= 0x22) {
        m_194 = mode;
        m_moveMode = -1;
        return 1;
    }
    if (mode >= 0x17) {
        EmitMoveCueQ(mode);
        return 0;
    }
    return ApplyMoveMode(mode);
}

// ---------------------------------------------------------------------------
// CGrunt::GruntInRadius(col, row)  @0x67b00  (__thiscall, ret 8)
// Resolve the grunt occupying cell (col, row) via the tile-mgr's 15-wide cell grid
// (m_tileMgr + (15*col + row)*4 + 0x1c), gate it (live, entrance committed m_1fc, not
// state 0x36), then test whether the squared tile-distance from this grunt's HUD
// tile to it is within the (this->m_reachRadius + that->m_defenderRadius)^2 radius-sum threshold.
//
// Shared return-0 tail: the 3 gates collapse into one `&&` chain so each lowers to
// `test;je <tail>` against the single trailing `return 0;` (docs/patterns/
// homogeneous-predicate-chain-and-shared-tail.md) instead of inlining 3 epilogues -
// CFG + tail now byte-exact (35%->83%).
// @early-stop
// load-result register coin-flip: the resolved `other` lands in edx where retail
// reuses the dead index reg eax (`mov eax,[edx+eax*4+0x1c]`), cascading the edx/eax
// pairing through the m_17c/m_180 loads. Source-invariant on a leaf (reorder /
// grid-base-first / typed-grid all keep edx). Deferred to the final sweep.
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

static const char s_GRUNTZ_ENTRANCEZ[] = "GRUNTZ_ENTRANCEZ";
static const char s_GRUNTZ_ENTRANCEZ_ONE[] = "GRUNTZ_ENTRANCEZ_ONE";
static const char s_GRUNTZ_ENTRANCEZ_TWO[] = "GRUNTZ_ENTRANCEZ_TWO";
static const char s_GRUNTZ_ENTRANCEZ_THREE[] = "GRUNTZ_ENTRANCEZ_THREE";
static const char s_GRUNTZ_ENTRANCEZ_DROP[] = "GRUNTZ_ENTRANCEZ_DROP";
static const char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
static const char s_GRUNTZ_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";

static const char s_exitKeyB[] = "B";                            // 0x60d1bc
static const char s_GRUNTZ_EXITZ[] = "GRUNTZ_EXITZ";             // 0x60bd28
static const char s_GRUNTZ_EXITZ_ONE[] = "GRUNTZ_EXITZ_ONE";     // 0x60e250
static const char s_GRUNTZ_EXITZ_TWO[] = "GRUNTZ_EXITZ_TWO";     // 0x60e23c
static const char s_GRUNTZ_EXITZ_THREE[] = "GRUNTZ_EXITZ_THREE"; // 0x60e224

static const char s_GRUNTZ_GOKARTGRUNT[] = "GRUNTZ_GOKARTGRUNT_GOKARTGRUNTLOOP";       // 0x60e1f8
static const char s_GRUNTZ_BIGWHEELGRUNT[] = "GRUNTZ_BIGWHEELGRUNT_BIGWHEELGRUNTLOOP"; // 0x60e1c8

RVA(0x00067bd0, 0x2ef)
void CGrunt::BuildEntranceAnimation(i32 mode) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_animKeyK));

    m_entranceArmed = 1;
    m_entranceCommitted = 0;
    m_entranceActive = 1;
    if (m_object->m_sortKey != 0xcf850) {
        m_object->m_sortKey = 0xcf850;
        m_object->m_flags |= 0x20000;
    }

    EntrancePrepare(); // thunk_FUN_0044b240 (a void this-method)

    CString key;

    // The on-screen / focused-grunt gate: fire the cue when the grunt is inside
    // the visible view rect, or when it is the registry's focused grunt and its
    // m_tileOwnerHi matches the focus sentinel.
    i32 onScreen = 0;
    CGruntzMgr* g = g_gameReg;
    {
        i32 x = m_object->m_screenX;
        i32 y = m_object->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            onScreen = 1;
        } else {
            // The focused object IS a grunt (the identity test below is against
            // `this`, a CGrunt) - typed so; ex a CEntranceAnimPlayer* + downcast.
            CGrunt* focus = 0;
            i32* cell = reinterpret_cast<i32*>((reinterpret_cast<char*>(g) + 0x68));
            CGrunt** slot = reinterpret_cast<CGrunt**>((*cell));
            if ((reinterpret_cast<i32*>(slot))[0x24c / 4] == 1) {
                i32* idxObj = (reinterpret_cast<i32**>(slot))[0x244 / 4];
                i32* vec = reinterpret_cast<i32*>(idxObj[2]);
                i32 a = vec[0];
                i32 b = vec[1];
                i32 off = a * 15 + b;
                focus = ((slot))[off + 0x1c / 4];
            }
            if (this == focus && m_tileOwnerHi == g_curPlayer) {
                onScreen = 1;
            }
        }
    }

    CAniElement* found = 0; // the ANIM registry resolves elements (the image registry holds the sprites)
    const char* base;

    if (mode == 1) {
        i32 r = GruntRand() % 0x1e1;
        if (r > 0x140) {
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ENTRANCEZ_ONE, reinterpret_cast<void*&>(found));
            if (onScreen) {
                g->m_cueSink->CueA(this, 0x37a, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else if (r > 0xa0) {
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ENTRANCEZ_TWO, reinterpret_cast<void*&>(found));
            if (onScreen) {
                g->m_cueSink->CueA(this, 0x37b, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        } else {
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ENTRANCEZ_THREE, reinterpret_cast<void*&>(found));
            if (onScreen) {
                g->m_cueSink->CueA(this, 0x37c, -1, 0, -1, -1);
            }
            base = s_GRUNTZ_ENTRANCEZ;
        }
    } else if (mode == 2) {
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ENTRANCEZ_DROP, reinterpret_cast<void*&>(found));
        base = s_GRUNTZ_ENTRANCEZ_DROP;
    } else {
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ENTRANCEZ_RESSURECT, reinterpret_cast<void*&>(found));
        base = s_GRUNTZ_DEATHZ_MELT;
    }

    key = base;

    if (!found) {
        ResetEntranceAnimation(1, 0, 0);
    } else {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(found);
        CAniElement* desc = m_38->m_1a0.m_14;
        i32* elem = desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        EntranceApplyFrame(key, elem[0x14 / 4]);
    }
}

RVA(0x00067f80, 0x313)
void CGrunt::LoadEntranceConfig() {
    if (m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta)) == 1) {
        CGruntzMgr* g = g_gameReg;
        CWwdGameObjectA* h = m_object;
        CTileGrid* grid = g->m_tileGrid;
        i32 tx = h->m_screenX >> 5;
        i32 ty = h->m_screenY >> 5;

        i32 flags;
        if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
            flags = 1;
        } else {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        }

        if (flags & 0x20000000) {
            i32 owner;
            if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width) || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
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

        // Re-stamp the occupancy grid: clear old tile, set new tile.
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
            CTileGrid* og = g_gameReg->m_tileGrid; // implicit upcast (the one board class)
            (reinterpret_cast<char*>(&og->m_rowInts[oldTileY][oldTileX * 7]))[3] &= ~0x20;
            og->m_rowInts[oldTileY][oldTileX * 7 + 1] = -1;
        }
        {
            CTileGrid* ng = static_cast<CTileGrid*>(g_gameReg->m_tileGrid);
            (reinterpret_cast<char*>(&ng->m_rowInts[newTileY][newTileX * 7]))[3] |= 0x20;
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
        void* found_ob = 0;
        void* cached = p->m_1a0.m_14;
        p->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ENTRANCEZ_DROP, found_ob);
        CAniElement* found = static_cast<CAniElement*>(found_ob);
        if (static_cast<void*>(found) == cached) {
            if (m_tileOwnerHi == g_curPlayer) {
                g_gameReg->m_cueSink->CueA(this, 0x33f, -1, 0, -1, -1);
            }
            m_tileMgr->ResetCell(m_tileOwnerHi, m_tileOwnerLo, 0, 0);
            m_entranceDropActive = 1;
            m_entranceSafeTimeLo = g_buteMgr.GetDwordDef(s_Grunt, s_EntranceSafeTime, 5000);
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = g_frameTime;
            m_entranceClockHi = 0;
            m_858 = 0;
            m_85c = 0;
        } else {
            if (m_tileMgr->RecordListHas(m_tileOwnerHi, m_tileOwnerLo)) {
                EntranceOnReleased();
            }
        }
        m_entranceActive = 0;
        ReadConfigFromButeMgr();
        LoadCellAnimNames(0, 0);
        EntranceFinishWire(0, 0);
    }

    if (m_38->m_1a0.m_28 == 0 || m_38->m_1a0.m_20 != 0) {
        return;
    }
    ResetEntranceAnimation(1, 0, 0);
}

// ---------------------------------------------------------------------------
// CGrunt::RearmEntranceDrop() @0x68370 - re-arms the entrance "drop" geometry.
// Re-points the entrance player's geometry sub-player at the default source, and
// when the sub-player just became ready (m_1a0.m_28 set, m_1a0.m_20 clear) it
// re-inits geometry to the ITEM2 pose, re-applies the per-cell frame name, and
// arms the drop gate. Then, if the drop hasn't been latched (m_22c==0), it looks
// up the tile under the grunt's HUD point and either claims it (SetTile drop +
// owner) or marks the entrance committed (m_1fc=1). __thiscall, ret 0.
// @early-stop
// scheduling tail: logic/CFG/member-offsets/calls exact (same entrance cell-math
// `(3*col+row+0xb)*0x68`, SetGeoSourceR/SetGeometry/GetName/SetAnimFrame/LookupTile/
// SetTile all match). Residue = cl hoists the GetName(0) `push 0` + reuses the
// `m_154+0x1a0` address in a reg earlier than mine (same entropy-class scheduling
// as ResetGeometry @0x616e0) - no source lever flips it. ~88.5%.
RVA(0x00068370, 0x14c)
void CGrunt::RearmEntranceDrop() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
        m_22c = 0;
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(m_poseItem2);

        CAniElement* desc = m_38->m_1a0.m_14;
        i32* elem = desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem[0x14 / 4];

        i32 col = m_entranceCell.col;
        i32 row = m_entranceCell.row;
        // The retail call really is _zdvec::IndexToPtr(0) on the m_item CString slot
        // (the two classes share the head layout); the old (3col+row+0xb)*0x68 spelling
        // was the m_cells base folded into the index - array form proven byte-identical.
        const char* name = reinterpret_cast<const char*>(
            reinterpret_cast<_zdvec*>(&m_cells[3 * col + row].m_item)->IndexToPtr(0));
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
}

// ---------------------------------------------------------------------------
// CGrunt::StartBombGruntRun()   @0x68520   (__thiscall, ret 0)
// Begin the bomb-grunt run reaction: run the anim-dispatch step, retire all seven
// HUD stat sprites, clear the grunt-kind, and (when powered-up with no live
// neighbor) reset the entrance + idle state. Latch the entrance/struck state, apply
// the move-state; if the move-state driver declines (returns 0) just re-notify the
// move at the current HUD pos and return. Otherwise pick a random adjacent tile
// (rand%3-1 in each axis, forced non-zero), play the directional move sound, latch
// the resolved tile + the "M" run anim-set, load RunningTimePerTile, fire the
// on-screen spawn cue when in view, drive the _ITEM geometry, and re-stamp the
// entrance-cell frame name. Returns 0.
// @early-stop
// ~98.7%: FRAME NOW REPRODUCED. The dead m_entranceCell.reason spill (`sub esp,0xc`) IS
// a by-value 3-int struct copy (GruntEntranceCell cell = *ptr) - MSVC5 loads all
// three, dead-stores `reason`; the prior "un-reproducible DCE miss" verdict was wrong
// (a 3-explicit-locals source DCEs it, a struct copy does not). GruntStrGetBuffer is
// the real __thiscall CString::GetBuffer (ecx=&cell). Residue = an edx<->ecx coin-flip
// in the m_prevEntranceDesc/SetGeometry(m_poseItem) tail + the m_5c/m_60 load-order
// schedule in the PlayMoveSoundAtTile block (pure regalloc, no source lever).
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
        ReseedIdleReset(1, 0, 0);
    }
    m_entranceActive = 1;
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);
    ApplySetState1(1);
    SetEntrancePos(1, 1);
    if (SetMoveStateA(1, 1, 0, 1) == 0) {
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
    m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeM));
    m_timePerTile = g_buteMgr.GetIntDef(s_BOMBGRUNT, s_RunningTimePerTile, 0x64);
    m_22c = 1;
    {
        CWwdGameObjectA* h = m_object;
        i32 vx = h->m_screenX;
        i32 vy = h->m_screenY;
        char* sc = *reinterpret_cast<char**>((reinterpret_cast<char*>(g_gameReg->m_world) + 0x24));
        i32* rect = reinterpret_cast<i32*>((*reinterpret_cast<char**>(sc + 0x5c) + 0x40));
        if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
            g_gameReg->m_cueSink->CueSpawn(this, 8, -1, -1, -1);
        }
    }
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(m_poseItem);
    GruntEntranceCell cell = m_entranceCell;
    i32 col = cell.row + cell.col * 2;
    i32 base = cell.col + col; // (the old +0xb folded the m_cells base into the index)
    char* cn = m_cells[base].m_item.GetBuffer(0);
    m_38->ApplyName(cn);
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::LoadWingzGruntSprites(int enable)   @0x68880   (ret 4)
// When the grunt's wingz timer is active (enable != 0) it stamps the flying ITEM
// sprite name into every direction cell, sets up the wingz-duration timer and the
// pose-index lookups against the flying set, then fires the on-screen spawn cue.
// When disabled it retires the wingz-time HUD sprite and re-stamps the normal
// WALK/IDLE walking set. Both paths finish by re-stamping the current entrance-cell
// frame keyed by the active anim type code ("D" = walk pose, "A" = idle pose).
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// every cell operator=, the 8 Lookup blocks, the cue, the ScratchResolve/strcmp tail
// and the entrance-cell frame re-stamp are byte-correct in shape/offsets/symbols/CFG.
// Residue (compounded over 8 lookups): retail reuses the consumed `enable` arg slot as
// the single lookup `out` local (esp+0x20, 21 refs) under a `sub esp,0xc` frame that
// also spills a dead `reason=m_entranceCell.reason`, and SINKS each lookup's `out=0` store
// past the &out/key pushes; cl allocates a fresh out slot (esp+0x14, no frame) and
// hoists the zero-init. Source-invariant (the documented Lookup-family scheduling
// coin-flip); deferred to the final sweep. ~75%.
RVA(0x00068880, 0x67c)
i32 CGrunt::LoadWingzGruntSprites(i32 enable) {
    CAniElement* _out;
    if (enable != 0) {
        m_wingzEnabled = 1;
        m_wingzDurationLo = static_cast<i32>((static_cast<double>(m_wingzTime) * g_wingzScale - g_wingzBias));
        m_wingzDurationHi = 0;
        m_wingzClockLo = static_cast<i32>(g_frameTime);
        m_wingzClockHi = 0;
        CreateWingzTimeSprite();

        m_cells[0].m_idle = s_NW_ITEM;
        m_cells[1].m_idle = s_N_ITEM;
        m_cells[2].m_idle = s_NE_ITEM;
        m_cells[3].m_idle = s_W_ITEM;
        m_cells[4].m_idle = s_N_ITEM;
        m_cells[5].m_idle = s_E_ITEM;
        m_cells[6].m_idle = s_SW_ITEM;
        m_cells[7].m_idle = s_S_ITEM;
        m_cells[8].m_idle = s_SE_ITEM;
        m_cells[0].m_walk = s_NW_ITEM;
        m_cells[1].m_walk = s_N_ITEM;
        m_cells[2].m_walk = s_NE_ITEM;
        m_cells[3].m_walk = s_W_ITEM;
        m_cells[4].m_walk = s_N_ITEM;
        m_cells[5].m_walk = s_E_ITEM;
        m_cells[6].m_walk = s_SW_ITEM;
        m_cells[7].m_walk = s_S_ITEM;
        m_cells[8].m_walk = s_SE_ITEM;

        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_ITEM, reinterpret_cast<void*&>(_out));
        m_poseWalk = _out;
        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_ITEM, reinterpret_cast<void*&>(_out));
        m_poseIdle[2] = 0;
        m_poseIdle[0] = _out;
        m_poseIdle[1] = _out;
        m_poseIdle4 = 0;
        m_poseIdle5 = 0;

        CGruntzMgr* g = g_gameReg;
        i32 y = m_object->m_screenY;
        i32 x = m_object->m_screenX;
        CCueRect* r = reinterpret_cast<CCueRect*>(&g->m_world->m_level->m_mainPlane->m_originX);
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
            g->m_cueSink->CueSpawn(this, 8, -1, -1, -1);
        }
    } else {
        m_wingzEnabled = 0;
        m_wingzDurationLo = 0;
        m_wingzDurationHi = 0;
        if (m_wingzTimeSprite != 0) {
            m_wingzTimeSprite->m_flags |= 0x10000;
            m_wingzTimeSprite = 0;
        }

        m_cells[0].m_walk = s_NW_WALK;
        m_cells[1].m_walk = s_N_WALK;
        m_cells[2].m_walk = s_NE_WALK;
        m_cells[3].m_walk = s_W_WALK;
        m_cells[4].m_walk = s_N_WALK;
        m_cells[5].m_walk = s_E_WALK;
        m_cells[6].m_walk = s_SW_WALK;
        m_cells[7].m_walk = s_S_WALK;
        m_cells[8].m_walk = s_SE_WALK;
        m_cells[0].m_idle = s_NW_IDLE;
        m_cells[1].m_idle = s_N_IDLE;
        m_cells[2].m_idle = s_NE_IDLE;
        m_cells[3].m_idle = s_W_IDLE;
        m_cells[4].m_idle = s_N_IDLE;
        m_cells[5].m_idle = s_E_IDLE;
        m_cells[6].m_idle = s_SW_IDLE;
        m_cells[7].m_idle = s_S_IDLE;
        m_cells[8].m_idle = s_SE_IDLE;

        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_WALK, reinterpret_cast<void*&>(_out));
        m_poseWalk = _out;
        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_IDLE1, reinterpret_cast<void*&>(_out));
        m_poseIdle[0] = _out;
        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_IDLE2, reinterpret_cast<void*&>(_out));
        m_poseIdle[1] = _out;
        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_IDLE3, reinterpret_cast<void*&>(_out));
        m_poseIdle[2] = _out;
        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_IDLE4, reinterpret_cast<void*&>(_out));
        m_poseIdle4 = _out;
        _out = 0;
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_WG_IDLE5, reinterpret_cast<void*&>(_out));
        m_poseIdle5 = _out;
    }

    // Re-stamp the current entrance-cell frame keyed by the active anim type.
    CAnimNameRecord* rec = g_typeColl.ScratchResolve(m_objAux->m_1c);
    GruntScratchTeardown();
    if (strcmp(rec->m_name, s_codeD) == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(m_poseWalk);
        CAniElement* desc = m_38->m_1a0.m_14;
        i32* elem = desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem[0x14 / 4];
        i32 idx = 3 * m_entranceCell.col + m_entranceCell.row;
        char* buf = GruntStrGetBuffer(&m_cells[idx].m_walk, 0);
        m_38->ApplyLookupSprite(buf, frame);
        return 1;
    }

    CAnimNameRecord* rec2 = g_typeColl.ScratchResolve(m_objAux->m_1c);
    GruntScratchTeardown();
    if (strcmp(rec2->m_name, "A") == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(m_poseIdle[0]);
        CAniElement* desc = m_38->m_1a0.m_14;
        i32* elem = desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem[0x14 / 4];
        i32 idx = 3 * m_entranceCell.col + m_entranceCell.row;
        char* buf = GruntStrGetBuffer(&m_cells[idx].m_idle, 0);
        m_38->ApplyLookupSprite(buf, frame);
    }
    return 1;
}
// ---------------------------------------------------------------------------
// CGrunt::UpdateEntranceAnim()   @0x690a0   (__thiscall, ret 0)
// The per-frame entrance-anim / arrival update step. Re-seeds the entrance
// player's geometry sub-player (m_154->m_1a0) and bails unless it is armed-but-not-
// running (m_28!=0 && m_20==0). On the FIRST pass (m_entranceStamped==0) it stamps the
// TOY-BREAK pose geometry, applies the active descriptor's frame to the +0x448
// name CString, latches m_entranceStamped=1, and kicks the move-kind apply (m_moveVariant ? : m_moveKind).
// On a later pass (m_entranceStamped!=0) it (when arrived) builds the three HUD stat sprites,
// re-latches the "A"(idle) anim-set node into m_objAux->m_1c, drives the move state
// (SetMoveStateA(m_19c,1,0,0)), clears m_entranceActive, then either - when the
// grunt's last tile carries the 0x80 attribute - commits the arrival move
// (SetEntrancePos(1,1); tileMgr->CommitArrivalMove(this, lastX, lastY)) or else
// bumps the HUD z-clamp (m_object->m_74 = m_60 + 0x186a0; m_8 |= 0x20000).
//
// @early-stop
// reloc-masked-extern plateau: CFG, every member offset/gate, the board index math
// (stride 7, attr bit 0x80), the z-clamp constant 0x186a0, and all call shapes are
// byte-faithful. Residue = the engine callees reached through incremental-link
// thunks (SetGeoSourceR/SetGeometry/GetBuffer/SetAnimFrame/LookupAnimSet, the
// CreateHealthSprite/Stamina/Toy creators, SetMoveStateA/SetEntrancePos/the apply
// + CommitArrivalMove thunks) are unnamed externals, so their `call rel32`
// displacements pair to differently-named retail thunks and score fuzzy. Naming
// that whole referent set is a final-sweep task.
RVA(0x000690a0, 0x1c5)
i32 CGrunt::UpdateEntranceAnim() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    if (m_38->m_1a0.m_28 == 0 || m_38->m_1a0.m_20 != 0) {
        return 0;
    }

    if (m_entranceStamped == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(m_poseToyBreak);

        CAniElement* desc = m_38->m_1a0.m_14;
        i32* elem = desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem[0x14 / 4];

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
    m_objAux->m_1c = static_cast<void*>(g_buteTree.Find("A"));
    SetMoveStateA(m_19c, 1, 0, 0);
    m_entranceActive = 0;

    CGruntzMgr* g = g_gameReg;
    i32 tx = m_lastTilePxX >> 5;
    i32 ty = m_lastTilePxY >> 5;
    CGruntzMapMgr* board = g->m_tileGrid;
    i32 flags;
    if (static_cast<u32>(tx) >= static_cast<u32>(board->m_width) || static_cast<u32>(ty) >= static_cast<u32>(board->m_height)) {
        flags = 1;
    } else {
        flags = (reinterpret_cast<i32*>(board->m_rowBytes[ty]))[tx * 7];
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

// ---------------------------------------------------------------------------
// CGrunt::StepArrivalCommit()   @0x692f0   (__thiscall, ret 0; always returns 0)
// The per-tick death/struck reaction + arrival-commit dispatch. Gated on the
// entrance being committed (m_1fc), it resolves the grunt's current anim-set name
// and dispatches on its single-letter type code (A/D -> finalize; I -> arrival
// re-notify; G/L/P -> idle reseed; O -> commit-move; J -> re-latch "D" + drive the
// move mode; N -> align-down/drop-ready snap; M -> tile set), then runs the shared
// finalize tail (arrival consider, clear the HUD stat sprites, latch the entrance
// reset, commit the tile, re-latch the "Q" anim set, and apply the DEATHZ_FREEZE
// geometry + first frame). __thiscall, ret 0.
//
// @early-stop
// global zero-pin regalloc wall (~1.8%): the J-block `sub esp,0xc` frame is NOW
// reproduced (GruntEntranceCell by-value copy) and GruntStrGetBuffer is the real
// __thiscall CString::GetBuffer. The alignment-collapsing residue is a GLOBAL
// register-allocation decision: retail SINKS `xor ebx,ebx` (ebx=0 for the finalize
// sprite-clear/m_1a4 stores + the GetBuffer arg) to after the 10-way strcmp cascade,
// so every arm tests with `test eax,eax`/`mov bl,[edi]` scratch; my 0x850 body's many
// zero-uses drive cl to HOIST `xor ebx,ebx` to entry, so all 10 arms emit `cmp eax,ebx`
// and the literal byte can't land in bl. Proven not locally steerable: a standalone
// repro of the exact first-check + `bool eq` cascade does NOT pin (uses test) - the pin
// only emerges at this body's size/zero-density. No source lever; the entry-vs-sunk
// materialization desync rolls the faithful carcass to ~2%. Final sweep.
RVA(0x000692f0, 0x850)
i32 CGrunt::StepArrivalCommit() {
    if (m_entranceCommitted == 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0);
    if (eq) {
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (eq) {
        goto finalize;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        if (m_entranceReason == 0x13) {
            g_gameReg->m_cueSink->Cue1(m_object->m_188);
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
        // code "J": clear the entrance gate; if the PREVIOUS anim set was "D",
        // re-latch a fresh "D" set + drive the WALK geometry + stamp the cell frame.
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
            m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeD));
            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup_15c2d0(m_poseWalk);
            GruntEntranceCell cell = m_entranceCell;
            i32 colv = cell.row + cell.col * 2;
            i32 base = cell.col + colv;
            char* nm = m_cells[base].m_walk.GetBuffer(0);
            m_38->ApplyName(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    // default: the M / N reject codes.
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
            m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeD));
            OnCoordCommit(m_coordToggle);
        }
        goto finalize;
    }
    {
        char* prev = g_typeColl.GetNameRecords(m_objAux->m_1c)->m_name;
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
        g_gameReg->m_cueSink->Cue1(m_object->m_188);
    }
    SetMoveStateA(m_19c, 1, 0, 0);
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
        SetMoveStateA(mode, 1, 0, 1);
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
        EmitMoveCueQ(mode);
        goto finalize;
    }
    SetMoveStateA(mode, 1, 0, 1);
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
    m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeQ));
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
        i32* elem = desc->m_records.GetSize() > 0 ? reinterpret_cast<i32*>(desc->m_records.GetAt(0)) : 0;
        i32 frame = elem[0x14 / 4];
        m_38->ApplyLookupSprite(s_GRUNTZ_DEATHZ_FREEZE, frame);
    }
    m_freezeUnfrozen = 0;
    m_freezeDelayDone = 1;
    return 0;
}

// ---------------------------------------------------------------------------
// CGrunt::LoadFreezeSpellAssets()  @0x69d60  (__thiscall, ret 0)
// The freeze-spell entrance-anim finalize step. Arm the geometry source; when the
// sub-player is armed-but-not-running (sub+0x28 != 0 && sub+0x20 == 0):
//   * if the +0x240 "finalized" latch is set, clear the entrance, re-init the anim
//     name table, reseed the idle reset, and (if the last tile carries the high
//     occupancy bit) commit the arrival move - then return.
//   * else (+0x240 clear) stamp the DEATHZ_SPARKLE finalize geometry, seed the
//     freeze-delay idle window (Spellz/FreezeDelay bute, default 0x2710) anchored
//     at the game clock, and clear the +0x23c latch.
// Then, when +0x23c is clear and the idle-delay window has elapsed, stamp the
// DEATHZ_UNFREEZE geometry, fire the on-screen 6-arg entrance cue (0x35c) when the
// grunt's HUD point is in view, and set the +0x240/+0x23c latches. Returns 0.
// @early-stop
// callee-save rematerialization wall (docs/patterns/shrink-wrapped-callee-save-push):
// retail reloads m_lastTilePxX/Y for CommitArrivalMove, so the tile-read uses 4 regs
// (no ebp push); cl caches them in a callee-saved reg, pushing ebp and shifting the
// 64-bit idle-timer compare's regalloc. Body byte-exact apart from that one-register
// cascade (~87.4%). Logic complete; deferred to the final sweep.
RVA(0x00069d60, 0x1e1)
i32 CGrunt::LoadFreezeSpellAssets() {
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    if (m_38->m_1a0.m_28 != 0 && m_38->m_1a0.m_20 == 0) {
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
            char* sc = *reinterpret_cast<char**>((reinterpret_cast<char*>(g_gameReg->m_world) + 0x24));
            i32* rect = reinterpret_cast<i32*>((*reinterpret_cast<char**>(sc + 0x5c) + 0x40));
            if (vx < rect[2] && vx >= rect[0] && vy < rect[3] && vy >= rect[1]) {
                g_gameReg->m_cueSink->CueA(this, 0x35c, -1, 0, -1, -1);
            }
            m_freezeUnfrozen = 1;
            m_freezeDelayDone = 1;
        }
    }
    return 0;
}

RVA(0x00069fd0, 0x69)
i32 CGrunt::FinishEntranceMove() {
    // 0x15c360 = CAniAdvanceCursor::Advance (cast the m_1a0 geometry facet)
    m_38->m_1a0.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* cur = &m_38->m_1a0; // one +0x1a0 sub-object base, two member reads
    if (cur->m_28 == 0 || cur->m_20 != 0) {
        return 0;
    }
    if (m_36c == 0) {
        // 0x79fb0 = CTriggerMgr::NotifyCell (the reused registry slot), not a CGruntTileMgr method
        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
    }
    m_38->m_flags |= 0x10000;
    return 0;
}

static char s_MovingDeathTime[] = "MovingDeathTime";
static const char s_animKeyS[] = "S";

i32 g_moveVecE[3];  // 0x644aa0
i32 g_moveVecN[3];  // 0x644ab0
i32 g_moveVecS[3];  // 0x644ac0
i32 g_moveVecW[3];  // 0x644ad0
i32 g_moveVecNE[3]; // 0x644ae0
i32 g_moveVecNW[3]; // 0x644b18
i32 g_moveVecSE[3]; // 0x644b28
i32 g_moveVecSW[3]; // 0x644b48

// @early-stop
// jump-table-placement wall (0.4% stub -> 22%): logic is complete and correct -
// the intro (bute GetDwordDef -> m_400 double, the board tile-attr sample, the
// sel = g_gameReg->m_curState->+0x20 selector) is byte-exact, and BOTH dense switches
// lower to the retail two-level byte-index+jptr tables with the 8 compass bodies
// laid out in retail's .text order (case groups reordered per
// docs/patterns/switch-cases-source-order.md, +6% over ascending-value order).
// Residual: cl (this build) emits each switch's inline jump-table DATA *between*
// the indirect `jmpl` and the case bodies, whereas retail pools BOTH switches'
// tables past the `ret` at the function end (0x6a4a0+, outside the 0x43d body) -
// a ~200-byte data insertion in the middle that shifts every case body offset and
// desyncs objdiff's alignment. Not source-steerable in MSVC5 (same family as
// docs/patterns/switch-jumptable-separate-comdat.md). Final sweep.
RVA(0x0006a060, 0x43d)
i32 CGrunt::LoadGruntMovingDeathConfig() {
    m_400 = 16.0 / static_cast<double>(g_buteMgr.GetDwordDef(s_Grunt, s_MovingDeathTime, 0x3e8));

    CGruntzMgr* g = g_gameReg;
    void* sub2c = *reinterpret_cast<void**>((reinterpret_cast<char*>(g) + 0x2c));
    CGruntzMapMgr* b = g->m_tileGrid;
    CWwdGameObjectA* h = m_object;
    i32 xbound = b->m_width;
    i32 tileY = h->m_screenY >> 5;
    i32 tileX = h->m_screenX >> 5;
    i32 dir;
    if (static_cast<u32>(tileX) >= static_cast<u32>(xbound) || static_cast<u32>(tileY) >= static_cast<u32>(b->m_height)) {
        dir = 0;
    } else {
        dir = (reinterpret_cast<i32*>(b->m_rowBytes[tileY]))[tileX * 7 + 3];
    }

    i32 sel = *reinterpret_cast<i32*>((reinterpret_cast<char*>(sub2c) + 0x20));

// Latch the compass velocity triple into m_entranceCell[0..2] + step the last-tile
// pixel position. Case groups are laid out so cl emits the distinct (tail-merged)
// blocks in retail's .text order (docs/patterns/switch-cases-source-order.md).
#define MV_VEC(V)                                                                                  \
    m_entranceCell.col = g_moveVec##V[0];                                                          \
    m_entranceCell.row = g_moveVec##V[1];                                                          \
    m_entranceCell.reason = g_moveVec##V[2]
#define MV_N                                                                                       \
    MV_VEC(N);                                                                                     \
    m_lastTilePxY -= 0x10
#define MV_S                                                                                       \
    MV_VEC(S);                                                                                     \
    m_lastTilePxY += 0x10
#define MV_E                                                                                       \
    MV_VEC(E);                                                                                     \
    m_lastTilePxX += 0x10
#define MV_W                                                                                       \
    MV_VEC(W);                                                                                     \
    m_lastTilePxX -= 0x10
#define MV_NE                                                                                      \
    MV_VEC(NE);                                                                                    \
    m_lastTilePxX += 0x10;                                                                         \
    m_lastTilePxY -= 0x10
#define MV_NW                                                                                      \
    MV_VEC(NW);                                                                                    \
    m_lastTilePxX -= 0x10;                                                                         \
    m_lastTilePxY -= 0x10
#define MV_SE                                                                                      \
    MV_VEC(SE);                                                                                    \
    m_lastTilePxX += 0x10;                                                                         \
    m_lastTilePxY += 0x10
#define MV_SW                                                                                      \
    MV_VEC(SW);                                                                                    \
    m_lastTilePxX -= 0x10;                                                                         \
    m_lastTilePxY += 0x10

    if (sel >= 5) {
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
    } else {
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
    m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_animKeyS));
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::StepAnimDispatchB()   @0x6a6d0   (ret 0)
// @early-stop
// large-state-machine + zero-register-pinning plateau: the 12-way type-code cascade,
// the m_1a0 mode sub-dispatch, the K arrival arm, the coord recycle, and the
// LookupAnimSet re-latch are reconstructed in shape/order. Residue: retail pins the
// strcmp sentinels 0/-1 in callee-saved ebx/ebp
// (docs/patterns/zero-register-pinning.md), the scratch loop-strength-reduction, the
// grid/board raw-offset chains, and cross-arm regalloc. Deferred to the final sweep.
RVA(0x0006a6d0, 0x936)
i32 CGrunt::StepAnimDispatchB() {
    bool eq;
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "A") == 0);
    if (eq) {
        goto kArm;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
    if (eq) {
        goto kArm;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "I") == 0);
    if (eq) {
        if (m_entranceReason == 0x13) {
            EmitMoveCueShort(m_object->m_188, 0, 0);
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
        ApplySetState1(1);
        CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
        return 1;
    }
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), "J") == 0);
    if (eq) {
        m_entranceActive = 0;
        if (m_poweredUp == 0 && m_neighborValid == 0) {
            m_entranceCommitted = 0;
            ReseedIdleReset(1, 0, 0);
        }
        m_35c = 0;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = static_cast<void*>(g_buteTree.Find(s_codeD));
        m_value = m_38->m_1a0.m_14;
        m_38->m_1a0.Setup_15c2d0(m_poseWalk);
        // by-value cell copy dead-spills `reason` (esp+0x24) -> sub esp frame, then
        // GetBuffer(0)/CacheFirstFrame (retail J-arm identical to StepAnimDispatchA).
        GruntEntranceCell cell = m_entranceCell;
        i32 col = cell.row + cell.col * 2;
        i32 base = cell.col + col;
        char* nm = m_cells[base].m_walk.GetBuffer(0);
        m_38->ApplyName(nm);
        goto modeDispatch;
    } else {
        ApplySetState1(1);
        goto modeDispatch;
    }

idleReseed:
    SetMoveStateA(m_19c, 1, 0, 1);
    goto modeDispatch;

modeDispatch: {
    i32 mode = m_moveMode;
    if (mode >= 0x32) {
        SetMoveStateA(mode, 1, 0, 1);
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
        EmitMoveCueQ(mode);
        return 1;
    }
    SetMoveStateA(mode, 1, 0, 1);
    m_moveMode = -1;
    return 1;
}

kArm:
    // code "K": the arrival arm - re-anchor + re-stamp the grid cell.
    eq = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeK) == 0);
    if (eq && m_entranceArmed != 0) {
        CommitMoveA(m_lastTilePxY, m_lastTilePxX, 0);
        StepDropApply();
    }
    return 1;
}

RVA(0x0006b260, 0x5)
void CGrunt::DispatchVtbl24() {
    StepAttackFire(); // the slot-9 (+0x24) virtual self-dispatch (tail jmp [vt+0x24])
}

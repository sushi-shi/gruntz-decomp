// TerrainTileLoader.cpp - CTriggerMgr::LoadTileArrivalFx, the per-tile
// terrain-action sprite loader (C:\Proj\Gruntz). The backlog's second-biggest
// megafunction (4905 B): given a tile coordinate (edi=x, ebp=y) and an action
// descriptor, it dispatches on the tile's terrain-action type and registers /
// resolves the matching sprite, particle, lighting or puddle asset set in the
// game image registry (*g_gameReg). It is a /GX EH-framed routine (a CString
// diagnostic temp at [esp+0x74] gives it the exception frame) with three nested
// jump-table switches over the action-type id.
//
// IDENTITY (the ex-`CTerrainTileLoader` placeholder class, DISSOLVED 2026-07-16):
// 0x75e90 IS ?LoadTileArrivalFx@CTriggerMgr@@ - the declared-only 6-arg driver
// TriggerMgr.h already carried (thunk 0x3945). Proof: (1) every retail caller
// dispatches it on ecx = [grunt+0x260] == CGrunt::m_tileMgr == the ONE CTriggerMgr
// (gruntz sema xref 0x3945: StepAnimDispatchA/B, CommitNeighbor, StepArrivalCommit,
// ... + CTriggerMgr::ApplyTriggerB/ClearCell self-calls); (2) the view's only own
// member - the level holder at +0x22c - IS CTriggerMgr::m_level (the identical
// m_level->m_level->m_mainPlane clamp walk as the siblings WireTileSwitchLogic /
// ApplySwitch); (3) GruntEntranceArrival's two other call sites already bound the
// same thunk as m_tileMgr->LoadTileArrivalFx.
//
// The $SG string set ("GAME_DIRT"/"LEVEL_DIRT", "GAME_GAUNTLETBRICK1"/
// "LEVEL_GAUNTLETROCK1", "LEVEL_ROCKBREAK", "GAME_HIDDENITEM"/
// "GAME_LIGHTING_HIDDENITEM", "GAME_WATER"/"GAME_WATERSPLASH",
// "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2", "Particlez", "LightFx", "ToyPeek", and the
// "No giant rock logic found at: x=" diagnostic) identifies it as the terrain
// tile-action loader. Reconstructed as an EH unit (the CString temp).
//
// Structure: it first resolves the tile cell from the grid (m_level->m_24 level /
// g_gameReg->m_curState play-state chain), reads the cell's object id, then runs
// the outer action switch ([esp+0x84]-3, range 0..0xf -> jump table 0x4771bc)
// whose arms (DIRT, GIANTROCK, ROCKBREAK, WATER, HIDDENITEM-lighting, TOY,
// PUDDLE) each look up a GAME_*/LEVEL_* namespace pair in the registry hashtable
// (Lookup @0x1b8438) and register / free / resolve the resolved set. The giant
// case 0x22/0x23 reads the cell occupancy; the lighting cases 0x96..0x99 and the
// toy cases 0x1e/0x1f/0x21 each have their own inner switch.
//
// @early-stop  (1.1% -> 7.9%: return type corrected to int [retail materialises
// eax=1 before each ret]; reconstructed the always-run prologue - the action
// descriptor, the grid-cell type resolve [m_level->m_level->m_mainPlane clamp +
// cells[rowBase[y]+x] + tile-class GetCollisionAt], the pixel snap - the 0x4771bc
// byte-indexed outer switch mapped to actionTypes {3,5,7,0xd,0xf,0x12}, and the
// DIRT arm [actionType 0xd] with its a5 {-1,2,0x63} sub-dispatch + Particlez
// CreateSprite + tag-0x1a clear.) Residual is the documented /GX nested-jump-table
// megafunction wall: the retail frame is sub esp,0x54 - that size is fixed by the
// UNION of all 6 arms' locals + the descending EH-state scopes, so a partial body
// (this reconstructs 1 of 6 arms) frames at 0xc and shifts every [esp+X] slot;
// the descriptor + the 5 unreconstructed arms (each its own nested cellType jump
// table + CString diagnostic path) are needed to pin the frame and stop the DCE
// of the prologue's descriptor/cell reads. A leaf-first full-body redo is the fix
// (docs/patterns/jumptable-data-overlap.md; big-seh-fuzzy-desync.md;
// eh-state-numbering-base.md; o2-optimizer-bailout-framed.md).

#include <Mfc.h>                  // PtInRect (via <windows.h>), the CString diagnostic temp
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h> // this TU's class (LoadTileArrivalFx is a CTriggerMgr method)

#include <Gruntz/GameLevel.h>         // CGameLevel (m_level->m_24) + CTileImageSet (GetCollisionAt)
#include <Gruntz/GameRegistry.h>      // CGameRegistry / CDDrawSurfaceMgr (the *0x24556c singleton)
#include <Gruntz/Play.h>              // CPlay (g_gameReg->m_curState) - m_beginMarker @+0x2e4
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/TileTriggerContainer.h> // CTileTriggerContainer (FindInLists12/DelFromList1)
#include <Gruntz/TileTriggerLogic.h>     // CTileTriggerLogic (ApplyMove tags the found set)
#include <Gruntz/UserLogic.h>            // CGameObject (the created Particlez sprite)
#include <Wwd/WwdFile.h>                 // CDDrawWorkerHost - the canonical plane (cell lookup)
#include <rva.h>

RVA(0x00075e90, 0x1329)
i32 CTriggerMgr::LoadTileArrivalFx(
    i32 ownerHi,
    i32 ownerLo,
    i32 tileX,
    i32 tileY,
    i32 reason,
    i32 sel
) {
    static_cast<void>(ownerHi);
    static_cast<void>(ownerLo);
    CString diag; // the "No giant rock logic found" temp - forces the /GX EH frame

    CDDrawSurfaceMgr* level = m_world;
    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState); // retail reads [g_gameReg+0x2c]
    CGameLevel* grid = level->m_level;
    CDDrawWorkerHost* g = grid->m_mainPlane;

    // clamp the tile coords to the grid (tile-space bounds at +0x28/+0x2c)
    i32 cx = tileX;
    if (tileX < 0) {
        cx = 0;
    } else if (tileX >= g->m_gridW) {
        cx = g->m_gridW - 1;
    }
    i32 cy;
    if (tileY < 0) {
        cy = 0;
    } else if (tileY >= g->m_gridH) {
        cy = g->m_gridH - 1;
    } else {
        cy = tileY;
    }

    i32 cellType;
    i32 cell = g->m_tileGrid[g->m_colOffsets[cy] + cx];
    if (cell == static_cast<i32>(0xeeeeeeee) || cell == -1) {
        cellType = 0;
    } else {
        CTileImageSet* tc = static_cast<CTileImageSet*>(grid->m_imageSets.GetAt(cell & 0xffff));
        cellType = tc->GetCollisionAt(0, 0);
    }

    i32 px = tileX * 32 + 0x10;
    i32 py = tileY * 32 + 0x10;

    switch (reason - 3) {
        case 0xa: // reason 0xd - the DIRT / eye-candy arm
            if (sel == -1) {
                return 1;
            }
            if (sel == 2) {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect(reinterpret_cast<const RECT*>(&g_gameReg->m_viewOriginL), pt)) {
                    CWwdGameObjectA* set =
                        level->m_childGroup->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
                    if (set != 0) {
                        set->ApplyName("LEVEL_DIRT");
                        set->ApplyLookupGeometry("GAME_DIRT", 0);
                    }
                }
                return 1;
            }
            if (sel != 0x63) {
                return 1;
            }
            // sel == 0x63: clear the tile's registered tag-0x1a set, keyed by tile coord
            if (cellType == 0x22) {
                CTileTriggerContainer* reg = state->m_beginMarker;
                CTileTriggerLogic* found = reg->FindInLists12((tileX << 8) + tileY, 0x1a);
                if (found != 0) {
                    found->ApplyMove(0x22);
                    reg->DelFromList1(found);
                }
            }
            return 1;
        default:
            return 1;
    }
}

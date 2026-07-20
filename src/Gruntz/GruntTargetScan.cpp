// GruntTargetScan.cpp - CGrunt::ScanNearestTarget (0xf42f0), re-homed from
// src/Stub/ApiCallers.cpp. A direct sibling of GruntArrivalScan.cpp's ArrivalScanA/B/C
// (0xecc90/0xf0e20/0xf36a0) - it sits one slot after 0xf36a0 in retail and shares that
// family's whole idiom: a nested scan over the tile-mgr's 4x15 grunt board
// (m_tileMgr->m_grid / g_gameReg->m_cmdGrid->m_grid, == CGruntTileMgr), a
// reason->priority switch (inlined 12x = 2 per compare site x 6 sites) that gates each
// candidate, squared-distance min tracking, a PtInRect box gate, the m_defenderState
// mode dispatch (0=wander/seek, 1=lock, 2=arrive), and a rand()-driven idle-wander tail
// (idiv 0x7530 window + idiv m_extent.right/m_extent.bottom nearby jitter). All engine helpers +
// the manager/grid globals are external (reloc-masked); every CGrunt/CGruntHud field is
// reached through its real typed member on the canonical class (the old F/P raw-offset
// cast-hiding macros are gone; only offsets + code bytes are load-bearing).
//
// GruntArrivalScan.cpp): CGruntScan IS CGrunt (this method's owner), CScanReg IS
// CGameRegistry (g_gameReg), CScanTileMgr IS CGruntTileMgr (m_tileMgr / g_gameReg->
// m_cmdGrid, the CGrunt* m_grid[4][15] board), CScanCueMgr's cue fire IS
// CGruntCueSink::CueA (cast-free), CScanSub30/CScanSub24 are the m_world->m_level chain
// (CDDrawSurfaceMgr -> CGameLevel, board base at +0x5c). CScanGrid stays the shared
// <Gruntz/ScanGrid.h> board-grid view (dims).
//
// @early-stop
// Logic reconstructed in full (every branch, the 12 inlined priority switches, the grid
// scan, the m_defenderState dispatch, the wander tail). 2026-07-05: the m_defenderState
// dispatch is a `switch` (converted from if/else) -> retail's exact `sub eax; je case0; dec;
// je case1; dec; jne default(return 1); [case2 fall-through]` ladder with case2 nearest /
// case0 (seek+wander) farthest; +0.4% only, because the if/else already put mode2 first (unlike
// ChargeStep/UpdateArrival where the switch was worth +30%). Residual (base 1035 vs retail
// 1238 insns) is the family wall of the ArrivalScan siblings: (1) this-register colored ebx
// here vs edi in retail + frame 0x44 vs 0x40 + the scan-loop scheduling, (2) the case-2
// powered-up recheck DCE (same artifact as ChargeStep/UpdateArrival - dead because the
// switch runs only with m_poweredUp==0), (3) the shared-return tail-merge cl won't permute.
// Final-sweep candidate.
#include <Mfc.h> // afx-first: <Gruntz/GruntSpawnConfig.h> pulls MFC; keep windows.h MFC-safe
#include <Gruntz/GruntSpawnConfig.h> // complete type for the cue calls
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <string.h>

#include <rva.h>
#include <Gruntz/Grunt.h>        // canonical CGrunt / CGruntCueSink / CGameRegistry
#include <Gruntz/TriggerMgr.h>   // the ONE CTriggerMgr (ex the CGruntTileMgr view)
#include <Gruntz/GameRegistry.h> // CGameRegistry / CDDrawSurfaceMgr
#include <Gruntz/GameLevel.h>    // CGameLevel / CLevelPlane (world->m_24->m_mainPlane->m_originX)
#include <Gruntz/ScanGrid.h>     // CScanGrid (the shared board-grid dims view)
#include <stdlib.h>              // engine rand (0x11fee0)

// The reason(m_170)->priority map, inlined at each of the 6 candidate-compare sites
// (12 jump tables). Case bodies emit the priority constants 2..0x17 in value order
// (retail's block layout); the jump table carries the reason permutation. reason 20
// and every out-of-range reason fall to the default 0x17.
#define PRIO(dst, r)                                                                               \
    switch (r) {                                                                                   \
        case 1:                                                                                    \
            dst = 2;                                                                               \
            break;                                                                                 \
        case 21:                                                                                   \
            dst = 3;                                                                               \
            break;                                                                                 \
        case 16:                                                                                   \
            dst = 4;                                                                               \
            break;                                                                                 \
        case 9:                                                                                    \
            dst = 5;                                                                               \
            break;                                                                                 \
        case 4:                                                                                    \
            dst = 6;                                                                               \
            break;                                                                                 \
        case 11:                                                                                   \
            dst = 7;                                                                               \
            break;                                                                                 \
        case 13:                                                                                   \
            dst = 8;                                                                               \
            break;                                                                                 \
        case 2:                                                                                    \
            dst = 9;                                                                               \
            break;                                                                                 \
        case 14:                                                                                   \
            dst = 10;                                                                              \
            break;                                                                                 \
        case 5:                                                                                    \
            dst = 11;                                                                              \
            break;                                                                                 \
        case 22:                                                                                   \
            dst = 12;                                                                              \
            break;                                                                                 \
        case 15:                                                                                   \
            dst = 13;                                                                              \
            break;                                                                                 \
        case 3:                                                                                    \
            dst = 14;                                                                              \
            break;                                                                                 \
        case 8:                                                                                    \
            dst = 15;                                                                              \
            break;                                                                                 \
        case 12:                                                                                   \
            dst = 16;                                                                              \
            break;                                                                                 \
        case 7:                                                                                    \
            dst = 17;                                                                              \
            break;                                                                                 \
        case 18:                                                                                   \
            dst = 18;                                                                              \
            break;                                                                                 \
        case 6:                                                                                    \
            dst = 19;                                                                              \
            break;                                                                                 \
        case 17:                                                                                   \
            dst = 20;                                                                              \
            break;                                                                                 \
        case 10:                                                                                   \
            dst = 21;                                                                              \
            break;                                                                                 \
        case 19:                                                                                   \
            dst = 22;                                                                              \
            break;                                                                                 \
        default:                                                                                   \
            dst = 23;                                                                              \
            break;                                                                                 \
    }


// __cdecl board rect predicate (0x401127): point-in-board-rect (the visible CCueRect).
extern "C" i32 BoardTest(CCueRect* board, i32 x, i32 y); // 0x401127

RVA(0x000f42f0, 0x1193)
i32 CGrunt::ScanNearestTarget() {
    i32 ownerHi = m_tileOwnerHi;
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    i32 cx = m_lastTilePxX >> 5;
    i32 cy = m_lastTilePxY >> 5;

    // Scan the tile-mgr grunt board for the nearest higher-or-equal-priority target.
    CGrunt* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 row = 0; row < 4; row++) {
        if (row == ownerHi) {
            continue;
        }
        CTriggerMgr* board = g_gameReg->m_cmdGrid;
        for (i32 col = 0; col < 15; col++) {
            CGrunt* cand = board->m_grid[row * TM_GRID_COLS + col];
            if (cand != 0 && cand->m_entranceCommitted != 0 && cand->m_gruntKind != 0x36) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, cand->m_entranceReason);
                if (pa <= pb) {
                    i32 dx = (cand->m_10->m_screenX >> 5) - cx;
                    i32 dy = (cand->m_10->m_screenY >> 5) - cy;
                    i32 d = dx * dx + dy * dy;
                    if (d < bestDist) {
                        best = cand;
                        bestDist = d;
                    }
                }
            }
        }
    }

    // Recompute the scan box (center +- (m_2dc + m_298 + 1)) and reject `best` when its
    // center falls outside it.
    i32 halfBox = m_defenderRadius + m_reachRadius + 1;
    i32 pt[2];
    GetScreenPos(reinterpret_cast<GruntTilePos*>(pt));
    i32 by = pt[1] >> 5;
    GetScreenPos(reinterpret_cast<GruntTilePos*>(pt));
    i32 bx = pt[0] >> 5;
    GetScreenPos(reinterpret_cast<GruntTilePos*>(pt));
    i32 t3y = pt[1] >> 5;
    GetScreenPos(reinterpret_cast<GruntTilePos*>(pt));
    i32 t4x = pt[0] >> 5;
    RECT box;
    box.left = t4x - halfBox;
    box.top = t3y - halfBox;
    box.right = bx + halfBox + 1;
    box.bottom = by + halfBox + 1;
    if (best != 0) {
        POINT pt;
        pt.x = best->m_lastTilePxX >> 5;
        pt.y = best->m_lastTilePxY >> 5;
        if (!PtInRect(&box, pt)) {
            best = 0;
        }
    }

    // atTarget: `best` has reached its own last-tile pixel AND the tile probes free.
    i32 atTarget = 0;
    if (best != 0) {
        i32 x = best->m_10->m_screenX;
        if (x == best->m_lastTilePxX && best->m_10->m_screenY == best->m_lastTilePxY
            && this->RectContains(x, best->m_10->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    // Powered-up reset gate (identical to ArrivalScanB's m_poweredUp path).
    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && best == 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        }
        if (m_neighborValid != 0) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    // m_defenderState mode dispatch (switch -> retail's sub/dec ladder: tests 0, 1, then 2
    // falls through; default (m_defenderState not 0/1/2) returns 1). Case bodies lay out
    // case 2 nearest, case 0 (seek/wander) farthest, matching retail.
    switch (m_defenderState) {
        case 0: {
            // seek / commit toward `best`, else idle wander.
            if (best == 0) {
                goto L_wander;
            }
            if (m_poweredUp == 0 && m_stamina >= 100 && best->m_10->m_screenX == best->m_lastTilePxX
                && best->m_10->m_screenY == best->m_lastTilePxY) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, best->m_entranceReason);
                if (pa <= pb
                    && this->RectContains(best->m_10->m_screenX, best->m_10->m_screenY) != 0) {
                    CommitNeighbor(
                        best->m_tileOwnerHi,
                        best->m_tileOwnerLo,
                        best->m_lastTilePxX,
                        best->m_lastTilePxY
                    );
                    return 1;
                }
            }

            // seek: probe-move toward best's center, stamp the move, fire the cue.
            if (best == 0) {
                goto L_wander;
            }
            {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, best->m_entranceReason);
                if (pa > pb) {
                    goto L_wander;
                }
            }
            if (static_cast<u32>(m_dwell) <= 0x3e8) {
                goto L_wander;
            }
            m_defenderX = m_lastTilePxX;
            m_defenderY = m_lastTilePxY;
            {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, best->m_entranceReason);
                if (pa > pb) {
                    goto L_scanDone;
                }
            }
            if (this->GruntInRadius(best->m_tileOwnerHi, best->m_tileOwnerLo) == 0) {
                goto L_scanDone;
            }
            {
                i32 cc[4];
                best->GetScreenPos(reinterpret_cast<GruntTilePos*>(cc));
                if (this->TileSwitch(cc[0] >> 5, cc[1] >> 5, 0, m_arrivalFlags, 1, 0) == 0) {
                    goto L_scanDone;
                }
            }
            SetEntrancePos(1, 1);
            m_arrivalCol = best->m_tileOwnerHi;
            m_arrivalRow = best->m_tileOwnerLo;
            m_defenderState = 1;
            {
                if (BoardTest(
                        reinterpret_cast<CCueRect*>(&g_gameReg->m_world->m_level->m_mainPlane->m_originX),
                        m_10->m_screenX,
                        m_10->m_screenY
                    )
                    != 0) {
                    g_gameReg->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                }
            }
        L_scanDone:
            m_dwell = 0;
            return 1;

        L_wander:
            if (m_resetApplied != 0 || m_318 == 0 || static_cast<u32>(m_dwell) <= 0xbb8) {
                return 1;
            }
            // 64-bit elapsed = g_frameTime - {m_308:m_30c}; compare with window {m_310:m_314}.
            {
                i32 lo = static_cast<i32>(g_frameTime) - m_arrivalRerollLo;
                i32 hi = 0 - m_arrivalRerollHi
                         - (static_cast<u32>(static_cast<i32>(g_frameTime)) < static_cast<u32>(m_arrivalRerollLo) ? 1 : 0);
                i32 winHi = m_arrivalRerollWindowHi;
                if (hi > winHi || (hi == winHi && static_cast<u32>(lo) >= static_cast<u32>(m_arrivalRerollWindowLo))) {
                    // window elapsed: re-arm the idle timer with a fresh rand()%0x7530+0x7530.
                    ResetEntranceAnimation(1, 1, 0);
                    m_arrivalRerollLo = 0;
                    m_arrivalRerollWindowLo = 0;
                    m_arrivalRerollHi = 0;
                    m_arrivalRerollWindowHi = 0;
                    m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                    m_arrivalRerollWindowHi = 0;
                    m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                    m_arrivalRerollHi = 0;
                } else {
                    // not elapsed: jitter to a random nearby board cell.
                    CGameObject* hud = m_10;
                    i32 baseCol = hud->m_extent.left;
                    i32 spanX = hud->m_extent.right - baseCol;
                    i32 baseRow = hud->m_extent.top;
                    spanX = (spanX ^ (spanX >> 31)) - (spanX >> 31);
                    i32 spanY = hud->m_extent.bottom - baseRow;
                    spanY = (spanY ^ (spanY >> 31)) - (spanY >> 31);
                    if (spanX != 0) {
                        baseCol += rand() % spanX;
                    }
                    if (spanY != 0) {
                        baseRow += rand() % spanY;
                    }
                    CScanGrid* grid = reinterpret_cast<CScanGrid*>(g_gameReg->m_tileGrid);
                    if (static_cast<u32>(baseCol) < static_cast<u32>(grid->m_c) && static_cast<u32>(baseRow) < static_cast<u32>(grid->m_10)) {
                        this->TileSwitch(baseCol, baseRow, 0, m_arrivalFlags, 1, 0);
                    }
                    if (CoordCount() != 0) {
                        if (spanX > spanY) {
                            spanX = spanY;
                        }
                        if (CoordCount() > spanX) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
            }
            m_dwell = 0;
            return 1;
        }
        case 1: {
            CGrunt* sg = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
            if (best != 0 && best != sg) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
                return 1;
            }
            if (sg == 0) {
                goto L_clearMode;
            }
            i32 pa;
            PRIO(pa, m_entranceReason);
            i32 pb;
            PRIO(pb, sg->m_entranceReason);
            if (pa > pb) {
                goto L_clearMode;
            }
            if (sg->m_entranceCommitted == 0) {
                goto L_clearMode;
            }
            if (this->GruntInRadius(sg->m_tileOwnerHi, sg->m_tileOwnerLo) == 0) {
                goto L_clearMode;
            }
            if (static_cast<u32>(m_dwell) > 0x1f4) {
                StepArrivalDrop(sg->m_lastTilePxX, sg->m_lastTilePxY, m_arrivalFlags, 0, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp != 0 || m_stamina < 100) {
                return 1;
            }
            if (this->RectContains(sg->m_10->m_screenX, sg->m_10->m_screenY) == 0) {
                return 1;
            }
            if (sg->m_10->m_screenX != sg->m_lastTilePxX
                || sg->m_10->m_screenY != sg->m_lastTilePxY) {
                return 1;
            }
            CommitNeighbor(
                sg->m_tileOwnerHi,
                sg->m_tileOwnerLo,
                sg->m_lastTilePxX,
                sg->m_lastTilePxY
            );
            m_defenderState = 2;
            return 1;
        L_clearMode:
            m_defenderState = 0;
            return 1;
        }
        case 2: {
            if (m_poweredUp != 0) {
                CGrunt* sg = m_tileMgr->m_grid[m_arrivalCol * TM_GRID_COLS + m_arrivalRow];
                if (sg == 0) {
                    goto L_setLock;
                }
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, sg->m_entranceReason);
                if (pa > pb) {
                    goto L_setLock;
                }
                if (this->GruntInRadius(sg->m_tileOwnerHi, sg->m_tileOwnerLo) == 0) {
                    goto L_setLock;
                }
                if (sg->m_entranceCommitted == 0) {
                    goto L_setLock;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < 100) {
                    return 1;
                }
                if (this->RectContains(sg->m_10->m_screenX, sg->m_10->m_screenY) == 0) {
                    goto L_setLock;
                }
                if (sg->m_10->m_screenX != sg->m_lastTilePxX
                    || sg->m_10->m_screenY != sg->m_lastTilePxY) {
                    goto L_setLock;
                }
                CommitNeighbor(
                    sg->m_tileOwnerHi,
                    sg->m_tileOwnerLo,
                    sg->m_lastTilePxX,
                    sg->m_lastTilePxY
                );
                m_defenderState = 2;
                return 1;
            L_setLock:
                m_defenderState = 1;
                m_dwell = 0x1f4;
                return 1;
            }
            m_defenderState = 1;
            m_dwell = 0x1f4;
            return 1;
        }
    }
    return 1;
}

#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000f0db0, 0x48)

i32 CellTargetable(i32 tileX, i32 tileY) {
    Coord tile(tileX, tileY);
    CPtrList& list = g_gameReg->m_triggerMgr->m_baseList;
    POSITION pos = list.GetHeadPosition();

    if (pos != NULL) {
        do {
            CGruntPuddle* p = static_cast<CGruntPuddle*>(list.GetNext(pos));
            if (p->m_pending == false) {
                if (p->m_tile == tile) {
                    return 1;
                }
            }
        } while (pos != NULL);
    }
    return 0;
}

RVA(0x000f0e20, 0x928)
i32 CGrunt::StepGooSuckerBehavior() {
    bool eqI = ANIMATION_ACT_EQUALS("I");
    if (eqI) {
        return 1;
    }
    m_defenderPx = m_lastTilePx;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    grid->Clip(NULL);

    Coord selfTile;
    GetScreenTile(&selfTile);

    FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget)

    b32 powered = m_poweredUp;
    if (powered != false) {
        b32 neighborValid = m_neighborValid;
        if (neighborValid == false) {
            if (m_combatActive != false) {
                goto L_yes;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    goto L_yes;
                }
                if (atTarget && g == NULL) {
                    goto L_yes;
                }
                if (m_poweredUp == false) {
                    goto L_yes;
                }
            } else {
                if (atTarget) {
                    goto L_yes;
                }
                if (m_poweredUp == false) {
                    goto L_yes;
                }
            }
            if (m_neighborValid != false) {
                goto L_yes;
            }
            RESET_GRUNT_POWERED_STATE(this)
        } else {
            m_neighborValid = false;
        }
    L_yes:
        return 1;
    }

    if (g != NULL) {
        if (m_neighborValid != false) {
            return 1;
        }
        if (m_combatActive == false && m_stamina >= STAMINA_FULL) {
            if (atTarget) {
                COMMIT_GRUNT_NEIGHBOR(g);
                if (CoordCount() != 0) {
                    RECYCLE_GRUNT_COORDS(this)
                }
                return 1;
            }
        } else {
            if (atTarget) {
                if (CoordCount() != 0) {
                    RECYCLE_GRUNT_COORDS(this)
                }
                return 1;
            }
        }
    } else {
        m_blockedVoicePending = false;
    }

L_ed006b:
    if (g == NULL || GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
        m_blockedVoicePending = false;
        goto L_scanb;
    }
    if (m_poweredUp != false) {
        goto L_scanb;
    }
    if (m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(g)
        && RectContains(g->m_object->m_screenPosition.m_x, g->m_object->m_screenPosition.m_y)
               != 0) {
        COMMIT_GRUNT_NEIGHBOR(g);
    }
    if (m_poweredUp != false) {
        goto L_scanb;
    }
    if (static_cast<u32>(m_dwell) <= DWELL_REPATH_MS) {
        goto L_scanb;
    }
    {
        Coord cc;
        g->GetScreenTile(&cc);
        if (TileSwitch(cc.m_x, cc.m_y, 0, m_arrivalFlags, 1, 0) != 0) {
            if (m_blockedVoicePending != false) {
                Coord voicePosition = m_object->ScreenPos();
                CGruntzMgr* game = g_gameReg;
                if (CGameLevel::PointInBounds(
                        &game->m_world->m_level->m_mainPlane->m_planeViewRect,
                        voicePosition.m_x,
                        voicePosition.m_y
                    )
                    != 0) {
                    game->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = false;
            }
            m_dwell = 0;
        }
    }

L_scanb:
    if (CoordCount() == 0) {
        if (static_cast<u32>(m_dwell) <= DWELL_SEEK_PATH_MS) {
            return 1;
        }

        i32 r = m_defenderRadius;
        CRect box(selfTile.m_x - r, selfTile.m_y - r, selfTile.m_x + r, selfTile.m_y + r);
        CRect gridBounds(0, 0, grid->m_width, grid->m_height);
        CRect isect;
        if (!isect.IntersectRect(&box, &gridBounds)) {
            isect = box;
        }
        grid->Clip(&isect);

        i32 best = INT_MAX;
        Coord bestTile(0, 0);

        POSITION pos = m_triggerMgr->m_baseList.GetHeadPosition();
        while (pos != NULL) {
            CGruntPuddle* gg = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetNext(pos));
            if (gg->m_pending == false) {
                Coord puddleTile = gg->m_tile;
                Coord puddlePosition = puddleTile;
                TileCenter(&puddlePosition);
                if (RectContains(puddlePosition.m_x, puddlePosition.m_y) != 0) {
                    m_triggerMgr->UseEquippedToolAt(
                        m_playerIndex,
                        m_unitIndex,
                        puddlePosition.m_x,
                        puddlePosition.m_y
                    );
                    grid->Clip(NULL);
                    return 1;
                }
                Coord distance = (puddleTile - selfTile).GetAbs();
                i32 dist = distance.m_x + distance.m_y;
                if (dist < best) {
                    if (::PtInRect(&isect, puddleTile.m_x, puddleTile.m_y)) {
                        best = dist;
                        bestTile = puddleTile;
                    }
                }
            }
        }
        if (best != INT_MAX) {
            Coord distance = (bestTile - selfTile).GetAbs();
            if (distance.m_x <= 1 && distance.m_y <= 1) {
                Coord bestPosition = bestTile;
                TileCenter(&bestPosition);
                m_triggerMgr->UseEquippedToolAt(
                    m_playerIndex,
                    m_unitIndex,
                    bestPosition.m_x,
                    bestPosition.m_y
                );
                SetEntrancePos(1, 1);
            } else {
                TileSwitch(bestTile.m_x, bestTile.m_y, 0, m_arrivalFlags, 1, 0);
            }
        }
        grid->Clip(NULL);
    } else {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        Coord targetTile = *coord;
        if (CellTargetable(targetTile.m_x, targetTile.m_y) == 0) {
            return 1;
        }
        Coord targetPosition = targetTile;
        TileCenter(&targetPosition);
        m_triggerMgr
            ->UseEquippedToolAt(m_playerIndex, m_unitIndex, targetPosition.m_x, targetPosition.m_y);
        SetEntrancePos(1, 1);
    }
    m_dwell = 0;
    return 1;
}

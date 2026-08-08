#include <rva.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdFile.h>

// @early-stop
// cl tail-merges the four inlined CDDrawSubMgrLeafScan::PlayCue expansions onto one
// shared LeafCue::PlayIfElapsed call; retail keeps three separate calls and inlines
// the fourth (see docs/patterns/tail-merged-inline-helper-collapses-repeat-cues.md).
RVA(0x00075e90, 0x1329)
i32 CTriggerMgr::LoadTileArrivalFx(
    i32 ownerHi,
    i32 ownerLo,
    i32 tileX,
    i32 tileY,
    PickupType reason,
    TileArrivalFxCue cue
) {
    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
    CGameLevel* grid = m_world->m_level;
    CGrunt* unit = m_grid[ownerHi * TM_GRID_COLS + ownerLo];

    i32 cx = tileX;
    if (tileX < 0) {
        cx = 0;
    } else if (tileX >= grid->m_mainPlane->m_gridW) {
        cx = grid->m_mainPlane->m_gridW - 1;
    }
    i32 cy = tileY;
    if (tileY < 0) {
        cy = 0;
    } else if (tileY >= grid->m_mainPlane->m_gridH) {
        cy = grid->m_mainPlane->m_gridH - 1;
    }

    TileCollisionKind cellType;
    i32 cell = grid->m_mainPlane->m_tileGrid[grid->m_mainPlane->m_colOffsets[cy] + cx];
    if (cell == UNINIT_FILL || cell == -1) {
        cellType = TILEKIND_PASSABLE;
    } else {
        CTileImageSet* tc = static_cast<CTileImageSet*>(grid->m_imageSets.GetAt(cell & 0xffff));
        // Ingest: the raw WWD attribute byte for this cell.
        cellType = tc->GetCollisionAt(0, 0);
    }

    i32 px = tileX * 32 + 0x10;
    i32 py = tileY * 32 + 0x10;

    switch (reason) {
        case PICKUP_SHOVEL:
            if (cue == TILE_ARRIVAL_FX_END) {
                return 1;
            }
            if (cue == TILE_ARRIVAL_FX_IMPACT) {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    CWwdGameObjectA* set =
                        m_world->m_childGroup
                            ->CreateSprite(0, px, py, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
                    if (set != NULL) {
                        set->ApplyName("LEVEL_DIRT");
                        set->ApplyLookupGeometry("GAME_DIRT", 0);
                    }
                }
                return 1;
            }
            if (cue != TILE_ARRIVAL_FX_APPLY) {
                return 1;
            }

            if (cellType == TILEKIND_COVERED_POWERUP) {
                CTileTriggerLogic* found = state->m_beginMarker->FindInLists12(
                    (tileX << 8) + tileY,
                    TRIGID_COVERED_POWERUP_26
                );
                if (found != NULL) {
                    found->ApplyMove(TILEKIND_COVERED_POWERUP);
                    state->m_beginMarker->DelFromList1(found);
                    return 1;
                }
                // Retail re-reads the cell off the plane rather than reusing the
                // value the head classified, and writes through g_gameReg's copy
                // of the world while reading through this->m_world.
                CGruntzMgr* reg = g_gameReg;
                i32 uncovered =
                    m_world->m_level->m_mainPlane
                        ->m_tileGrid[m_world->m_level->m_mainPlane->m_colOffsets[tileY] + tileX]
                    + 1;
                reg->m_world->m_level->m_mainPlane->SetCell(tileX, tileY, uncovered);
                reg->m_tileGrid->ComputeCellFlags(tileX, tileY, uncovered);
                return 1;
            }
            if (cellType == TILEKIND_REVEALED_POWERUP) {
                i32 recovered =
                    m_world->m_level->m_mainPlane
                        ->m_tileGrid[m_world->m_level->m_mainPlane->m_colOffsets[tileY] + tileX]
                    - 1;
                CDDrawWorkerHost* dst = g_gameReg->m_world->m_level->m_mainPlane;
                dst->m_tileGrid[dst->m_colOffsets[tileY] + tileX] = recovered;
                g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, recovered);
                return 1;
            }
            return 0;

        case PICKUP_GAUNTLETZ:
            if (cue == TILE_ARRIVAL_FX_END) {
                return 1;
            }
            if (cue == TILE_ARRIVAL_FX_IMPACT) {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    switch (cellType) {
                        case TILEKIND_GAUNTLET_ROCK_A:
                        case TILEKIND_GAUNTLET_ROCK_B:
                        case TILEKIND_GIANT_ROCK:
                            m_world->m_soundRegistry->PlayCue("LEVEL_GAUNTLETROCK1");
                            return 1;
                        case TILEKIND_GAUNTLET_BRICK_A:
                        case TILEKIND_GAUNTLET_BRICK_B:
                        case TILEKIND_GAUNTLET_BRICK_C:
                            m_world->m_soundRegistry->PlayCue("GAME_GAUNTLETBRICK1");
                            return 1;
                    }
                    return 0;
                }
                return 1;
            }
            if (cue != TILE_ARRIVAL_FX_APPLY) {
                return 1;
            }

            if (cellType == TILEKIND_GAUNTLET_ROCK_A || cellType == TILEKIND_GAUNTLET_ROCK_B) {
                CTileTriggerLogic* found = state->m_beginMarker->FindInLists12(
                    (tileX << 8) + tileY,
                    TRIGID_COVERED_POWERUP_26
                );
                if (found != NULL) {
                    found->ApplyMove(cellType);
                    state->m_beginMarker->DelFromList1(found);
                } else if (cellType == TILEKIND_GAUNTLET_ROCK_A) {
                    CDDrawWorkerHost* dst = g_gameReg->m_world->m_level->m_mainPlane;
                    dst->m_tileGrid[dst->m_colOffsets[tileY] + tileX] = 0x5a;
                    g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, 0x5a);
                } else {
                    CDDrawWorkerHost* dst = g_gameReg->m_world->m_level->m_mainPlane;
                    dst->m_tileGrid[dst->m_colOffsets[tileY] + tileX] = 0x5b;
                    g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, 0x5b);
                }
            } else if (cellType == TILEKIND_GIANT_ROCK) {
                CGiantRockLogic* rock = state->m_beginMarker->ScanNeighborhood(tileX, tileY);
                if (rock == NULL) {
                    CString diag;
                    diag.Format("No giant rock logic found at: x=%d, y=%d", px, py);
                    g_gameReg->EnterModalUI(static_cast<const char*>(diag));
                    g_gameReg->ReportError(
                        IDX(TRIGERR_LOOKUP_MISS),
                        IDX(TRIGSITE_ARRIVAL_GIANT_ROCK)
                    );
                    return 0;
                }
                rock->BuildRockBreakInGameText();
                state->m_beginMarker->DelFromList1(rock);
                return 1;
            } else if (cellType == TILEKIND_GAUNTLET_BRICK_A
                       || cellType == TILEKIND_GAUNTLET_BRICK_B
                       || cellType == TILEKIND_GAUNTLET_BRICK_C) {
                CTileActionEvent* event =
                    state->m_beginMarker->FindActionByCellKey((tileX << 8) + tileY);
                if (event->Process(unit) != 0) {
                    state->m_beginMarker->DelFromList3(event);
                }
                return 1;
            } else {
                return 0;
            }

            {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    CWwdGameObjectA* particle =
                        m_world->m_childGroup
                            ->CreateSprite(0, px, py, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
                    if (particle != NULL) {
                        particle->ApplyName("LEVEL_ROCKBREAK");
                        particle->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                        m_world->m_soundRegistry->PlayCue("LEVEL_ROCKBREAK");
                    }
                }
            }
            return 1;

        case PICKUP_GOOBER:
            if (cue == TILE_ARRIVAL_FX_APPLY || cue == TILE_ARRIVAL_FX_END) {
                // Retail primes the gauge id before the scan, so a puddle-less
                // arrival still credits the default Grunt.
                i32 gruntType = 25;
                i32 removed = 0;
                POSITION pos = m_baseList.GetHeadPosition();
                while (pos != NULL && removed == 0) {
                    POSITION current = pos;
                    CGruntPuddle* puddle = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
                    if (puddle->m_tileX == tileX && puddle->m_tileY == tileY) {
                        if (cue == TILE_ARRIVAL_FX_END) {
                            puddle->m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                            puddle->SetBute("B");
                            puddle->m_placed = 1;
                            puddle->m_pending = 0;
                            puddle->m_value = puddle->m_object->m_animCursor.m_animation;
                            puddle->m_object->ApplyLookupGeometry(g_puddleSpriteKey, 0);
                            return 1;
                        }
                        gruntType = puddle->m_gruntType;
                        puddle->m_object->m_flags |= 0x10000;
                        m_baseList.RemoveAt(current);
                        removed = 1;
                    }
                }
                if (removed != 0 && ownerHi == g_curPlayer) {
                    static_cast<CPlay*>(g_gameReg->m_curState)->m_guts->AdvanceGauge(gruntType);
                }
            }
            return 1;

        case PICKUP_SPY:
            if (cue == TILE_ARRIVAL_FX_APPLY) {
                for (i32 radius = 1; radius <= 2; radius++) {
                    i32 topY = tileY - radius;
                    i32 bottomY = tileY + radius;
                    for (i32 scanX = tileX - radius; scanX <= tileX + radius; scanX++) {
                        if (state->m_beginMarker->SetCell(scanX, topY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = scanX * 0x20 + 0x10;
                            i32 fxY = topY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                m_world->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 1000000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        AddrWord<char> objectKey;
                        objectKey.m_word = 0;
                        if (static_cast<u32>(scanX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(topY) < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[topY][scanX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            g_gameReg->m_world->m_childGroup->m_map48.Lookup(
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &= ~0x40000;
                                }
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = scanX * 0x20 + 0x10;
                                        i32 fxY = topY * 0x20 + 0x10;
                                        CWwdGameObjectA* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fxX,
                                                fxY,
                                                1000000,
                                                "LightFx",
                                                0x40003
                                            );
                                        light->m_animWorker->m_notify(light);
                                        static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                1
                                            );
                                        CWwdGameObjectA* peek = m_world->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            900000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }

                        if (state->m_beginMarker->SetCell(scanX, bottomY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = scanX * 0x20 + 0x10;
                            i32 fxY = bottomY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                m_world->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 1000000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        objectKey.m_word = 0;
                        if (static_cast<u32>(scanX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(bottomY) < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[bottomY][scanX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            g_gameReg->m_world->m_childGroup->m_map48.Lookup(
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &= ~0x40000;
                                }
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = scanX * 0x20 + 0x10;
                                        i32 fxY = bottomY * 0x20 + 0x10;
                                        CWwdGameObjectA* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fxX,
                                                fxY,
                                                1000000,
                                                "LightFx",
                                                0x40003
                                            );
                                        light->m_animWorker->m_notify(light);
                                        static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                1
                                            );
                                        CWwdGameObjectA* peek = m_world->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            1000000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    i32 leftX = tileX - radius;
                    i32 rightX = tileX + radius;
                    for (i32 scanY = tileY - radius + 1; scanY < tileY + radius; scanY++) {
                        if (state->m_beginMarker->SetCell(leftX, scanY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = leftX * 0x20 + 0x10;
                            i32 fxY = scanY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                m_world->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 900000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        AddrWord<char> objectKey;
                        objectKey.m_word = 0;
                        if (static_cast<u32>(leftX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(scanY) < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[scanY][leftX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            g_gameReg->m_world->m_childGroup->m_map48.Lookup(
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &= ~0x40000;
                                }
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = leftX * 0x20 + 0x10;
                                        i32 fxY = scanY * 0x20 + 0x10;
                                        CWwdGameObjectA* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fxX,
                                                fxY,
                                                1000000,
                                                "LightFx",
                                                0x40003
                                            );
                                        light->m_animWorker->m_notify(light);
                                        static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                1
                                            );
                                        CWwdGameObjectA* peek = m_world->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            1000000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }

                        if (state->m_beginMarker->SetCell(rightX, scanY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = rightX * 0x20 + 0x10;
                            i32 fxY = scanY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                m_world->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 1000000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        objectKey.m_word = 0;
                        if (static_cast<u32>(rightX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(scanY) < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[scanY][rightX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            g_gameReg->m_world->m_childGroup->m_map48.Lookup(
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &= ~0x40000;
                                }
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = rightX * 0x20 + 0x10;
                                        i32 fxY = scanY * 0x20 + 0x10;
                                        CWwdGameObjectA* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fxX,
                                                fxY,
                                                1000000,
                                                "LightFx",
                                                0x40003
                                            );
                                        light->m_animWorker->m_notify(light);
                                        static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                1
                                            );
                                        CWwdGameObjectA* peek = m_world->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            1000000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 1;

        case PICKUP_BRICK:
            if (cue != TILE_ARRIVAL_FX_APPLY) {
                return 1;
            }
            if (cellType == TILEKIND_HIDDEN_POWERUP) {
                BrickTileId actionCode;
                switch (unit->m_brickPickupType) {
                    case PICKUP_REDBRICK:
                        actionCode = BRICKTILE_RED_1;
                        break;
                    case PICKUP_GOLDBRICK:
                        actionCode = BRICKTILE_GOLD_1;
                        break;
                    case PICKUP_BLUEBRICK:
                        actionCode = BRICKTILE_BLUE_1;
                        break;
                    case PICKUP_BLACKBRICK:
                        actionCode = BRICKTILE_BLACK_1;
                        break;
                    default:
                        actionCode = BRICKTILE_BROWN_1;
                        break;
                }
                if (state->m_beginMarker
                        ->AddToList3Switch(actionCode, tileX, tileY, (tileX << 8) + tileY, ownerHi)
                    == NULL) {
                    return 0;
                }
                unit->m_pendingTriggerPx.m_x = px;
                unit->m_brickPickupType = PICKUP_BROWNBRICK;
                unit->m_entrancePickup = PICKUP_INVALID;
                unit->m_pendingTriggerPx.m_y = py;
                unit->m_pendingTrigger = 1;
                return 1;
            }
            if (cellType == TILEKIND_GAUNTLET_BRICK_A || cellType == TILEKIND_GAUNTLET_BRICK_B) {
                CTileActionEvent* event =
                    state->m_beginMarker->FindActionByCellKey((tileX << 8) + tileY);
                if (event->MorphByTool(unit->m_brickPickupType, static_cast<PlayerSlot>(ownerHi))
                    == 0) {
                    return 0;
                }
                unit->m_brickPickupType = PICKUP_BROWNBRICK;
                unit->m_entrancePickup = PICKUP_INVALID;
                if (cellType == TILEKIND_GAUNTLET_BRICK_A) {
                    unit->m_pendingTriggerPx.m_x = px;
                    unit->m_pendingTriggerPx.m_y = py;
                    unit->m_pendingTrigger = 1;
                }
                return 1;
            }
            return 0;

        case PICKUP_TOOB: {
            if (cue != TILE_ARRIVAL_FX_APPLY) {
                return 1;
            }
            i32 waterX = unit->m_object->m_screenX;
            i32 waterY = unit->m_object->m_screenY;
            // Retail spells the viewport test out here rather than calling PtInRect.
            if (CGameLevel::PointInBounds(&g_gameReg->m_viewBounds, waterX, waterY)) {
                CWwdGameObjectA* splash = m_world->m_childGroup->CreateSprite(
                    0,
                    waterX,
                    waterY,
                    SORTKEY_ACTOR_BEHIND,
                    "Particlez",
                    0x40003
                );
                if (splash != NULL) {
                    splash->ApplyName("GAME_WATER");
                    splash->ApplyLookupGeometry("GAME_WATER", 0);
                    m_world->m_soundRegistry->PlayCue("GAME_WATERSPLASH");
                }
            }
            return 1;
        }
    }
    return 0;
}

#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <AddrWord.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/LogicRecord.h>
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
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

RVA(0x00075e90, 0x1400)
i32 CTriggerMgr::LoadTileArrivalFx(
    i32 playerIndex,
    i32 unitIndex,
    i32 tileX,
    i32 tileY,
    PickupType reason,
    WwdAniDrawValue cue
) {
    CGrunt* unit = m_units[playerIndex * TM_UNITS_PER_PLAYER + unitIndex];
    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
    CGameLevel* grid = m_world->m_level;

    Coord tile(tileX, tileY);
    Coord clamped = tile;
    clamped.Max(Coord(0, 0));
    clamped.Min(
        Coord(grid->m_mainPlane->m_tileGridSize.cx - 1, grid->m_mainPlane->m_tileGridSize.cy - 1)
    );

    TileCollisionKind cellType;
    i32 cell = grid->m_mainPlane
                   ->m_tileHandles[grid->m_mainPlane->m_tileRowOffsets[clamped.m_y] + clamped.m_x];
    if (cell == UNINIT_FILL || cell == -1) {
        cellType = TILEKIND_PASSABLE;
    } else {
        CTileImageSet* tc = static_cast<CTileImageSet*>(
            grid->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
        );
        cellType = tc->GetCollisionAt(0, 0);
    }

    Coord pixel = tile;
    TileCenter(&pixel);

    switch (reason) {
        case PICKUP_SHOVEL:
            if (cue == WWDDRAW_NO_ANIMATION) {
                return 1;
            }
            if (cue == WWDDRAW_EFFECT_FRAME) {
                CPoint pt(pixel.m_x, pixel.m_y);
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    CWwdSpriteObject* set = m_world->m_childGroup->CreateSprite(
                        0,
                        pixel.m_x,
                        pixel.m_y,
                        SORTKEY_ACTOR_BEHIND,
                        "Particlez",
                        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                    );
                    if (set != NULL) {
                        set->SetImageSetByName("LEVEL_DIRT");
                        set->SetAnimationByName("GAME_DIRT", 0);
                    }
                }
                return 1;
            }
            if (cue != WWDDRAW_TOOL_APPLIES) {
                return 1;
            }

            if (cellType == TILEKIND_COVERED_POWERUP) {
                CTileTriggerLogic* found = state->m_tileTriggers->FindLogic(
                    (tileX << 8) + tileY,
                    TRIGID_COVERED_POWERUP_26
                );
                if (found != NULL) {
                    found->ApplyMove(TILEKIND_COVERED_POWERUP);
                    state->m_tileTriggers->RemoveIdleLogic(found);
                    return 1;
                }
                CGruntzMgr* reg = g_gameReg;
                i32 uncovered = m_world->m_level->m_mainPlane->m_tileHandles
                                    [m_world->m_level->m_mainPlane->m_tileRowOffsets[tileY] + tileX]
                                + 1;
                reg->m_world->m_level->m_mainPlane->SetCell(tileX, tileY, uncovered);
                reg->m_tileGrid->ComputeCellFlags(tileX, tileY, uncovered);
                return 1;
            }
            if (cellType == TILEKIND_REVEALED_POWERUP) {
                i32 recovered = m_world->m_level->m_mainPlane->m_tileHandles
                                    [m_world->m_level->m_mainPlane->m_tileRowOffsets[tileY] + tileX]
                                - 1;
                CDDrawWorkerHost* dst = g_gameReg->m_world->m_level->m_mainPlane;
                SET_WORKER_HOST_CELL(dst, tileX, tileY, recovered);
                g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, recovered);
                return 1;
            }
            return 0;

        case PICKUP_GAUNTLETZ:
            if (cue == WWDDRAW_NO_ANIMATION) {
                return 1;
            }
            if (cue == WWDDRAW_EFFECT_FRAME) {
                CPoint pt(pixel.m_x, pixel.m_y);
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
            if (cue != WWDDRAW_TOOL_APPLIES) {
                return 1;
            }

            if (cellType == TILEKIND_GAUNTLET_ROCK_A || cellType == TILEKIND_GAUNTLET_ROCK_B) {
                CTileTriggerLogic* found = state->m_tileTriggers->FindLogic(
                    (tileX << 8) + tileY,
                    TRIGID_COVERED_POWERUP_26
                );
                if (found != NULL) {
                    found->ApplyMove(cellType);
                    state->m_tileTriggers->RemoveIdleLogic(found);
                } else if (cellType == TILEKIND_GAUNTLET_ROCK_A) {
                    CDDrawWorkerHost* dst = g_gameReg->m_world->m_level->m_mainPlane;
                    SET_WORKER_HOST_CELL(dst, tileX, tileY, 0x5a);
                    g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, 0x5a);
                } else {
                    CDDrawWorkerHost* dst = g_gameReg->m_world->m_level->m_mainPlane;
                    SET_WORKER_HOST_CELL(dst, tileX, tileY, 0x5b);
                    g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, 0x5b);
                }
            } else if (cellType == TILEKIND_GIANT_ROCK) {
                CGiantRockLogic* rock = state->m_tileTriggers->ScanNeighborhood(tileX, tileY);
                if (rock == NULL) {
                    CString diag;
                    diag.Format("No giant rock logic found at: x=%d, y=%d", pixel.m_x, pixel.m_y);
                    g_gameReg->EnterModalUI(static_cast<const char*>(diag));
                    g_gameReg->ReportError(
                        IDX(TRIGERR_LOOKUP_MISS),
                        IDX(TRIGSITE_ARRIVAL_GIANT_ROCK)
                    );
                    return 0;
                }
                rock->BuildRockBreakInGameText();
                state->m_tileTriggers->RemoveIdleLogic(rock);
                return 1;
            } else if (cellType == TILEKIND_GAUNTLET_BRICK_A
                       || cellType == TILEKIND_GAUNTLET_BRICK_B
                       || cellType == TILEKIND_GAUNTLET_BRICK_C) {
                CTileActionEvent* event =
                    state->m_tileTriggers->FindActionByCellKey((tileX << 8) + tileY);
                if (event->BreakTopBrick(unit) != 0) {
                    state->m_tileTriggers->RemoveActionEvent(event);
                }
                return 1;
            } else {
                return 0;
            }

            {
                CPoint pt(pixel.m_x, pixel.m_y);
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    CWwdSpriteObject* particle = m_world->m_childGroup->CreateSprite(
                        0,
                        pixel.m_x,
                        pixel.m_y,
                        SORTKEY_ACTOR_BEHIND,
                        "Particlez",
                        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                    );
                    if (particle != NULL) {
                        particle->SetImageSetByName("LEVEL_ROCKBREAK");
                        particle->SetAnimationByName("LEVEL_ROCKBREAK", 0);
                        m_world->m_soundRegistry->PlayCue("LEVEL_ROCKBREAK");
                    }
                }
            }
            return 1;

        case PICKUP_GOOBER:
            if (cue == WWDDRAW_TOOL_APPLIES || cue == WWDDRAW_NO_ANIMATION) {
                i32 gaugePoints = 25;
                i32 removed = 0;
                POSITION pos = m_baseList.GetHeadPosition();
                while (pos != NULL && removed == 0) {
                    POSITION current = pos;
                    CGruntPuddle* puddle = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
                    if (puddle->m_tile == tile) {
                        if (cue == WWDDRAW_NO_ANIMATION) {
                            puddle->m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                            puddle->SetBute("B");
                            puddle->m_placed = true;
                            puddle->m_pending = false;
                            puddle->m_value = puddle->m_wwdObject->m_animationCursor.m_animation;
                            puddle->m_wwdObject->SetAnimationByName(g_puddleSpriteKey, 0);
                            return 1;
                        }
                        gaugePoints = puddle->m_gaugePoints;
                        puddle->m_wwdObject->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                        m_baseList.RemoveAt(current);
                        removed = 1;
                    }
                }
                if (removed != 0 && playerIndex == g_curPlayer) {
                    static_cast<CPlay*>(g_gameReg->m_curState)
                        ->m_statusBar->AdvanceGruntWell(gaugePoints);
                }
            }
            return 1;

        case PICKUP_SPY:
            if (cue == WWDDRAW_TOOL_APPLIES) {
                for (i32 radius = 1; radius <= 2; radius++) {
                    CRect revealBounds(
                        tileX - radius,
                        tileY - radius,
                        tileX + radius,
                        tileY + radius
                    );
                    for (i32 scanX = tileX - radius; scanX <= tileX + radius; scanX++) {
                        if (state->m_tileTriggers->SetCell(scanX, revealBounds.top, playerIndex)
                                != 0
                            && playerIndex == g_curPlayer) {
                            Coord fx(scanX, revealBounds.top);
                            TileCenter(&fx);
                            CWwdSpriteObject* light = m_world->m_childGroup->CreateSprite(
                                0,
                                fx.m_x,
                                fx.m_y,
                                1000000,
                                "LightFx",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            light->m_logicRecord->m_dispatch(light);
                            static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, true);
                        }

                        AddrWord<char> objectKey;
                        objectKey.m_word = 0;
                        if (static_cast<u32>(scanX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(revealBounds.top)
                                   < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[revealBounds.top][scanX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            CWwdGameObject* mapped = NULL;
                            MapLookup(
                                g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &=
                                        ~IDX(CELL_FLAG_IN_GAME_ICON);
                                }
                            } else {
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(mapped->m_logicRecord->m_userLogic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = playerIndex;
                                    icon->HandleInput();
                                    if (playerIndex == g_curPlayer) {
                                        Coord fx(scanX, revealBounds.top);
                                        TileCenter(&fx);
                                        CWwdSpriteObject* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                1000000,
                                                "LightFx",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        light->m_logicRecord->m_dispatch(light);
                                        static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                true
                                            );
                                        CWwdSpriteObject* peek =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                900000,
                                                "ToyPeek",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }

                        if (state->m_tileTriggers->SetCell(scanX, revealBounds.bottom, playerIndex)
                                != 0
                            && playerIndex == g_curPlayer) {
                            Coord fx(scanX, revealBounds.bottom);
                            TileCenter(&fx);
                            CWwdSpriteObject* light = m_world->m_childGroup->CreateSprite(
                                0,
                                fx.m_x,
                                fx.m_y,
                                1000000,
                                "LightFx",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            light->m_logicRecord->m_dispatch(light);
                            static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, true);
                        }

                        objectKey.m_word = 0;
                        if (static_cast<u32>(scanX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(revealBounds.bottom)
                                   < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[revealBounds.bottom][scanX]
                                    .m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            CWwdGameObject* mapped = NULL;
                            MapLookup(
                                g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &=
                                        ~IDX(CELL_FLAG_IN_GAME_ICON);
                                }
                            } else {
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(mapped->m_logicRecord->m_userLogic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = playerIndex;
                                    icon->HandleInput();
                                    if (playerIndex == g_curPlayer) {
                                        Coord fx(scanX, revealBounds.bottom);
                                        TileCenter(&fx);
                                        CWwdSpriteObject* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                1000000,
                                                "LightFx",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        light->m_logicRecord->m_dispatch(light);
                                        static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                true
                                            );
                                        CWwdSpriteObject* peek =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                900000,
                                                "ToyPeek",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    for (i32 scanY = revealBounds.top + 1; scanY < revealBounds.bottom; scanY++) {
                        if (state->m_tileTriggers->SetCell(revealBounds.left, scanY, playerIndex)
                                != 0
                            && g_curPlayer == playerIndex) {
                            Coord fx(revealBounds.left, scanY);
                            TileCenter(&fx);
                            CWwdSpriteObject* light = m_world->m_childGroup->CreateSprite(
                                0,
                                fx.m_x,
                                fx.m_y,
                                900000,
                                "LightFx",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            light->m_logicRecord->m_dispatch(light);
                            static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, true);
                        }

                        AddrWord<char> objectKey;
                        objectKey.m_word = 0;
                        if (static_cast<u32>(revealBounds.left) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(scanY) < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[scanY][revealBounds.left].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            CWwdGameObject* mapped = NULL;
                            MapLookup(
                                g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &=
                                        ~IDX(CELL_FLAG_IN_GAME_ICON);
                                }
                            } else {
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(mapped->m_logicRecord->m_userLogic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = playerIndex;
                                    icon->HandleInput();
                                    if (playerIndex == g_curPlayer) {
                                        Coord fx(revealBounds.left, scanY);
                                        TileCenter(&fx);
                                        CWwdSpriteObject* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                1000000,
                                                "LightFx",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        light->m_logicRecord->m_dispatch(light);
                                        static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                true
                                            );
                                        CWwdSpriteObject* peek =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                1000000,
                                                "ToyPeek",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        if (peek != NULL) {
                                            peek->m_smarts = icon->m_object->m_points;
                                        }
                                    }
                                }
                            }
                        }

                        if (state->m_tileTriggers->SetCell(revealBounds.right, scanY, playerIndex)
                                != 0
                            && playerIndex == g_curPlayer) {
                            Coord fx(revealBounds.right, scanY);
                            TileCenter(&fx);
                            CWwdSpriteObject* light = m_world->m_childGroup->CreateSprite(
                                0,
                                fx.m_x,
                                fx.m_y,
                                1000000,
                                "LightFx",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            light->m_logicRecord->m_dispatch(light);
                            static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, true);
                        }

                        objectKey.m_word = 0;
                        if (static_cast<u32>(revealBounds.right) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(scanY) < g_gameReg->m_tileGrid->m_height) {
                            objectKey.m_word =
                                g_gameReg->m_tileGrid->m_rows[scanY][revealBounds.right].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            CWwdGameObject* mapped = NULL;
                            MapLookup(
                                g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                                objectKey.m_addr,
                                mapped
                            );
                            if (mapped == NULL) {
                                if (static_cast<u32>(tileX) < g_gameReg->m_tileGrid->m_width
                                    && static_cast<u32>(tileY) < g_gameReg->m_tileGrid->m_height) {
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_objectId = 0;
                                    g_gameReg->m_tileGrid->m_rows[tileY][tileX].m_flags &=
                                        ~IDX(CELL_FLAG_IN_GAME_ICON);
                                }
                            } else {
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(mapped->m_logicRecord->m_userLogic);
                                if (icon->m_object->m_smarts == IDX(PICKUP_TOYBOX)) {
                                    icon->m_object->m_score = playerIndex;
                                    icon->HandleInput();
                                    if (playerIndex == g_curPlayer) {
                                        Coord fx(revealBounds.right, scanY);
                                        TileCenter(&fx);
                                        CWwdSpriteObject* light =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                1000000,
                                                "LightFx",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                            );
                                        light->m_logicRecord->m_dispatch(light);
                                        static_cast<CLightFx*>(light->m_logicRecord->m_userLogic)
                                            ->Activate(
                                                "GAME_LIGHTING_HIDDENITEM",
                                                "GAME_HIDDENITEM",
                                                2,
                                                true
                                            );
                                        CWwdSpriteObject* peek =
                                            m_world->m_childGroup->CreateSprite(
                                                0,
                                                fx.m_x,
                                                fx.m_y,
                                                1000000,
                                                "ToyPeek",
                                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
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
            if (cue != WWDDRAW_TOOL_APPLIES) {
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
                if (state->m_tileTriggers->AddSwitchActionEvent(
                        actionCode,
                        tileX,
                        tileY,
                        (tileX << 8) + tileY,
                        playerIndex
                    )
                    == NULL) {
                    return 0;
                }
                unit->m_pendingTriggerPx = pixel;
                unit->m_brickPickupType = PICKUP_BROWNBRICK;
                unit->m_entrancePickup = PICKUP_INVALID;
                unit->m_pendingTrigger = true;
                return 1;
            }
            if (cellType == TILEKIND_GAUNTLET_BRICK_A || cellType == TILEKIND_GAUNTLET_BRICK_B) {
                CTileActionEvent* event =
                    state->m_tileTriggers->FindActionByCellKey((tileX << 8) + tileY);
                if (event
                        ->MorphByTool(unit->m_brickPickupType, static_cast<PlayerSlot>(playerIndex))
                    == 0) {
                    return 0;
                }
                unit->m_brickPickupType = PICKUP_BROWNBRICK;
                unit->m_entrancePickup = PICKUP_INVALID;
                if (cellType == TILEKIND_GAUNTLET_BRICK_A) {
                    unit->m_pendingTriggerPx = pixel;
                    unit->m_pendingTrigger = true;
                }
                return 1;
            }
            return 0;

        case PICKUP_TOOB: {
            if (cue != WWDDRAW_TOOL_APPLIES) {
                return 1;
            }
            Coord waterPosition = unit->m_object->ScreenPos();
            if (::PtInRect(&g_gameReg->m_viewBounds, waterPosition.m_x, waterPosition.m_y)) {
                CWwdSpriteObject* splash = m_world->m_childGroup->CreateSprite(
                    0,
                    waterPosition.m_x,
                    waterPosition.m_y,
                    SORTKEY_ACTOR_BEHIND,
                    "Particlez",
                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                );
                if (splash != NULL) {
                    splash->SetImageSetByName("GAME_WATER");
                    splash->SetAnimationByName("GAME_WATER", 0);
                    m_world->m_soundRegistry->PlayCue("GAME_WATERSPLASH");
                }
            }
            return 1;
        }
    }
    return 0;
}

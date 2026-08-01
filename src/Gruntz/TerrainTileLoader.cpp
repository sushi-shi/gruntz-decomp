

#include <Mfc.h>
#include <AddrWord.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>

#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Play.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/WwdFile.h>
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
    CString diag;

    CDDrawSurfaceMgr* level = m_world;
    CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
    CGameLevel* grid = level->m_level;
    CDDrawWorkerHost* plane = grid->m_mainPlane;
    CGrunt* unit = m_grid[ownerHi * TM_GRID_COLS + ownerLo];

    i32 cx = tileX;
    if (tileX < 0) {
        cx = 0;
    } else if (tileX >= plane->m_gridW) {
        cx = plane->m_gridW - 1;
    }
    i32 cy;
    if (tileY < 0) {
        cy = 0;
    } else if (tileY >= plane->m_gridH) {
        cy = plane->m_gridH - 1;
    } else {
        cy = tileY;
    }

    i32 cellType;
    i32 cell = plane->m_tileGrid[plane->m_colOffsets[cy] + cx];
    if (cell == static_cast<i32>(0xeeeeeeee) || cell == -1) {
        cellType = 0;
    } else {
        CTileImageSet* tc = static_cast<CTileImageSet*>(grid->m_imageSets.GetAt(cell & 0xffff));
        cellType = tc->GetCollisionAt(0, 0);
    }

    i32 px = tileX * 32 + 0x10;
    i32 py = tileY * 32 + 0x10;
    CTileTriggerContainer* triggers = state->m_beginMarker;
    i32 cellKey = (tileX << 8) + tileY;

    switch (reason) {
        case 3:
            if (sel != 0x63 || unit == 0) {
                return 1;
            }
            if (cellType == TILEKIND_HIDDEN_POWERUP) {
                i32 actionCode;
                switch (unit->m_toyBlendPct) {
                    case 0x23:
                        actionCode = 0x132;
                        break;
                    case 0x24:
                        actionCode = 0x138;
                        break;
                    case 0x25:
                        actionCode = 0x13e;
                        break;
                    case 0x26:
                        actionCode = 0x144;
                        break;
                    default:
                        actionCode = 0x12f;
                        break;
                }
                if (triggers->AddToList3Switch(actionCode, tileX, tileY, cellKey, ownerHi) != 0) {
                    unit->m_458 = px;
                    unit->m_toyBlendPct = TILEKIND_COVERED_POWERUP;
                    unit->m_moveMode = -1;
                    unit->m_45c = py;
                    unit->m_454 = 1;
                }
            } else if (cellType == TILEKIND_GAUNTLET_BRICK_A
                       || cellType == TILEKIND_GAUNTLET_BRICK_B) {
                CTileActionEvent* event = triggers->FindByField0C(cellKey);
                if (event != 0 && event->MorphByTool(unit->m_toyBlendPct, ownerHi) != 0) {
                    unit->m_toyBlendPct = TILEKIND_COVERED_POWERUP;
                    unit->m_moveMode = -1;
                    if (cellType == TILEKIND_GAUNTLET_BRICK_A) {
                        unit->m_458 = px;
                        unit->m_45c = py;
                        unit->m_454 = 1;
                    }
                }
            }
            return 1;

        case 5:
            if (sel == -1) {
                return 1;
            }
            if (sel == 2) {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    if (cellType == TILEKIND_GAUNTLET_ROCK_A || cellType == TILEKIND_GAUNTLET_ROCK_B
                        || cellType == TILEKIND_GIANT_ROCK) {
                        level->m_soundRegistry->RefreshAsset("LEVEL_GAUNTLETROCK1");
                    } else if (cellType == TILEKIND_GAUNTLET_BRICK_A
                               || cellType == TILEKIND_GAUNTLET_BRICK_B
                               || cellType == TILEKIND_GAUNTLET_BRICK_C) {
                        level->m_soundRegistry->RefreshAsset("GAME_GAUNTLETBRICK1");
                    }
                }
                return 1;
            }
            if (sel != 0x63) {
                return 1;
            }

            if (cellType == TILEKIND_GAUNTLET_ROCK_A || cellType == TILEKIND_GAUNTLET_ROCK_B) {
                CTileTriggerLogic* found =
                    triggers->FindInLists12(cellKey, TRIGID_COVERED_POWERUP_26);
                if (found == 0) {
                    i32 replacement = cellType == TILEKIND_GAUNTLET_ROCK_A ? 0x5a : 0x5b;
                    plane->SetCell(tileX, tileY, replacement);
                    g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, replacement);
                } else {
                    found->ApplyMove(cellType);
                    triggers->DelFromList1(found);
                }
            } else if (cellType == TILEKIND_GIANT_ROCK) {
                CGiantRockLogic* rock = triggers->ScanNeighborhood(tileX, tileY);
                if (rock == 0) {
                    diag.Format("No giant rock logic found at: x=%d, y=%d", px, py);
                    g_gameReg->EnterModalUI(static_cast<const char*>(diag));
                    g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_ARRIVAL_GIANT_ROCK);
                    return 0;
                }
                rock->BuildRockBreakInGameText();
                triggers->DelFromList1(rock);
                return 1;
            } else if (cellType == TILEKIND_GAUNTLET_BRICK_A
                       || cellType == TILEKIND_GAUNTLET_BRICK_B
                       || cellType == TILEKIND_GAUNTLET_BRICK_C) {
                CTileActionEvent* event = triggers->FindByField0C(cellKey);
                if (event != 0 && event->Process(unit) != 0) {
                    triggers->DelFromList3(event);
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
                        level->m_childGroup->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
                    if (particle != 0) {
                        particle->ApplyName("LEVEL_ROCKBREAK");
                        particle->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                    }
                    level->m_soundRegistry->RefreshAsset("LEVEL_ROCKBREAK");
                }
            }
            return 1;

        case 7: {
            POSITION pos = m_baseList.GetHeadPosition();
            while (pos != 0) {
                POSITION current = pos;
                CGruntPuddle* puddle = static_cast<CGruntPuddle*>(m_baseList.GetNext(pos));
                if (puddle->m_tileX != tileX || puddle->m_tileY != tileY) {
                    continue;
                }
                if (sel == -1) {
                    puddle->m_object->m_stateFlags &= ~1;
                    puddle->SetBute("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2");
                    puddle->m_pending = 1;
                    puddle->m_placed = 0;
                    puddle->m_value = puddle->m_object->m_1a0.m_14;
                    puddle->m_object->ApplyLookupGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2", 0);
                    return 1;
                }
                i32 gruntType = puddle->m_gruntType;
                puddle->m_object->m_flags |= 0x10000;
                m_baseList.RemoveAt(current);
                if (ownerHi == g_curPlayer) {
                    state->m_guts->AdvanceGauge(gruntType);
                }
                return 1;
            }
            return 1;
        }

        case 13:
            if (sel == -1) {
                return 1;
            }
            if (sel == 2) {
                POINT pt;
                pt.x = px;
                pt.y = py;
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
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

            if (cellType == TILEKIND_COVERED_POWERUP) {
                CTileTriggerLogic* found =
                    triggers->FindInLists12(cellKey, TRIGID_COVERED_POWERUP_26);
                if (found != 0) {
                    found->ApplyMove(0x22);
                    triggers->DelFromList1(found);
                } else {
                    i32 replacement = cell + 1;
                    plane->SetCell(tileX, tileY, replacement);
                    g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, replacement);
                }
            } else if (cellType == TILEKIND_REVEALED_POWERUP) {
                i32 replacement = cell - 1;
                plane->SetCell(tileX, tileY, replacement);
                g_gameReg->m_tileGrid->ComputeCellFlags(tileX, tileY, replacement);
            }
            return 1;

        case 15:
            if (sel == 0x63) {
                CMapMgr* pathGrid = g_gameReg->m_tileGrid;
                for (i32 radius = 1; radius <= 2; radius++) {
                    i32 topY = tileY - radius;
                    i32 bottomY = tileY + radius;
                    for (i32 scanX = tileX - radius; scanX <= tileX + radius; scanX++) {
                        if (triggers->SetCell(scanX, topY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = scanX * 0x20 + 0x10;
                            i32 fxY = topY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                level->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 1000000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        AddrWord objectKey;
                        objectKey.m_word = 0;
                        if (static_cast<u32>(scanX) < pathGrid->m_width
                            && static_cast<u32>(topY) < pathGrid->m_height) {
                            objectKey.m_word = pathGrid->m_rows[topY][scanX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            level->m_childGroup->m_map48.Lookup(objectKey.m_addr, mapped);
                            if (mapped == 0) {
                                pathGrid->m_rows[tileY][tileX].m_objectId = 0;
                                pathGrid->m_rows[tileY][tileX].m_0 &= ~0x40000;
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_124 == 0x55) {
                                    icon->m_object->m_114 = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = scanX * 0x20 + 0x10;
                                        i32 fxY = topY * 0x20 + 0x10;
                                        CWwdGameObjectA* light = level->m_childGroup->CreateSprite(
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
                                        CWwdGameObjectA* peek = level->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            900000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != 0) {
                                            peek->m_124 = icon->m_object->m_118;
                                        }
                                    }
                                }
                            }
                        }

                        if (triggers->SetCell(scanX, bottomY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = scanX * 0x20 + 0x10;
                            i32 fxY = bottomY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                level->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 1000000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        objectKey.m_word = 0;
                        if (static_cast<u32>(scanX) < pathGrid->m_width
                            && static_cast<u32>(bottomY) < pathGrid->m_height) {
                            objectKey.m_word = pathGrid->m_rows[bottomY][scanX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            level->m_childGroup->m_map48.Lookup(objectKey.m_addr, mapped);
                            if (mapped == 0) {
                                pathGrid->m_rows[tileY][tileX].m_objectId = 0;
                                pathGrid->m_rows[tileY][tileX].m_0 &= ~0x40000;
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_124 == 0x55) {
                                    icon->m_object->m_114 = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = scanX * 0x20 + 0x10;
                                        i32 fxY = bottomY * 0x20 + 0x10;
                                        CWwdGameObjectA* light = level->m_childGroup->CreateSprite(
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
                                        CWwdGameObjectA* peek = level->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            1000000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != 0) {
                                            peek->m_124 = icon->m_object->m_118;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    i32 leftX = tileX - radius;
                    i32 rightX = tileX + radius;
                    for (i32 scanY = tileY - radius + 1; scanY < tileY + radius; scanY++) {
                        if (triggers->SetCell(leftX, scanY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = leftX * 0x20 + 0x10;
                            i32 fxY = scanY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                level->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 900000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        AddrWord objectKey;
                        objectKey.m_word = 0;
                        if (static_cast<u32>(leftX) < pathGrid->m_width
                            && static_cast<u32>(scanY) < pathGrid->m_height) {
                            objectKey.m_word = pathGrid->m_rows[scanY][leftX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            level->m_childGroup->m_map48.Lookup(objectKey.m_addr, mapped);
                            if (mapped == 0) {
                                pathGrid->m_rows[tileY][tileX].m_objectId = 0;
                                pathGrid->m_rows[tileY][tileX].m_0 &= ~0x40000;
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_124 == 0x55) {
                                    icon->m_object->m_114 = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = leftX * 0x20 + 0x10;
                                        i32 fxY = scanY * 0x20 + 0x10;
                                        CWwdGameObjectA* light = level->m_childGroup->CreateSprite(
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
                                        CWwdGameObjectA* peek = level->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            1000000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != 0) {
                                            peek->m_124 = icon->m_object->m_118;
                                        }
                                    }
                                }
                            }
                        }

                        if (triggers->SetCell(rightX, scanY, ownerHi) != 0
                            && ownerHi == g_curPlayer) {
                            i32 fxX = rightX * 0x20 + 0x10;
                            i32 fxY = scanY * 0x20 + 0x10;
                            CWwdGameObjectA* light =
                                level->m_childGroup
                                    ->CreateSprite(0, fxX, fxY, 1000000, "LightFx", 0x40003);
                            light->m_animWorker->m_notify(light);
                            static_cast<CLightFx*>(light->m_animWorker->m_logic)
                                ->Activate("GAME_LIGHTING_HIDDENITEM", "GAME_HIDDENITEM", 2, 1);
                        }

                        objectKey.m_word = 0;
                        if (static_cast<u32>(rightX) < pathGrid->m_width
                            && static_cast<u32>(scanY) < pathGrid->m_height) {
                            objectKey.m_word = pathGrid->m_rows[scanY][rightX].m_objectId;
                        }
                        if (objectKey.m_word != 0) {
                            void* mapped = 0;
                            level->m_childGroup->m_map48.Lookup(objectKey.m_addr, mapped);
                            if (mapped == 0) {
                                pathGrid->m_rows[tileY][tileX].m_objectId = 0;
                                pathGrid->m_rows[tileY][tileX].m_0 &= ~0x40000;
                            } else {
                                CWwdGameObject* obj = static_cast<CWwdGameObject*>(mapped);
                                CInGameIcon* icon =
                                    static_cast<CInGameIcon*>(obj->m_animWorker->m_logic);
                                if (icon->m_object->m_124 == 0x55) {
                                    icon->m_object->m_114 = ownerHi;
                                    icon->HandleInput();
                                    if (ownerHi == g_curPlayer) {
                                        i32 fxX = rightX * 0x20 + 0x10;
                                        i32 fxY = scanY * 0x20 + 0x10;
                                        CWwdGameObjectA* light = level->m_childGroup->CreateSprite(
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
                                        CWwdGameObjectA* peek = level->m_childGroup->CreateSprite(
                                            0,
                                            fxX,
                                            fxY,
                                            1000000,
                                            "ToyPeek",
                                            0x40003
                                        );
                                        if (peek != 0) {
                                            peek->m_124 = icon->m_object->m_118;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 1;

        case 18:
            if (sel == 0x63 && unit != 0) {
                i32 waterX = unit->m_object->m_screenX;
                i32 waterY = unit->m_object->m_screenY;
                POINT pt;
                pt.x = waterX;
                pt.y = waterY;
                if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                    CWwdGameObjectA* splash =
                        level->m_childGroup
                            ->CreateSprite(0, waterX, waterY, 0xcf84f, "Particlez", 0x40003);
                    if (splash != 0) {
                        splash->ApplyName("GAME_WATER");
                        splash->ApplyLookupGeometry("GAME_WATER", 0);
                        level->m_soundRegistry->RefreshAsset("GAME_WATERSPLASH");
                    }
                }
            }
            return 1;

        default:
            return 0;
    }
}

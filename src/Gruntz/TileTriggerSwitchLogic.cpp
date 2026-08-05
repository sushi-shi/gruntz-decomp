#include <rva.h>

#include <Gruntz/TileTriggerSwitchLogic.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Enums.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CombatCueKind.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <string.h>

RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {

    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_initGate = 0;
}

RVA(0x00110460, 0x64)
i32 CTileTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    TrigLogicId typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    i32 linkGate,
    i32 damageParam,
    i32 checkpointType
) {
    if (m_initGate != 0) {
        return 0;
    }
    if (typeId == TRIGID_EXCLUSIVE_SWITCH_4 && rect[0].left == 0) {
        return 0;
    }
    memcpy(m_block, rect, sizeof(m_block));
    return Setup(owner, typeId, tileX, tileY, cellKey, linkGate, damageParam, checkpointType);
}

RVA(0x001104f0, 0x56)
i32 CTileTriggerSwitchLogic::Setup(
    CTileTriggerContainer* owner,
    TrigLogicId typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 linkGate,
    i32 damageParam,
    i32 checkpointType
) {
    if (m_initGate) {
        return 0;
    }
    m_typeId = typeId;
    m_tileX = tileX;
    m_tileY = tileY;
    m_cellKey = cellKey;
    m_owner = owner;
    m_damageParam = damageParam;
    m_checkpointType = checkpointType;
    m_reserved1c = 0;
    m_linkGate = linkGate;
    m_initGate = 1;
    return 1;
}

RVA(0x001107f0, 0x1c)
CTileTriggerLogic::CTileTriggerLogic() {

    for (i32 i = 0; i < 24; i++) {
        m_linkKeys[i] = 0;
    }
    m_initGate = 0;
}

RVA(0x00110820, 0x23)
i32 CTileTriggerLogic::FindIndexByKey(i32 key) {
    for (i32 i = 0; i < 24; i++) {
        if (m_linkKeys[i] == key) {
            return 1;
        }
    }
    return 0;
}

static __inline TileCollisionKind PbResolveCell(CGameLevel* level, i32 x, i32 y) {
    if (x < 0) {
        x = 0;
    } else if (x >= level->m_mainPlane->m_gridW) {
        x = level->m_mainPlane->m_gridW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= level->m_mainPlane->m_gridH) {
        y = level->m_mainPlane->m_gridH - 1;
    }
    CDDrawWorkerHost* plane = level->m_mainPlane;
    i32 cell = plane->m_tileGrid[plane->m_colOffsets[y] + x];
    if (cell == UNINIT_FILL || cell == TILE_CLEAR) {
        return TILEKIND_PASSABLE;
    }

    // Ingest: the raw WWD attribute byte for this cell.
    CTileImageSet* set = static_cast<CTileImageSet*>(level->m_imageSets[cell & 0xffff]);
    return static_cast<TileCollisionKind>(set->GetCollisionAt(0, 0));
}

static __inline TileCollisionKind PbResolveCellHandle(CGameLevel* level, i32 x, i32 y) {
    if (x < 0) {
        x = 0;
    } else if (x >= level->m_mainPlane->m_gridW) {
        x = level->m_mainPlane->m_gridW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= level->m_mainPlane->m_gridH) {
        y = level->m_mainPlane->m_gridH - 1;
    }
    i32 cell = level->m_mainPlane->GetTileHandle(x, y);
    if (cell == UNINIT_FILL || cell == TILE_CLEAR) {
        return TILEKIND_PASSABLE;
    }
    // Ingest: the raw WWD attribute byte for this cell.
    CTileImageSet* set = static_cast<CTileImageSet*>(level->m_imageSets[cell & 0xffff]);
    return static_cast<TileCollisionKind>(set->GetCollisionAt(0, 0));
}

static __inline char* PbStr(const CString& s) {
    return const_cast<char*>(static_cast<const char*>(s));
}

RVA(0x00110860, 0x2e6)
void CTileTriggerLogic::LoadBridgeMove(TileCollisionKind type) {
    i32 px, py;
    CGruntzMgr* r;
    CDDrawSubMgrLeafScan* set;
    switch (type) {
        // The retail byte index table starts at 15 (0x0f) and its slot 0 is an
        // arm distinct from the default: four empty cases that widen the dense
        // range to 102 entries.
        case TILEKIND_ARROW_UP_B:
        case TILEKIND_ARROW_DOWN_B:
        case TILEKIND_ARROW_LEFT_B:
        case TILEKIND_ARROW_RIGHT_B:
            break;
        case TILEKIND_CHECKPOINTPYRAMID_DOWN:
        case TILEKIND_CHECKPOINTPYRAMID_UP:
        case TILEKIND_WHITEPYRAMID_DOWN:
        case TILEKIND_WHITEPYRAMID_UP:
        case TILEKIND_ORANGEPYRAMID_DOWN:
        case TILEKIND_ORANGEPYRAMID_UP:
        case TILEKIND_BLACKPYRAMID_DOWN:
        case TILEKIND_BLACKPYRAMID_UP:
        case TILEKIND_GREENPYRAMID_DOWN:
        case TILEKIND_GREENPYRAMID_UP:
        case TILEKIND_REDPYRAMID_DOWN:
        case TILEKIND_REDPYRAMID_UP:
        case TILEKIND_PURPLEPYRAMID_DOWN:
        case TILEKIND_PURPLEPYRAMID_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                set = r->m_world->m_soundRegistry;
                if (set->m_emitGate == 0) {
                    LeafCue* e = static_cast<LeafCue*>(set->Lookup("GAME_PYRAMIDMOVE"));
                    if (e) {
                        e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case TILEKIND_WATERBRIDGE_DOWN:
        case TILEKIND_WATERBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                set = r->m_world->m_soundRegistry;
                if (set->m_emitGate == 0) {
                    LeafCue* e = static_cast<LeafCue*>(set->Lookup("LEVEL_WATERBRIDGEMOVE"));
                    if (e) {
                        e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
            }
            return;
        case TILEKIND_TOGGLEWATERBRIDGE_DOWN:
        case TILEKIND_TOGGLEWATERBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_WATERBRIDGEMOVE");
            }
            return;
        case TILEKIND_DEATHBRIDGE_DOWN:
        case TILEKIND_DEATHBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case TILEKIND_TOGGLEDEATHBRIDGE_DOWN:
        case TILEKIND_TOGGLEDEATHBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case TILEKIND_CRUMBLEWATERBRIDGE:
        case TILEKIND_CRUMBLEDEATHBRIDGE:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            r = g_gameReg;
            if (px < r->m_viewBounds.right && px >= r->m_viewBounds.left
                && py < r->m_viewBounds.bottom && py >= r->m_viewBounds.top) {
                r->m_world->m_soundRegistry->RefreshAsset("LEVEL_CRUMBLE");
            }
            return;
    }
}

// @early-stop
// referent set matches retail exactly; residue is a this=ebp/edi swap and a
// zero-register phase (retail pins 0 in ebx and compares against it where cl
// uses test reg,reg).
RVA(0x00110c10, 0xeee)
i32 CTileTriggerLogic::Tick() {
    CDDrawSurfaceMgr* world = g_gameReg->m_world;
    CTileTriggerTransition* trans = 0;

    TileCollisionKind srcId = PbResolveCell(world->m_level, m_tileX, m_tileY);

    {
        i32 sy = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
        i32 sx = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
        POINT pt;
        pt.x = sx;
        pt.y = sy;
        if (PtInRect(&g_gameReg->m_viewBounds, pt) && srcId != TILEKIND_REDPYRAMID_UP
            && srcId != TILEKIND_REDPYRAMID_DOWN) {
            CGameObject* trig =
                world->m_childGroup->CreateSprite(0, sx, sy, 0, "TileTriggerTransition", 0x40003);
            if (trig == NULL) {
                return 0;
            }
            trig->m_animWorker->m_notify(trig);
            trans = static_cast<CTileTriggerTransition*>(trig->m_animWorker->m_logic);
        }
    }

    CString key;
    CString anim;

    switch (srcId) {
        case TILEKIND_ARROW_UP_B: {
            if (trans != NULL) {
                trans->m_wwdObject->m_flags |= 0x10000;
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xca;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xca);
            break;
        }
        case TILEKIND_ARROW_DOWN_B: {
            if (trans != NULL) {
                trans->m_wwdObject->m_flags |= 0x10000;
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xc9;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xc9);
            break;
        }
        case TILEKIND_ARROW_LEFT_B: {
            if (trans != NULL) {
                trans->m_wwdObject->m_flags |= 0x10000;
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xcc;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xcc);
            break;
        }
        case TILEKIND_ARROW_RIGHT_B: {
            if (trans != NULL) {
                trans->m_wwdObject->m_flags |= 0x10000;
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xcb;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xcb);
            break;
        }

        case TILEKIND_REDPYRAMID_DOWN:
        case TILEKIND_REDPYRAMID_UP: {
            i32 pxX = 0x10;
            for (i32 gx = 0; gx < world->m_level->m_mainPlane->m_gridW; gx++, pxX += 0x20) {
                i32 pxY = 0x10;
                for (i32 gy = 0; gy < world->m_level->m_mainPlane->m_gridH; gy++, pxY += 0x20) {
                    i32 hit = 0;
                    if (PbResolveCell(world->m_level, gx, gy) == TILEKIND_REDPYRAMID_UP) {
                        CGruntzMgr* reg = g_gameReg;
                        CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
                        pl->m_tileGrid[pl->m_colOffsets[gy] + gx] = 0xfd;
                        reg->m_tileGrid->ComputeCellFlags(gx, gy, 0xfd);
                        anim = "GAME_PYRAMIDUP";
                        hit = 1;
                    } else if (PbResolveCell(world->m_level, gx, gy) == TILEKIND_REDPYRAMID_DOWN) {
                        CGruntzMgr* reg = g_gameReg;
                        CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
                        pl->m_tileGrid[pl->m_colOffsets[gy] + gx] = 0xfe;
                        reg->m_tileGrid->ComputeCellFlags(gx, gy, 0xfe);
                        anim = "GAME_PYRAMIDDOWN";
                        hit = 1;
                    }
                    if (hit != 0) {
                        POINT pt;
                        pt.x = pxX;
                        pt.y = pxY;
                        if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                            CGameObject* o = world->m_childGroup->CreateSprite(
                                0,
                                pxX,
                                pxY,
                                0,
                                "TileTriggerTransition",
                                0x40003
                            );
                            if (o == NULL) {
                                return 0;
                            }
                            o->m_animWorker->m_notify(o);
                            CTileTriggerTransition* lg =
                                static_cast<CTileTriggerTransition*>(o->m_animWorker->m_logic);
                            if (lg->ApplyAnimation("GAME_REDPYRAMIDZ", PbStr(anim)) == 0) {
                                lg->m_wwdObject->m_flags |= 0x10000;
                            }
                        }
                    }
                }
            }
            LoadBridgeMove(srcId);
            return 0;
        }

        default:
            return 0;

        case TILEKIND_GREENPYRAMID_DOWN:
        case TILEKIND_GREENPYRAMID_UP: {
            key = "GAME_GREENPYRAMIDZ";
            if (srcId == TILEKIND_GREENPYRAMID_UP) {
                anim = "GAME_PYRAMIDUP";
            } else {
                anim = "GAME_PYRAMIDDOWN";
            }
            TileCollisionKind now = PbResolveCell(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            if (now == TILEKIND_GREENPYRAMID_UP) {
                pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xfb;
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xfb);
            } else {
                pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xfc;
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xfc);
            }
            break;
        }
        case TILEKIND_PURPLEPYRAMID_DOWN:
        case TILEKIND_PURPLEPYRAMID_UP: {
            key = "GAME_PURPLEPYRAMIDZ";
            if (srcId == TILEKIND_PURPLEPYRAMID_UP) {
                anim = "GAME_PYRAMIDUP";
            } else {
                anim = "GAME_PYRAMIDDOWN";
            }
            TileCollisionKind now = PbResolveCell(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            if (now == TILEKIND_PURPLEPYRAMID_UP) {
                pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xff;
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xff);
            } else {
                pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0x100;
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0x100);
            }
            break;
        }
        case TILEKIND_ORANGEPYRAMID_DOWN:
        case TILEKIND_ORANGEPYRAMID_UP: {
            key = "GAME_ORANGEPYRAMIDZ";
            if (srcId == TILEKIND_ORANGEPYRAMID_UP) {
                anim = "GAME_PYRAMIDUP";
            } else {
                anim = "GAME_PYRAMIDDOWN";
            }
            TileCollisionKind now = PbResolveCell(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            if (now == TILEKIND_ORANGEPYRAMID_UP) {
                pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xf7;
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xf7);
            } else {
                pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xf8;
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xf8);
            }
            break;
        }
        case TILEKIND_BLACKPYRAMID_DOWN:
        case TILEKIND_BLACKPYRAMID_UP: {
            key = "GAME_BLACKPYRAMIDZ";
            if (srcId == TILEKIND_BLACKPYRAMID_UP) {
                anim = "GAME_PYRAMIDUP";
            } else {
                anim = "GAME_PYRAMIDDOWN";
            }
            TileCollisionKind now = PbResolveCell(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_BLACKPYRAMID_UP) {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0xf9);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xf9);
            } else {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0xfa);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xfa);
            }
            break;
        }
        case TILEKIND_WHITEPYRAMID_DOWN:
        case TILEKIND_WHITEPYRAMID_UP: {
            key = "GAME_WHITEPYRAMIDZ";
            if (srcId == TILEKIND_WHITEPYRAMID_UP) {
                anim = "GAME_PYRAMIDUP";
            } else {
                anim = "GAME_PYRAMIDDOWN";
            }
            TileCollisionKind now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_WHITEPYRAMID_UP) {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0xf5);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xf5);
            } else {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0xf6);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xf6);
            }
            break;
        }
        case TILEKIND_CHECKPOINTPYRAMID_DOWN:
        case TILEKIND_CHECKPOINTPYRAMID_UP: {
            key = "GAME_CHECKPOINTPYRAMIDZ";
            if (srcId == TILEKIND_CHECKPOINTPYRAMID_UP) {
                anim = "GAME_PYRAMIDUP";
            } else {
                anim = "GAME_PYRAMIDDOWN";
            }
            TileCollisionKind now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_CHECKPOINTPYRAMID_UP) {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0xd5);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xd5);
            } else {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0xd6);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xd6);
            }
            break;
        }
        case TILEKIND_WATERBRIDGE_DOWN:
        case TILEKIND_WATERBRIDGE_UP: {
            key = "LEVEL_WATERBRIDGE";
            if (srcId == TILEKIND_WATERBRIDGE_UP) {
                anim = "LEVEL_BRIDGEUP";
            } else {
                anim = "LEVEL_BRIDGEDOWN";
            }
            TileCollisionKind now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_WATERBRIDGE_UP) {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0x101);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0x101);
            } else {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0x102);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0x102);
            }
            break;
        }
        case TILEKIND_DEATHBRIDGE_DOWN:
        case TILEKIND_DEATHBRIDGE_UP: {
            key = "LEVEL_DEATHBRIDGE";
            if (srcId == TILEKIND_DEATHBRIDGE_UP) {
                anim = "LEVEL_BRIDGEUP";
            } else {
                anim = "LEVEL_BRIDGEDOWN";
            }
            TileCollisionKind now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_DEATHBRIDGE_UP) {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0x103);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0x103);
            } else {
                reg->m_world->m_level->m_mainPlane->SetCell(tx, ty, 0x104);
                reg->m_tileGrid->ComputeCellFlags(tx, ty, 0x104);
            }
            break;
        }

        case TILEKIND_TOGGLEWATERBRIDGE_DOWN:
        case TILEKIND_TOGGLEWATERBRIDGE_UP: {
            key = "LEVEL_TOGGLEWATERBRIDGE";
            if (srcId == TILEKIND_TOGGLEWATERBRIDGE_UP) {
                anim = "LEVEL_BRIDGEUP";
            } else {
                anim = "LEVEL_BRIDGEDOWN";
            }
            if (world->m_level->LookupTile(m_tileX, m_tileY)
                == IDX(TILEKIND_TOGGLEWATERBRIDGE_UP)) {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, 0x107);
            } else {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, 0x108);
            }
            break;
        }
        case TILEKIND_TOGGLEDEATHBRIDGE_DOWN:
        case TILEKIND_TOGGLEDEATHBRIDGE_UP: {
            key = "LEVEL_TOGGLEDEATHBRIDGE";
            if (srcId == TILEKIND_TOGGLEDEATHBRIDGE_UP) {
                anim = "LEVEL_BRIDGEUP";
            } else {
                anim = "LEVEL_BRIDGEDOWN";
            }
            if (world->m_level->LookupTile(m_tileX, m_tileY)
                == IDX(TILEKIND_TOGGLEDEATHBRIDGE_UP)) {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, 0x109);
            } else {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, 0x10a);
            }
            break;
        }

        case TILEKIND_CRUMBLEWATERBRIDGE: {
            key = "LEVEL_CRUMBLEWATERBRIDGE";
            anim = "LEVEL_CRUMBLEBRIDGE";
            g_gameReg->SetCellHeight(m_tileX, m_tileY, m_tileToken);
            break;
        }
        case TILEKIND_CRUMBLEDEATHBRIDGE: {
            key = "LEVEL_CRUMBLEDEATHBRIDGE";
            anim = "LEVEL_CRUMBLEBRIDGE";
            g_gameReg->SetCellHeight(m_tileX, m_tileY, m_tileToken);
            break;
        }
    }

    if (trans != NULL) {
        if (trans->ApplyAnimation(PbStr(key), PbStr(anim)) == 0) {
            trans->m_wwdObject->m_flags |= 0x10000;
        }
    }
    LoadBridgeMove(srcId);
    return 1;
}

RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(i32 row, i32 col, i32 value) {
    CDDrawWorkerHost* grid = m_world->m_level->m_mainPlane;
    i32 idx = grid->m_colOffsets[col] + row;
    grid->m_tileGrid[idx] = value;

    m_tileGrid->ComputeCellFlags(row, col, value);
}

RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

// @interleaver VerifyBlockLinksB - 196 B lone body at 0x111f40, between ?0CTileMultiTriggerSwitchLogic
// (tileswitchlogic) and ?0CTileExclusiveTriggerSwitchLogic (tileswitchlogic): a first-use placement.
RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinksB() {
    if (m_linkGate == 0) {
        return 0;
    }

    POSITION pos = m_owner->m_list1.GetHeadPosition();
    i32 found = 0;

    CTileTriggerLogic* child;
    while (pos != NULL) {
        if (found != 0) {
            break;
        }
        child = static_cast<CTileTriggerLogic*>(m_owner->m_list1.GetNext(pos));
        if (child != NULL && child->FindIndexByKey(m_cellKey) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_LINKSB_NO_OWNER));
        return 0;
    }

    for (i32 i = 0; i < 24; i++) {
        i32 key = child->m_linkKeys[i];
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_MULTI_SWITCH_3);
        if (c == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_LINKSB_KEY_MISS));
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
    }
    return 0;
}

RVA(0x00112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {}

// @early-stop
RVA(0x00112080, 0x138)
i32 CTileExclusiveTriggerSwitchLogic::SwitchDown() {

    i32 done = 0;
    i32 counter = 0;
    CTileTriggerSwitchLogic::SwitchDown();
    i32 i = 0;
    while (done == 0) {
        if (i >= 0x18) {
            return 1;
        }
        i32 key = m_block[i];
        CTileTriggerSwitchLogic* node = m_owner->FindChild(key, TRIGID_EXCLUSIVE_SWITCH_4);
        if (node == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_BCAST_KEY_MISS));
            return 0;
        }
        if (node->m_cellKey != m_cellKey && node->m_linkGate != 0) {
            node->SwitchUp();
            i32 any = 0;
            POSITION pos = m_owner->m_list1.GetHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* o =
                    static_cast<CTileTriggerLogic*>(m_owner->m_list1.GetNext(pos));
                if (o != NULL && o->FindIndexByKey(node->m_cellKey)) {
                    o->Tick();
                    counter++;
                    any = 1;
                }
            }
            if (any == 0) {
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_BCAST_NO_CLAIM));
                return 0;
            }
        }
        i++;
        if (m_block[i] == 0) {
            done = 1;
        }
    }
    return 1;
}

RVA(0x00112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {}

RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {}

RVA(0x00112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {}

// @early-stop
RVA(0x001122a0, 0x241)
void CGiantRockLogic::BuildRockBreakInGameText() {

    CDDrawSurfaceMgr* gameMgr = g_gameReg->m_world;

    i32 inRect = 0;
    POINT pt;
    pt.y = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    pt.x = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
        inRect = 1;
    }

    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = m_matrix[j * 3 + i];
            i32 px = i + m_tileX - 1;
            i32 py = j + m_tileY - 1;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* plane = reg->m_world->m_level->m_mainPlane;
            plane->m_tileGrid[plane->m_colOffsets[py] + px] = value;
            reg->m_tileGrid->ComputeCellFlags(px, py, value);
            i32 sx = ((i + m_tileX) << TILE_SHIFT_PX) - 0x10;
            i32 sy = ((j + m_tileY) << TILE_SHIFT_PX) - 0x10;
            if (inRect) {
                CWwdGameObjectA* spr =
                    gameMgr->m_childGroup
                        ->CreateSprite(0, sx, sy, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
                if (spr != NULL) {
                    spr->ApplyName("LEVEL_ROCKBREAK");
                    spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                }
            }
        }
    }

    i32 cx = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    i32 cy = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    g_gameReg->m_cmdGrid->LoadPowerupIconSprites(
        static_cast<PickupType>(m_powerupType),
        cx,
        cy,
        static_cast<i32>(m_dutyOffSpan),
        1,
        0
    );

    if (m_textId != 0) {
        CGameObject* txt = g_gameReg->m_world->m_childGroup
                               ->CreateSprite(0, cx, cy, 0x17318, "InGameText", 0x40003);
        if (txt == NULL) {
            return;
        }
        txt->m_smarts = m_textId;
    }

    i32 bx = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    i32 by = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (bx >= g_gameReg->m_viewBounds.right || bx < g_gameReg->m_viewBounds.left
        || by >= g_gameReg->m_viewBounds.bottom || by < g_gameReg->m_viewBounds.top) {
        return;
    }
    CDDrawSubMgrLeafScan* sreg = gameMgr->m_soundRegistry;
    if (sreg->m_emitGate != 0) {
        return;
    }
    void* out_ob = 0;
    sreg->m_cues.Lookup("LEVEL_ROCKBREAK", out_ob);
    LeafCue* out = static_cast<LeafCue*>(out_ob);
    if (out == NULL) {
        return;
    }
    i32 tag = g_sndCueTag;
    if (g_sndEnabled == 0) {
        return;
    }
    i32 kc = g_killCueClock;
    if (static_cast<u32>((kc - out->m_lastPlayTime)) < static_cast<u32>(out->m_replayDelay)) {
        return;
    }
    out->m_lastPlayTime = kc;
    out->m_sound->ConfigureItem(tag, 0, 0, 0);
}

// @early-stop
RVA(0x00112590, 0x166)
i32 CTileTriggerLogic::ApplyMove(TileCollisionKind verb) {
    i32 tok = m_tileToken;
    if (tok != 0) {
        CGruntzMgr* reg = g_gameReg;
        i32 ty = m_tileY;
        i32 tx = m_tileX;
        CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
        L->m_tileGrid[L->m_colOffsets[ty] + tx] = tok;
        (reg->m_tileGrid)->ComputeCellFlags(tx, ty, tok);
    } else {
        switch (verb) {
            case TILEKIND_COVERED_POWERUP: {
                CGruntzMgr* reg = g_gameReg;
                i32 tx = m_tileX;
                i32 ty = m_tileY;
                CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
                i32 v = L->m_tileGrid[L->m_colOffsets[ty] + tx] + 1;
                CDDrawWorkerHost* L2 = g_gameReg->m_world->m_level->m_mainPlane;
                L2->m_tileGrid[L2->m_colOffsets[ty] + tx] = v;
                (reg->m_tileGrid)->ComputeCellFlags(tx, ty, v);
                break;
            }
            case TILEKIND_GAUNTLET_ROCK_B: {
                CGruntzMgr* reg = g_gameReg;
                i32 ty = m_tileY;
                i32 tx = m_tileX;
                CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[ty] + tx] = 0x5b;
                (reg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5b);
                break;
            }
            case TILEKIND_GAUNTLET_ROCK_A: {
                CGruntzMgr* reg = g_gameReg;
                i32 ty = m_tileY;
                i32 tx = m_tileX;
                CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[ty] + tx] = 0x5a;
                (reg->m_tileGrid)->ComputeCellFlags(tx, ty, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGruntzMgr* reg = g_gameReg;
    i32 py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    i32 px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    reg->m_cmdGrid->LoadPowerupIconSprites(
        static_cast<PickupType>(m_dutyOnSpan),
        px,
        py,
        m_dutyOffSpan,
        1,
        0
    );
    if (m_leadInSpan != 0) {
        CGameObject* rec =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 95000, "InGameText", 0x40003);
        if (rec != NULL) {
            rec->m_smarts = m_leadInSpan;
        }
    }
    return 1;
}

RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {}

RVA(0x00112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {}

RVA(0x001127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {}

RVA(0x001127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {}

RVA(0x00112820, 0xc)
i32 CTileSecretTriggerSwitchLogic::SwitchDown() {
    return CTileTriggerSwitchLogic::SwitchDown() != 0;
}

RVA(0x00112840, 0xc)
i32 CTileTimeTriggerSwitchLogic::SwitchDown() {
    return CTileTriggerSwitchLogic::SwitchDown() != 0;
}

RVA(0x00112860, 0xc)
i32 CTileTimeTriggerSwitchLogic::SwitchUp() {
    return CTileTriggerSwitchLogic::SwitchUp() != 0;
}

RVA(0x00112880, 0x12)
void CTileTriggerLogic::RecordMove() {
    m_startClock = g_frameTime;
    m_owner->MoveList1ToList2(this);
}

// @early-stop
RVA(0x001128b0, 0x88)
i32 CTileSecretTriggerLogic::Tick() {
    i32 oldTok = m_tileToken;
    if (oldTok == 0) {
        g_gameReg->ReportError(IDX(CMD_TOGGLE_SOUND), 0x451);
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    i32 grp = m_tileX;
    i32 idx = m_tileY;
    i32 newTok = mgr->m_world->m_level->m_mainPlane
                     ->m_tileGrid[mgr->m_world->m_level->m_mainPlane->m_colOffsets[idx] + grp];
    g_gameReg->m_world->m_level->m_mainPlane
        ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[idx] + grp] = oldTok;
    mgr->m_tileGrid->ComputeCellFlags(grp, idx, oldTok);
    m_tileToken = newTok;
    return 1;
}

// @early-stop
RVA(0x00112970, 0xad)
i32 CTileTriggerLogic::Classify(i32 arg) {
    u32 elapsed = g_frameTime - m_startClock;
    if (elapsed <= m_leadInSpan) {
        goto ret1;
    }
    elapsed -= m_leadInSpan;
    {
        u32 period = m_dutyOnSpan + m_dutyOffSpan;
        if (elapsed > period) {
            if (m_typeTag == TRIGID_TILE_TRIGGER_24) {
                Tick();
                return 0;
            }
            if (m_typeTag != TRIGID_TIME_TRIGGER_23) {

                if (m_dutyOn == 1) {
                    Tick();
                }
                return -1;
            }
        }
        u32 rem = elapsed % period;
        if (rem < m_dutyOnSpan) {
            if (m_dutyOn != 0) {
                goto ret1;
            }
            Tick();
            m_dutyOn = 1;
            if (m_typeTag != TRIGID_TILE_TRIGGER_24) {
                goto ret1;
            }
            return 0;
        }
        if (m_dutyOn != 1) {
            goto ret1;
        }
        Tick();
        m_dutyOn = 0;

        if (m_typeTag != TRIGID_TIME_TRIGGER_23) {
            return -1;
        }
    }
ret1:
    return 1;
}

// @early-stop
RVA(0x00112a50, 0xdd)

i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    TrigLogicId typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    i32 linkGate,
    i32 damageParam,
    i32 checkpointType
) {
    i32 px;
    i32 py;
    CWwdGameObjectA* spr;

    if (m_initGate != 0) {
        goto fail;
    }
    if (typeId == TRIGID_EXCLUSIVE_SWITCH_4 && rect[0].left == 0) {
        goto fail;
    }
    memcpy(m_block, rect, sizeof(m_block));
    if (!Setup(owner, typeId, tileX, tileY, cellKey, linkGate, damageParam, checkpointType)) {
        goto fail;
    }
    px = (tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    py = (tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (checkpointType == 0) {
        return 1;
    }
    spr = g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 0, "BehindCandy", 0x40001);
    if (!spr) {
        goto fail;
    }
    spr->m_animWorker->m_notify(spr);
    spr->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", checkpointType);
    if (spr->m_layer == NULL) {
        goto fail;
    }
    return 1;
fail:
    return 0;
}

// @early-stop
// second walk is byte-correct now; cl still colours colOffsets/tileGrid into
// ebx/ebp the other way round and buys a 4-byte spill slot for layer2.
RVA(0x00112b70, 0x5a)
i32 CCheckpointTriggerSwitchLogic::SwitchDown() {
    CGruntzMgr* reg = g_gameReg;
    i32 tileX = m_tileX;
    i32 tileY = m_tileY;
    CDDrawWorkerHost* layer = reg->m_world->m_level->m_mainPlane;
    i32 v = layer->m_tileGrid[tileX + layer->m_colOffsets[tileY]] + 1;
    // write through the un-cached global: defeats the address-CSE so the store
    // re-walks m_world->m_level->m_mainPlane exactly as retail does
    CDDrawWorkerHost* layer2 = g_gameReg->m_world->m_level->m_mainPlane;
    i32 idx2 = tileX + layer2->m_colOffsets[tileY];
    layer2->m_tileGrid[idx2] = v;
    reg->m_tileGrid->ComputeCellFlags(tileX, tileY, v);
    m_linkGate = 1;
    return 1;
}

// @early-stop
RVA(0x00112bf0, 0x5e)
i32 CCheckpointTriggerSwitchLogic::SwitchUp() {
    CGruntzMgr* reg = g_gameReg;
    i32 tileX = m_tileX;
    i32 tileY = m_tileY;
    CDDrawWorkerHost* layer = reg->m_world->m_level->m_mainPlane;
    i32 v = layer->m_tileGrid[tileX + layer->m_colOffsets[tileY]] - 1;
    // write through the un-cached global: defeats the address-CSE so the store
    // re-walks m_world->m_level->m_mainPlane exactly as retail does
    CDDrawWorkerHost* layer2 = g_gameReg->m_world->m_level->m_mainPlane;
    i32 idx2 = tileX + layer2->m_colOffsets[tileY];
    layer2->m_tileGrid[idx2] = v;
    reg->m_tileGrid->ComputeCellFlags(tileX, tileY, v);
    m_linkGate = 0;
    return 1;
}

RVA(0x00112c70, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinks() {
    if (m_linkGate == 0) {
        return 0;
    }

    POSITION pos = m_owner->m_list1.GetHeadPosition();
    i32 found = 0;

    CTileTriggerLogic* child;
    while (pos != NULL) {
        if (found != 0) {
            break;
        }
        child = static_cast<CTileTriggerLogic*>(m_owner->m_list1.GetNext(pos));
        if (child != NULL && child->FindIndexByKey(m_cellKey) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_LINKS_NO_OWNER));
        return 0;
    }

    for (i32 i = 0; i < 24; i++) {
        i32 key = child->m_linkKeys[i];
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (c == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_LINKS_KEY_MISS));
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
    }
    return 0;
}

RVA(0x00112d80, 0xa)
CTileActionEvent::CTileActionEvent() {
    m_live = 0;
}

// @early-stop
RVA(0x00112da0, 0x100)
i32 CTileActionEvent::SetActionCode(i32 code) {
    m_actionCode = code;
    if (m_playerFlags[g_curPlayer] == 0 && static_cast<u32>((code - BRICKTILE_BROWN_1)) <= 0x1a) {
        switch (code) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                code = BRICKTILE_BROWN_1;
                break;
            case BRICKTILE_BROWN_2:
            case BRICKTILE_RED_2_LOW:
            case BRICKTILE_RED_2_TOP:
            case BRICKTILE_BLUE_2_LOW:
            case BRICKTILE_BLUE_2_TOP:
            case BRICKTILE_GOLD_2_LOW:
            case BRICKTILE_GOLD_2_TOP:
            case BRICKTILE_BLACK_2_LOW:
            case BRICKTILE_BLACK_2_TOP:
                code = BRICKTILE_BROWN_2;
                break;
            case BRICKTILE_BROWN_3:
            case BRICKTILE_RED_3_LOW:
            case BRICKTILE_RED_3_MID:
            case BRICKTILE_RED_3_TOP:
            case BRICKTILE_BLUE_3_LOW:
            case BRICKTILE_BLUE_3_MID:
            case BRICKTILE_BLUE_3_TOP:
            case BRICKTILE_GOLD_3_LOW:
            case BRICKTILE_GOLD_3_MID:
            case BRICKTILE_GOLD_3_TOP:
            case BRICKTILE_BLACK_3_LOW:
            case BRICKTILE_BLACK_3_MID:
            case BRICKTILE_BLACK_3_TOP:
                code = BRICKTILE_BROWN_3;
                break;
        }
    }

    CGruntzMgr* reg = g_gameReg;
    i32 ty = m_tileY;
    i32 tx = m_tileX;
    if (reg->m_world->m_level->m_mainPlane
            ->m_tileGrid[reg->m_world->m_level->m_mainPlane->m_colOffsets[ty] + tx]
        == code) {
        return 0;
    }
    // The write goes through the UN-CACHED global on purpose: it defeats the
    // read/write address CSE so cl re-walks the chain, as retail does.
    g_gameReg->m_world->m_level->m_mainPlane
        ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[ty] + tx] = code;
    reg->m_tileGrid->ComputeCellFlags(tx, ty, code);
    return 1;
}

// @early-stop
// three residues: the px/py + g_gameReg schedule, an eax/edx swap on the player
// flag stores, and `add edi,-0x132` where retail keeps edi with `lea eax,[edi-0x132]`.
RVA(0x00112ee0, 0x42b)
i32 CTileActionEvent::Process(CGrunt* brick) {
    i32 newCode = m_actionCode;
    i32 effect = 0;
    // Arm order is proven by the retail jump table at 0x113278: the tiers are
    // written across the four colours (all the 1s, then the 2-TOPs, the 2-LOWs,
    // the 3-TOPs), the plain 3-LOW/3-MID demotions follow, and the brown chain
    // sits last - cl suffix-merges each brown arm into the black arm above it.
    switch (m_actionCode) {
        case BRICKTILE_RED_1:
            effect = BRICKTILE_RED_1;
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_BLUE_1:
            effect = BRICKTILE_BLUE_1;
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_GOLD_1:
            effect = BRICKTILE_GOLD_1;
            if (brick != NULL) {
                break;
            }
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_BLACK_1:
            effect = BRICKTILE_BLACK_1;
            // fall through
        case BRICKTILE_BROWN_1:
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_RED_2_TOP:
            effect = BRICKTILE_RED_1;
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_BLUE_2_TOP:
            effect = BRICKTILE_BLUE_1;
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_GOLD_2_TOP:
            effect = BRICKTILE_GOLD_1;
            if (brick != NULL) {
                break;
            }
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_BLACK_2_TOP:
            effect = BRICKTILE_BLACK_1;
            // fall through
        case BRICKTILE_BROWN_2:
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_RED_2_LOW:
            newCode = BRICKTILE_RED_1;
            break;
        case BRICKTILE_BLUE_2_LOW:
            newCode = BRICKTILE_BLUE_1;
            break;
        case BRICKTILE_GOLD_2_LOW:
            newCode = BRICKTILE_GOLD_1;
            break;
        case BRICKTILE_BLACK_2_LOW:
            newCode = BRICKTILE_BLACK_1;
            break;
        case BRICKTILE_RED_3_TOP:
            effect = BRICKTILE_RED_1;
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_BLUE_3_TOP:
            effect = BRICKTILE_BLUE_1;
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_GOLD_3_TOP:
            effect = BRICKTILE_GOLD_1;
            if (brick != NULL) {
                break;
            }
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_BLACK_3_TOP:
            effect = BRICKTILE_BLACK_1;
            // fall through
        case BRICKTILE_BROWN_3:
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_RED_3_LOW:
            newCode = BRICKTILE_RED_2_LOW;
            break;
        case BRICKTILE_RED_3_MID:
            newCode = BRICKTILE_RED_2_TOP;
            break;
        case BRICKTILE_BLUE_3_LOW:
            newCode = BRICKTILE_BLUE_2_LOW;
            break;
        case BRICKTILE_BLUE_3_MID:
            newCode = BRICKTILE_BLUE_2_TOP;
            break;
        case BRICKTILE_GOLD_3_LOW:
            newCode = BRICKTILE_GOLD_2_LOW;
            break;
        case BRICKTILE_GOLD_3_MID:
            newCode = BRICKTILE_GOLD_2_TOP;
            break;
        case BRICKTILE_BLACK_3_LOW:
            newCode = BRICKTILE_BLACK_2_LOW;
            break;
        case BRICKTILE_BLACK_3_MID:
            newCode = BRICKTILE_BLACK_2_TOP;
            break;
    }

    if (effect != 0 && brick != NULL) {
        if (effect == BRICKTILE_RED_1) {
            brick->LoadGruntTypeTable(PICKUP_NONE, 1, 0, 0);
            brick->m_entranceActive = 0;
        } else if (effect == BRICKTILE_BLUE_1) {
            g_gameReg->m_cmdGrid->CombatCue(
                (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX,
                (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX,
                1,
                CUE_TELEPORT,
                -1
            );
        } else if (effect == BRICKTILE_GOLD_1) {
            i32 px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            i32 py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
                && py < g_gameReg->m_viewBounds.bottom && py >= g_gameReg->m_viewBounds.top
                && g_gameReg->m_world->m_soundRegistry->m_emitGate == 0) {
                LeafCue* snd = static_cast<LeafCue*>(
                    g_gameReg->m_world->m_soundRegistry->Lookup("GRUNTZ_NORMALGRUNT_IMPACTMM3")
                );
                if (snd != NULL) {
                    snd->PlayIfElapsed(static_cast<i32>(g_sndCueTag), 0, 0, 0);
                }
            }
            i32 slot = brick->m_tileOwnerHi;
            if (slot == PLAYERSLOT_ALL) {
                i32* flags = m_playerFlags;
                flags[0] = 1;
                flags[1] = 1;
                flags[2] = 1;
                flags[3] = 1;
                SetActionCode(m_actionCode);
                return 0;
            }
            m_playerFlags[slot] = 1;
            SetActionCode(m_actionCode);
            return 0;
        } else if (effect == BRICKTILE_BLACK_1) {
            g_gameReg->m_cmdGrid->LoadExplosionSprites(
                (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX,
                (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX,
                -1,
                2
            );
        }
    }

    i32 px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
    i32 py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
    if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
        && py < g_gameReg->m_viewBounds.bottom && py >= g_gameReg->m_viewBounds.top) {
        CWwdGameObjectA* spr =
            g_gameReg->m_world->m_childGroup
                ->CreateSprite(0, px, py, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
        if (spr != NULL) {
            spr->ApplyLookupGeometry("GAME_BRICKBREAK", 0);

            switch (effect) {
                case BRICKTILE_RED_1:
                    spr->ApplyName("GAME_REDBRICKBREAK");
                    break;
                case BRICKTILE_BLUE_1:
                    spr->ApplyName("GAME_BLUEBRICKBREAK");
                    break;
                case BRICKTILE_GOLD_1:
                    spr->ApplyName("GAME_GOLDBRICKBREAK");
                    break;
                case BRICKTILE_BLACK_1:
                    spr->ApplyName("GAME_BLACKBRICKBREAK");
                    break;
                default:
                    spr->ApplyName("GAME_BRICKBREAK");
                    if (spr->m_layer == NULL) {
                        spr->m_flags |= 0x10000;
                    }
                    break;
            }
        }
    }

    if (newCode != m_actionCode) {
        SetActionCode(newCode);
    }
    return newCode == BRICKTILE_CLEARED;
}

// @early-stop
// commit block is byte-correct except the literal 1: cl hoists it into esi
// above the playerSlot test, retail keeps it an immediate at the else store
// and the return and materializes it inside the taken arm.
RVA(0x00113420, 0x358)
i32 CTileActionEvent::MorphByTool(i32 toolId, i32 playerSlot) {
    if (toolId == PICKUP_BROWNBRICK) {
        switch (m_actionCode) {
            case BRICKTILE_BROWN_1:
                m_actionCode = BRICKTILE_BROWN_2;

            commit: {

                i32* flags = m_playerFlags;
                memset(flags, 0, sizeof(m_playerFlags));
                if (playerSlot == PLAYERSLOT_ALL) {
                    flags[0] = 1;
                    flags[1] = 1;
                    flags[2] = 1;
                    flags[3] = 1;
                } else {
                    m_playerFlags[playerSlot] = 1;
                }
                SetActionCode(m_actionCode);
                return 1;
            }
            case BRICKTILE_RED_1:
                m_actionCode = BRICKTILE_RED_2_LOW;
                goto commit;
            case BRICKTILE_BLUE_1:
                m_actionCode = BRICKTILE_BLUE_2_LOW;
                goto commit;
            case BRICKTILE_GOLD_1:
                m_actionCode = BRICKTILE_GOLD_2_LOW;
                goto commit;
            case BRICKTILE_BLACK_1:
                m_actionCode = BRICKTILE_BLACK_2_LOW;
                goto commit;
            case BRICKTILE_BROWN_2:
                m_actionCode = BRICKTILE_BROWN_3;
                goto commit;
            case BRICKTILE_RED_2_LOW:
                m_actionCode = BRICKTILE_RED_3_LOW;
                goto commit;
            case BRICKTILE_RED_2_TOP:
                m_actionCode = BRICKTILE_RED_3_MID;
                goto commit;
            case BRICKTILE_BLUE_2_LOW:
                m_actionCode = BRICKTILE_BLUE_3_LOW;
                goto commit;
            case BRICKTILE_BLUE_2_TOP:
                m_actionCode = BRICKTILE_BLUE_3_MID;
                goto commit;
            case BRICKTILE_GOLD_2_LOW:
                m_actionCode = BRICKTILE_GOLD_3_LOW;
                goto commit;
            case BRICKTILE_GOLD_2_TOP:
                m_actionCode = BRICKTILE_GOLD_3_MID;
                goto commit;
            case BRICKTILE_BLACK_2_LOW:
                m_actionCode = BRICKTILE_BLACK_3_LOW;
                goto commit;
            case BRICKTILE_BLACK_2_TOP:
                m_actionCode = BRICKTILE_BLACK_3_MID;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_REDBRICK) {
        switch (m_actionCode) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_actionCode = BRICKTILE_RED_2_TOP;
                goto commit;
            case BRICKTILE_BROWN_2:
            case BRICKTILE_RED_2_LOW:
            case BRICKTILE_RED_2_TOP:
            case BRICKTILE_BLUE_2_LOW:
            case BRICKTILE_BLUE_2_TOP:
            case BRICKTILE_GOLD_2_LOW:
            case BRICKTILE_GOLD_2_TOP:
            case BRICKTILE_BLACK_2_LOW:
            case BRICKTILE_BLACK_2_TOP:
                m_actionCode = BRICKTILE_RED_3_TOP;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_BLUEBRICK) {
        switch (m_actionCode) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_actionCode = BRICKTILE_BLUE_2_TOP;
                goto commit;
            case BRICKTILE_BROWN_2:
            case BRICKTILE_RED_2_LOW:
            case BRICKTILE_RED_2_TOP:
            case BRICKTILE_BLUE_2_LOW:
            case BRICKTILE_BLUE_2_TOP:
            case BRICKTILE_GOLD_2_LOW:
            case BRICKTILE_GOLD_2_TOP:
            case BRICKTILE_BLACK_2_LOW:
            case BRICKTILE_BLACK_2_TOP:
                m_actionCode = BRICKTILE_BLUE_3_TOP;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_BLACKBRICK) {
        switch (m_actionCode) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_actionCode = BRICKTILE_BLACK_2_TOP;
                goto commit;
            case BRICKTILE_BROWN_2:
            case BRICKTILE_RED_2_LOW:
            case BRICKTILE_RED_2_TOP:
            case BRICKTILE_BLUE_2_LOW:
            case BRICKTILE_BLUE_2_TOP:
            case BRICKTILE_GOLD_2_LOW:
            case BRICKTILE_GOLD_2_TOP:
            case BRICKTILE_BLACK_2_LOW:
            case BRICKTILE_BLACK_2_TOP:
                m_actionCode = BRICKTILE_BLACK_3_TOP;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_GOLDBRICK) {
        switch (m_actionCode) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_actionCode = BRICKTILE_GOLD_2_TOP;
                goto commit;
            case BRICKTILE_BROWN_2:
            case BRICKTILE_RED_2_LOW:
            case BRICKTILE_RED_2_TOP:
            case BRICKTILE_BLUE_2_LOW:
            case BRICKTILE_BLUE_2_TOP:
            case BRICKTILE_GOLD_2_LOW:
            case BRICKTILE_GOLD_2_TOP:
            case BRICKTILE_BLACK_2_LOW:
            case BRICKTILE_BLACK_2_TOP:
                m_actionCode = BRICKTILE_GOLD_3_TOP;
                goto commit;
            default:
                return 0;
        }
    }

    goto commit;
}

RVA(0x00113860, 0x3b)
i32 CTileTriggerSwitchLogic::ValidateByType(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 pObj
) {
    if (s == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (!SaveState(s)) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (!LoadState(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x001138b0, 0xb4)
i32 CTileTriggerSwitchLogic::SaveState(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    ar->Write(&m_tileX, sizeof(m_tileX));
    ar->Write(&m_tileY, sizeof(m_tileY));
    ar->Write(&m_cellKey, sizeof(m_cellKey));
    ar->Write(&m_linkGate, sizeof(m_linkGate));
    ar->Write(&m_damageParam, sizeof(m_damageParam));
    ar->Write(&m_reserved1c, sizeof(m_reserved1c));
    ar->Write(&m_initGate, sizeof(m_initGate));
    ar->Write(&m_checkpointType, sizeof(m_checkpointType));
    i32* p = m_block;
    i32 n = 24;
    do {
        ar->Write(p, sizeof(*p));
        p++;
    } while (--n);
    return 1;
}

RVA(0x001139a0, 0xb4)
i32 CTileTriggerSwitchLogic::LoadState(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Read(&m_tileX, sizeof(m_tileX));
    s->Read(&m_tileY, sizeof(m_tileY));
    s->Read(&m_cellKey, sizeof(m_cellKey));
    s->Read(&m_linkGate, sizeof(m_linkGate));
    s->Read(&m_damageParam, sizeof(m_damageParam));
    s->Read(&m_reserved1c, sizeof(m_reserved1c));
    s->Read(&m_initGate, sizeof(m_initGate));
    s->Read(&m_checkpointType, sizeof(m_checkpointType));
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, sizeof(*p));
        p++;
    }
    return 1;
}

RVA(0x00113a90, 0x3b)
i32 CTileTriggerLogic::ValidateByType(
    void* archive,
    SerialMode mode,
    LogicTypeId typeId,
    i32 pObj
) {
    if (archive == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (Serialize(static_cast<CFileMemBase*>(archive)) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (Deserialize(static_cast<CFileMemBase*>(archive)) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x00113ae0, 0xe8)
i32 CTileTriggerLogic::Serialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Write(&m_tileX, sizeof(m_tileX));
    s->Write(&m_tileY, sizeof(m_tileY));
    s->Write(&m_cellKey, sizeof(m_cellKey));
    s->Write(&m_reserved14, sizeof(m_reserved14));
    s->Write(&m_reserved18, sizeof(m_reserved18));
    s->Write(&m_initGate, sizeof(m_initGate));
    s->Write(&m_dutyOnSpan, sizeof(m_dutyOnSpan));
    s->Write(&m_leadInSpan, sizeof(m_leadInSpan));
    s->Write(&m_dutyOffSpan, sizeof(m_dutyOffSpan));
    s->Write(&m_tileToken, sizeof(m_tileToken));
    s->Write(&m_dutyOn, sizeof(m_dutyOn));
    s->Write(&m_startClock, sizeof(m_startClock));
    i32* p = m_linkKeys;
    for (i32 i = 0; i < 24; i++) {
        s->Write(p, sizeof(*p));
        p++;
    }
    return 1;
}

RVA(0x00113c10, 0xe8)
i32 CTileTriggerLogic::Deserialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Read(&m_tileX, sizeof(m_tileX));
    s->Read(&m_tileY, sizeof(m_tileY));
    s->Read(&m_cellKey, sizeof(m_cellKey));
    s->Read(&m_reserved14, sizeof(m_reserved14));
    s->Read(&m_reserved18, sizeof(m_reserved18));
    s->Read(&m_initGate, sizeof(m_initGate));
    s->Read(&m_dutyOnSpan, sizeof(m_dutyOnSpan));
    s->Read(&m_leadInSpan, sizeof(m_leadInSpan));
    s->Read(&m_dutyOffSpan, sizeof(m_dutyOffSpan));
    s->Read(&m_tileToken, sizeof(m_tileToken));
    s->Read(&m_dutyOn, sizeof(m_dutyOn));
    s->Read(&m_startClock, sizeof(m_startClock));
    i32* p = m_linkKeys;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, sizeof(*p));
        p++;
    }
    return 1;
}

RVA(0x00113d40, 0x6f)
i32 CGiantRockLogic::ApplyByType(void* archive, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (archive == NULL) {
        return 0;
    }
    if (ValidateByType(archive, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (SerializeMatrix(static_cast<CFileMemBase*>(archive)) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (DeserializeMatrix(static_cast<CFileMemBase*>(archive)) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x00113dd0, 0x7b)
i32 CGiantRockLogic::SerializeMatrix(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Write(&m_powerupType, sizeof(m_powerupType));
    s->Write(&m_textId, sizeof(m_textId));

    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(&m_matrix[r * 3 + c], sizeof(m_matrix[r * 3 + c]));
        }
    }
    return 1;
}

RVA(0x00113e70, 0x7b)
i32 CGiantRockLogic::DeserializeMatrix(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    s->Read(&m_powerupType, sizeof(m_powerupType));
    s->Read(&m_textId, sizeof(m_textId));

    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Read(&m_matrix[r * 3 + c], sizeof(m_matrix[r * 3 + c]));
        }
    }
    return 1;
}

RVA(0x00113f10, 0x3b)
i32 CTileActionEvent::Serialize(void* ar, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (SerializeFields(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (DeserializeFields(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x00113f60, 0xa2)
i32 CTileActionEvent::SerializeFields(void* ar) {
    CFileMemBase* a = static_cast<CFileMemBase*>(ar);
    if (a == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    a->Write(&m_actionCode, sizeof(m_actionCode));
    a->Write(&m_tileX, sizeof(m_tileX));
    a->Write(&m_tileY, sizeof(m_tileY));
    a->Write(&m_cellKey, sizeof(m_cellKey));
    a->Write(&m_live, sizeof(m_live));
    a->Write(&m_playerFlags[0], sizeof(m_playerFlags[0]));
    a->Write(&m_playerFlags[1], sizeof(m_playerFlags[1]));
    a->Write(&m_playerFlags[2], sizeof(m_playerFlags[2]));
    a->Write(&m_playerFlags[3], sizeof(m_playerFlags[3]));
    return 1;
}

RVA(0x00114040, 0xa2)
i32 CTileActionEvent::DeserializeFields(void* ar) {
    CFileMemBase* a = static_cast<CFileMemBase*>(ar);
    if (a == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    a->Read(&m_actionCode, sizeof(m_actionCode));
    a->Read(&m_tileX, sizeof(m_tileX));
    a->Read(&m_tileY, sizeof(m_tileY));
    a->Read(&m_cellKey, sizeof(m_cellKey));
    a->Read(&m_live, sizeof(m_live));
    a->Read(&m_playerFlags[0], sizeof(m_playerFlags[0]));
    a->Read(&m_playerFlags[1], sizeof(m_playerFlags[1]));
    a->Read(&m_playerFlags[2], sizeof(m_playerFlags[2]));
    a->Read(&m_playerFlags[3], sizeof(m_playerFlags[3]));
    return 1;
}

#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/TileTriggerSwitchLogic.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BridgeTileId.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAreaEffectKind.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelCollisionInline.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCoordMacros.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchInline.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <string.h>

RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {

    for (i32 i = 0; i < 24; i++) {
        m_linkedSwitchKeys[i] = 0;
    }
    m_initialized = false;
}

RVA(0x00110460, 0x64)
i32 CTileTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    TrigLogicId typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    b32 active,
    i32 damageParam,
    i32 requiredPickupType
) {
    if (m_initialized != false) {
        return 0;
    }
    if (typeId == TRIGID_EXCLUSIVE_SWITCH_4 && rect[0].left == 0) {
        return 0;
    }
    memcpy(m_linkedSwitchKeys, rect, sizeof(m_linkedSwitchKeys));
    return Setup(owner, typeId, tileX, tileY, cellKey, active, damageParam, requiredPickupType);
}

RVA(0x001104f0, 0x56)
i32 CTileTriggerSwitchLogic::Setup(
    CTileTriggerContainer* owner,
    TrigLogicId typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    b32 active,
    i32 damageParam,
    i32 requiredPickupType
) {
    if (m_initialized) {
        return 0;
    }
    m_typeId = typeId;
    m_tileX = tileX;
    m_tileY = tileY;
    m_cellKey = cellKey;
    m_owner = owner;
    m_damageParam = damageParam;
    m_requiredPickupType = requiredPickupType;
    m_reserved1c = 0;
    m_active = active;
    m_initialized = true;
    return 1;
}

RVA(0x00110570, 0xfb)
i32 CTileTriggerSwitchLogic::SwitchDown() {
    i32 tileY = m_tileY;
    CGruntzMgr* reg = g_gameReg;
    CLevelPlane* layer = reg->World()->GetLevel()->m_mainPlane;
    i32 tileX = m_tileX;
    i32 v = layer->m_tileHandles[tileX + layer->m_tileRowOffsets[tileY]] + 1;
    SET_MAIN_PLANE_TILE(g_gameReg, tileX, tileY, v);

    DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, m_tileX, m_tileY)
    if (::PtInRect(&g_gameReg->m_viewBounds, px, py)) {
        PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_SWITCHDOWN");
    }
    m_active = true;
    return 1;
}

RVA(0x001106b0, 0xf4)
i32 CTileTriggerSwitchLogic::SwitchUp() {
    i32 tileY = m_tileY;
    CGruntzMgr* reg = g_gameReg;
    CLevelPlane* layer = reg->World()->GetLevel()->m_mainPlane;
    i32 tileX = m_tileX;
    i32 v = layer->m_tileHandles[tileX + layer->m_tileRowOffsets[tileY]] - 1;
    SET_MAIN_PLANE_TILE(g_gameReg, tileX, tileY, v);

    DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, m_tileX, m_tileY)
    if (::PtInRect(&g_gameReg->m_viewBounds, px, py)) {
        PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_SWITCHUP");
    }
    m_active = false;
    return 1;
}

RVA(0x001107f0, 0x1c)
CTileTriggerLogic::CTileTriggerLogic() {

    for (i32 i = 0; i < 24; i++) {
        m_linkKeys[i] = 0;
    }
    m_initialized = false;
}

RVA(0x00110820, 0x23)
i32 CTileTriggerLogic::HasLinkKey(i32 key) {
    for (i32 i = 0; i < 24; i++) {
        if (m_linkKeys[i] == key) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00110860, 0x2e6)
void CTileTriggerLogic::PlayMovementSound(TileCollisionKind type) {
    i32 px, py;
    CGruntzMgr* gameMgr;
    SoundCueRegistry* registry;
    switch (type) {
        case TILEKIND_ARROW_UP_B:
        case TILEKIND_ARROW_DOWN_B:
        case TILEKIND_ARROW_LEFT_B:
        case TILEKIND_ARROW_RIGHT_B:
            goto done;
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
            gameMgr = g_gameReg;
            if (::PtInRect(&gameMgr->m_viewBounds, px, py)) {
                registry = gameMgr->World()->SoundRegistry();
                if (registry->IsSilent() == false) {
                    SoundCue* cue = static_cast<SoundCue*>(registry->Lookup("GAME_PYRAMIDMOVE"));
                    if (cue) {
                        cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                    }
                }
            }
            return;
        case TILEKIND_WATERBRIDGE_DOWN:
        case TILEKIND_WATERBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            gameMgr = g_gameReg;
            if (::PtInRect(&gameMgr->m_viewBounds, px, py)) {
                registry = gameMgr->World()->SoundRegistry();
                if (registry->IsSilent() == false) {
                    SoundCue* cue =
                        static_cast<SoundCue*>(registry->Lookup("LEVEL_WATERBRIDGEMOVE"));
                    if (cue) {
                        cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                    }
                }
            }
            return;
        case TILEKIND_TOGGLEWATERBRIDGE_DOWN:
        case TILEKIND_TOGGLEWATERBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            gameMgr = g_gameReg;
            if (::PtInRect(&gameMgr->m_viewBounds, px, py)) {
                gameMgr->World()->SoundRegistry()->PlayCueIfElapsed("LEVEL_WATERBRIDGEMOVE");
            }
            return;
        case TILEKIND_DEATHBRIDGE_DOWN:
        case TILEKIND_DEATHBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            gameMgr = g_gameReg;
            if (::PtInRect(&gameMgr->m_viewBounds, px, py)) {
                gameMgr->World()->SoundRegistry()->PlayCueIfElapsed("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case TILEKIND_TOGGLEDEATHBRIDGE_DOWN:
        case TILEKIND_TOGGLEDEATHBRIDGE_UP:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            gameMgr = g_gameReg;
            if (::PtInRect(&gameMgr->m_viewBounds, px, py)) {
                gameMgr->World()->SoundRegistry()->PlayCueIfElapsed("LEVEL_DEATHBRIDGEMOVE");
            }
            return;
        case TILEKIND_CRUMBLEWATERBRIDGE:
        case TILEKIND_CRUMBLEDEATHBRIDGE:
            py = (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX;
            px = (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX;
            gameMgr = g_gameReg;
            if (::PtInRect(&gameMgr->m_viewBounds, px, py)) {
                gameMgr->World()->SoundRegistry()->PlayCueIfElapsed("LEVEL_CRUMBLE");
            }
            return;
    }
done:
    return;
}

// @early-stop
RVA(0x00110c10, 0xeee)
i32 CTileTriggerLogic::Tick() {
    CDDrawSurfaceMgr* world = g_gameReg->World();
    CTileTriggerTransition* trans = NULL;

    TileCollisionKind srcId = PbResolveCell(world->GetLevel(), m_tileX, m_tileY);

    {
        CPoint pt(
            (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX,
            (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX
        );
        if (PtInRect(&g_gameReg->m_viewBounds, pt) && srcId != TILEKIND_REDPYRAMID_UP
            && srcId != TILEKIND_REDPYRAMID_DOWN) {
            CGameObject* trig = world->ChildGroup()->CreateSprite(
                0,
                pt.x,
                pt.y,
                0,
                "TileTriggerTransition",
                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
            );
            if (trig == NULL) {
                return 0;
            }
            trig->GetLogicRecord()->Dispatch(trig);
            trans = static_cast<CTileTriggerTransition*>(trig->GetLogicRecord()->UserLogic());
        }
    }

    CString key;
    CString anim;

    switch (srcId) {
        case TILEKIND_ARROW_UP_B: {
            if (trans != NULL) {
                trans->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            SET_MAIN_PLANE_TILE(reg, tx, ty, 0xca);
            break;
        }
        case TILEKIND_ARROW_DOWN_B: {
            if (trans != NULL) {
                trans->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            SET_MAIN_PLANE_TILE(reg, tx, ty, 0xc9);
            break;
        }
        case TILEKIND_ARROW_LEFT_B: {
            if (trans != NULL) {
                trans->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            SET_MAIN_PLANE_TILE(reg, tx, ty, 0xcc);
            break;
        }
        case TILEKIND_ARROW_RIGHT_B: {
            if (trans != NULL) {
                trans->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                trans = NULL;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            SET_MAIN_PLANE_TILE(reg, tx, ty, 0xcb);
            break;
        }

        case TILEKIND_REDPYRAMID_DOWN:
        case TILEKIND_REDPYRAMID_UP: {
            for (i32 gx = 0; gx < world->GetLevel()->m_mainPlane->GetTileColumns(); gx++) {
                i32 pxX = (gx << TILE_SHIFT_PX) + TILE_HALF_PX;
                for (i32 gy = 0; gy < world->GetLevel()->m_mainPlane->GetTileRows(); gy++) {
                    i32 pxY = (gy << TILE_SHIFT_PX) + TILE_HALF_PX;
                    i32 hit = 0;
                    if (PbResolveCell(world->GetLevel(), gx, gy) == TILEKIND_REDPYRAMID_UP) {
                        CGruntzMgr* reg = g_gameReg;
                        SET_MAIN_PLANE_TILE(reg, gx, gy, 0xfd);
                        anim = "GAME_PYRAMIDUP";
                        hit = 1;
                    } else if (PbResolveCell(world->GetLevel(), gx, gy)
                               == TILEKIND_REDPYRAMID_DOWN) {
                        CGruntzMgr* reg = g_gameReg;
                        SET_MAIN_PLANE_TILE(reg, gx, gy, 0xfe);
                        anim = "GAME_PYRAMIDDOWN";
                        hit = 1;
                    }
                    if (hit != 0) {
                        CPoint pt(pxX, pxY);
                        if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
                            CGameObject* o = world->ChildGroup()->CreateSprite(
                                0,
                                pt.x,
                                pt.y,
                                0,
                                "TileTriggerTransition",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            if (o == NULL) {
                                return 0;
                            }
                            o->GetLogicRecord()->Dispatch(o);
                            CTileTriggerTransition* lg = static_cast<CTileTriggerTransition*>(
                                o->GetLogicRecord()->UserLogic()
                            );
                            if (lg->ApplyAnimation("GAME_REDPYRAMIDZ", PbStr(anim)) == 0) {
                                lg->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                            }
                        }
                    }
                }
            }
            PlayMovementSound(srcId);
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
            TileCollisionKind now = PbResolveCell(world->GetLevel(), m_tileX, m_tileY);
            i32 tx = m_tileX;
            i32 ty = m_tileY;
            CGruntzMgr* reg = g_gameReg;
            CLevelPlane* pl = reg->m_world->GetLevel()->m_mainPlane;
            if (now == TILEKIND_GREENPYRAMID_UP) {
                SET_WORKER_HOST_CELL(pl, tx, ty, 0xfb);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xfb);
            } else {
                SET_WORKER_HOST_CELL(pl, tx, ty, 0xfc);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xfc);
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
            TileCollisionKind now = PbResolveCell(world->GetLevel(), m_tileX, m_tileY);
            i32 tx = m_tileX;
            i32 ty = m_tileY;
            CGruntzMgr* reg = g_gameReg;
            CLevelPlane* pl = reg->m_world->GetLevel()->m_mainPlane;
            if (now == TILEKIND_PURPLEPYRAMID_UP) {
                SET_WORKER_HOST_CELL(pl, tx, ty, 0xff);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xff);
            } else {
                SET_WORKER_HOST_CELL(pl, tx, ty, 0x100);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0x100);
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
            TileCollisionKind now = PbResolveCell(world->GetLevel(), m_tileX, m_tileY);
            i32 tx = m_tileX;
            i32 ty = m_tileY;
            CGruntzMgr* reg = g_gameReg;
            CLevelPlane* pl = reg->m_world->GetLevel()->m_mainPlane;
            if (now == TILEKIND_ORANGEPYRAMID_UP) {
                SET_WORKER_HOST_CELL(pl, tx, ty, 0xf7);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xf7);
            } else {
                SET_WORKER_HOST_CELL(pl, tx, ty, 0xf8);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xf8);
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
            TileCollisionKind now = PbResolveCell(world->GetLevel(), m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_BLACKPYRAMID_UP) {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, 0xf9);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xf9);
            } else {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, 0xfa);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xfa);
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
            TileCollisionKind now = PbResolveCellHandle(world->GetLevel(), m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_WHITEPYRAMID_UP) {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, 0xf5);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xf5);
            } else {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, 0xf6);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xf6);
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
            TileCollisionKind now = PbResolveCellHandle(world->GetLevel(), m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_CHECKPOINTPYRAMID_UP) {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, 0xd5);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xd5);
            } else {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, 0xd6);
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, 0xd6);
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
            TileCollisionKind now = PbResolveCellHandle(world->GetLevel(), m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_WATERBRIDGE_UP) {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, IDX(BRIDGETILE_WATER_UP));
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, IDX(BRIDGETILE_WATER_UP));
            } else {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, IDX(BRIDGETILE_WATER_DOWN));
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, IDX(BRIDGETILE_WATER_DOWN));
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
            TileCollisionKind now = PbResolveCellHandle(world->GetLevel(), m_tileX, m_tileY);
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            if (now == TILEKIND_DEATHBRIDGE_UP) {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, IDX(BRIDGETILE_DEATH_UP));
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, IDX(BRIDGETILE_DEATH_UP));
            } else {
                reg->m_world->GetLevel()->m_mainPlane->SetCell(tx, ty, IDX(BRIDGETILE_DEATH_DOWN));
                reg->GetTileGrid()->ComputeCellFlags(tx, ty, IDX(BRIDGETILE_DEATH_DOWN));
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
            if (world->GetLevel()->LookupTile(m_tileX, m_tileY) == TILEKIND_TOGGLEWATERBRIDGE_UP) {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, IDX(BRIDGETILE_TOGGLE_WATER_UP));
            } else {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, IDX(BRIDGETILE_TOGGLE_WATER_DOWN));
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
            if (world->GetLevel()->LookupTile(m_tileX, m_tileY) == TILEKIND_TOGGLEDEATHBRIDGE_UP) {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, IDX(BRIDGETILE_TOGGLE_DEATH_UP));
            } else {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, IDX(BRIDGETILE_TOGGLE_DEATH_DOWN));
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
            trans->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        }
    }
    PlayMovementSound(srcId);
    return 1;
}

RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(i32 x, i32 y, i32 value) {
    CLevelPlane* grid = m_world->GetLevel()->m_mainPlane;
    i32 idx = grid->m_tileRowOffsets[y] + x;
    grid->m_tileHandles[idx] = value;

    m_tileGrid->ComputeCellFlags(x, y, value);
}

RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::AreMultiSwitchLinksActive() {
    if (m_active == false) {
        return 0;
    }

    POSITION pos = m_owner->GetIdleHeadPosition();
    b32 found = false;

    CTileTriggerLogic* child;
    while (pos != NULL) {
        if (found != false) {
            break;
        }
        child = m_owner->GetNextIdleLogic(pos);
        if (child != NULL && child->HasLinkKey(m_cellKey) != 0) {
            found = true;
        }
    }
    if (found == false) {
        g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_LINKSB_NO_OWNER));
        return 0;
    }

    for (i32 i = 0; i < 24; i++) {
        i32 key = child->GetLinkKeys()[i];
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindSwitchLogic(key, TRIGID_MULTI_SWITCH_3);
        if (c == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_LINKSB_KEY_MISS));
            return 0;
        }
        if (c->m_active == false) {
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

    b32 done = false;
    i32 counter = 0;
    CTileTriggerSwitchLogic::SwitchDown();
    i32 i = 0;
    while (done == false) {
        if (i >= 0x18) {
            return 1;
        }
        i32 key = m_linkedSwitchKeys[i];
        CTileTriggerSwitchLogic* node = m_owner->FindSwitchLogic(key, TRIGID_EXCLUSIVE_SWITCH_4);
        if (node == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_BCAST_KEY_MISS));
            return 0;
        }
        if (node->GetCellKey() != m_cellKey && node->m_active != false) {
            node->SwitchUp();
            b32 any = false;
            POSITION pos = m_owner->GetIdleHeadPosition();
            while (pos != NULL) {
                CTileTriggerLogic* o = m_owner->GetNextIdleLogic(pos);
                if (o != NULL && o->HasLinkKey(node->GetCellKey())) {
                    o->Tick();
                    counter++;
                    any = true;
                }
            }
            if (any == false) {
                g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_BCAST_NO_CLAIM));
                return 0;
            }
        }
        i++;
        if (m_linkedSwitchKeys[i] == 0) {
            done = true;
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

RVA(0x001122a0, 0x241)
i32 CGiantRockLogic::BreakRock() {

    CDDrawSurfaceMgr* gameMgr = g_gameReg->World();

    i32 inRect = 0;
    POINT pt;
    SET_POINT_COMPONENTS(
        pt,
        (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX,
        (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX
    );
    if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
        inRect = 1;
    }

    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = m_replacementTiles[j * 3 + i];
            i32 py = j + m_tileY - 1;
            i32 px = i + m_tileX - 1;
            CGruntzMgr* reg = g_gameReg;
            SET_MAIN_PLANE_TILE(reg, px, py, value);
            i32 sx = ((i + m_tileX) << TILE_SHIFT_PX) - 0x10;
            i32 sy = ((j + m_tileY) << TILE_SHIFT_PX) - 0x10;
            if (inRect) {
                CreateParticlez(
                    gameMgr->ChildGroup(),
                    sx,
                    sy,
                    "LEVEL_ROCKBREAK",
                    "LEVEL_ROCKBREAK"
                );
            }
        }
    }

    DECLARE_TILE_CENTER_PIXEL_PAIR(cx, cy, m_tileX, m_tileY)
    g_gameReg->GetTriggerMgr()
        ->SpawnPickup(m_pickupType, cx, cy, static_cast<i32>(m_dutyOffSpan), 1, 0);

    if (m_textId != 0) {
        CGameObject* txt = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            cx,
            cy,
            0x17318,
            "InGameText",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        if (txt == NULL) {
            return 0;
        }
        txt->SetSmarts(m_textId);
    }

    DECLARE_TILE_CENTER_PIXEL_PAIR_Y_FIRST(by, bx, m_tileY, m_tileX)
    if (!::PtInRect(&g_gameReg->m_viewBounds, bx, by)) {
        return 0;
    }
    PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "LEVEL_ROCKBREAK");
    return 0;
}

// @early-stop
RVA(0x00112590, 0x166)
i32 CTileTriggerLogic::UncoverPickup(TileCollisionKind tileKind) {
    i32 tok = m_tileToken;
    if (tok != 0) {
        i32 ty = m_tileY;
        i32 tx = m_tileX;
        SET_MAIN_PLANE_TILE(g_gameReg, tx, ty, tok);
    } else {
        switch (tileKind) {
            case TILEKIND_COVERED_POWERUP: {
                i32 ty = m_tileY;
                CGruntzMgr* reg = g_gameReg;
                CLevelPlane* L = reg->World()->GetLevel()->m_mainPlane;
                i32 tx = m_tileX;
                i32 v = L->m_tileHandles[tx + L->m_tileRowOffsets[ty]] + 1;
                CLevelPlane* L2 = g_gameReg->World()->GetLevel()->m_mainPlane;
                SET_WORKER_HOST_CELL(L2, tx, ty, v);
                (reg->GetTileGrid())->ComputeCellFlags(tx, ty, v);
                break;
            }
            case TILEKIND_GAUNTLET_ROCK_B: {
                i32 ty = m_tileY;
                i32 tx = m_tileX;
                SET_MAIN_PLANE_TILE(g_gameReg, tx, ty, 0x5b);
                break;
            }
            case TILEKIND_GAUNTLET_ROCK_A: {
                i32 ty = m_tileY;
                i32 tx = m_tileX;
                SET_MAIN_PLANE_TILE(g_gameReg, tx, ty, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGruntzMgr* reg = g_gameReg;
    DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, m_tileX, m_tileY)
    reg->GetTriggerMgr()
        ->SpawnPickup(static_cast<PickupType>(m_dutyOnSpan), px, py, m_dutyOffSpan, 1, 0);
    if (m_leadInSpan != 0) {
        CGruntzMgr* registry = g_gameReg;
        CGameObject* rec =
            registry->World()
                ->ChildGroup()
                ->CreateSprite(0, px, py, 95000, "InGameText", WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE);
        if (rec == NULL) {
            return 0;
        }
        rec->SetSmarts(m_leadInSpan);
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
void CTileTriggerLogic::StartTimedSequence() {
    m_startClock = g_frameTime;
    m_owner->ActivateTimedLogic(this);
}

// @early-stop
RVA(0x001128b0, 0x88)
i32 CTileSecretTriggerLogic::Tick() {
    i32 oldTok = m_tileToken;
    if (oldTok == 0) {
        g_gameReg->ReportError(IDX(IDS_DEFAULT_ERROR), 0x451);
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    i32 grp = m_tileX;
    i32 idx = m_tileY;
    i32 newTok =
        mgr->World()
            ->m_level->m_mainPlane
            ->m_tileHandles[mgr->World()->m_level->m_mainPlane->m_tileRowOffsets[idx] + grp];
    SET_WORKER_HOST_CELL(g_gameReg->World()->m_level->m_mainPlane, grp, idx, oldTok);
    mgr->GetTileGrid()->ComputeCellFlags(grp, idx, oldTok);
    m_tileToken = newTok;
    return 1;
}

// @early-stop
RVA(0x00112970, 0xad)
i32 CTileTriggerLogic::UpdateTimedSequence(i32 unusedFrameDelta) {
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

                if (m_dutyOn == true) {
                    Tick();
                }
                return -1;
            }
        }
        u32 rem = elapsed % period;
        if (rem < m_dutyOnSpan) {
            if (m_dutyOn != false) {
                goto ret1;
            }
            Tick();
            m_dutyOn = true;
            if (m_typeTag != TRIGID_TILE_TRIGGER_24) {
                goto ret1;
            }
            return 0;
        }
        if (m_dutyOn != true) {
            goto ret1;
        }
        Tick();
        m_dutyOn = false;

        if (m_typeTag != TRIGID_TIME_TRIGGER_23) {
            return -1;
        }
    }
ret1:
    return 1;
}

RVA(0x00112a50, 0xdd)

i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    TrigLogicId typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    b32 active,
    i32 damageParam,
    i32 requiredPickupType
) {
    b32 ok;
    if (m_initialized != false) {
        ok = false;
    } else if (typeId == TRIGID_EXCLUSIVE_SWITCH_4 && rect[0].left == 0) {
        ok = false;
    } else {
        memcpy(m_linkedSwitchKeys, rect, sizeof(m_linkedSwitchKeys));
        ok = Setup(owner, typeId, tileX, tileY, cellKey, active, damageParam, requiredPickupType);
    }
    if (ok == false) {
        return 0;
    }
    DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, tileX, tileY)
    if (requiredPickupType != 0) {
        CWwdSpriteObject* spr = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            px,
            py,
            0,
            "BehindCandy",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPACE_SKIP_COLLISION
        );
        if (!spr) {
            return 0;
        }
        spr->GetLogicRecord()->Dispatch(spr);
        spr->SetImageFrameByName("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", requiredPickupType);
        if (spr->GetFrameImage() == NULL) {
            return 0;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00112b70, 0x5a)
i32 CCheckpointTriggerSwitchLogic::SwitchDown() {
    i32 tileY = m_tileY;
    CGruntzMgr* reg = g_gameReg;
    CLevelPlane* layer = reg->World()->GetLevel()->m_mainPlane;
    i32 tileX = m_tileX;
    i32 v = layer->m_tileHandles[tileX + layer->m_tileRowOffsets[tileY]] + 1;
    SET_MAIN_PLANE_TILE(g_gameReg, tileX, tileY, v);
    m_active = true;
    return 1;
}

// @early-stop
RVA(0x00112bf0, 0x5e)
i32 CCheckpointTriggerSwitchLogic::SwitchUp() {
    i32 tileY = m_tileY;
    CGruntzMgr* reg = g_gameReg;
    CLevelPlane* layer = reg->World()->GetLevel()->m_mainPlane;
    i32 tileX = m_tileX;
    i32 v = layer->m_tileHandles[tileX + layer->m_tileRowOffsets[tileY]] - 1;
    SET_MAIN_PLANE_TILE(g_gameReg, tileX, tileY, v);
    m_active = false;
    return 1;
}

RVA(0x00112c70, 0xc4)
i32 CTileTriggerSwitchLogic::AreCheckpointSwitchLinksActive() {
    if (m_active == false) {
        return 0;
    }

    POSITION pos = m_owner->GetIdleHeadPosition();
    b32 found = false;

    CTileTriggerLogic* child;
    while (pos != NULL) {
        if (found != false) {
            break;
        }
        child = m_owner->GetNextIdleLogic(pos);
        if (child != NULL && child->HasLinkKey(m_cellKey) != 0) {
            found = true;
        }
    }
    if (found == false) {
        g_gameReg->ReportError(IDX(TRIGERR_LINK_BROKEN), IDX(TRIGSITE_LINKS_NO_OWNER));
        return 0;
    }

    for (i32 i = 0; i < 24; i++) {
        i32 key = child->GetLinkKeys()[i];
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindSwitchLogic(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (c == NULL) {
            g_gameReg->ReportError(IDX(TRIGERR_LOOKUP_MISS), IDX(TRIGSITE_LINKS_KEY_MISS));
            return 0;
        }
        if (c->m_active == false) {
            return 0;
        }
    }
    return 0;
}

RVA(0x00112d80, 0xa)
CBrickStack::CBrickStack() {
    m_initialized = false;
}

// @early-stop
RVA(0x00112da0, 0x100)
i32 CBrickStack::SetBrickTile(BrickTileId code) {
    m_brickTile = code;
    if (IsRevealedToPlayer(g_curPlayer) == 0
        && static_cast<u32>(IDX(code) - IDX(BRICKTILE_BROWN_1)) <= 0x1a) {
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

    i32 ty = m_tileY;
    CGruntzMgr* reg = g_gameReg;
    CLevelPlane* layer = reg->World()->GetLevel()->m_mainPlane;
    i32 tx = m_tileX;
    if (layer->m_tileHandles[tx + layer->m_tileRowOffsets[ty]] == IDX(code)) {
        return 0;
    }
    CLevelPlane* layer2 = g_gameReg->World()->GetLevel()->m_mainPlane;
    SET_WORKER_HOST_CELL(layer2, tx, ty, IDX(code));
    reg->GetTileGrid()->ComputeCellFlags(tx, ty, IDX(code));
    return 1;
}

// @early-stop
RVA(0x00112ee0, 0x42b)
i32 CBrickStack::BreakTopBrick(CGrunt* grunt) {
    BrickTileId newCode = m_brickTile;
    i32 effect = 0;
    switch (m_brickTile) {
        case BRICKTILE_RED_1:
            effect = IDX(BRICKTILE_RED_1);
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_BLUE_1:
            effect = IDX(BRICKTILE_BLUE_1);
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_GOLD_1:
            effect = IDX(BRICKTILE_GOLD_1);
            if (grunt != NULL) {
                break;
            }
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_BLACK_1:
            effect = IDX(BRICKTILE_BLACK_1);
            // fall through
        case BRICKTILE_BROWN_1:
            newCode = BRICKTILE_CLEARED;
            break;
        case BRICKTILE_RED_2_TOP:
            effect = IDX(BRICKTILE_RED_1);
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_BLUE_2_TOP:
            effect = IDX(BRICKTILE_BLUE_1);
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_GOLD_2_TOP:
            effect = IDX(BRICKTILE_GOLD_1);
            if (grunt != NULL) {
                break;
            }
            newCode = BRICKTILE_BROWN_1;
            break;
        case BRICKTILE_BLACK_2_TOP:
            effect = IDX(BRICKTILE_BLACK_1);
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
            effect = IDX(BRICKTILE_RED_1);
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_BLUE_3_TOP:
            effect = IDX(BRICKTILE_BLUE_1);
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_GOLD_3_TOP:
            effect = IDX(BRICKTILE_GOLD_1);
            if (grunt != NULL) {
                break;
            }
            newCode = BRICKTILE_BROWN_2;
            break;
        case BRICKTILE_BLACK_3_TOP:
            effect = IDX(BRICKTILE_BLACK_1);
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

    BrickTileId brickEffect = static_cast<BrickTileId>(effect);
    if (effect != 0 && grunt != NULL) {
        if (brickEffect == BRICKTILE_RED_1) {
            grunt->ApplyPickup(PICKUP_NONE, 1, 0, 0);
            grunt->SetBusy(false);
        } else if (brickEffect == BRICKTILE_BLUE_1) {
            g_gameReg->GetTriggerMgr()->ApplyGruntAreaEffect(
                (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX,
                (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX,
                1,
                GRUNT_AREA_EFFECT_TELEPORT,
                -1
            );
        } else if (brickEffect == BRICKTILE_GOLD_1) {
            DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, m_tileX, m_tileY)
            if (::PtInRect(&g_gameReg->m_viewBounds, px, py)
                && g_gameReg->World()->SoundRegistry()->IsSilent() == false) {
                SoundCue* snd = static_cast<SoundCue*>(
                    g_gameReg->World()->SoundRegistry()->Lookup("GRUNTZ_NORMALGRUNT_IMPACTMM3")
                );
                if (snd != NULL) {
                    snd->PlayIfElapsed(static_cast<i32>(g_soundVolumePercent), 0, 0, false);
                }
            }
            i32 slot = grunt->GetPlayerIndex();
            if (slot == IDX(PLAYER_SLOT_ALL)) {
                i32* flags = m_revealedToPlayer;
                flags[0] = 1;
                flags[1] = 1;
                flags[2] = 1;
                flags[3] = 1;
                SetBrickTile(m_brickTile);
                return 0;
            }
            m_revealedToPlayer[slot] = 1;
            SetBrickTile(m_brickTile);
            return 0;
        } else if (brickEffect == BRICKTILE_BLACK_1) {
            g_gameReg->GetTriggerMgr()->SpawnExplosion(
                (m_tileX << TILE_SHIFT_PX) + TILE_HALF_PX,
                (m_tileY << TILE_SHIFT_PX) + TILE_HALF_PX,
                -1,
                2
            );
        }
    }

    DECLARE_TILE_CENTER_PIXEL_PAIR(px, py, m_tileX, m_tileY)
    if (::PtInRect(&g_gameReg->m_viewBounds, px, py)) {
        CWwdSpriteObject* spr = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            px,
            py,
            SORTKEY_ACTOR_BEHIND,
            "Particlez",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        if (spr != NULL) {
            spr->SetAnimationByName("GAME_BRICKBREAK", 0);

            switch (brickEffect) {
                case BRICKTILE_RED_1:
                    spr->SetImageSetByName("GAME_REDBRICKBREAK");
                    break;
                case BRICKTILE_BLUE_1:
                    spr->SetImageSetByName("GAME_BLUEBRICKBREAK");
                    break;
                case BRICKTILE_GOLD_1:
                    spr->SetImageSetByName("GAME_GOLDBRICKBREAK");
                    break;
                case BRICKTILE_BLACK_1:
                    spr->SetImageSetByName("GAME_BLACKBRICKBREAK");
                    break;
                default:
                    spr->SetImageSetByName("GAME_BRICKBREAK");
                    if (spr->GetFrameImage() == NULL) {
                        spr->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                    }
                    break;
            }
        }
    }

    if (newCode != m_brickTile) {
        SetBrickTile(newCode);
    }
    return newCode == BRICKTILE_CLEARED;
}

// @early-stop
RVA(0x00113420, 0x358)
i32 CBrickStack::AddTopBrick(PickupType toolId, PlayerSlot playerSlot) {
    if (toolId == PICKUP_BROWNBRICK) {
        switch (m_brickTile) {
            case BRICKTILE_BROWN_1:
                m_brickTile = BRICKTILE_BROWN_2;
                break;
            case BRICKTILE_RED_1:
                m_brickTile = BRICKTILE_RED_2_LOW;
                break;
            case BRICKTILE_BLUE_1:
                m_brickTile = BRICKTILE_BLUE_2_LOW;
                break;
            case BRICKTILE_GOLD_1:
                m_brickTile = BRICKTILE_GOLD_2_LOW;
                break;
            case BRICKTILE_BLACK_1:
                m_brickTile = BRICKTILE_BLACK_2_LOW;
                break;
            case BRICKTILE_BROWN_2:
                m_brickTile = BRICKTILE_BROWN_3;
                break;
            case BRICKTILE_RED_2_LOW:
                m_brickTile = BRICKTILE_RED_3_LOW;
                break;
            case BRICKTILE_RED_2_TOP:
                m_brickTile = BRICKTILE_RED_3_MID;
                break;
            case BRICKTILE_BLUE_2_LOW:
                m_brickTile = BRICKTILE_BLUE_3_LOW;
                break;
            case BRICKTILE_BLUE_2_TOP:
                m_brickTile = BRICKTILE_BLUE_3_MID;
                break;
            case BRICKTILE_GOLD_2_LOW:
                m_brickTile = BRICKTILE_GOLD_3_LOW;
                break;
            case BRICKTILE_GOLD_2_TOP:
                m_brickTile = BRICKTILE_GOLD_3_MID;
                break;
            case BRICKTILE_BLACK_2_LOW:
                m_brickTile = BRICKTILE_BLACK_3_LOW;
                break;
            case BRICKTILE_BLACK_2_TOP:
                m_brickTile = BRICKTILE_BLACK_3_MID;
                break;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_REDBRICK) {
        switch (m_brickTile) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_brickTile = BRICKTILE_RED_2_TOP;
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
                m_brickTile = BRICKTILE_RED_3_TOP;
                break;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_BLUEBRICK) {
        switch (m_brickTile) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_brickTile = BRICKTILE_BLUE_2_TOP;
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
                m_brickTile = BRICKTILE_BLUE_3_TOP;
                break;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_BLACKBRICK) {
        switch (m_brickTile) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_brickTile = BRICKTILE_BLACK_2_TOP;
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
                m_brickTile = BRICKTILE_BLACK_3_TOP;
                break;
            default:
                return 0;
        }
    } else if (toolId == PICKUP_GOLDBRICK) {
        switch (m_brickTile) {
            case BRICKTILE_BROWN_1:
            case BRICKTILE_RED_1:
            case BRICKTILE_BLUE_1:
            case BRICKTILE_GOLD_1:
            case BRICKTILE_BLACK_1:
                m_brickTile = BRICKTILE_GOLD_2_TOP;
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
                m_brickTile = BRICKTILE_GOLD_3_TOP;
                break;
            default:
                return 0;
        }
    }

    i32* flags = m_revealedToPlayer;
    memset(flags, 0, sizeof(m_revealedToPlayer));
    if (playerSlot == PLAYER_SLOT_ALL) {
        flags[0] = 1;
        flags[1] = 1;
        flags[2] = 1;
        flags[3] = 1;
    } else {
        m_revealedToPlayer[IDX(playerSlot)] = 1;
    }
    SetBrickTile(m_brickTile);
    return 1;
}

RVA(0x00113860, 0x3b)
i32 CTileTriggerSwitchLogic::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (!SaveState(ar)) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (!LoadState(ar)) {
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
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    ar->Write(&m_tileX, sizeof(m_tileX));
    ar->Write(&m_tileY, sizeof(m_tileY));
    ar->Write(&m_cellKey, sizeof(m_cellKey));
    ar->Write(&m_active, sizeof(m_active));
    ar->Write(&m_damageParam, sizeof(m_damageParam));
    ar->Write(&m_reserved1c, sizeof(m_reserved1c));
    ar->Write(&m_initialized, sizeof(m_initialized));
    ar->Write(&m_requiredPickupType, sizeof(m_requiredPickupType));
    i32* p = m_linkedSwitchKeys;
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
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    s->Read(&m_tileX, sizeof(m_tileX));
    s->Read(&m_tileY, sizeof(m_tileY));
    s->Read(&m_cellKey, sizeof(m_cellKey));
    s->Read(&m_active, sizeof(m_active));
    s->Read(&m_damageParam, sizeof(m_damageParam));
    s->Read(&m_reserved1c, sizeof(m_reserved1c));
    s->Read(&m_initialized, sizeof(m_initialized));
    s->Read(&m_requiredPickupType, sizeof(m_requiredPickupType));
    i32* p = m_linkedSwitchKeys;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, sizeof(*p));
        p++;
    }
    return 1;
}

RVA(0x00113a90, 0x3b)
i32 CTileTriggerLogic::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (Serialize(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (Deserialize(ar) == 0) {
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
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    s->Write(&m_tileX, sizeof(m_tileX));
    s->Write(&m_tileY, sizeof(m_tileY));
    s->Write(&m_cellKey, sizeof(m_cellKey));
    s->Write(&m_reserved14, sizeof(m_reserved14));
    s->Write(&m_reserved18, sizeof(m_reserved18));
    s->Write(&m_initialized, sizeof(m_initialized));
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
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    s->Read(&m_tileX, sizeof(m_tileX));
    s->Read(&m_tileY, sizeof(m_tileY));
    s->Read(&m_cellKey, sizeof(m_cellKey));
    s->Read(&m_reserved14, sizeof(m_reserved14));
    s->Read(&m_reserved18, sizeof(m_reserved18));
    s->Read(&m_initialized, sizeof(m_initialized));
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
i32 CGiantRockLogic::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    if (CTileTriggerLogic::SerializeDispatch(ar, mode, typeId, payload) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (SaveRockFields(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (LoadRockFields(ar) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

RVA(0x00113dd0, 0x7b)
i32 CGiantRockLogic::SaveRockFields(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    s->Write(&m_pickupType, sizeof(m_pickupType));
    s->Write(&m_textId, sizeof(m_textId));

    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(&m_replacementTiles[r * 3 + c], sizeof(m_replacementTiles[r * 3 + c]));
        }
    }
    return 1;
}

RVA(0x00113e70, 0x7b)
i32 CGiantRockLogic::LoadRockFields(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    s->Read(&m_pickupType, sizeof(m_pickupType));
    s->Read(&m_textId, sizeof(m_textId));

    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Read(&m_replacementTiles[r * 3 + c], sizeof(m_replacementTiles[r * 3 + c]));
        }
    }
    return 1;
}

RVA(0x00113f10, 0x3b)
i32 CBrickStack::Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
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
i32 CBrickStack::SerializeFields(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    ar->Write(&m_brickTile, sizeof(m_brickTile));
    ar->Write(&m_tileX, sizeof(m_tileX));
    ar->Write(&m_tileY, sizeof(m_tileY));
    ar->Write(&m_cellKey, sizeof(m_cellKey));
    ar->Write(&m_initialized, sizeof(m_initialized));
    ar->Write(&m_revealedToPlayer[0], sizeof(m_revealedToPlayer[0]));
    ar->Write(&m_revealedToPlayer[1], sizeof(m_revealedToPlayer[1]));
    ar->Write(&m_revealedToPlayer[2], sizeof(m_revealedToPlayer[2]));
    ar->Write(&m_revealedToPlayer[3], sizeof(m_revealedToPlayer[3]));
    return 1;
}

RVA(0x00114040, 0xa2)
i32 CBrickStack::DeserializeFields(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    ar->Read(&m_brickTile, sizeof(m_brickTile));
    ar->Read(&m_tileX, sizeof(m_tileX));
    ar->Read(&m_tileY, sizeof(m_tileY));
    ar->Read(&m_cellKey, sizeof(m_cellKey));
    ar->Read(&m_initialized, sizeof(m_initialized));
    ar->Read(&m_revealedToPlayer[0], sizeof(m_revealedToPlayer[0]));
    ar->Read(&m_revealedToPlayer[1], sizeof(m_revealedToPlayer[1]));
    ar->Read(&m_revealedToPlayer[2], sizeof(m_revealedToPlayer[2]));
    ar->Read(&m_revealedToPlayer[3], sizeof(m_revealedToPlayer[3]));
    return 1;
}

RVA(0x00114120, 0x70)
i32 SoundCueRegistry::PlayCueIfElapsed(const char* key) {
    if (m_silentMode != false) {
        return 0;
    }
    SoundCue* found = FindCue(key);
    if (found == NULL) {
        return 0;
    }
    b32 soundEnabled = g_soundEnabled;
    i32 volumePercent = g_soundVolumePercent;
    if (soundEnabled == false) {
        return 0;
    }
    SoundCue* cue = found;

    if (g_soundCueTimeMs - static_cast<u32>(cue->m_lastPlayTimeMs)
        >= static_cast<u32>(cue->m_replayDelayMs)) {
        cue->m_lastPlayTimeMs = g_soundCueTimeMs;
        return cue->GetSound()->AcquireAndPlay(volumePercent, 0, 0, false);
    }
    return 0;
}

#include <string.h>         // memcpy -> the /Oi `rep movsd` in BuildSmall
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Io/FileMem.h>        // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Mfc.h>
#include <rva.h>

#include <Gruntz/GruntzMgr.h> // the REAL singleton class
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerContainer.h> // the owner container (m_owner and its four CPtrLists)
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileActionEvent.h>
#include <Wwd/WwdFile.h> // CDDrawWorkerHost - the canonical plane
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TileTriggerTransition.h> // Tick's spawned transition leaf (ApplyAnimation)

// TileActionEvent.cpp - the per-tile game-action event record (trace placeholder
// tomalla-108). Methods in ascending retail-RVA order. The record shape comes from
// <Gruntz/TileActionEvent.h>; the serializer is the shared CFileMemBase; the game
// registry singleton (g_gameReg) is modeled here with only the offsets these paths
// touch. All engine callees are reloc-masked (no body).
//
// BANKED (code byte-exact, 100% fuzzy): ResetFlag (0x112d80), SetActionCode
//   (0x112da0), MorphByTool (0x113420), Serialize (0x113f10), SerializeFields
//   (0x113f60). The big Process (0x112ee0) is a complete logical reconstruction
//   parked at the two-jump-table wall (@early-stop) for the final sweep.
// <Mfc.h> (not <Win32.h>): UserLogic.h pulls afx via ButeMgr.h/String.h, so the
// umbrella must be the MFC superset kept first (mfc-wall-is-breakable doctrine).
#include <Gruntz/CurPlayer.h>
// ---------------------------------------------------------------------------
// The game registry singleton (?g_gameReg@@3PAUWwdGameRegZ@@A at VA 0x64556c).
// Only the offsets this cluster reaches are modeled; reloc-masked DIR32.
// ---------------------------------------------------------------------------

RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {
    // vptr stamp is now IMPLICIT (real polymorphic class) - cl prepends
    // `mov [this], offset ??_7CTileTriggerSwitchLogic@@6B@`, exactly the retail
    // ctor's first instruction, replacing the manual struct stamp.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_initGate = 0;
}

RVA(0x00110460, 0x64)
i32 CTileTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    const RECT* rect,
    i32 linkGate,
    i32 a8,
    i32 a9
) {
    if (m_initGate != 0) {
        return 0;
    }
    if (typeId == TRIGID_EXCLUSIVE_SWITCH_4 && rect[0].left == 0) {
        return 0;
    }
    memcpy(m_block, rect, sizeof(m_block));
    return Setup(owner, typeId, tileX, tileY, cellKey, linkGate, a8, a9);
}

RVA(0x001104f0, 0x56)
i32 CTileTriggerSwitchLogic::Setup(
    CTileTriggerContainer* owner,
    i32 typeId,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 linkGate,
    i32 a8,
    i32 a9
) {
    if (m_initGate) {
        return 0;
    }
    m_typeId = typeId;
    m_08 = tileX;
    m_key0c = tileY;
    m_key1 = cellKey;
    m_owner = owner;
    m_18 = a8;
    m_28 = a9;
    m_1c = 0;
    m_linkGate = linkGate;
    m_initGate = 1;
    return 1;
}

RVA(0x001107f0, 0x1c)
CTileTriggerLogic::CTileTriggerLogic() {
    // m_block initialised before m_1c so the optimiser emits the rep stosl
    // first and reuses the zero register for the +0x1c store afterwards.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_initGate = 0;
}

RVA(0x00110820, 0x23)
i32 CTileTriggerLogic::FindIndexByKey(i32 key) {
    for (i32 i = 0; i < 24; i++) {
        if (m_block[i] == key) {
            return 1;
        }
    }
    return 0;
}

// Clamp (x,y) into the main plane's tile extent, read the tile handle straight out of
// the grid and ask the tile's image set for the collision kind at its (0,0) sub-pixel.
// `m_mainPlane` is deliberately re-read per use: retail loads it three times (once per
// clamp, once for the grid pair), which is what a cached local would NOT produce.
static __inline i32 PbResolveCell(CGameLevel* level, i32 x, i32 y) {
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
    if (cell == TILE_UNINIT || cell == TILE_CLEAR) {
        return 0;
    }
    // CObArray stores CObject*; the element cast is the devs' own (GameLevel.h).
    CTileImageSet* set = static_cast<CTileImageSet*>(level->m_imageSets[cell & 0xffff]);
    return set->GetCollisionAt(0, 0);
}

// The same probe spelled through CDDrawWorkerHost::GetTileHandle. That member lives in
// WwdFile.cpp, so this variant keeps the out-of-line call where PbResolveCell above
// inlines the two grid loads - the white/checkpoint/bridge arms of Tick use this one.
static __inline i32 PbResolveCellHandle(CGameLevel* level, i32 x, i32 y) {
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
    if (cell == TILE_UNINIT || cell == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(level->m_imageSets[cell & 0xffff]);
    return set->GetCollisionAt(0, 0);
}

// CString -> the `char*` CTileTriggerTransition::ApplyAnimation declares (retail's
// mangled name is ...PAD0@Z, so the parameter really is non-const). Lowers to the
// m_pchData load retail does (`mov ecx,[esp+0x10]`).
static __inline char* PbStr(const CString& s) {
    return const_cast<char*>(static_cast<const char*>(s));
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Tick (0x110c10) - slot 0, the pyramid/bridge tile-transition
// dispatcher, run on this trigger's own (m_tileX, m_tileY).
//
// Resolve the collision kind of the cell under the trigger. If that tile is inside the
// expanded view bounds AND is not one of the two RED-pyramid latch kinds, spawn a
// "TileTriggerTransition" sprite centred on it, run the sprite's worker notify and keep
// the bound CTileTriggerTransition.
//
// Then a 102-slot jump table over (kind - 15) picks the arm - `sema disasm 0x110c10
// --switch` reads it out of 0x511a50/0x511a98; 24 of the 102 selectors reach 17 real
// arms and the rest fall to the shared `return 0`. Every pyramid/bridge kind owns a
// consecutive (DOWN, UP) pair of ids, and each arm
//   - names the sprite key  ("GAME_<COLOR>PYRAMIDZ" / "LEVEL_<X>BRIDGE"), then the
//     transition geometry ("GAME_PYRAMIDUP"/"...DOWN", "LEVEL_BRIDGEUP"/"...DOWN")
//     chosen by comparing the ORIGINAL kind against the pair's UP member,
//   - RE-probes the very same cell and writes the matching cell id back through the
//     plane's tile grid + CMapMgr::ComputeCellFlags (the toggle/crumble bridge arms
//     instead re-drive the world height grid via CGruntzMgr::SetCellHeight).
// The GREEN/PURPLE/ORANGE/BLACK arms inline the grid probe/store; the WHITE/CHECKPOINT/
// WATER/DEATH bridge arms spell the same two operations through the out-of-line
// CDDrawWorkerHost::GetTileHandle / ::SetCell members - hence PbResolveCellHandle.
//
// The four low arms (15..18) have no sprite: they flag the just-spawned transition for
// self-destruct (m_38->m_flags |= 0x10000), drop it, and stamp one fixed cell id.
// The RED arm is the odd one out - it ignores its own tile and sweeps the WHOLE grid,
// converting every red-pyramid cell it finds and spawning a transition over each, then
// returns 0 like the unhandled kinds.
//
// The shared tail plays (key, geometry) on the surviving transition, flags the sprite
// for self-destruct if the animation will not start, fires LoadBridgeMove(kind) for the
// sound cue and returns 1.
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00110c10, 0xe3f)
i32 CTileTriggerLogic::Tick() {
    CDDrawSurfaceMgr* world = g_gameReg->m_world; // ebx (spilled to [esp+0x24])
    CTileTriggerTransition* trans = 0;            // [esp+0x1c] transition logic handle

    // ---- resolve the source cell id at this trigger's tile (the switch key) ----
    i32 srcId = PbResolveCell(world->m_level, m_tileX, m_tileY); // [esp+0x18]

    // ---- the PtInRect transition gate (rect = the view rect at +0x13c) ----
    {
        i32 sy = (m_tileY << 5) + 0x10;
        i32 sx = (m_tileX << 5) + 0x10;
        POINT pt;
        pt.x = sx;
        pt.y = sy;
        if (PtInRect(&g_gameReg->m_viewBounds, pt) && srcId != TILEKIND_REDPYRAMID_UP
            && srcId != TILEKIND_REDPYRAMID_DOWN) {
            CGameObject* trig =
                world->m_childGroup->CreateSprite(0, sx, sy, 0, "TileTriggerTransition", 0x40003);
            if (trig == 0) {
                return 0; // the pre-CString early exit (0x111140)
            }
            trig->m_7c->m_notify(trig);
            trans = static_cast<CTileTriggerTransition*>(trig->m_7c->m_logic);
        }
    }

    // ---- the two CString sprite-key temps (real locals -> the /GX dtor states) ----
    CString key;  // [esp+0x14] constructed first: GAME_<COLOR>PYRAMIDZ / LEVEL_<X>BRIDGE
    CString anim; // [esp+0x10] constructed second: GAME_PYRAMIDUP/DOWN, LEVEL_BRIDGEUP/DOWN

    switch (srcId) {
        case 15: {
            if (trans != 0) {
                trans->m_38->m_flags |= 0x10000;
                trans = 0;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xca;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xca);
            break;
        }
        case 16: {
            if (trans != 0) {
                trans->m_38->m_flags |= 0x10000;
                trans = 0;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xc9;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xc9);
            break;
        }
        case 17: {
            if (trans != 0) {
                trans->m_38->m_flags |= 0x10000;
                trans = 0;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xcc;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xcc);
            break;
        }
        case 18: {
            if (trans != 0) {
                trans->m_38->m_flags |= 0x10000;
                trans = 0;
            }
            i32 ty = m_tileY;
            i32 tx = m_tileX;
            CGruntzMgr* reg = g_gameReg;
            CDDrawWorkerHost* pl = reg->m_world->m_level->m_mainPlane;
            pl->m_tileGrid[pl->m_colOffsets[ty] + tx] = 0xcb;
            reg->m_tileGrid->ComputeCellFlags(tx, ty, 0xcb);
            break;
        }

        // The red pyramid is the level-wide one: sweep every cell of the grid, flip each
        // red-pyramid tile to its opposite state and give each one its own transition
        // sprite. `pxX`/`pxY` are the tile centres in pixels (tile*32 + 16).
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
                            if (o == 0) {
                                return 0;
                            }
                            o->m_7c->m_notify(o);
                            CTileTriggerTransition* lg =
                                static_cast<CTileTriggerTransition*>(o->m_7c->m_logic);
                            if (lg->ApplyAnimation("GAME_REDPYRAMIDZ", PbStr(anim)) == 0) {
                                lg->m_38->m_flags |= 0x10000;
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
            i32 now = PbResolveCell(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCell(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCell(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCell(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
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
            i32 now = PbResolveCellHandle(world->m_level, m_tileX, m_tileY);
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

        // The toggle bridges re-drive the world HEIGHT grid instead of the tile grid,
        // and they re-probe through CGameLevel::LookupTile rather than the image set.
        case TILEKIND_TOGGLEWATERBRIDGE_DOWN:
        case TILEKIND_TOGGLEWATERBRIDGE_UP: {
            key = "LEVEL_TOGGLEWATERBRIDGE";
            if (srcId == TILEKIND_TOGGLEWATERBRIDGE_UP) {
                anim = "LEVEL_BRIDGEUP";
            } else {
                anim = "LEVEL_BRIDGEDOWN";
            }
            if (world->m_level->LookupTile(m_tileX, m_tileY) == TILEKIND_TOGGLEWATERBRIDGE_UP) {
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
            if (world->m_level->LookupTile(m_tileX, m_tileY) == TILEKIND_TOGGLEDEATHBRIDGE_UP) {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, 0x109);
            } else {
                g_gameReg->SetCellHeight(m_tileX, m_tileY, 0x10a);
            }
            break;
        }

        // The crumble bridges have a single state: they hand the trigger's own tile token
        // to the height grid (no re-probe, no up/down geometry).
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

    if (trans != 0) {
        if (trans->ApplyAnimation(PbStr(key), PbStr(anim)) == 0) {
            trans->m_38->m_flags |= 0x10000;
        }
    }
    LoadBridgeMove(srcId);
    return 1;
}

RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinksB
// The FindChild(key, 3) variant of VerifyBlockLinks (byte-identical structure,
// different diagnostic codes 0x44d/0x44e and the slow-lookup kind arg 3 vs 8):
// gate on m_linkGate, find the owner's child that claims this object (FindIndexByKey),
// ack 0x44d on miss; then validate each nonzero block key resolves (FindChild
// (key, 3)) to a gated child, acking 0x44e on a lookup miss.
// ---------------------------------------------------------------------------
// EXACT since 2026-07-29. The old "this-spill frame wall" note was wrong twice over:
// the `push ecx` dword is the UNINITIALIZED `child` local (spelling `= 0` emits
// `xor edi,edi` and deletes the slot), and the hand-rolled `i32* p` block cursor stole
// the callee-saved register the loop COUNTER wants - subscripting `child->m_block[i]`
// lets strength reduction build the cursor and restores retail's `xor ebx,ebx` first.
// @interleaver CTileTriggerSwitchLogic::VerifyBlockLinksB emitted-in <boundary:
// GruntzMgr2.cpp SetCellHeight @0x111ec0 (before) + GroupOps.cpp Broadcast @0x112080
// (after)>. A /Gy first-use COMDAT the linker scattered between two OTHER units.
RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinksB() {
    if (m_linkGate == 0) {
        return 0;
    }
    // walk the owner CONTAINER's m_list1 (head @ container+0x20) - the 0x9c
    // CTileTriggerLogic children live there.
    POSITION pos = m_owner->m_list1.GetHeadPosition();
    i32 found = 0;
    // `child` is deliberately UNINITIALIZED: retail reserves a `push ecx` dword for it
    // and seeds the enregistered cursor from that (garbage) slot (`mov edi,[esp+0x10]`).
    // Spelling `= 0` emits `xor edi,edi` and drops the slot + all four `pop ecx`.
    CTileTriggerLogic* child;
    while (pos != 0) {
        if (found != 0) {
            break;
        }
        child = static_cast<CTileTriggerLogic*>(m_owner->m_list1.GetNext(pos));
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_LINKSB_NO_OWNER);
        return 0;
    }
    // Subscripted, not a hand-rolled `i32* p` cursor: retail inits the COUNTER first
    // (`xor ebx,ebx`) and lets strength reduction make the esi cursor - a user `p`
    // takes the callee-saved register and pushes the counter onto the dead child reg.
    for (i32 i = 0; i < 24; i++) {
        i32 key = child->m_block[i]; // child+0x3c (the child is a 0x9c CTileTriggerLogic)
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_MULTI_SWITCH_3);
        if (c == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_LINKSB_KEY_MISS);
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

// ---------------------------------------------------------------------------
// CTileExclusiveTriggerSwitchLogic::SwitchDown (0x112080; the class's slot-2
// override, vtable 0x1eaecc slot 2 -> this body) - chain the base SwitchDown,
// then walk this switch's m_block key array; each key must resolve
// (owner->FindChild(key, 4), acking 0x44f on a miss) to a sibling switch; for a
// resolved sibling that is not THIS switch and is link-gated, run its slot-3
// virtual, then Tick every m_list1 logic child that claims it (FindIndexByKey),
// acking 0x450 if none does.
// RE-HOMED from GroupOps.cpp (the whole `CGroupBroadcast`/`CFindNode`/
// classes: same layout field-for-field, and this RVA sits inside THIS TU's
// interval 0x110430..0x1140e2 - first-link contiguity says it was defined here).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00112080, 0x138)
i32 CTileExclusiveTriggerSwitchLogic::SwitchDown() {
    // retail: a DIRECT `call 0x2e0f` (the base slot-2 body's ILT thunk) - the
    // qualified base chain.
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
        if (node == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_BCAST_KEY_MISS);
            return 0;
        }
        if (node->m_key1 != m_key1 && node->m_linkGate != 0) {
            node->SwitchUp(); // virtual slot 3
            i32 any = 0;
            POSITION pos = m_owner->m_list1.GetHeadPosition();
            while (pos != 0) {
                CTileTriggerLogic* o =
                    static_cast<CTileTriggerLogic*>(m_owner->m_list1.GetNext(pos));
                if (o != 0 && o->FindIndexByKey(node->m_key1)) {
                    o->Tick(); // slot 0
                    counter++;
                    any = 1;
                }
            }
            if (any == 0) {
                g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_BCAST_NO_CLAIM);
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
    // The world holder: the ex-CWorldZ view IS CDDrawSurfaceMgr (one object at +0x30;
    // and the sound host at +0x28 - were declared identically on both.
    CDDrawSurfaceMgr* gameMgr = g_gameReg->m_world; // cached only for the loop sprite

    // (1) in-rect gate: is the tile center inside the view rect (+0x13c)?
    i32 inRect = 0;
    POINT pt;
    pt.y = (m_tileY << 5) + 0x10;
    pt.x = (m_tileX << 5) + 0x10;
    if (PtInRect(&g_gameReg->m_viewBounds, pt)) {
        inRect = 1;
    }

    // (2) 3x3 neighborhood: write each saved cell value into the level plane + notify
    // the tile grid; when in-rect, spawn a Particlez/LEVEL_ROCKBREAK sprite per cell.
    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = m_matrix[j * 3 + i];
            i32 px = i + m_tileX - 1;
            i32 py = j + m_tileY - 1;
            CDDrawWorkerHost* plane = g_gameReg->m_world->m_level->m_mainPlane;
            plane->m_tileGrid[plane->m_colOffsets[py] + px] = value;
            g_gameReg->m_tileGrid->ComputeCellFlags(px, py, value);
            if (inRect) {
                CWwdGameObjectA* spr = gameMgr->m_childGroup->CreateSprite(
                    0,
                    ((i + m_tileX) << 5) - 0x10,
                    ((j + m_tileY) << 5) - 0x10,
                    0xcf84f,
                    "Particlez",
                    0x40003
                );
                if (spr != 0) {
                    spr->ApplyName("LEVEL_ROCKBREAK");
                    spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                }
            }
        }
    }

    // (3) fire the command-grid effect at the tile center (cx/cy reused by step 4).
    i32 cx = (m_tileX << 5) + 0x10;
    i32 cy = (m_tileY << 5) + 0x10;
    g_gameReg->m_cmdGrid
        ->LoadPowerupIconSprites(m_powerupType, cx, cy, static_cast<i32>(m_dutyOffSpan), 1, 0);

    // (4) when +0xc4 is set, spawn an InGameText sprite carrying it.
    if (m_textId != 0) {
        CGameObject* txt = g_gameReg->m_world->m_childGroup
                               ->CreateSprite(0, cx, cy, 0x17318, "InGameText", 0x40003);
        if (txt == 0) {
            return;
        }
        txt->m_124 = m_textId;
    }

    // (5) on-screen + no active override -> play the LEVEL_ROCKBREAK cue.
    if ((m_tileX << 5) + 0x10 >= g_gameReg->m_viewBounds.right
        || (m_tileX << 5) + 0x10 < g_gameReg->m_viewBounds.left
        || (m_tileY << 5) + 0x10 >= g_gameReg->m_viewBounds.bottom
        || (m_tileY << 5) + 0x10 < g_gameReg->m_viewBounds.top) {
        return;
    }
    CDDrawSubMgrLeafScan* sreg =
        gameMgr
            ->m_soundRegistry; // m_28 typed CDDrawSubMgrLeafScan* on the canonical holder (GameRegistry.h)
    if (sreg->m_emitGate != 0) {
        return;
    }
    void* out_ob = 0;
    sreg->m_10.Lookup("LEVEL_ROCKBREAK", out_ob);
    LeafCue* out = static_cast<LeafCue*>(out_ob);
    if (out == 0) {
        return;
    }
    if (g_sndEnabled == 0) {
        return;
    }
    i32 kc = g_killCueClock;
    if (static_cast<u32>((kc - out->m_14)) < static_cast<u32>(out->m_18)) {
        return;
    }
    out->m_14 = kc;
    out->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::ApplyMove
// Edits the (m_08,m_0c) tile cell of the active layer: an explicit override m_34
// when set, else by the verb (0x1e->0x5a, 0x1f->0x5b, 0x21->cell+1), marks the
// cell dirty, flags the surrounding screen rect, and (when m_2c is set) posts an
// in-game-text record stamped with m_2c.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00112590, 0x166)
i32 CTileTriggerLogic::ApplyMove(i32 verb) {
    i32 v;
    if (m_tileToken != 0) {
        CGruntzMgr* reg = g_gameReg;
        CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
        L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] = m_tileToken;
        v = m_tileToken;
        (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, v);
    } else {
        switch (verb) {
            case 0x22: {
                CGruntzMgr* reg = g_gameReg;
                CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
                v = L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] + 1;
                CDDrawWorkerHost* L2 = reg->m_world->m_level->m_mainPlane;
                L2->m_tileGrid[L2->m_colOffsets[m_tileY] + m_tileX] = v;
                (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, v);
                break;
            }
            case 0x1f: {
                CGruntzMgr* reg = g_gameReg;
                CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] = 0x5b;
                (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, 0x5b);
                break;
            }
            case 0x1e: {
                CGruntzMgr* reg = g_gameReg;
                CDDrawWorkerHost* L = reg->m_world->m_level->m_mainPlane;
                L->m_tileGrid[L->m_colOffsets[m_tileY] + m_tileX] = 0x5a;
                (reg->m_tileGrid)->ComputeCellFlags(m_tileX, m_tileY, 0x5a);
                break;
            }
            default:
                break;
        }
    }
    CGruntzMgr* reg = g_gameReg;
    i32 py = (m_tileY << 5) + 0x10;
    i32 px = (m_tileX << 5) + 0x10;
    reg->m_cmdGrid->LoadPowerupIconSprites(m_dutyOnSpan, px, py, m_dutyOffSpan, 1, 0);
    if (m_leadInSpan != 0) {
        CGameObject* rec =
            reg->m_world->m_childGroup->CreateSprite(0, px, py, 95000, "InGameText", 0x40003);
        if (rec != 0) {
            rec->m_124 = m_leadInSpan;
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

// ---------------------------------------------------------------------------
// CTileSecretTriggerLogic::Tick (slot-0 override, 0x1128b0). The vtable slot map
// proves the identity: ??_7CTileSecretTriggerLogic@0x1eaf14 slot 0 holds this body
// via ILT thunk 0x18d4.
// The secret trigger's duty tick: swap this trigger's parked tile token with the
// one in the MAIN plane's tile grid at (m_08, m_0c), recompute the cell flags,
// and adopt the previously-parked token. An empty token reports the 0x8009/0x451
// diagnostic and returns 0.
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x001128b0, 0x88)
i32 CTileSecretTriggerLogic::Tick() {
    i32 oldTok = m_tileToken;
    if (oldTok == 0) {
        g_gameReg->ReportError(0x8009, 0x451);
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

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Classify
// Drives the command's on/off duty cycle off the running game clock: while the
// elapsed time is within the lead-in (m_2c) it stays active (+1); past it, the
// remainder modulo the on+off period (m_28+m_30) selects the on or off phase,
// firing the slot-0 tick and latching m_dutyOn on each edge.  Returns +1 (active),
// 0 (just turned on, one-shot of type 0x18) or -1 (just turned off, not 0x17).
// ---------------------------------------------------------------------------
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
                // The overflow arm returns -1 EITHER WAY - the guard only gates the
                // Tick(). Retail: `cmp [esi+0x38],1 / jne <the shared or eax,-1 block>`
                // (0x1128ba -> 0x11300f), then Tick() and a TAIL-DUPLICATED `or eax,-1`.
                // A `goto ret1` here would have to be `jne <mov eax,1>`, which retail is
                // not; and it is what forced the two exit blocks out of retail's order.
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
        // Inverted on purpose: cl gives the last conditional's FALL-THROUGH to the
        // if-BODY, so spelling the -1 exit as the body is what emits retail's
        // `cmp eax,0x17 / je <ret1, last block>` with `or eax,-1` falling through.
        if (m_typeTag != TRIGID_TIME_TRIGGER_23) {
            return -1;
        }
    }
ret1:
    return 1;
}

// Slot 2: reads the active tile layer's cell at (col,row), stores value+1 back, republishes
// it through the tile grid, and SETS the +0x14 flag.  Returns 1.
// @early-stop
RVA(0x00112b70, 0x5a)
i32 CCheckpointTriggerSwitchLogic::SwitchDown() {
    CGruntzMgr* reg = g_gameReg;
    CDDrawWorkerHost* layer = reg->m_world->m_level->m_mainPlane;
    i32 v = layer->m_tileGrid[m_08 + layer->m_colOffsets[m_key0c]] + 1;
    CDDrawWorkerHost* layer2 = reg->m_world->m_level->m_mainPlane;
    layer2->m_tileGrid[m_08 + layer2->m_colOffsets[m_key0c]] = v;
    (reg->m_tileGrid)->ComputeCellFlags(m_08, m_key0c, v);
    m_linkGate = 1;
    return 1;
}

// Slot 3: the decrement sibling - same cell read/write path, value-1, and CLEARS the +0x14
// flag.  Returns 1.
// @early-stop
RVA(0x00112bf0, 0x5e)
i32 CCheckpointTriggerSwitchLogic::SwitchUp() {
    CGruntzMgr* reg = g_gameReg;
    CDDrawWorkerHost* layer = reg->m_world->m_level->m_mainPlane;
    i32 v = layer->m_tileGrid[m_08 + layer->m_colOffsets[m_key0c]] - 1;
    CDDrawWorkerHost* layer2 = reg->m_world->m_level->m_mainPlane;
    layer2->m_tileGrid[m_08 + layer2->m_colOffsets[m_key0c]] = v;
    (reg->m_tileGrid)->ComputeCellFlags(m_08, m_key0c, v);
    m_linkGate = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinks
// Linkage validator: if this->m_linkGate is clear, succeed (return 0 short-circuit on
// the null gate).  Otherwise walk the owner's child list (head @ owner->m_20),
// asking each child's FindIndexByKey(this->m_key1) until one claims this object.
// If none does, ack diagnostic 0x452 and fail.  Then, for the claiming child,
// scan its 24-dword key block (child->m_block[4..27]): an empty slot succeeds;
// each nonzero key must resolve via owner->FindChild(key, 8) to a child whose
// m_linkGate is set (else fail, acking 0x453 when the lookup itself misses).
// Returns 1 on the early empty-slot success, 0 otherwise.
// ---------------------------------------------------------------------------
// EXACT since 2026-07-29 (twin of VerifyBlockLinksB 0x111f40): the `push ecx` dword is
// the UNINITIALIZED `child` local, and the block walk is a SUBSCRIPT, not a hand-rolled
// `i32* p` cursor - see the note there.
RVA(0x00112c70, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinks() {
    if (m_linkGate == 0) {
        return 0;
    }
    // walk the owner CONTAINER's m_list1 (head @ container+0x20) - the 0x9c
    // CTileTriggerLogic children live there.
    POSITION pos = m_owner->m_list1.GetHeadPosition();
    i32 found = 0;
    // `child` is deliberately UNINITIALIZED: retail reserves a `push ecx` dword for it
    // and seeds the enregistered cursor from that (garbage) slot (`mov edi,[esp+0x10]`).
    // Spelling `= 0` emits `xor edi,edi` and drops the slot + all four `pop ecx`.
    CTileTriggerLogic* child;
    while (pos != 0) {
        if (found != 0) {
            break;
        }
        child = static_cast<CTileTriggerLogic*>(m_owner->m_list1.GetNext(pos));
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->ReportError(TRIGERR_LINK_BROKEN, TRIGSITE_LINKS_NO_OWNER);
        return 0;
    }
    // Subscripted, not a hand-rolled `i32* p` cursor: retail inits the COUNTER first
    // (`xor ebx,ebx`) and lets strength reduction make the esi cursor - a user `p`
    // takes the callee-saved register and pushes the counter onto the dead child reg.
    for (i32 i = 0; i < 24; i++) {
        i32 key = child->m_block[i]; // child+0x3c (the child is a 0x9c CTileTriggerLogic)
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, TRIGID_CHECKPOINT_SWITCH_8);
        if (c == 0) {
            g_gameReg->ReportError(TRIGERR_LOOKUP_MISS, TRIGSITE_LINKS_KEY_MISS);
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
RVA(0x00112da0, 0xf0) // span includes the inline switch jump table (base COMDAT 0xf0)
i32 CTileActionEvent::SetActionCode(i32 code) {
    m_actionCode = code;
    if (m_playerFlags[g_curPlayer] == 0 && static_cast<u32>((code - 0x12f)) <= 0x1a) {
        switch (code) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                code = 0x12f;
                break;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                code = 0x130;
                break;
            case 0x131:
            case 0x135:
            case 0x136:
            case 0x137:
            case 0x13b:
            case 0x13c:
            case 0x13d:
            case 0x141:
            case 0x142:
            case 0x143:
            case 0x147:
            case 0x148:
            case 0x149:
                code = 0x131;
                break;
        }
    }
    // The plane chain is re-derived in EACH statement, not bound to a `grid` local:
    // MSVC5's CSE keeps the g_gameReg global load and the this-relative m_tileX/m_tileY
    // across the branch, but does NOT CSE a load through a loaded pointer - so retail
    // reads m_world->m_level->m_mainPlane->{m_colOffsets,m_tileGrid} once for the
    // compare and again for the store.
    i32 ty = m_tileY;
    i32 tx = m_tileX;
    if (g_gameReg->m_world->m_level->m_mainPlane
            ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[ty] + tx]
        == code) {
        return 0;
    }
    g_gameReg->m_world->m_level->m_mainPlane
        ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[ty] + tx] = code;
    g_gameReg->m_tileGrid->ComputeCellFlags(tx, ty, code);
    return 1;
}

// ===========================================================================
// CTileActionEvent::Process  (0x112ee0) - __thiscall, ret 4
// ===========================================================================
// @early-stop
RVA(0x00112ee0, 0x35e)
i32 CTileActionEvent::Process(CGrunt* brick) {
    i32 newCode = m_actionCode;
    i32 effect = 0;
    switch (m_actionCode) {
        case 0x12f:
            newCode = 0x12d;
            break;
        case 0x130:
            newCode = 0x12f;
            break;
        case 0x131:
            newCode = 0x130;
            break;
        case 0x132:
            effect = 0x132;
            newCode = 0x12d;
            break;
        case 0x133:
            newCode = 0x132;
            break;
        case 0x134:
            effect = 0x132;
            newCode = 0x12f;
            break;
        case 0x135:
            newCode = 0x133;
            break;
        case 0x136:
            newCode = 0x134;
            break;
        case 0x137:
            effect = 0x132;
            newCode = 0x130;
            break;
        case 0x138:
            effect = 0x138;
            newCode = 0x12d;
            break;
        case 0x139:
            newCode = 0x138;
            break;
        case 0x13a:
            effect = 0x138;
            newCode = 0x12f;
            break;
        case 0x13b:
            newCode = 0x139;
            break;
        case 0x13c:
            newCode = 0x13a;
            break;
        case 0x13d:
            effect = 0x138;
            newCode = 0x130;
            break;
        case 0x13e:
            effect = 0x13e;
            if (brick != 0) {
                break;
            }
            newCode = 0x12d;
            break;
        case 0x13f:
            newCode = 0x13e;
            break;
        case 0x140:
            effect = 0x13e;
            if (brick != 0) {
                break;
            }
            newCode = 0x12f;
            break;
        case 0x141:
            newCode = 0x13f;
            break;
        case 0x142:
            newCode = 0x140;
            break;
        case 0x143:
            effect = 0x13e;
            if (brick != 0) {
                break;
            }
            newCode = 0x130;
            break;
        case 0x144:
            effect = 0x144;
            newCode = 0x12d;
            break;
        case 0x145:
            newCode = 0x144;
            break;
        case 0x146:
            effect = 0x144;
            newCode = 0x12f;
            break;
        case 0x147:
            newCode = 0x145;
            break;
        case 0x148:
            newCode = 0x146;
            break;
        case 0x149:
            effect = 0x144;
            newCode = 0x130;
            break;
    }

    if (effect != 0 && brick != 0) {
        if (effect == 0x132) {
            brick->LoadGruntTypeTable(0, 1, 0, 0);
            brick->m_entranceActive = 0;
        } else if (effect == 0x138) {
            g_gameReg->m_cmdGrid->CombatCue((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, 1, 2, -1);
        } else if (effect == 0x13e) {
            i32 px = (m_tileX << 5) + 0x10;
            i32 py = (m_tileY << 5) + 0x10;
            if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
                && py < g_gameReg->m_viewBounds.bottom && py >= g_gameReg->m_viewBounds.top
                && g_gameReg->m_world->m_soundRegistry->m_emitGate == 0) {
                LeafCue* snd = static_cast<LeafCue*>(
                    g_gameReg->m_world->m_soundRegistry->Lookup("GRUNTZ_NORMALGRUNT_IMPACTMM3")
                );
                if (snd != 0) {
                    snd->PlayIfElapsed(static_cast<i32>(g_sndCueTag), 0, 0, 0);
                }
            }
            if (brick->m_tileOwnerHi == 5) {
                m_playerFlags[0] = 1;
                m_playerFlags[1] = 1;
                m_playerFlags[2] = 1;
                m_playerFlags[3] = 1;
                SetActionCode(m_actionCode);
                return 0;
            }
            m_playerFlags[brick->m_tileOwnerHi] = 1;
            SetActionCode(m_actionCode);
            return 0;
        } else if (effect == 0x144) {
            g_gameReg->m_cmdGrid
                ->LoadExplosionSprites((m_tileX << 5) + 0x10, (m_tileY << 5) + 0x10, -1, 2);
        }
    }

    i32 px = (m_tileX << 5) + 0x10;
    i32 py = (m_tileY << 5) + 0x10;
    if (px < g_gameReg->m_viewBounds.right && px >= g_gameReg->m_viewBounds.left
        && py < g_gameReg->m_viewBounds.bottom && py >= g_gameReg->m_viewBounds.top) {
        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup
                                   ->CreateSprite(0, px, py, 0xcf84f, "Particlez", 0x40003);
        if (spr != 0) {
            spr->ApplyLookupGeometry("GAME_BRICKBREAK", 0);
            // Inner dense byte-mapped switch on (effect - 0x132), span 0x12 (MSVC5
            // emits `lea eax,[edi-0x132]; cmp eax,0x12; ja default; mov dl,[bytemap];
            // jmp [jumptable+dl*4]`). effect is one of the four brick "base" codes
            // {0x132,0x138,0x13e,0x144} (set by the outer switch) -> its color break
            // sprite; anything else -> default GAME_BRICKBREAK (which sets the +0x8
            // uncached flag). Mapping verified against retail byte map @0x5132f8 /
            // jump table @0x5132e4.
            switch (effect) {
                case 0x132:
                    spr->ApplyName("GAME_REDBRICKBREAK");
                    break;
                case 0x138:
                    spr->ApplyName("GAME_BLUEBRICKBREAK");
                    break;
                case 0x13e:
                    spr->ApplyName("GAME_GOLDBRICKBREAK");
                    break;
                case 0x144:
                    spr->ApplyName("GAME_BLACKBRICKBREAK");
                    break;
                default:
                    spr->ApplyName("GAME_BRICKBREAK");
                    if (spr->m_layer == 0) {
                        spr->m_flags |= 0x10000;
                    }
                    break;
            }
        }
    }

    if (newCode != m_actionCode) {
        SetActionCode(newCode);
    }
    return newCode == 0x12d;
}

// Span = the base COMDAT (0x350), NOT the extent to the next function (0x440): both were
// measured 2026-07-29 and the COMDAT wins by a mile (92.1% vs 53.9%). Same for the two
// siblings above - SetActionCode 0xf0 beats 0x140 (66.6 vs 47.2) and Process 0x35e beats
// 0x540 (58.9 vs 47.4). The opposite choice is right when the tables sit BETWEEN the code
// and the next function with nothing else in the gap (see DispatchMsg @0xbf7c0 and
// ComputeCellFlags @0x77790); here the gap holds more than this function's own tables, so
// carrying it in makes both sides carve different content. MEASURE, do not assume.
RVA(0x00113420, 0x350) // span includes the 5 inline jump tables (base COMDAT 0x350)
i32 CTileActionEvent::MorphByTool(i32 toolId, i32 playerSlot) {
    if (toolId == 0x22) {
        switch (m_actionCode) {
            case 0x12f:
                m_actionCode = 0x130;
            // The COMMIT block lives INSIDE the first case (retail fallthrough
            // position at 0x113453); every other case in all five tool switches
            // jumps here (the shared jmp-0x113453 targets).
            commit: {
                // retail: zeroes thru a COPY (esi) of the flags base, ones thru
                // the base itself (eax) - both pointer locals live.
                i32* flags = m_playerFlags;
                i32* p = flags;
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
                p[3] = 0;
                if (playerSlot == 5) {
                    flags[0] = 1;
                    flags[1] = 1;
                    flags[2] = 1;
                    flags[3] = 1;
                } else {
                    flags[playerSlot] = 1;
                }
                SetActionCode(m_actionCode);
                return 1;
            }
            case 0x132:
                m_actionCode = 0x133;
                goto commit;
            case 0x138:
                m_actionCode = 0x139;
                goto commit;
            case 0x13e:
                m_actionCode = 0x13f;
                goto commit;
            case 0x144:
                m_actionCode = 0x145;
                goto commit;
            case 0x130:
                m_actionCode = 0x131;
                goto commit;
            case 0x133:
                m_actionCode = 0x135;
                goto commit;
            case 0x134:
                m_actionCode = 0x136;
                goto commit;
            case 0x139:
                m_actionCode = 0x13b;
                goto commit;
            case 0x13a:
                m_actionCode = 0x13c;
                goto commit;
            case 0x13f:
                m_actionCode = 0x141;
                goto commit;
            case 0x140:
                m_actionCode = 0x142;
                goto commit;
            case 0x145:
                m_actionCode = 0x147;
                goto commit;
            case 0x146:
                m_actionCode = 0x148;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == 0x23) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x134;
                goto commit;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x137;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == 0x24) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x13a;
                goto commit;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x13d;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == 0x26) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x146;
                goto commit;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x149;
                goto commit;
            default:
                return 0;
        }
    } else if (toolId == 0x25) {
        switch (m_actionCode) {
            case 0x12f:
            case 0x132:
            case 0x138:
            case 0x13e:
            case 0x144:
                m_actionCode = 0x140;
                goto commit;
            case 0x130:
            case 0x133:
            case 0x134:
            case 0x139:
            case 0x13a:
            case 0x13f:
            case 0x140:
            case 0x145:
            case 0x146:
                m_actionCode = 0x143;
                goto commit;
            default:
                return 0;
        }
    }

    goto commit;
}

RVA(0x00113860, 0x3b)
i32 CTileTriggerSwitchLogic::ValidateByType(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
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
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    ar->Write(&m_08, 4);
    ar->Write(&m_key0c, 4);
    ar->Write(&m_key1, 4);
    ar->Write(&m_linkGate, 4);
    ar->Write(&m_18, 4);
    ar->Write(&m_1c, 4);
    ar->Write(&m_initGate, 4);
    ar->Write(&m_28, 4);
    i32* p = m_block;
    i32 n = 24;
    do {
        ar->Write(p, 4);
        p++;
    } while (--n);
    return 1;
}

RVA(0x001139a0, 0xb4)
i32 CTileTriggerSwitchLogic::LoadState(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_08, 4);
    s->Read(&m_key0c, 4);
    s->Read(&m_key1, 4);
    s->Read(&m_linkGate, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_initGate, 4);
    s->Read(&m_28, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerLogic::ValidateByType - the 0x9c family's save/load dispatcher.
// Returns 0 if the archive is null; type 4 saves (Serialize), type 7 loads
// (Deserialize); any other type passes (returns 1).
//
// RE-HOMED from CTileTriggerSwitchLogic (0x8c): CTileTriggerFactory::Build calls it
// (ILT 0x1abe) at 0x117aa7 on a freshly-`new`ed 0x9c CTileTriggerLogic. Its two callees
// were modeled as `__stdcall TileSwitchCheckType4/7(void* obj)` free functions - but retail
// emits `push eax; call <rel32>` with ECX UNTOUCHED, i.e. they are __thiscall methods run on
// `this` (0x291e -> Serialize @0x113ae0, 0x3102 -> Deserialize @0x113c10). Same bytes; the
// free-function spelling only matched because `this` happened to still be live in ecx.
// ---------------------------------------------------------------------------
RVA(0x00113a90, 0x3b)
i32 CTileTriggerLogic::ValidateByType(void* archive, i32 mode, i32 typeId, i32 pObj) {
    if (archive == 0) {
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

// ---------------------------------------------------------------------------
// CTileTriggerLogic::Serialize  (the type-4 save ValidateByType dispatches to)
// Returns 0 if the stream is null or the active game-manager (g_gameReg+0x30) is
// null; otherwise transfers the scalar fields then the 24-dword m_block through
// the stream's Write slot and returns 1.
//
// RE-HOMED off the invented `CTileTriggerLogic` (see TileGridCommand.h @identity-TODO):
// ValidateByType reaches it with `this` in ecx, and that `this` is a 0x9c CTileTriggerLogic
// straight off the allocation site. Fields are the same offsets under this class's names
// (m_block -> m_block, m_dutyOn kept).
// ---------------------------------------------------------------------------
RVA(0x00113ae0, 0xe8)
i32 CTileTriggerLogic::Serialize(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_tileX, 4);
    s->Write(&m_tileY, 4);
    s->Write(&m_10, 4);
    s->Write(&m_14, 4);
    s->Write(&m_18, 4);
    s->Write(&m_initGate, 4);
    s->Write(&m_dutyOnSpan, 4);
    s->Write(&m_leadInSpan, 4);
    s->Write(&m_dutyOffSpan, 4);
    s->Write(&m_tileToken, 4);
    s->Write(&m_dutyOn, 4);
    s->Write(&m_startClock, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Write(p, 4);
        p++;
    }
    return 1;
}

RVA(0x00113c10, 0xe8)
i32 CTileTriggerLogic::Deserialize(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_tileX, 4);
    s->Read(&m_tileY, 4);
    s->Read(&m_10, 4);
    s->Read(&m_14, 4);
    s->Read(&m_18, 4);
    s->Read(&m_initGate, 4);
    s->Read(&m_dutyOnSpan, 4);
    s->Read(&m_leadInSpan, 4);
    s->Read(&m_dutyOffSpan, 4);
    s->Read(&m_tileToken, 4);
    s->Read(&m_dutyOn, 4);
    s->Read(&m_startClock, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

RVA(0x00113d40, 0x6f)
i32 CGiantRockLogic::ApplyByType(void* archive, i32 mode, i32 typeId, i32 pObj) {
    if (archive == 0) {
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

// ---------------------------------------------------------------------------
// CGiantRockLogic::SerializeMatrix
// Streams two header dwords (+0xc0, +0xc4) then the 3x3 dword matrix (+0x9c..) via
// the stream's Write slot.  Returns 0 if the stream or the active game-manager
// (g_gameReg+0x30) is null, else 1.
//
// RE-HOMED from CTileTriggerSwitchLogic. These +0xc0/+0xc4 writes are what made the old
// owner's array run to m_block[38] and overrun its 0x8c allocation - the contradiction that
// blocked the layout fix. They are CGiantRockLogic's own tail (0x9c base + 0x24 matrix + 8).
// ---------------------------------------------------------------------------
// EXACT since 2026-07-29: the old "esi/edi regalloc wall" was the hand-rolled `i32* p`
// matrix cursor - retail subscripts, so the cursor is a strength-reduced IV coalesced
// with `this`, and that is what decides which callee-saved register each of this/stream gets.
RVA(0x00113dd0, 0x7b)
i32 CGiantRockLogic::SerializeMatrix(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_powerupType, 4);
    s->Write(&m_textId, 4);
    // Subscripted, not a hand-rolled `i32* p`: retail's cursor is a strength-reduced
    // IV coalesced with `this` (`add esi,0x9c` / `add esi,4`), which is what decides
    // the this/stream callee-saved pair.
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(&m_matrix[r * 3 + c], 4);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGiantRockLogic::DeserializeMatrix (0x113e70) - the READ mirror of SerializeMatrix:
// streams two header dwords (+0xc0, +0xc4) then the 3x3 dword matrix (+0x9c..) via the
// stream's Read slot. Returns 0 if the stream or the active game-manager (g_gameReg+0x30)
// is null, else 1. This is the type-7 (load) apply ApplyByType dispatches to (thunk 0x3cd3).
// EXACT since 2026-07-29 (twin of SerializeMatrix): the residual was the hand-rolled
// `i32* p` cursor, not a register wall - see the note there.
RVA(0x00113e70, 0x7b)
i32 CGiantRockLogic::DeserializeMatrix(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_powerupType, 4);
    s->Read(&m_textId, 4);
    // Subscripted, not a hand-rolled `i32* p`: retail's cursor is a strength-reduced
    // IV coalesced with `this` (`add esi,0x9c` / `add esi,4`), which is what decides
    // the this/stream callee-saved pair.
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Read(&m_matrix[r * 3 + c], 4);
        }
    }
    return 1;
}

RVA(0x00113f10, 0x3b)
i32 CTileActionEvent::Serialize(void* ar, i32 mode, i32 typeId, i32 pObj) {
    if (ar == 0) {
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
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    a->Write(&m_actionCode, 4);
    a->Write(&m_tileX, 4);
    a->Write(&m_tileY, 4);
    a->Write(&m_cellKey, 4);
    a->Write(&m_live, 4);
    a->Write(&m_playerFlags[0], 4);
    a->Write(&m_playerFlags[1], 4);
    a->Write(&m_playerFlags[2], 4);
    a->Write(&m_playerFlags[3], 4);
    return 1;
}

RVA(0x00114040, 0xa2)
i32 CTileActionEvent::DeserializeFields(void* ar) {
    CFileMemBase* a = static_cast<CFileMemBase*>(ar);
    if (a == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    a->Read(&m_actionCode, 4);
    a->Read(&m_tileX, 4);
    a->Read(&m_tileY, 4);
    a->Read(&m_cellKey, 4);
    a->Read(&m_live, 4);
    a->Read(&m_playerFlags[0], 4);
    a->Read(&m_playerFlags[1], 4);
    a->Read(&m_playerFlags[2], 4);
    a->Read(&m_playerFlags[3], 4);
    return 1;
}

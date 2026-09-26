#include <rva.h>

#include <Gruntz/GruntEntranceMove.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntActionInline.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/MovingDeathTileId.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Lith/BDefs.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x0020e924)
static char s_gruntzEntrancezRessurect[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
DATA(0x0020e944)
static char s_gruntzEntrancezDrop[] = "GRUNTZ_ENTRANCEZ_DROP";
DATA(0x0020e960)
static char s_gruntzEntrancezThree[] = "GRUNTZ_ENTRANCEZ_THREE";
DATA(0x0020e97c)
static char s_gruntzEntrancezTwo[] = "GRUNTZ_ENTRANCEZ_TWO";
DATA(0x0020e9ac)
static char s_gruntzEntrancezOne[] = "GRUNTZ_ENTRANCEZ_ONE";
DATA(0x0020e9c8)
static char s_wgIdle5[] = "GRUNTZ_WINGZGRUNT_IDLE5";
DATA(0x0020e9e4)
static char s_wgIdle4[] = "GRUNTZ_WINGZGRUNT_IDLE4";
DATA(0x0020ea00)
static char s_wgIdle3[] = "GRUNTZ_WINGZGRUNT_IDLE3";
DATA(0x0020ea1c)
static char s_wgIdle2[] = "GRUNTZ_WINGZGRUNT_IDLE2";
DATA(0x0020ea38)
static char s_wgIdle1[] = "GRUNTZ_WINGZGRUNT_IDLE1";
DATA(0x0020ea54)
static char s_wgWalk[] = "GRUNTZ_WINGZGRUNT_WALK";
DATA(0x0020ea70)
static char s_seIdle[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_IDLE";
DATA(0x0020ea98)
static char s_sIdle[] = "GRUNTZ_WINGZGRUNT_SOUTH_IDLE";
DATA(0x0020eabc)
static char s_swIdle[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_IDLE";
DATA(0x0020eae4)
static char s_eIdle[] = "GRUNTZ_WINGZGRUNT_EAST_IDLE";
DATA(0x0020eb08)
static char s_wIdle[] = "GRUNTZ_WINGZGRUNT_WEST_IDLE";
DATA(0x0020eb2c)
static char s_neIdle[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_IDLE";
DATA(0x0020eb54)
static char s_nIdle[] = "GRUNTZ_WINGZGRUNT_NORTH_IDLE";
DATA(0x0020eb78)
static char s_nwIdle[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_IDLE";
DATA(0x0020eba0)
static char s_seWalk[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_WALK";
DATA(0x0020ebc8)
static char s_sWalk[] = "GRUNTZ_WINGZGRUNT_SOUTH_WALK";
DATA(0x0020ebec)
static char s_swWalk[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_WALK";
DATA(0x0020ec14)
static char s_eWalk[] = "GRUNTZ_WINGZGRUNT_EAST_WALK";
DATA(0x0020ec38)
static char s_wWalk[] = "GRUNTZ_WINGZGRUNT_WEST_WALK";
DATA(0x0020ec5c)
static char s_neWalk[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_WALK";
DATA(0x0020ec84)
static char s_nWalk[] = "GRUNTZ_WINGZGRUNT_NORTH_WALK";
DATA(0x0020eca8)
static char s_nwWalk[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_WALK";
DATA(0x0020ecd0)
static char s_wgItem[] = "GRUNTZ_WINGZGRUNT_ITEM";
DATA(0x0020ecec)
static char s_seItem[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_ITEM";
DATA(0x0020ed14)
static char s_sItem[] = "GRUNTZ_WINGZGRUNT_SOUTH_ITEM";
DATA(0x0020ed38)
static char s_swItem[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_ITEM";
DATA(0x0020ed60)
static char s_eItem[] = "GRUNTZ_WINGZGRUNT_EAST_ITEM";
DATA(0x0020ed84)
static char s_wItem[] = "GRUNTZ_WINGZGRUNT_WEST_ITEM";
DATA(0x0020eda8)
static char s_neItem[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_ITEM";
DATA(0x0020edd0)
static char s_nItem[] = "GRUNTZ_WINGZGRUNT_NORTH_ITEM";
DATA(0x0020edf4)
static char s_nwItem[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_ITEM";
DATA(0x0020ee1c)
static char s_gruntzDeathzUnfreeze[] = "GRUNTZ_DEATHZ_UNFREEZE";
DATA(0x0020ee38)
static char s_freezeDelay[] = "FreezeDelay";
DATA(0x0020ee48)
static char s_gruntzDeathzSparkle[] = "GRUNTZ_DEATHZ_SPARKLE";
DATA(0x0020ee64)
static char s_movingDeathTime[] = "MovingDeathTime";

// @early-stop
RVA(0x00067850, 0x214)
i32 CGrunt::RunEntranceMove() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, static_cast<u32>(g_engineFrameDelta))
    if (!((cur->m_finished != false && cur->m_frameTicksLeft == 0)
          || m_entrancePickup == PICKUP_NONE)) {
        return 0;
    }

    RestorePreviousAppearance();

    if (m_arrived != false) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    PickupType mode = m_entrancePickup;
    if (mode == PICKUP_INVALID) {
        return 0;
    }
    if (mode >= PICKUP_POWERUPZ_FIRST) {
        return LoadTypeTableClearMove(mode);
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        m_brickPickupType = mode;
        m_entrancePickup = PICKUP_INVALID;
        return 1;
    }
    if (mode < PICKUP_BRICKZ_FIRST) {
        if (mode >= PICKUP_TOYZ_FIRST) {
            return LoadVehicleGruntSprites(mode);
        }
        return LoadTypeTableClearMove(mode);
    }
    return 0;
}

RVA(0x00067b00, 0x92)
i32 CGrunt::GruntInRadius(i32 playerIndex, i32 unitIndex) {
    CGrunt* other = m_triggerMgr->UnitAt(playerIndex, unitIndex);
    if (other != NULL && other->m_entranceCommitted != false && other->m_gruntKind != GRUNT_GHOST) {
        i32 ox = other->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 oy = other->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 tx = m_defenderPx.m_x >> TILE_SHIFT_PX;
        i32 ty = m_defenderPx.m_y >> TILE_SHIFT_PX;
        i32 dx = ox - tx;
        i32 dy = oy - ty;
        i32 sum = m_defenderRadius + m_reachRect.right;
        i32 dist2 = abs(SquaredDistance(dy, dx));
        return dist2 < SQR(sum) ? 1 : 0;
    }
    return 0;
}

// @early-stop
RVA(0x00067bd0, 0x2ef)
i32 CGrunt::BuildEntranceAnimation(GruntEntranceMode mode) {
    SET_ANIMATION_ACT("K");

    m_entranceArmed = true;
    m_entranceCommitted = false;
    m_entranceActive = true;
    CWwdSpriteObject* h = m_object;
    SET_SORT_KEY_IF_CHANGED(h, SORTKEY_ACTOR)

    ClearAllSprites();

    CString key;

    CAniElement* found;

    if (mode == GRUNT_ENTRANCE_WORMHOLE) {
        i32 onScreen = 0;
        {
            i32 y = m_object->m_screenY;
            i32 x = m_object->m_screenX;
            if (::PtInRect(&g_gameReg->m_viewBounds, x, y)) {
                onScreen = 1;
            } else {

                CGrunt* focus;
                CTriggerMgr* tm = g_gameReg->m_triggerMgr;
                if (tm->m_recList.GetCount() != 1) {
                    focus = NULL;
                } else {
                    Coord* rec = tm->HeadRec();
                    focus = tm->UnitAt(rec->m_x, rec->m_y);
                }
                if (this == focus && m_playerIndex == g_curPlayer) {
                    onScreen = 1;
                }
            }
        }

        i32 r = rand() % 0x1e1;
        if (r > 0x140) {
            found = MapFind<CAniElement>(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_gruntzEntrancezOne
            );
            if (onScreen) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x37a, -1, 0, -1, -1);
            }
            key = "GRUNTZ_ENTRANCEZ";
        } else if (r > 0xa0) {
            found = MapFind<CAniElement>(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_gruntzEntrancezTwo
            );
            if (onScreen) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x37b, -1, 0, -1, -1);
            }
            key = "GRUNTZ_ENTRANCEZ";
        } else {
            found = MapFind<CAniElement>(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_gruntzEntrancezThree
            );
            if (onScreen) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x37c, -1, 0, -1, -1);
            }
            key = "GRUNTZ_ENTRANCEZ";
        }
    } else if (mode == GRUNT_ENTRANCE_DROP) {
        found = MapFind<CAniElement>(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_gruntzEntrancezDrop
        );
        key = s_gruntzEntrancezDrop;
    } else {
        found = MapFind<CAniElement>(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_gruntzEntrancezRessurect
        );
        key = "GRUNTZ_DEATHZ_MELT";
    }

    if (!found) {
        ResetEntranceAnimation(1, 0, 0);
    } else {
        SwitchAnimation(found);
        APPLY_CURRENT_ANIMATION_FRAME_SPRITE(key, desc, elem)
    }
    return 0;
}

inline void CGrunt::ResolveEntranceOccupant() {
    CGruntzMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 ty = m_object->m_screenY >> TILE_SHIFT_PX;
    i32 flags = grid->CellFlagsAt(tx, ty);
    if (flags & BRICKZ_CELL_OCCUPIED) {
        i32 owner = grid->OccupantAt(static_cast<u32>(tx), static_cast<u32>(ty));
        i32 playerIndex = (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
        i32 unitIndex = owner & GRUNT_IDENTITY_COMPONENT_MASK;
        if (m_playerIndex != playerIndex || m_unitIndex != unitIndex) {
            m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_SQUASH, m_playerIndex);
        }
    }
}

#define COMPLETE_ENTRANCE_COMMIT()                                                                 \
    do {                                                                                           \
        m_entranceCommitted = true;                                                                \
        i32 sortKey = m_object->m_screenY + 0x186a0;                                               \
        SET_SORT_KEY_IF_CHANGED(m_object, sortKey)                                                 \
        CAniElement* found = NULL;                                                                 \
        CAniElement* cached = m_wwdObject->m_animationCursor.m_animation;                          \
        MapLookup(                                                                                 \
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,                                 \
            s_gruntzEntrancezDrop,                                                                 \
            found                                                                                  \
        );                                                                                         \
        if (found == cached) {                                                                     \
            if (m_playerIndex == g_curPlayer) {                                                    \
                g_gameReg->m_voiceManager->PlayVoice(this, 0x33f, -1, 0, -1, -1);                  \
                m_triggerMgr->ResetCell(m_playerIndex, m_unitIndex, 0, 0);                         \
            }                                                                                      \
            m_entranceDropActive = true;                                                           \
            m_entranceTiming.m_intervalLo = g_buteMgr.GetDword("Grunt", "EntranceSafeTime", 5000); \
            m_entranceTiming.m_intervalHi = 0;                                                     \
            m_entranceTiming.m_startLo = g_frameTime;                                              \
            m_entranceTiming.m_startHi = 0;                                                        \
            m_flashTiming.m_intervalLo = 0;                                                        \
            m_flashTiming.m_intervalHi = 0;                                                        \
        } else if (m_triggerMgr->RecordListHas(m_playerIndex, m_unitIndex)) {                      \
            CommitArrival();                                                                       \
        }                                                                                          \
        m_entranceActive = false;                                                                  \
        ReadConfigFromButeMgr();                                                                   \
        LoadCellAnimNames(0, 0);                                                                   \
        LoadAnimNameTable(0, 0);                                                                   \
    } while (0)

RVA(0x00067f80, 0x313)
i32 CGrunt::LoadEntranceConfig() {
    if (m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta)) == 1) {
        ResolveEntranceOccupant();
        CWwdSpriteObject* h = m_object;
        i32 oldX = m_lastTilePx.m_x;
        m_entranceArmed = false;
        i32 newPxX = h->m_screenX;
        i32 newPxY = h->m_screenY;
        i32 oldTileX = oldX >> TILE_SHIFT_PX;
        i32 oldTileY = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 newTileX = newPxX >> TILE_SHIFT_PX;
        i32 newTileY = newPxY >> TILE_SHIFT_PX;

        if (oldX != -1 && m_lastTilePx.m_y != -1) {
            CGruntzMapMgr* og = g_gameReg->m_tileGrid;
            og->ReleaseCellOccupancy(oldTileX, oldTileY);
        }
        {
            CGruntzMapMgr* ng = g_gameReg->m_tileGrid;
            ng->AcquireCellOccupancy(newTileX, newTileY, m_playerIndex, m_unitIndex);
        }
        m_lastTilePx.m_x = newPxX;
        m_lastTilePx.m_y = newPxY;
        m_triggerMgr->WireTileSwitchLogic(this, newPxX, newPxY);

        COMPLETE_ENTRANCE_COMMIT();
    }

    CAniAdvanceCursor* cur = &m_wwdObject->m_animationCursor;
    if (cur->m_finished == false || cur->m_frameTicksLeft != 0) {
        return 0;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 0;
}

RVA(0x00068370, 0x14c)
i32 CGrunt::RearmEntranceDrop() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, static_cast<u32>(g_engineFrameDelta))
    if (cur->IsComplete()) {
        m_bombRunActive = false;
        SwitchAnimation(AT(m_poseItem, GRUNT_ITEM2));

        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)

        const char* name = EntranceCell()->ItemName().GetBuffer(0);
        SetImageFrameByName(name, frame);
    }

    if (m_bombRunActive == false) {
        i32 playerIndex;
        i32 unitIndex;
        m_entranceCommitted = false;
        if (m_triggerMgr
                ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &playerIndex, &unitIndex, 0)
            != NULL) {
            m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_EXPLODE, -1);
            m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
        } else {
            m_entranceCommitted = true;
        }
    }
    return 0;
}

RVA(0x00068520, 0x2a2)
i32 CGrunt::StartBombGruntRun() {
    FinishActiveAction();
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_powerupSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_selectedSprite)
    m_gruntKind = GRUNT_NORMAL;
    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    BeginGruntEntranceAndReleaseCell(this);
    SnapToLastTile(1);
    SetEntrancePos(1, 1);
    if (LoadGruntTypeTable(PICKUP_BOMB, 1, 0, 1) == 0) {
        CWwdSpriteObject* h = m_object;
        m_triggerMgr->LoadExplosionSprites(h->m_screenX, h->m_screenY, -1, 0);
        return 0;
    }
    i32 dx = rand() % 3 - 1;
    i32 dy = rand() % 3 - 1;
    if (dx == 0 && dy == 0) {
        dx = 1;
    }
    {
        CWwdSpriteObject* h = m_object;
        dx += h->m_screenX >> TILE_SHIFT_PX;
        dy += h->m_screenY >> TILE_SHIFT_PX;
    }
    FaceTowardTile(dx, dy);
    m_moveTile.m_x = dx;
    m_moveTile.m_y = dy;
    SET_ANIMATION_ACT("M");
    m_timePerTile = static_cast<i32>(g_buteMgr.GetDword("BOMBGRUNT", "RunningTimePerTile", 0x64));
    m_bombRunActive = true;
    PLAY_GRUNT_CUE_IN_VIEW(8);
    SwitchAnimation(AT(m_poseItem, GRUNT_ITEM1));
    char* cn = EntranceCell()->ItemName().GetBuffer(0);
    SetImageSetByName(cn);
    return 0;
}

// @early-stop
RVA(0x00068880, 0x67c)
i32 CGrunt::LoadWingzGruntSprites(b32 enable) {
    if (enable != false) {
        m_wingzEnabled = true;
        m_wingzTiming.Start(
            static_cast<i32>((static_cast<double>(m_wingzTime) * g_wingzScale - g_wingzBias))
        );
        CreateWingzTimeSprite();

        m_cells[0].IdleName() = s_nwItem;
        m_cells[1].IdleName() = s_nItem;
        m_cells[2].IdleName() = s_neItem;
        m_cells[3].IdleName() = s_wItem;
        m_cells[4].IdleName() = s_nItem;
        m_cells[5].IdleName() = s_eItem;
        m_cells[6].IdleName() = s_swItem;
        m_cells[7].IdleName() = s_sItem;
        m_cells[8].IdleName() = s_seItem;
        m_cells[0].WalkName() = s_nwItem;
        m_cells[1].WalkName() = s_nItem;
        m_cells[2].WalkName() = s_neItem;
        m_cells[3].WalkName() = s_wItem;
        m_cells[4].WalkName() = s_nItem;
        m_cells[5].WalkName() = s_eItem;
        m_cells[6].WalkName() = s_swItem;
        m_cells[7].WalkName() = s_sItem;
        m_cells[8].WalkName() = s_seItem;

        m_poseWalk =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgItem);
        CAniElement* pose =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgItem);
        AT(m_poseIdle, GRUNT_IDLE3) = NULL;
        AT(m_poseIdle, GRUNT_IDLE1) = pose;
        AT(m_poseIdle, GRUNT_IDLE2) = pose;
        AT(m_poseIdle, GRUNT_IDLE4) = NULL;
        AT(m_poseIdle, GRUNT_IDLE5) = NULL;

        PLAY_GRUNT_CUE_IN_VIEW(8);
    } else {
        m_wingzEnabled = false;
        m_wingzTiming.m_intervalLo = 0;
        m_wingzTiming.m_intervalHi = 0;
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)

        m_cells[0].WalkName() = s_nwWalk;
        m_cells[1].WalkName() = s_nWalk;
        m_cells[2].WalkName() = s_neWalk;
        m_cells[3].WalkName() = s_wWalk;
        m_cells[4].WalkName() = s_nWalk;
        m_cells[5].WalkName() = s_eWalk;
        m_cells[6].WalkName() = s_swWalk;
        m_cells[7].WalkName() = s_sWalk;
        m_cells[8].WalkName() = s_seWalk;
        m_cells[0].IdleName() = s_nwIdle;
        m_cells[1].IdleName() = s_nIdle;
        m_cells[2].IdleName() = s_neIdle;
        m_cells[3].IdleName() = s_wIdle;
        m_cells[4].IdleName() = s_nIdle;
        m_cells[5].IdleName() = s_eIdle;
        m_cells[6].IdleName() = s_swIdle;
        m_cells[7].IdleName() = s_sIdle;
        m_cells[8].IdleName() = s_seIdle;

        m_poseWalk =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgWalk);
        AT(m_poseIdle, GRUNT_IDLE1) =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgIdle1);
        AT(m_poseIdle, GRUNT_IDLE2) =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgIdle2);
        AT(m_poseIdle, GRUNT_IDLE3) =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgIdle3);
        AT(m_poseIdle, GRUNT_IDLE4) =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgIdle4);
        AT(m_poseIdle, GRUNT_IDLE5) =
            MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_wgIdle5);
    }

    if (IsAnimationAct("D")) {
        SwitchAnimation(m_poseWalk);
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
        char* buf = EntranceCell()->WalkName().GetBuffer(0);
        SetImageFrameByName(buf, frame);
        return 1;
    }

    if (IsAnimationAct("A")) {
        SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE1));
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
        char* buf = EntranceCell()->IdleName().GetBuffer(0);
        SetImageFrameByName(buf, frame);
    }
    return 1;
}

RVA(0x000690a0, 0x1c5)
i32 CGrunt::UpdateEntranceAnim() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(anim, static_cast<u32>(g_engineFrameDelta))
    if (!anim->IsComplete()) {
        return 0;
    }

    if (m_entranceStamped == false) {
        SwitchAnimation(AT(m_poseToy, GRUNT_TOY_BREAK));

        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)

        char* buf = (&m_frameSetName)->GetBuffer(0);
        SetImageFrameByName(buf, frame);

        m_entranceStamped = true;
        i32 v = m_moveVariant;
        if (v != 0) {
            LoadGruntAbilityTuning(v);
        } else {
            LoadGruntAbilityTuning(m_moveKind);
        }
        return 0;
    }

    if (m_arrived != false) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }

    SET_ANIMATION_ACT("A");
    LoadGruntTypeTable(m_toolId, 1, 0, 0);
    m_entranceActive = false;

    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 flags = board->CellFlagsAt(tx, ty);

    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        return 0;
    }

    CWwdSpriteObject* h = m_object;
    i32 z = h->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(h, z)
    return 0;
}

RVA(0x000692f0, 0x850)
i32 CGrunt::StepArrivalCommit() {
    if (m_entranceCommitted == false) {
        return 0;
    }

    bool eq;

    eq = IsNotAnimationAct("A");
    if (!eq) {
        goto finalize;
    }
    eq = IsNotAnimationAct("D");
    if (!eq) {
        goto finalize;
    }
    eq = IsAnimationAct("I");
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
        }
        ClearMoveTileFx(this);
        if (m_entranceReason != PICKUP_BOMB) {
            goto finalize;
        }
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
        return 0;
    }
    if (GRUNT_IS_USING_TOY(eq)) {
        goto idleReseed;
    }
    if (SettleActiveKnockback()) {
        goto finalize;
    }
    if (APPLY_ACTIVE_ENTRANCE_PICKUP(eq)) {
        goto finalize;
    }

    if (SETTLE_ACTIVE_TUBE_MOVE(eq)) {
        goto finalize;
    }
    if (TERMINATE_ACTIVE_BOMB_RUN(eq)) {
        return 0;
    }
    goto finalize;

idleReseed:
    RestoreToolAfterToyUse(0);

finalize:
    ConsiderArrival(1);
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)
    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    BeginGruntEntranceAndReleaseCell(this);
    SET_ANIMATION_ACT("Q");
    {
        i32 z = m_object->m_screenY + 0x186a0;
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, z)
    }
    SwitchAnimationByName("GRUNTZ_DEATHZ_FREEZE", 0);
    {
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
        APPLY_LOOKUP_SPRITE_INLINE("GRUNTZ_DEATHZ_FREEZE", frame);
    }
    m_freezeUnfrozen = false;
    m_freezeDelayDone = true;
    return 0;
}

// @early-stop
RVA(0x00069d60, 0x1e1)
i32 CGrunt::LoadFreezeSpellAssets() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, static_cast<u32>(g_engineFrameDelta))
    if (cur->IsComplete()) {
        if (m_freezeUnfrozen != false) {
            m_entranceActive = false;
            ReadConfigFromButeMgr();
            LoadCellAnimNames(0, 0);
            LoadAnimNameTable(0, 0);
            ResetEntranceAnimation(1, 0, 0);
            if (g_gameReg->m_tileGrid->CellFlagsAt(
                    m_lastTilePx.m_x >> TILE_SHIFT_PX,
                    m_lastTilePx.m_y >> TILE_SHIFT_PX
                )
                & 0x80) {
                m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            }
            return 0;
        }
        SwitchAnimationByName(s_gruntzDeathzSparkle, 0);
        m_idleDelayTiming.Start(g_buteMgr.GetDword("Spellz", s_freezeDelay, 0x2710));
        m_freezeDelayDone = false;
    }
    if (m_freezeDelayDone == false) {
        if (m_idleDelayTiming.Expired()) {
            SwitchAnimationByName(s_gruntzDeathzUnfreeze, 0);
            PLAY_VOICE_IN_VIEW(0x35c);
            m_freezeUnfrozen = true;
            m_freezeDelayDone = true;
        }
    }
    return 0;
}

RVA(0x00069fd0, 0x69)
i32 CGrunt::FinishEntranceMove() {

    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, static_cast<u32>(g_engineFrameDelta))
    if (!cur->IsComplete()) {
        return 0;
    }
    UnregisterFromBoard(this, 0);
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return 0;
}

RVA(0x0006a060, 0x520)
i32 CGrunt::LoadGruntMovingDeathConfig() {
    m_moveSpeed = 16.0 / static_cast<double>(g_buteMgr.GetDword("Grunt", s_movingDeathTime, 0x3e8));

    CGruntzMgr* g = g_gameReg;
    CState* state = g->m_curState;
    CGruntzMapMgr* b = g->m_tileGrid;
    CWwdSpriteObject* h = m_object;
    i32 xbound = b->m_width;
    i32 tileY = h->m_screenY >> TILE_SHIFT_PX;
    i32 tileX = h->m_screenX >> TILE_SHIFT_PX;
    i32 tileId;
    if (static_cast<u32>(tileX) >= static_cast<u32>(xbound)
        || static_cast<u32>(tileY) >= static_cast<u32>(b->m_height)) {
        tileId = 0;
    } else {
        tileId = b->m_rowInts[tileY][tileX * 7 + 3];
    }

    LevelArea area = state->m_levelType;

    if (area < AREA_TILESET_B_FIRST) {
        switch (static_cast<MovingDeathTileSetAId>(tileId)) {
            case MOVING_DEATH_A_S_1:
            case MOVING_DEATH_A_S_2:
                MV_S;
                break;
            case MOVING_DEATH_A_SW_1:
            case MOVING_DEATH_A_SW_2:
            case MOVING_DEATH_A_SW_3:
                MV_SW;
                break;
            case MOVING_DEATH_A_W_1:
            case MOVING_DEATH_A_W_2:
                MV_W;
                break;
            case MOVING_DEATH_A_NW_1:
            case MOVING_DEATH_A_NW_2:
            case MOVING_DEATH_A_NW_3:
                MV_NW;
                break;
            case MOVING_DEATH_A_N_1:
            case MOVING_DEATH_A_N_2:
                MV_N;
                break;
            case MOVING_DEATH_A_NE_1:
            case MOVING_DEATH_A_NE_2:
            case MOVING_DEATH_A_NE_3:
                MV_NE;
                break;
            case MOVING_DEATH_A_E_1:
            case MOVING_DEATH_A_E_2:
                MV_E;
                break;
            case MOVING_DEATH_A_SE_1:
            case MOVING_DEATH_A_SE_2:
            case MOVING_DEATH_A_SE_3:
                MV_SE;
                break;
            default:
                return 0;
        }
    } else {
        switch (static_cast<MovingDeathTileSetBId>(tileId)) {
            case MOVING_DEATH_B_N_1:
            case MOVING_DEATH_B_N_2:
            case MOVING_DEATH_B_N_3:
            case MOVING_DEATH_B_N_4:
                MV_N;
                break;
            case MOVING_DEATH_B_NE_1:
            case MOVING_DEATH_B_NE_2:
            case MOVING_DEATH_B_NE_3:
            case MOVING_DEATH_B_NE_4:
            case MOVING_DEATH_B_NE_5:
                MV_NE;
                break;
            case MOVING_DEATH_B_E_1:
            case MOVING_DEATH_B_E_2:
            case MOVING_DEATH_B_E_3:
            case MOVING_DEATH_B_E_4:
                MV_E;
                break;
            case MOVING_DEATH_B_SE_1:
            case MOVING_DEATH_B_SE_2:
            case MOVING_DEATH_B_SE_3:
            case MOVING_DEATH_B_SE_4:
            case MOVING_DEATH_B_SE_5:
            case MOVING_DEATH_B_SE_6:
                MV_SE;
                break;
            case MOVING_DEATH_B_S_1:
            case MOVING_DEATH_B_S_2:
                MV_S;
                break;
            case MOVING_DEATH_B_SW_1:
            case MOVING_DEATH_B_SW_2:
            case MOVING_DEATH_B_SW_3:
            case MOVING_DEATH_B_SW_4:
            case MOVING_DEATH_B_SW_5:
            case MOVING_DEATH_B_SW_6:
                MV_SW;
                break;
            case MOVING_DEATH_B_W_1:
            case MOVING_DEATH_B_W_2:
            case MOVING_DEATH_B_W_3:
            case MOVING_DEATH_B_W_4:
                MV_W;
                break;
            case MOVING_DEATH_B_NW_1:
            case MOVING_DEATH_B_NW_2:
            case MOVING_DEATH_B_NW_3:
            case MOVING_DEATH_B_NW_4:
            case MOVING_DEATH_B_NW_5:
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

    SET_ANIMATION_ACT("S");
    return 1;
}

#define FINISH_ENTRANCE_DROP()                                                                     \
    do {                                                                                           \
        ResolveEntranceOccupant();                                                                 \
        m_entranceArmed = false;                                                                   \
        i32 newX = m_object->m_screenX;                                                            \
        i32 newY = m_object->m_screenY;                                                            \
        i32 oldTx = m_lastTilePx.m_x >> TILE_SHIFT_PX;                                             \
        i32 oldTy = m_lastTilePx.m_y >> TILE_SHIFT_PX;                                             \
        i32 newTx = newX >> TILE_SHIFT_PX;                                                         \
        i32 newTy = newY >> TILE_SHIFT_PX;                                                         \
        if (oldTx != -1 && oldTy != -1) {                                                          \
            CGruntzMapMgr* oldGrid = g_gameReg->m_tileGrid;                                        \
            oldGrid->ReleaseCellOccupancy(oldTx, oldTy);                                           \
        }                                                                                          \
        CGruntzMapMgr* newGrid = g_gameReg->m_tileGrid;                                            \
        newGrid->AcquireCellOccupancy(newTx, newTy, m_playerIndex, m_unitIndex);                   \
        m_lastTilePx.m_x = newX;                                                                   \
        m_lastTilePx.m_y = newY;                                                                   \
        m_triggerMgr->WireTileSwitchLogic(this, newX, newY);                                       \
        COMPLETE_ENTRANCE_COMMIT();                                                                \
    } while (0)

RVA(0x0006a6d0, 0x936)
i32 CGrunt::FinishActiveAction() {
    bool ne;
    ne = IsNotAnimationAct("A");
    if (!ne) {
        goto retZero;
    }
    ne = IsNotAnimationAct("D");
    if (!ne) {
        goto retZero;
    }
    bool eq;
    eq = IsAnimationAct("I");
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
        }
        ClearMoveTileFx(this);
        return 1;
    }
    if (GRUNT_IS_USING_TOY(eq)) {
        goto idleReseed;
    }
    if (SettleActiveKnockback()) {
        return 1;
    }
    if (APPLY_ACTIVE_ENTRANCE_PICKUP(eq)) {
        return 1;
    }

    if (SETTLE_ACTIVE_TUBE_MOVE(eq)) {
        return 1;
    }

    eq = IsAnimationAct("K");
    if (!eq || m_entranceArmed == false) {
        goto retZero;
    }

    FINISH_ENTRANCE_DROP();
    return 1;

idleReseed:
    RestoreToolAfterToyUse(1);
    return 1;

retZero:
    return 0;
}

#undef FINISH_ENTRANCE_DROP
#undef COMPLETE_ENTRANCE_COMMIT

RVA(0x0006b260, 0x5)
i32 CGrunt::StepAttackAction() {

    return StepAttackFire();
}

RVA(0x0006b270, 0x1b)
CObject* CAniElement::AtChecked(i32 i) const {
    return GetAt(i);
}

RVA(0x0006b2a0, 0x23)
CAniElement* AnimationRegistry::FindAnimation(const char* key) {
    CAniElement* animation = MapFind<CAniElement>(m_animations, key);
    return animation;
}

RVA(0x0006b2e0, 0x39)
void CWapX::ApplyAnimation(CAniElement* animation, i32 advanceImmediately) {
    m_value = m_wwdObject->m_animationCursor.m_animation;
    CAniAdvanceCursor* anim = &m_wwdObject->m_animationCursor;
    anim->SetAnimation(animation);
    if (advanceImmediately != 0) {
        anim->Advance(static_cast<i32>(g_engineFrameDelta));
    }
}

RVA(0x0006b330, 0x2a)
i32 CGameLevel::PointInBounds(const LevelCoordRect* r, i32 x, i32 y) {
    return ::PtInRect(r, x, y);
}

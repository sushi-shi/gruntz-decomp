#include <rva.h>

#include <Gruntz/GruntEntranceMove.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirectionOffset.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/MapCellFlags.h>
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
static char s_GRUNTZ_ENTRANCEZ_RESSURECT[] = "GRUNTZ_ENTRANCEZ_RESSURECT";
DATA(0x0020e944)
static char s_GRUNTZ_ENTRANCEZ_DROP[] = "GRUNTZ_ENTRANCEZ_DROP";
DATA(0x0020e960)
static char s_GRUNTZ_ENTRANCEZ_THREE[] = "GRUNTZ_ENTRANCEZ_THREE";
DATA(0x0020e97c)
static char s_GRUNTZ_ENTRANCEZ_TWO[] = "GRUNTZ_ENTRANCEZ_TWO";
DATA(0x0020e9ac)
static char s_GRUNTZ_ENTRANCEZ_ONE[] = "GRUNTZ_ENTRANCEZ_ONE";
DATA(0x0020e9c8)
static char s_WG_IDLE5[] = "GRUNTZ_WINGZGRUNT_IDLE5";
DATA(0x0020e9e4)
static char s_WG_IDLE4[] = "GRUNTZ_WINGZGRUNT_IDLE4";
DATA(0x0020ea00)
static char s_WG_IDLE3[] = "GRUNTZ_WINGZGRUNT_IDLE3";
DATA(0x0020ea1c)
static char s_WG_IDLE2[] = "GRUNTZ_WINGZGRUNT_IDLE2";
DATA(0x0020ea38)
static char s_WG_IDLE1[] = "GRUNTZ_WINGZGRUNT_IDLE1";
DATA(0x0020ea54)
static char s_WG_WALK[] = "GRUNTZ_WINGZGRUNT_WALK";
DATA(0x0020ea70)
static char s_SE_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_IDLE";
DATA(0x0020ea98)
static char s_S_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTH_IDLE";
DATA(0x0020eabc)
static char s_SW_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_IDLE";
DATA(0x0020eae4)
static char s_E_IDLE[] = "GRUNTZ_WINGZGRUNT_EAST_IDLE";
DATA(0x0020eb08)
static char s_W_IDLE[] = "GRUNTZ_WINGZGRUNT_WEST_IDLE";
DATA(0x0020eb2c)
static char s_NE_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_IDLE";
DATA(0x0020eb54)
static char s_N_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTH_IDLE";
DATA(0x0020eb78)
static char s_NW_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_IDLE";
DATA(0x0020eba0)
static char s_SE_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_WALK";
DATA(0x0020ebc8)
static char s_S_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTH_WALK";
DATA(0x0020ebec)
static char s_SW_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_WALK";
DATA(0x0020ec14)
static char s_E_WALK[] = "GRUNTZ_WINGZGRUNT_EAST_WALK";
DATA(0x0020ec38)
static char s_W_WALK[] = "GRUNTZ_WINGZGRUNT_WEST_WALK";
DATA(0x0020ec5c)
static char s_NE_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_WALK";
DATA(0x0020ec84)
static char s_N_WALK[] = "GRUNTZ_WINGZGRUNT_NORTH_WALK";
DATA(0x0020eca8)
static char s_NW_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_WALK";
DATA(0x0020ecd0)
static char s_WG_ITEM[] = "GRUNTZ_WINGZGRUNT_ITEM";
DATA(0x0020ecec)
static char s_SE_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_ITEM";
DATA(0x0020ed14)
static char s_S_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTH_ITEM";
DATA(0x0020ed38)
static char s_SW_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_ITEM";
DATA(0x0020ed60)
static char s_E_ITEM[] = "GRUNTZ_WINGZGRUNT_EAST_ITEM";
DATA(0x0020ed84)
static char s_W_ITEM[] = "GRUNTZ_WINGZGRUNT_WEST_ITEM";
DATA(0x0020eda8)
static char s_NE_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_ITEM";
DATA(0x0020edd0)
static char s_N_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTH_ITEM";
DATA(0x0020edf4)
static char s_NW_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_ITEM";
DATA(0x0020ee1c)
static char s_GRUNTZ_DEATHZ_UNFREEZE[] = "GRUNTZ_DEATHZ_UNFREEZE";
DATA(0x0020ee38)
static char s_FreezeDelay[] = "FreezeDelay";
DATA(0x0020ee48)
static char s_GRUNTZ_DEATHZ_SPARKLE[] = "GRUNTZ_DEATHZ_SPARKLE";
DATA(0x0020ee64)
static char s_MovingDeathTime[] = "MovingDeathTime";

// @early-stop
RVA(0x00067850, 0x214)
i32 CGrunt::RunEntranceMove() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, static_cast<u32>(g_engineFrameDelta))
    if (!((cur->m_finished != false && cur->m_frameTicksLeft == 0)
          || m_entrancePickup == PICKUP_NONE)) {
        return 0;
    }

    m_entranceActive = false;
    CString* previousActName = g_typeColl.ScratchResolve(m_previousAnimationActId);
    ActNameConstructGrownSlots();
    const char* previousActNameText = *previousActName;
    bool previousActWasD;
    previousActWasD = (strcmp(previousActNameText, DATA_COMPGEN(0x0020cca4, "D")) == 0);
    if (previousActWasD) {
        if (m_poweredUp != false && m_neighborValid == false) {
            RESET_GRUNT_POWERED_STATE(this)
        }
        m_tileMoveCommitted = false;
        SET_ANIMATION_ACT("D");
        SwitchAnimation(m_poseWalk);
        char* nm = EntranceCell()->WalkName().GetBuffer(0);
        SetImageSetByName(nm);
    } else {
        ResetEntranceAnimation(1, 0, 0);
    }

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
        goto clearMove;
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        goto brick;
    }
    if (mode < PICKUP_BRICKZ_FIRST) {
        goto toyCheck;
    }
    return 0;

brick:
    m_brickPickupType = mode;
    m_entrancePickup = PICKUP_INVALID;
    return 1;

toyCheck:
    if (mode >= PICKUP_TOYZ_FIRST) {
        return LoadVehicleGruntSprites(mode);
    }
    goto clearMove;

clearMove:
    return LoadTypeTableClearMove(mode);
}

// @early-stop
RVA(0x00067b00, 0x92)
i32 CGrunt::GruntInRadius(i32 playerIndex, i32 unitIndex) {
    CGrunt* other = m_triggerMgr->m_units[playerIndex * TM_UNITS_PER_PLAYER + unitIndex];
    if (other != NULL && other->m_entranceCommitted != false && other->m_gruntKind != GRUNT_GHOST) {
        Coord otherTile = other->m_lastTilePx;
        ScreenTile(&otherTile);
        Coord targetTile = m_defenderPx;
        ScreenTile(&targetTile);
        i32 sum = m_defenderRadius + m_reachRect.right;
        i32 dist2 = abs(otherTile.DistSqr(targetTile));
        return dist2 < SQR(sum) ? 1 : 0;
    }
    return 0;
}

// @early-stop
static inline CAniElement* LookupAnimation(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* result = NULL;
    MapLookup(map, name, result);
    return result;
}

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
            Coord position = m_object->ScreenPos();
            if (::PtInRect(&g_gameReg->m_viewBounds, position.m_x, position.m_y)) {
                onScreen = 1;
            } else {

                CGrunt* focus;
                CTriggerMgr* tm = g_gameReg->m_triggerMgr;
                if (tm->m_recList.GetCount() != 1) {
                    focus = NULL;
                } else {
                    Coord rec = *tm->HeadRec();
                    focus = tm->m_units[rec.m_x * TM_UNITS_PER_PLAYER + rec.m_y];
                }
                if (this == focus && m_playerIndex == g_curPlayer) {
                    onScreen = 1;
                }
            }
        }

        i32 r = rand() % 0x1e1;
        if (r > 0x140) {
            found = LookupAnimation(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_GRUNTZ_ENTRANCEZ_ONE
            );
            if (onScreen) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x37a, -1, 0, -1, -1);
            }
            key = "GRUNTZ_ENTRANCEZ";
        } else if (r > 0xa0) {
            found = LookupAnimation(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_GRUNTZ_ENTRANCEZ_TWO
            );
            if (onScreen) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x37b, -1, 0, -1, -1);
            }
            key = "GRUNTZ_ENTRANCEZ";
        } else {
            found = LookupAnimation(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_GRUNTZ_ENTRANCEZ_THREE
            );
            if (onScreen) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x37c, -1, 0, -1, -1);
            }
            key = "GRUNTZ_ENTRANCEZ";
        }
    } else if (mode == GRUNT_ENTRANCE_DROP) {
        found = LookupAnimation(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_GRUNTZ_ENTRANCEZ_DROP
        );
        key = s_GRUNTZ_ENTRANCEZ_DROP;
    } else {
        found = LookupAnimation(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_GRUNTZ_ENTRANCEZ_RESSURECT
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

// @early-stop
RVA(0x00067f80, 0x313)
i32 CGrunt::LoadEntranceConfig() {
    if (m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta)) == 1) {
        CGruntzMgr* g = g_gameReg;
        CWwdSpriteObject* h = m_object;
        CMapMgr* grid = g->m_tileGrid;
        Coord tile;
        GetScreenTile(&tile);

        i32 flags = grid->CellFlagsAt(tile.m_x, tile.m_y);

        if (flags & BRICKZ_CELL_OCCUPIED) {
            i32 owner;
            if (static_cast<u32>(tile.m_x) >= static_cast<u32>(grid->m_width)
                || static_cast<u32>(tile.m_y) >= static_cast<u32>(grid->m_height)) {
                owner = -1;
            } else {
                owner = grid->m_rows[tile.m_y][tile.m_x].m_occupantId;
            }
            i32 occupantPlayerIndex =
                (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
            i32 occupantUnitIndex = owner & GRUNT_IDENTITY_COMPONENT_MASK;
            if (m_playerIndex != occupantPlayerIndex || m_unitIndex != occupantUnitIndex) {
                m_triggerMgr->StartUnitDeath(
                    occupantPlayerIndex,
                    occupantUnitIndex,
                    DEATH_SQUASH,
                    m_playerIndex
                );
            }
        }

        h = m_object;
        m_entranceArmed = false;
        Coord oldPixel = m_lastTilePx;
        Coord newPixel = h->ScreenPos();
        Coord oldTile = oldPixel;
        ScreenTile(&oldTile);
        Coord newTile = newPixel;
        ScreenTile(&newTile);

        if (oldPixel.m_x != -1 && oldPixel.m_y != -1) {
            CMapMgr* og = g_gameReg->m_tileGrid;

            BrickzCell* oc = &og->m_rows[oldTile.m_y][oldTile.m_x];
            oc->m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
            oc->m_occupantId = -1;
        }
        {
            CMapMgr* ng = static_cast<CMapMgr*>(g_gameReg->m_tileGrid);

            BrickzCell* nc = &ng->m_rows[newTile.m_y][newTile.m_x];
            nc->m_flags |= BRICKZ_CELL_OCCUPIED;
            nc->m_occupantId = (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;
        }
        m_lastTilePx = newPixel;
        m_triggerMgr->WireTileSwitchLogic(this, newPixel.m_x, newPixel.m_y);

        h = m_object;
        m_entranceCommitted = true;
        SET_SORT_KEY_IF_CHANGED(h, h->m_screenPosition.m_y + 0x186a0)

        CWwdSpriteObject* p = m_wwdObject;
        CAniElement* found = NULL;
        CAniElement* cached = p->m_animationCursor.m_animation;
        MapLookup(p->OwnerMgr()->m_animRegistry->m_animations, s_GRUNTZ_ENTRANCEZ_DROP, found);
        if (cached == found) {
            if (m_playerIndex == g_curPlayer) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x33f, -1, 0, -1, -1);
            }
            m_triggerMgr->ResetCell(m_playerIndex, m_unitIndex, 0, 0);
            m_entranceDropActive = true;
            m_entranceSafeTimeLo = g_buteMgr.GetDword("Grunt", "EntranceSafeTime", 5000);
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = g_frameTime;
            m_entranceClockHi = 0;
            m_flashWindowLo = 0;
            m_flashWindowHi = 0;
        } else {
            if (m_triggerMgr->RecordListHas(m_playerIndex, m_unitIndex)) {
                CommitArrival();
            }
        }
        m_entranceActive = false;
        ReadConfigFromButeMgr();
        LoadCellAnimNames(0, 0);
        LoadAnimNameTable(0, 0);
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
    if (IsAniCursorComplete(cur)) {
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
        if (m_triggerMgr->HitTestCell(
                m_object->m_screenPosition.m_x,
                m_object->m_screenPosition.m_y,
                &playerIndex,
                &unitIndex,
                0
            )
            != NULL) {
            m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_EXPLODE, -1);
            m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
        } else {
            m_entranceCommitted = true;
        }
    }
    return 0;
}

// @early-stop
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
    BEGIN_GRUNT_ENTRANCE_AND_RELEASE_CELL
    SnapToLastTile(1);
    SetEntrancePos(1, 1);
    if (LoadGruntTypeTable(PICKUP_BOMB, 1, 0, 1) == 0) {
        CWwdSpriteObject* h = m_object;
        m_triggerMgr->LoadExplosionSprites(h->m_screenPosition.m_x, h->m_screenPosition.m_y, -1, 0);
        return 0;
    }
    i32 moveX = rand() % 3 - 1;
    i32 moveY = rand() % 3 - 1;
    Coord move(moveX, moveY);
    if (move == Coord(0, 0)) {
        move.Set(1, 0);
    }
    {
        CWwdSpriteObject* h = m_object;
        Coord tile;
        GetScreenTile(&tile);
        move += tile;
    }
    FaceTowardTile(move);
    m_moveTile = move;
    SET_ANIMATION_ACT("M");
    m_timePerTile = static_cast<i32>(g_buteMgr.GetDword("BOMBGRUNT", "RunningTimePerTile", 0x64));
    m_bombRunActive = true;
    {
        CWwdSpriteObject* h = m_object;
        Coord position = h->ScreenPos();
        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(rect, position.m_x, position.m_y)) {
            g_gameReg->m_voiceManager->PlayGruntVoiceCue(this, 8, -1, -1, -1);
        }
    }
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
        m_wingzDurationLo =
            static_cast<i32>((static_cast<double>(m_wingzTime) * g_wingzScale - g_wingzBias));
        m_wingzDurationHi = 0;
        m_wingzClockLo = static_cast<i32>(g_frameTime);
        m_wingzClockHi = 0;
        CreateWingzTimeSprite();

        m_cells[0].IdleName() = s_NW_ITEM;
        m_cells[1].IdleName() = s_N_ITEM;
        m_cells[2].IdleName() = s_NE_ITEM;
        m_cells[3].IdleName() = s_W_ITEM;
        m_cells[4].IdleName() = s_N_ITEM;
        m_cells[5].IdleName() = s_E_ITEM;
        m_cells[6].IdleName() = s_SW_ITEM;
        m_cells[7].IdleName() = s_S_ITEM;
        m_cells[8].IdleName() = s_SE_ITEM;
        m_cells[0].WalkName() = s_NW_ITEM;
        m_cells[1].WalkName() = s_N_ITEM;
        m_cells[2].WalkName() = s_NE_ITEM;
        m_cells[3].WalkName() = s_W_ITEM;
        m_cells[4].WalkName() = s_N_ITEM;
        m_cells[5].WalkName() = s_E_ITEM;
        m_cells[6].WalkName() = s_SW_ITEM;
        m_cells[7].WalkName() = s_S_ITEM;
        m_cells[8].WalkName() = s_SE_ITEM;

        m_poseWalk =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_ITEM);
        CAniElement* pose =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_ITEM);
        AT(m_poseIdle, GRUNT_IDLE3) = NULL;
        AT(m_poseIdle, GRUNT_IDLE1) = pose;
        AT(m_poseIdle, GRUNT_IDLE2) = pose;
        AT(m_poseIdle, GRUNT_IDLE4) = NULL;
        AT(m_poseIdle, GRUNT_IDLE5) = NULL;

        CGruntzMgr* g = g_gameReg;
        Coord position = m_object->ScreenPos();
        CCueRect* r = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(r, position.m_x, position.m_y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 8, -1, -1, -1);
        }
    } else {
        m_wingzEnabled = false;
        m_wingzDurationLo = 0;
        m_wingzDurationHi = 0;
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)

        m_cells[0].WalkName() = s_NW_WALK;
        m_cells[1].WalkName() = s_N_WALK;
        m_cells[2].WalkName() = s_NE_WALK;
        m_cells[3].WalkName() = s_W_WALK;
        m_cells[4].WalkName() = s_N_WALK;
        m_cells[5].WalkName() = s_E_WALK;
        m_cells[6].WalkName() = s_SW_WALK;
        m_cells[7].WalkName() = s_S_WALK;
        m_cells[8].WalkName() = s_SE_WALK;
        m_cells[0].IdleName() = s_NW_IDLE;
        m_cells[1].IdleName() = s_N_IDLE;
        m_cells[2].IdleName() = s_NE_IDLE;
        m_cells[3].IdleName() = s_W_IDLE;
        m_cells[4].IdleName() = s_N_IDLE;
        m_cells[5].IdleName() = s_E_IDLE;
        m_cells[6].IdleName() = s_SW_IDLE;
        m_cells[7].IdleName() = s_S_IDLE;
        m_cells[8].IdleName() = s_SE_IDLE;

        m_poseWalk =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_WALK);
        AT(m_poseIdle, GRUNT_IDLE1) =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE1);
        AT(m_poseIdle, GRUNT_IDLE2) =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE2);
        AT(m_poseIdle, GRUNT_IDLE3) =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE3);
        AT(m_poseIdle, GRUNT_IDLE4) =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE4);
        AT(m_poseIdle, GRUNT_IDLE5) =
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_WG_IDLE5);
    }

    CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
    ActNameConstructGrownSlots();
    bool eqWalk = (strcmp(*rec, "D") == 0);
    if (eqWalk) {
        SwitchAnimation(m_poseWalk);
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
        char* buf = EntranceCell()->WalkName().GetBuffer(0);
        SetImageFrameByName(buf, frame);
        return 1;
    }

    CString* rec2 = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
    ActNameConstructGrownSlots();
    bool eqIdle = (strcmp(*rec2, "A") == 0);
    if (eqIdle) {
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
    if (!IsAniCursorComplete(anim)) {
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

    CGruntzMapMgr* board = g_gameReg->m_tileGrid;
    Coord tile = m_lastTilePx;
    ScreenTile(&tile);
    i32 flags = board->CellFlagsAt(tile.m_x, tile.m_y);

    if (flags & IDX(CELL_FLAG_ARROW)) {
        SetEntrancePos(1, 1);
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        return 0;
    }

    CWwdSpriteObject* h = m_object;
    i32 z = h->m_screenPosition.m_y + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(h, z)
    return 0;
}

// @early-stop
RVA(0x000692f0, 0x850)
i32 CGrunt::StepArrivalCommit() {
    if (m_entranceCommitted == false) {
        return 0;
    }

    bool eq;

    eq = ANIMATION_ACT_DIFFERS("A");
    if (!eq) {
        goto finalize;
    }
    eq = ANIMATION_ACT_DIFFERS("D");
    if (!eq) {
        goto finalize;
    }
    eq = ANIMATION_ACT_EQUALS("I");
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
        }
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
        if (m_entranceReason != PICKUP_BOMB) {
            goto finalize;
        }
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
        return 0;
    }
    if ((eq = ANIMATION_ACT_EQUALS("G")) || (eq = ANIMATION_ACT_EQUALS("L"))
        || (eq = ANIMATION_ACT_EQUALS("P"))) {
        goto idleReseed;
    }
    eq = ANIMATION_ACT_EQUALS("O");
    if (eq) {
        SnapToLastTile(1);
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        goto finalize;
    }
    eq = ANIMATION_ACT_EQUALS("J");
    if (eq) {

        m_entranceActive = false;
        eq = (strcmp(*g_typeColl.GetNameRecord(m_previousAnimationActId), "D") == 0);
        if (eq) {
            if (m_poweredUp != false && m_neighborValid == false) {
                RESET_GRUNT_POWERED_STATE(this)
            }
            m_tileMoveCommitted = false;
            SET_ANIMATION_ACT("D");
            SwitchAnimation(m_poseWalk);
            char* nm = EntranceCell()->WalkName().GetBuffer(0);
            APPLY_NAME_INLINE(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    eq = ANIMATION_ACT_EQUALS("N");
    if (eq) {
        Coord snapped = m_object->ScreenPos();
        SnapTileCenter(&snapped);
        i32 redo = 1;
        if (snapped != m_lastTilePx) {
            if (IsDropReady(1)) {
                m_coordToggle = (m_coordToggle == false);
                redo = 0;
            }
        }
        SnapToLastTile(1);
        if (redo) {
            SET_ANIMATION_ACT("D");
            SetupTubeAnim(m_coordToggle);
        }
        goto finalize;
    }
    {
        const char* prev = *g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        eq = (strcmp(prev, "M") == 0);
        if (eq) {
            m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
            return 0;
        }
        goto finalize;
    }

idleReseed:
    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 0);
    {
        i32 z = m_object->m_screenPosition.m_y + 0x186a0;
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, z)
    }
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    m_toyTime = 0;
    StopVehicleLoopSound();
    goto finalize;

modeDispatch: {
    PickupType mode = m_entrancePickup;
    if (mode >= PICKUP_POWERUPZ_FIRST) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_entrancePickup = PICKUP_INVALID;
        m_helpCueId = 0;
        goto finalize;
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        m_brickPickupType = mode;
        m_entrancePickup = PICKUP_INVALID;
        goto finalize;
    }
    if (mode >= PICKUP_TOYZ_FIRST) {
        LoadVehicleGruntSprites(mode);
        goto finalize;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_entrancePickup = PICKUP_INVALID;
    goto finalize;
}

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
    BEGIN_GRUNT_ENTRANCE_AND_RELEASE_CELL
    SET_ANIMATION_ACT("Q");
    {
        i32 z = m_object->m_screenPosition.m_y + 0x186a0;
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
    if (IsAniCursorComplete(cur)) {
        if (m_freezeUnfrozen != false) {
            m_entranceActive = false;
            ReadConfigFromButeMgr();
            LoadCellAnimNames(0, 0);
            LoadAnimNameTable(0, 0);
            ResetEntranceAnimation(1, 0, 0);
            Coord tile = m_lastTilePx;
            ScreenTile(&tile);
            if (g_gameReg->m_tileGrid->CellFlagsAt(tile.m_x, tile.m_y) & IDX(CELL_FLAG_ARROW)) {
                m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            }
            return 0;
        }
        SwitchAnimationByName(s_GRUNTZ_DEATHZ_SPARKLE, 0);
        m_idleDelay = g_buteMgr.GetDword("Spellz", s_FreezeDelay, 0x2710);
        m_idleAnchor = g_frameTime;
        m_freezeDelayDone = false;
    }
    if (m_freezeDelayDone == false) {
        if (static_cast<i64>(g_frameTime) - m_idleAnchor >= m_idleDelay) {
            SwitchAnimationByName(s_GRUNTZ_DEATHZ_UNFREEZE, 0);
            CWwdSpriteObject* h = m_object;
            Coord position = h->ScreenPos();
            const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
            if (::PtInRect(rect, position.m_x, position.m_y)) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x35c, -1, 0, -1, -1);
            }
            m_freezeUnfrozen = true;
            m_freezeDelayDone = true;
        }
    }
    return 0;
}

RVA(0x00069fd0, 0x69)
i32 CGrunt::FinishEntranceMove() {

    ADVANCE_CURRENT_ANIMATION_CURSOR(cur, static_cast<u32>(g_engineFrameDelta))
    if (!IsAniCursorComplete(cur)) {
        return 0;
    }
    if (m_cellRemovalNotified == false) {

        m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
    }
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return 0;
}

RVA(0x0006a060, 0x520)
i32 CGrunt::LoadGruntMovingDeathConfig() {
    m_moveSpeed = 16.0 / static_cast<double>(g_buteMgr.GetDword("Grunt", s_MovingDeathTime, 0x3e8));

    CGruntzMgr* g = g_gameReg;
    CState* state = g->m_curState;
    CGruntzMapMgr* b = g->m_tileGrid;
    CWwdSpriteObject* h = m_object;
    i32 xbound = b->m_width;
    Coord tile;
    GetScreenTile(&tile);
    i32 tileId;
    if (static_cast<u32>(tile.m_x) >= static_cast<u32>(xbound)
        || static_cast<u32>(tile.m_y) >= static_cast<u32>(b->m_height)) {
        tileId = 0;
    } else {
        tileId = b->m_rows[tile.m_y][tile.m_x].m_tileId;
    }

    LevelArea area = state->m_levelType;

#define MV_DIR(direction)                                                                          \
    m_entranceCell = g_gruntDir##direction;                                                        \
    m_lastTilePx += GruntDirectionPixelOffset(m_entranceCell) / 2

    if (area < AREA_TILESET_B_FIRST) {
        switch (static_cast<MovingDeathTileSetAId>(tileId)) {
            case MOVING_DEATH_A_S_1:
            case MOVING_DEATH_A_S_2:
                MV_DIR(South);
                break;
            case MOVING_DEATH_A_SW_1:
            case MOVING_DEATH_A_SW_2:
            case MOVING_DEATH_A_SW_3:
                MV_DIR(SouthWest);
                break;
            case MOVING_DEATH_A_W_1:
            case MOVING_DEATH_A_W_2:
                MV_DIR(West);
                break;
            case MOVING_DEATH_A_NW_1:
            case MOVING_DEATH_A_NW_2:
            case MOVING_DEATH_A_NW_3:
                MV_DIR(NorthWest);
                break;
            case MOVING_DEATH_A_N_1:
            case MOVING_DEATH_A_N_2:
                MV_DIR(North);
                break;
            case MOVING_DEATH_A_NE_1:
            case MOVING_DEATH_A_NE_2:
            case MOVING_DEATH_A_NE_3:
                MV_DIR(NorthEast);
                break;
            case MOVING_DEATH_A_E_1:
            case MOVING_DEATH_A_E_2:
                MV_DIR(East);
                break;
            case MOVING_DEATH_A_SE_1:
            case MOVING_DEATH_A_SE_2:
            case MOVING_DEATH_A_SE_3:
                MV_DIR(SouthEast);
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
                MV_DIR(North);
                break;
            case MOVING_DEATH_B_NE_1:
            case MOVING_DEATH_B_NE_2:
            case MOVING_DEATH_B_NE_3:
            case MOVING_DEATH_B_NE_4:
            case MOVING_DEATH_B_NE_5:
                MV_DIR(NorthEast);
                break;
            case MOVING_DEATH_B_E_1:
            case MOVING_DEATH_B_E_2:
            case MOVING_DEATH_B_E_3:
            case MOVING_DEATH_B_E_4:
                MV_DIR(East);
                break;
            case MOVING_DEATH_B_SE_1:
            case MOVING_DEATH_B_SE_2:
            case MOVING_DEATH_B_SE_3:
            case MOVING_DEATH_B_SE_4:
            case MOVING_DEATH_B_SE_5:
            case MOVING_DEATH_B_SE_6:
                MV_DIR(SouthEast);
                break;
            case MOVING_DEATH_B_S_1:
            case MOVING_DEATH_B_S_2:
                MV_DIR(South);
                break;
            case MOVING_DEATH_B_SW_1:
            case MOVING_DEATH_B_SW_2:
            case MOVING_DEATH_B_SW_3:
            case MOVING_DEATH_B_SW_4:
            case MOVING_DEATH_B_SW_5:
            case MOVING_DEATH_B_SW_6:
                MV_DIR(SouthWest);
                break;
            case MOVING_DEATH_B_W_1:
            case MOVING_DEATH_B_W_2:
            case MOVING_DEATH_B_W_3:
            case MOVING_DEATH_B_W_4:
                MV_DIR(West);
                break;
            case MOVING_DEATH_B_NW_1:
            case MOVING_DEATH_B_NW_2:
            case MOVING_DEATH_B_NW_3:
            case MOVING_DEATH_B_NW_4:
            case MOVING_DEATH_B_NW_5:
                MV_DIR(NorthWest);
                break;
            default:
                return 0;
        }
    }
#undef MV_DIR

    SET_ANIMATION_ACT("S");
    return 1;
}

// @early-stop
RVA(0x0006a6d0, 0x936)
i32 CGrunt::FinishActiveAction() {
    bool ne;
    ne = ANIMATION_ACT_DIFFERS("A");
    if (!ne) {
        goto retZero;
    }
    ne = ANIMATION_ACT_DIFFERS("D");
    if (!ne) {
        goto retZero;
    }
    bool eq;
    eq = ANIMATION_ACT_EQUALS("I");
    if (eq) {
        if (m_entranceReason == PICKUP_WAND) {
            g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
        }
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
        return 1;
    }
    if ((eq = ANIMATION_ACT_EQUALS("G")) || (eq = ANIMATION_ACT_EQUALS("L"))
        || (eq = ANIMATION_ACT_EQUALS("P"))) {
        goto idleReseed;
    }
    eq = ANIMATION_ACT_EQUALS("O");
    if (eq) {
        SnapToLastTile(1);
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        return 1;
    }
    eq = ANIMATION_ACT_EQUALS("J");
    if (eq) {
        m_entranceActive = false;
        eq = (strcmp(*g_typeColl.GetNameRecord(m_previousAnimationActId), "D") == 0);
        if (eq) {
            if (m_poweredUp != false && m_neighborValid == false) {
                RESET_GRUNT_POWERED_STATE(this)
            }
            m_tileMoveCommitted = false;
            SET_ANIMATION_ACT("D");
            SwitchAnimation(m_poseWalk);

            char* nm = EntranceCell()->WalkName().GetBuffer(0);
            SetImageSetByName(nm);
        } else {
            ResetEntranceAnimation(1, 0, 0);
        }
        goto modeDispatch;
    }

    eq = ANIMATION_ACT_EQUALS("N");
    if (eq) {
        Coord pixel = m_object->ScreenPos();
        SnapTileCenter(&pixel);
        i32 redo = 1;
        if (pixel != m_lastTilePx && IsDropReady(1)) {
            m_coordToggle = (m_coordToggle == false);
            redo = 0;
        }
        SnapToLastTile(1);
        if (redo) {
            SET_ANIMATION_ACT("D");
            SetupTubeAnim(m_coordToggle);
        }
        return 1;
    }

    eq = ANIMATION_ACT_EQUALS("K");
    if (!eq || m_entranceArmed == false) {
        goto retZero;
    }

    {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        Coord newPixel = m_object->ScreenPos();
        Coord newTile = newPixel;
        ScreenTile(&newTile);
        i32 flags = grid->CellFlagsAt(newTile.m_x, newTile.m_y);

        if (flags & BRICKZ_CELL_OCCUPIED) {
            i32 owner;
            if (static_cast<u32>(newTile.m_x) >= static_cast<u32>(grid->m_width)
                || static_cast<u32>(newTile.m_y) >= static_cast<u32>(grid->m_height)) {
                owner = -1;
            } else {
                owner = grid->m_rows[newTile.m_y][newTile.m_x].m_occupantId;
            }
            i32 playerIndex =
                (owner >> GRUNT_IDENTITY_PLAYER_SHIFT) & GRUNT_IDENTITY_COMPONENT_MASK;
            i32 unitIndex = owner & GRUNT_IDENTITY_COMPONENT_MASK;
            if (m_playerIndex != playerIndex || m_unitIndex != unitIndex) {
                m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_SQUASH, m_playerIndex);
            }
        }

        m_entranceArmed = false;
        Coord oldTile = m_lastTilePx;
        ScreenTile(&oldTile);
        if (oldTile.m_x != -1 && oldTile.m_y != -1) {
            CMapMgr* oldGrid = g_gameReg->m_tileGrid;
            BrickzCell* oc = &oldGrid->m_rows[oldTile.m_y][oldTile.m_x];
            oc->m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
            oc->m_occupantId = -1;
        }
        CMapMgr* newGrid = g_gameReg->m_tileGrid;
        BrickzCell* nc = &newGrid->m_rows[newTile.m_y][newTile.m_x];
        nc->m_flags |= BRICKZ_CELL_OCCUPIED;
        nc->m_occupantId = (m_playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | m_unitIndex;
        m_lastTilePx = newPixel;
        m_triggerMgr->WireTileSwitchLogic(this, newPixel.m_x, newPixel.m_y);

        m_entranceCommitted = true;
        i32 sortKey = m_object->m_screenPosition.m_y + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(m_object, sortKey)

        CAniElement* found = NULL;
        CAniElement* cached = m_wwdObject->m_animationCursor.m_animation;
        MapLookup(
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
            s_GRUNTZ_ENTRANCEZ_DROP,
            found
        );
        if (found == cached) {
            if (m_playerIndex == g_curPlayer) {
                g_gameReg->m_voiceManager->PlayVoice(this, 0x33f, -1, 0, -1, -1);
                m_triggerMgr->ResetCell(m_playerIndex, m_unitIndex, 0, 0);
            }
            m_entranceDropActive = true;
            m_entranceSafeTimeLo = g_buteMgr.GetDword("Grunt", "EntranceSafeTime", 5000);
            m_entranceSafeTimeHi = 0;
            m_entranceClockLo = g_frameTime;
            m_entranceClockHi = 0;
            m_flashWindowLo = 0;
            m_flashWindowHi = 0;
        } else if (m_triggerMgr->RecordListHas(m_playerIndex, m_unitIndex)) {
            CommitArrival();
        }

        m_entranceActive = false;
        ReadConfigFromButeMgr();
        LoadCellAnimNames(0, 0);
        LoadAnimNameTable(0, 0);
        return 1;
    }

idleReseed:
    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        i32 sortKey = m_object->m_screenPosition.m_y + 0x186a0;
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, sortKey)
    }
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    m_toyTime = 0;
    StopVehicleLoopSound();
    return 1;

modeDispatch: {
    PickupType mode = m_entrancePickup;
    if (mode >= PICKUP_POWERUPZ_FIRST) {
        LoadGruntTypeTable(mode, 1, 0, 1);
        m_entrancePickup = PICKUP_INVALID;
        m_helpCueId = 0;
        return 1;
    }
    if (mode >= PICKUP_BRICKZ_FIRST) {
        m_brickPickupType = mode;
        m_entrancePickup = PICKUP_INVALID;
        return 1;
    }
    if (mode >= PICKUP_TOYZ_FIRST) {
        LoadVehicleGruntSprites(mode);
        return 1;
    }
    LoadGruntTypeTable(mode, 1, 0, 1);
    m_entrancePickup = PICKUP_INVALID;
    return 1;
}

retZero:
    return 0;
}

RVA(0x0006b260, 0x5)
i32 CGrunt::StepAttackAction() {

    return StepAttackFire();
}

RVA(0x0006b270, 0x1b)
CObject* CAniElement::AtChecked(i32 i) const {
    return GetAniElementAt(this, i);
}

RVA(0x0006b2a0, 0x23)
CAniElement* AnimationRegistry::FindAnimation(const char* key) {
    CAniElement* animation = NULL;
    MapLookup(m_animations, key, animation);
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

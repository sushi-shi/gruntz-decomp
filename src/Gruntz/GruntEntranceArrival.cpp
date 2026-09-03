#include <rva.h>

#include <Gruntz/GruntEntranceArrival.h>

#include <AddrWord.h>
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
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCombatClockInline.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/State.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Lith/ObjectUtilities.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Utils/MillisPer.h>
#include <Wap32/Object.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/Wap32.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x0020e194)
static char s_ToyTime[] = "ToyTime";
DATA(0x0020e1c8)
static char s_GRUNTZ_BIGWHEELGRUNT[] = "GRUNTZ_BIGWHEELGRUNT_BIGWHEELGRUNTLOOP";
DATA(0x0020e1f8)
static char s_GRUNTZ_GOKARTGRUNT[] = "GRUNTZ_GOKARTGRUNT_GOKARTGRUNTLOOP";
DATA(0x0020e224)
static char s_GRUNTZ_EXITZ_THREE[] = "GRUNTZ_EXITZ_THREE";
DATA(0x0020e23c)
static char s_GRUNTZ_EXITZ_TWO[] = "GRUNTZ_EXITZ_TWO";
DATA(0x0020e250)
static char s_GRUNTZ_EXITZ_ONE[] = "GRUNTZ_EXITZ_ONE";

RVA(0x000616e0, 0xa8)
i32 CGrunt::ResetGeometry() {
    SwitchAnimation(m_poseAttackIdle);

    DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)

    const char* name = EntranceCell()->AttackName().GetBuffer(0);
    SetImageFrameByName(name, frame);

    SET_ANIMATION_ACT("E");
    return 0;
}

RVA(0x000617c0, 0x127)
i32 CGrunt::UpdateGruntStatus() {
    if (m_poweredUp == false) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }

    m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_stamina >= STAMINA_FULL) {
        if (m_neighborValid != false) {
            m_neighborValid = false;
            CGrunt* n =
                m_triggerMgr
                    ->m_units[m_neighborPlayerIndex * TM_UNITS_PER_PLAYER + m_neighborUnitIndex];
            if (n != NULL && n->m_entranceCommitted != false) {
                Coord position = n->m_object->ScreenPos();
                if (RectContains(position.m_x, position.m_y)) {
                    CommitNeighbor(
                        m_neighborPlayerIndex,
                        m_neighborUnitIndex,
                        position.m_x,
                        position.m_y
                    );
                }
            }
        }
    } else if (m_stamina > STAMINA_HALF) {
        if (m_lowStaminaCued == false) {
            CGruntzMgr* g = g_gameReg;
            Coord position = m_object->ScreenPos();
            const RECT& vr = g->m_world->m_level->m_mainPlane->m_planeViewRect;
            if (::PtInRect(&vr, position.m_x, position.m_y)) {
                g->m_voiceManager->PlayGruntVoiceCue(this, 2, -1, -1, -1);
            }
            m_lowStaminaCued = true;
        }
    }
    return 0;
}

// @early-stop
RVA(0x00061940, 0x200)
i32 CGrunt::StartNeighborAttackAnimation(i32 targetPlayerIndex, i32 targetUnitIndex) {
    if (m_entranceReason >= PICKUP_TOYZ_FIRST) {
        return 0;
    }

    m_neighborPlayerIndex = targetPlayerIndex;
    m_neighborUnitIndex = targetUnitIndex;
    SET_ANIMATION_ACT("F");

    m_combatActive = true;

    i32 idx;
    switch (m_entranceReason) {
        case PICKUP_BOOMERANG:
            if (m_arrivalState != AI_NONE) {
                m_entranceActive = true;
            }
            idx = 1;
            break;
        case PICKUP_GUNHAT:
        case PICKUP_NERFGUN:
        case PICKUP_ROCK:
        case PICKUP_TIMEBOMB:
        case PICKUP_WARPSTONE:
        case PICKUP_WELDER:
        case PICKUP_WINGZ:
            idx = 1;
            break;
        default:
            idx = rand() % 2;
            break;
    }

    CreateHealthSprite();

    ArmGruntCombatTimeout(this);

    {
        CWwdSpriteObject* h = m_object;
        CGruntzMgr* g = g_gameReg;
        Coord position = h->ScreenPos();
        const RECT* rect = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(rect, position.m_x, position.m_y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 1, -1, -1, -1);
        }
    }

    {
        CWwdSpriteObject* h = m_object;
        i32 z = h->m_screenPosition.m_y + 0x186c1;
        SET_SORT_KEY_IF_CHANGED(h, z)
    }

    CWwdSpriteObject* p = m_wwdObject;
    m_value = p->m_animationCursor.m_animation;
    p->m_animationCursor.SetAnimation(m_poseAttack[idx]);

    DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)

    char* buf = EntranceCell()->AttackName().GetBuffer(0);
    SetImageFrameByName(buf, frame);
    m_struckPose = 1;
    return 0;
}

// @early-stop
RVA(0x00061bc0, 0xb2)
i32 CGrunt::StartRangedAttackAnimation() {
    SET_ANIMATION_ACT("F");

    CWwdSpriteObject* p = m_wwdObject;
    m_value = p->m_animationCursor.m_animation;
    p->m_animationCursor.SetAnimation(AT(m_poseAttack, GRUNT_ATTACK2));

    DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)

    char* buf = EntranceCell()->AttackName().GetBuffer(0);
    SetImageFrameByName(buf, frame);
    m_struckPose = 1;
    return 0;
}

// @early-stop
RVA(0x00061cb0, 0x380)
i32 CGrunt::StepAttackFire() {
    i32 advanced = m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    i32 flag = 0;
    if (advanced == WWDDRAW_EFFECT_FRAME) {

        switch (m_entranceReason) {
            case GRUNT_GUNHAT:
            case GRUNT_NERFGUN:
            case GRUNT_ROCK:
            case GRUNT_WELDER:
            case GRUNT_WINGZ: {
                CWwdSpriteObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenPosition.m_x,
                    m_object->m_screenPosition.m_y,
                    0,
                    "Projectile",
                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                );
                spr->m_logicRecord->m_dispatch(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_logicRecord->m_userLogic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_playerIndex,
                        m_unitIndex,
                        m_attackTargetPx.m_x,
                        m_attackTargetPx.m_y,
                        m_object->m_screenPosition.m_x,
                        m_object->m_screenPosition.m_y
                    )
                    == 0) {
                    s->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                }
                break;
            }
            case GRUNT_BOOMERANG: {
                CWwdSpriteObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenPosition.m_x,
                    m_object->m_screenPosition.m_y,
                    0,
                    "Boomerang",
                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                );
                spr->m_logicRecord->m_dispatch(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_logicRecord->m_userLogic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_playerIndex,
                        m_unitIndex,
                        m_attackTargetPx.m_x,
                        m_attackTargetPx.m_y,
                        m_object->m_screenPosition.m_x,
                        m_object->m_screenPosition.m_y
                    )
                    == 0) {
                    s->SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                }
                break;
            }
            case GRUNT_TIMEBOMB: {
                Coord position;
                EntranceTileOffset(&position.m_x);
                CGameObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    position.m_x,
                    position.m_y,
                    0xf,
                    "TimeBomb",
                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                );
                spr->m_damage = 0;
                spr->m_logicRecord->m_dispatch(spr);
                spr->m_smarts = m_playerIndex;
                break;
            }
            default: {

                CGrunt* tgt =
                    m_triggerMgr->m_units
                        [m_neighborPlayerIndex * TM_UNITS_PER_PLAYER + m_neighborUnitIndex];
                if (tgt != NULL) {
                    tgt->StepCombatReaction(
                        m_entranceReason,
                        m_struckPose,
                        m_playerIndex,
                        m_unitIndex,
                        m_object->m_screenPosition.m_x,
                        m_object->m_screenPosition.m_y,
                        0,
                        m_gruntKind
                    );
                    PickupType t = ArrivalPickup(tgt);
                    if (t == PICKUP_BOMB && m_gruntKind != GRUNT_INVULNERABLE) {
                        m_triggerMgr->StartUnitDeath(
                            m_playerIndex,
                            m_unitIndex,
                            DEATH_EXPLODE,
                            m_neighborPlayerIndex
                        );
                        return 0;
                    }
                    break;
                }
                flag = 1;
                break;
            }
        }

        m_entranceActive = true;
        i32 dt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), "AttackDowntime");
        if (m_gruntKind == GRUNT_ROIDZ) {
            dt = 0;
        }
        m_attackDowntimeLo = dt;
        m_attackDowntimeHi = 0;
        m_attackClockLo = static_cast<i32>(g_frameTime);
        m_attackClockHi = 0;
        m_lowStaminaCued = false;
        m_stamina = 0;
        if (m_healthSprite != NULL) {
            CreateStaminaSprite();
        }
        m_combatActive = false;
    }

    CAniAdvanceCursor* cur = &m_wwdObject->m_animationCursor;
    if ((cur->m_finished == false || cur->m_frameTicksLeft != 0) && flag == 0) {
        return 0;
    }
    if (m_entranceReason == GRUNT_BOOMERANG) {
        LoadGruntTypeTable(PICKUP_NONE, 1, 0, 0);
    }
    CWwdSpriteObject* h = m_object;
    i32 zkey = h->m_screenPosition.m_y + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(h, zkey)
    i32 v220 = m_poweredUp;
    m_entranceActive = false;
    if (v220 != 0) {
        ResetGeometry();
        return 0;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 0;
}

// @early-stop
RVA(0x00062110, 0x5bc)
i32 CGrunt::UpdateArrival(i32 walking, i32 commit) {
    if (commit != 0) {
        StopVehicleLoopSound();
        if (m_arrivalPhase == ARRIVAL_TAG_TRIGGER_B && m_arrivalActive != false) {
            CGrunt* occ =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (occ != NULL) {
                CGameObject* inner = occ->m_object;
                Coord position = inner->ScreenPos();
                Coord snapped = position;
                SnapTileCenter(&snapped);
                if (VehicleContactContains(snapped.m_x, snapped.m_y) != 0) {
                    m_triggerMgr->UseToyAt(m_playerIndex, m_unitIndex, position.m_x, position.m_y);
                }
            }
        }

        if (m_poweredUp != false && m_neighborValid == false) {
            RESET_GRUNT_POWERED_STATE(this)
        }
        m_entranceActive = true;
        SetEntrancePos(1, 1);

        if (CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS(this)
        }

        m_entranceStamped = false;
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)

        if (m_entranceReason == PICKUP_SCROLL) {
            SET_ANIMATION_ACT("P");
            i32 toyIdx = rand() % 2;
            SwitchAnimationAndMaybeAdvance(m_poseToy[toyIdx], 0);

            DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)
            char* buf = (&m_frameSetName)->GetBuffer(0);
            SetImageFrameByName(buf, frame);

            i32 cueTier = ((toyIdx != 0) ? 0xa : 0) + 0x406;
            i32 m380 = m_moveVariant;
            if (m380 != 0) {
                i32 tier = cueTier + m380 - 1;
                CGruntzMgr* g = g_gameReg;
                const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (CGameLevel::PointInBounds(
                        bounds,
                        m_object->m_screenPosition.m_x,
                        m_object->m_screenPosition.m_y
                    )
                    != 0) {
                    g->m_voiceManager->PlayVoice(this, tier, 0, -1, -1, -1);
                }
            } else {
                if (m_moveKind == 0) {
                    i32 md = 3;
                    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                        md = 6;
                    }
                    m_moveKind = GetRandom(1, md);
                }
                i32 tier = cueTier + m_moveKind - 1;
                CGruntzMgr* g = g_gameReg;
                const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (CGameLevel::PointInBounds(
                        bounds,
                        m_object->m_screenPosition.m_x,
                        m_object->m_screenPosition.m_y
                    )
                    != 0) {
                    g->m_voiceManager->PlayVoice(this, tier, 0, -1, -1, -1);
                }
            }
            return 0;
        } else {
            DWORD tt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), s_ToyTime);
            m_toyDuration = static_cast<u32>(tt);
            m_toyClock = static_cast<u32>(g_frameTime);
            m_toyTime = 0x64;
            CreateToyTimeSprite();
        }
    }

    if (walking != 0) {

        m_toyTileIndex = 0;
        if (m_poweredUp != false && m_neighborValid == false) {
            RESET_GRUNT_POWERED_STATE(this)
        }
        SET_ANIMATION_ACT("L");
        SwitchAnimation(m_poseWalk);
        char* nm = EntranceCell()->WalkName().GetBuffer(0);
        SetImageSetByName(nm);

        DWORD tt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), s_ToyTime);
        m_idleDelay = tt >> 1;
        m_idleAnchor = g_frameTime;
        return 0;
    }

    SET_ANIMATION_ACT("G");

    CWwdSpriteObject* h = m_object;
    i32 z = h->m_screenPosition.m_y + 0xc3500;
    SET_SORT_KEY_IF_CHANGED(h, z)

    i32 toy1DurationMs = AT(m_poseToy, GRUNT_TOY1)->m_durationMs;
    i32 toy2DurationMs = AT(m_poseToy, GRUNT_TOY2)->m_durationMs;
    i64 remainingMs = m_toyDuration - static_cast<i64>(g_frameTime) + m_toyClock;
    i32 availableMs = static_cast<i32>(remainingMs);
    if (remainingMs < 0) {
        availableMs = 0;
    }
    i32 toy1ExcessMs = 0;
    if (static_cast<u32>(toy1DurationMs) > static_cast<u32>(availableMs)) {
        toy1ExcessMs = toy1DurationMs - availableMs;
    }
    i32 toy2ExcessMs = 0;
    if (static_cast<u32>(toy2DurationMs) > static_cast<u32>(availableMs)) {
        toy2ExcessMs = toy2DurationMs - availableMs;
    }
    i32 sel;
    if (toy1ExcessMs == 0 && toy2ExcessMs == 0) {
        i32 r = rand() % 0x64 + 1;
        sel = (r >= m_toyBlendPct) ? 1 : 0;
    } else if (toy1ExcessMs != 0 && toy2ExcessMs == 0) {
        sel = 0;
    } else if (toy2ExcessMs != 0 && toy1ExcessMs == 0) {
        sel = 1;
    } else {
        sel = (static_cast<u32>(toy1ExcessMs) < static_cast<u32>(toy2ExcessMs)) ? 0 : 1;
    }

    CAniElement* want = m_poseToy[sel];
    if (m_wwdObject->m_animationCursor.m_animation != want) {
        SwitchAnimation(want);
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)
        char* buf = (&m_frameSetName)->GetBuffer(0);
        SetImageFrameByName(buf, frame);
    }

    CWwdSpriteObject* object = m_object;
    CGruntzMgr* g = g_gameReg;
    Coord position = object->ScreenPos();

    if (sel == 0) {
        if (::PtInRect(
                &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                position.m_x,
                position.m_y
            )) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xa, -1, -1, -1);
        }
    } else {
        if (::PtInRect(
                &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                position.m_x,
                position.m_y
            )) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xb, -1, -1, -1);
        }
    }
    return 0;
}

RVA(0x00062840, 0x25d)
i32 CGrunt::UpdateToyUseAnimation() {
    b32 ready = m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_wwdObject->m_animationCursor;
    if (IsAniCursorComplete(sub)) {
        if (m_arrived != false) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        SET_ANIMATION_ACT("A");
        LoadGruntTypeTable(m_toolId, 1, 0, 0);
        m_entranceActive = false;
        CGruntzMgr* g = g_gameReg;
        CMapMgr* grid = g->m_tileGrid;
        Coord tile = m_lastTilePx;
        ScreenTile(&tile);
        i32 flags = grid->CellFlagsAt(tile.m_x, tile.m_y);
        if (flags & IDX(CELL_FLAG_ARROW)) {
            SetEntrancePos(1, 1);
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            return 0;
        }
        CWwdSpriteObject* h = m_object;
        i32 v = h->m_screenPosition.m_y + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(h, v)
        return 0;
    }

    i64 diff = static_cast<i64>(g_frameTime) - m_toyClock;
    if (diff >= m_toyDuration && m_entranceStamped == false && ready == true) {
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
        SwitchAnimation(AT(m_poseToy, GRUNT_TOY_BREAK));
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
        char* nm = (&m_frameSetName)->GetBuffer(0);
        SetImageFrameByName(nm, frame);
        m_entranceStamped = true;
        CWwdSpriteObject* h = m_object;
        CGruntzMgr* g = g_gameReg;
        Coord position = h->ScreenPos();
        const RECT& r = g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(&r, position.m_x, position.m_y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xc, -1, -1, -1);
        }
        return 0;
    }
    StopVehicleLoopSound();
    if (ready == true) {
        UpdateArrival(0, 0);
    }
    return 0;
}

RVA(0x00062b40, 0x11)
i32 CGrunt::RecordFrameTick() {
    m_recordedFrameTick = g_frameTicks;
    return 1;
}

// @early-stop
RVA(0x00062b70, 0x205)
i32 CGrunt::RectSegProbe(RECT* p, POINT* e1, POINT* e2) {
    POINT first = *e1;
    POINT second = *e2;

    i32 py = p->top;
    if ((first.y < py) != (second.y < py)) {
        float t = static_cast<float>((py - first.y)) / static_cast<float>((second.y - first.y));
        float ix = static_cast<float>(first.x) + t * static_cast<float>((second.x - first.x));

        if (ix >= static_cast<float>(p->left) && ix <= static_cast<float>(p->right)) {
            return 1;
        }
    }

    i32 pyc = p->bottom;
    if ((first.y < pyc) != (second.y < pyc)) {
        float t = static_cast<float>((pyc - first.y)) / static_cast<float>((second.y - first.y));
        float ix = static_cast<float>(first.x) + t * static_cast<float>((second.x - first.x));

        if (ix >= static_cast<float>(p->left) && ix <= static_cast<float>(p->right)) {
            return 1;
        }
    }

    i32 px = p->left;
    if ((first.x > px) != (second.x > px)) {

        float t = static_cast<float>((px - first.x)) / static_cast<float>((second.x - first.x));
        float iy = static_cast<float>(first.y) + t * static_cast<float>((second.y - first.y));

        if (iy < static_cast<float>(p->bottom) && iy > static_cast<float>(p->top)) {
            return 1;
        }
    }

    i32 pxr = p->right;
    if ((first.x > pxr) != (second.x > pxr)) {

        float t = static_cast<float>((pxr - first.x)) / static_cast<float>((second.x - first.x));
        float iy = static_cast<float>(first.y) + t * static_cast<float>((second.y - first.y));

        if (iy < static_cast<float>(p->bottom) && iy > static_cast<float>(p->top)) {
            return 1;
        }
    }

    return 0;
}

// @early-stop
RVA(0x00062e10, 0x4a0)
void CGrunt::ResetEntranceAnimation(i32 refreshFrame, i32 chooseIdleVariant, i32 playVoiceCue) {
    m_resetApplied = false;

    bool notIdle = ANIMATION_ACT_DIFFERS("A");
    i32 applied = 0;

    if (notIdle && chooseIdleVariant == 0) {

        SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE1));
        m_idleWindow = static_cast<u32>(0x3a98);
        m_idleTimer = g_frameTime;
        i32 d = static_cast<i32>(g_buteMgr.GetDword("Grunt", "IdleDelay", 0x7530));
        m_idleDelay = static_cast<u32>(0x7530 + GetRandom(0, d));
        m_idleAnchor = g_frameTime;
        applied = 1;
    } else if (AT(m_poseIdle, GRUNT_IDLE2) != NULL) {
        if (chooseIdleVariant != 0) {

            i32 count = 1;
            if (AT(m_poseIdle, GRUNT_IDLE3) != NULL) {
                count = 2;
            }
            i32 idx = GetRandom(1, count);
            if (playVoiceCue != 0) {
                g_gameReg->Rand();
                b32 focused = (m_playerIndex == g_curPlayer);
                if (focused && idx > 0x5a) {
                    CGruntzMgr* g = g_gameReg;
                    if (CGameLevel::PointInBounds(
                            &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                            m_object->m_screenPosition.m_x,
                            m_object->m_screenPosition.m_y
                        )) {

                        g->m_voiceManager->PlayGruntVoiceCue(this, 4, -1, -1, -1);
                    }
                } else if (focused || m_entranceReason != PICKUP_NONE) {
                    switch (idx) {
                        case GRUNT_IDLE_VARIANT_PRIMARY: {
                            CGruntzMgr* g = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                                    m_object->m_screenPosition.m_x,
                                    m_object->m_screenPosition.m_y
                                )) {

                                g->m_voiceManager->PlayGruntVoiceCue(this, 5, -1, -1, -1);
                            }
                            break;
                        }
                        case GRUNT_IDLE_VARIANT_SECONDARY: {
                            CGruntzMgr* g = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                                    m_object->m_screenPosition.m_x,
                                    m_object->m_screenPosition.m_y
                                )) {

                                g->m_voiceManager->PlayGruntVoiceCue(this, 6, -1, -1, -1);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            SwitchAnimation(m_poseIdle[idx]);
            m_resetApplied = true;
            applied = 1;
        } else {

            if (m_wwdObject->m_animationCursor.m_animation == AT(m_poseIdle, GRUNT_IDLE1)) {
                goto latch;
            }
            SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE1));
            {
                i32 d = static_cast<i32>(g_buteMgr.GetDword("Grunt", "IdleDelay", 0x7530));
                applied = 1;
                m_idleDelay = static_cast<u32>(GetRandom(0x4e20, d));
                m_idleAnchor = g_frameTime;
            }
        }
    } else {

        SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE1));
    }

latch:
    SET_ANIMATION_ACT("A");

    if (!applied && refreshFrame == 0) {
        return;
    }

    GruntDirectionCell cell = m_entranceCell;
    if (m_wwdObject->m_animationCursor.m_animation != AT(m_poseIdle, GRUNT_IDLE1)) {
        switch (m_entranceCell.direction) {
            case DIR_NORTHEAST:
            case DIR_EAST:
                cell = g_gruntDirEast;
                break;
            case DIR_SOUTHEAST:
            case DIR_SOUTH:
                cell = g_gruntDirSouth;
                break;
            case DIR_SOUTHWEST:
            case DIR_WEST:
            case DIR_NORTHWEST:
                cell = g_gruntDirWest;
                break;
            default:
                break;
        }
    }

    {
        CString key = m_cells[3 * cell.row + cell.column].IdleName();

        APPLY_CURRENT_ANIMATION_FRAME_SPRITE(key, desc, elem)
    }
}

// @early-stop
RVA(0x000633e0, 0x2f1)
i32 CGrunt::ResolveEntranceArrival() {
    if (m_entranceActive != false && GRUNT_AT_SAVED_SCREEN_POS(this)) {
        CGruntzMgr* g = g_gameReg;
        CMapMgr* grid = g->m_tileGrid;
        Coord tile;
        GetScreenTile(&tile);
        i32 flags = grid->CellFlagsAt(tile.m_x, tile.m_y);
        if (!(flags & IDX(CELL_FLAG_ARROW))) {
            m_entranceActive = false;
        }
    }

    b32 ready = m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (static_cast<i64>(g_frameTime) - m_idleTimer >= m_idleWindow) {
        CGruntzMgr* g = g_gameReg;
        GameModeId mode = g->m_gameMode;
        if (mode != GAMEMODE_QUESTZ) {
            GruntzPlayer* slot = &g->m_players[m_playerIndex];
            if (slot != NULL && slot->m_humanControlled != false) {
                if (m_tileClaimed == false && m_arrivalNotified == false
                    && mode == GAMEMODE_MULTIPLAYER && g_curPlayer == m_playerIndex
                    && m_arrived == false) {
                    m_triggerMgr->EnqueueGuardBegin(m_playerIndex, m_unitIndex);
                    m_arrivalNotified = true;
                    goto tail;
                }
                if (mode != GAMEMODE_MULTIPLAYER && g_curPlayer == m_playerIndex
                    && m_arrived == false && m_tileClaimed != true) {
                    m_arrivalRerollLo = 0;
                    m_arrivalRerollWindowLo = 0;
                    m_arrivalRerollHi = 0;
                    m_arrivalRerollWindowHi = 0;
                    m_defenderPx = m_lastTilePx;
                    m_tileClaimed = true;
                    PickupType kind = m_entranceReason;

                    switch (kind) {
                        case PICKUP_BOOMERANG:
                        case PICKUP_GUNHAT:
                        case PICKUP_NERFGUN:
                        case PICKUP_ROCK:
                        case PICKUP_WELDER:
                        case PICKUP_WINGZ:
                            m_defenderRadius = 1;
                            break;
                        default:
                            m_defenderRadius =
                                g_buteMgr.GetInt("Grunt", "PlayerDefenderRadius", 3) + 1;
                            break;
                    }
                    m_arrivalCell.Set(-1, -1);
                    m_arrivalState = AI_DEFENDER;
                    m_defenderState = AISTATE_SEEK;
                    m_arrivalActive = false;
                    m_arrivalFlags |=
                        IDX(CELL_FLAG_SPECIAL | CELL_FLAG_SPIKES | CELL_FLAG_IN_GAME_ICON
                            | CELL_FLAG_STATIC_HAZARD | CELL_FLAG_ROLLING_BALL);
                    SetRectEmpty(&m_object->m_extent);
                    SetEntrancePos(0, 0);
                }
            }
        }
    }

tail:
    if (m_wwdObject->m_animationCursor.m_animation != AT(m_poseIdle, GRUNT_IDLE1)) {

        if (IsAniCursorComplete(&m_wwdObject->m_animationCursor)) {
            ResetEntranceAnimation(0, 0, 0);
        }
        return 0;
    }
    if (static_cast<i64>(g_frameTime) - m_idleAnchor >= m_idleDelay && ready == true) {
        ResetEntranceAnimation(0, 1, 1);
    }
    return 0;
}

// @early-stop
RVA(0x000637a0, 0x2f8)
i32 CGrunt::StepEntranceReinit() {
    bool eq;
    eq = ANIMATION_ACT_EQUALS("D");
    if (eq) {
        return 0;
    }
    eq = ANIMATION_ACT_EQUALS("L");
    if (eq) {
        return 0;
    }

    m_arrivalVoiceWindowLo = 0x7530;
    m_arrivalVoiceWindowHi = 0;
    m_arrivalVoiceClockLo = static_cast<i32>(g_frameTime);
    m_arrivalVoiceClockHi = 0;
    m_neighborScanEnabled = false;

    eq = ANIMATION_ACT_EQUALS("I");
    if (eq) {

        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
    }
    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    m_tileMoveCommitted = false;
    if (CoordCount() == 0) {
        return 0;
    }

    Coord* targetCoord = static_cast<Coord*>(m_coordList.GetHead());
    CMapMgr* tileGrid = g_gameReg->m_tileGrid;
    i32 targetCellFlags = tileGrid->CellFlagsAt(targetCoord->m_x, targetCoord->m_y);
    GruntDirectionCell cell;
    if (!(targetCellFlags & BRICKZ_CELL_OCCUPIED)) {
        SET_ANIMATION_ACT("D");
        SwitchAnimation(m_poseWalk);
        cell = m_entranceCell;
    } else {

        Coord currentTile;
        GetScreenTile(&currentTile);
        i32 currentCellFlags = tileGrid->CellFlagsAt(currentTile.m_x, currentTile.m_y);
        if (!(currentCellFlags & IDX(CELL_FLAG_ARROW))) {
            return 0;
        }
        SET_ANIMATION_ACT("D");
        SwitchAnimation(m_poseWalk);
        cell = m_entranceCell;
        m_entranceActive = true;
    }
    char* walkAnimationName = m_cells[3 * cell.row + cell.column].WalkName().GetBuffer(0);
    SetImageSetByName(walkAnimationName);
    return 0;
}

// @early-stop
RVA(0x00063b60, 0x1cf)
i32 CGrunt::StepArrivalReroll() {
    m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    i64 diff = static_cast<i64>(g_frameTime) - m_arrivalVoiceClock.m_v;

    u32 elapsed;
    if (diff < 0) {
        elapsed = 0;
    } else {
        elapsed = static_cast<u32>(diff);
    }
    if (elapsed <= 0x2710) {
        return 0;
    }
    if (elapsed % MILLIS_PER_SECOND != 0) {
        return 0;
    }
    i32 v;
    i32 range = 0x7531 - elapsed;
    if (range == 0) {
        if ((GetRandomNumber() & 1)) {
            v = elapsed;
        } else {
            v = 0x7530;
        }
    } else {
        v = GetRandomNumber() % range + elapsed;
    }
    if (v <= 0x7148) {
        return 0;
    }
    i32 pick = GetRandomNumber() % 0x65;
    CWwdSpriteObject* h = m_object;
    Coord position = h->ScreenPos();
    if (pick > 0x19) {
        if (::PtInRect(
                &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect,
                position.m_x,
                position.m_y
            )) {
            g_gameReg->m_voiceManager->PlayVoice(this, 0x15d, -1, 0, -1, -1);
        }
    } else {
        if (::PtInRect(
                &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect,
                position.m_x,
                position.m_y
            )) {
            g_gameReg->m_voiceManager->PlayGruntVoiceCue(this, 9, -1, -1, -1);
        }
    }
    return 0;
}

RVA(0x00063db0, 0x32f)
i32 CGrunt::LoadVehicleGruntAnimations() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, static_cast<u32>(g_engineFrameDelta))
    if (IsAniCursorComplete(sub)) {
        if (m_arrived) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        SET_ANIMATION_ACT("A");
        LoadGruntTypeTable(m_toolId, 1, 0, 0);
        m_entranceActive = false;

        CMapMgr* grid = g_gameReg->m_tileGrid;
        Coord tile = m_lastTilePx;
        ScreenTile(&tile);
        i32 flags = grid->CellFlagsAt(tile.m_x, tile.m_y);
        if (flags & IDX(CELL_FLAG_ARROW)) {
            SetEntrancePos(1, 1);
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        }
        return 0;
    }

    i64 elapsed = static_cast<i64>(g_frameTime) - m_toyClock;
    if (elapsed >= m_toyDuration) {
        if (m_entranceStamped == false && GRUNT_AT_SAVED_SCREEN_POS(this)) {
            HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
            SetEntrancePos(1, 1);
            m_entranceStamped = true;
            SwitchAnimationAndMaybeAdvance(AT(m_poseToy, GRUNT_TOY_BREAK), 0);

            DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
            char* buf = (&m_frameSetName)->GetBuffer(0);
            SetImageFrameByName(buf, frame);

            CWwdSpriteObject* h = m_object;
            CGruntzMgr* g = g_gameReg;
            Coord position = h->ScreenPos();
            const RECT& rect = g->m_world->m_level->m_mainPlane->m_planeViewRect;
            if (::PtInRect(&rect, position.m_x, position.m_y)) {
                g->m_voiceManager->PlayGruntVoiceCue(this, 0xc, -1, -1, -1);
                StopVehicleLoopSound();
                return 0;
            }
        }
        StopVehicleLoopSound();
        return 0;
    }

    i64 elapsed2 = static_cast<i64>(g_frameTime) - m_idleAnchor;
    if (elapsed2 >= m_idleDelay) {
        CWwdSpriteObject* h = m_object;
        CGruntzMgr* g = g_gameReg;
        Coord position = h->ScreenPos();
        const RECT& rect = g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(&rect, position.m_x, position.m_y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xd, -1, -1, -1);
        }
    }

    CWwdSpriteObject* h2 = m_object;
    CGruntzMgr* g2 = g_gameReg;
    Coord position = h2->ScreenPos();
    if (::PtInRect(&g2->m_viewBounds, position.m_x, position.m_y)) {
        if (m_entranceReason == PICKUP_GOKART) {
            EnsureVehicleLoopSound(s_GRUNTZ_GOKARTGRUNT);
            return 0;
        }
        if (m_entranceReason == PICKUP_BIGWHEEL) {
            EnsureVehicleLoopSound(s_GRUNTZ_BIGWHEELGRUNT);
            return 0;
        }
        return 0;
    }
    StopVehicleLoopSound();
    return 0;
}

RVA(0x000641b0, 0x2c1)
i32 CGrunt::BuildGruntExitAnimation() {
    if (m_deathAnimStarted != false) {
        return 0;
    }

    FinishActiveAction();
    STOP_GRUNT_LOOP_SOUNDS;

    m_object->m_stateFlags &= ~SPRITE_STATE_FLASHING;
    m_entranceCommitted = false;
    m_deathAnimStarted = true;

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

    SET_ANIMATION_ACT("B");

    CAniElement* found;
    i32 r = rand() % 0x1e1;
    if (r > 0x140) {
        found = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_GRUNTZ_EXITZ_ONE);
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                m_object->m_screenPosition.m_x,
                m_object->m_screenPosition.m_y
            )) {
            g->m_voiceManager->PlayVoice(this, 0x384, -1, 0, -1, -1);
        }
    } else if (r > 0xa0) {
        found = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_GRUNTZ_EXITZ_TWO);
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                m_object->m_screenPosition.m_x,
                m_object->m_screenPosition.m_y
            )) {
            g->m_voiceManager->PlayVoice(this, 0x385, -1, 0, -1, -1);
        }
    } else {
        found = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_GRUNTZ_EXITZ_THREE);
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                m_object->m_screenPosition.m_x,
                m_object->m_screenPosition.m_y
            )) {
            g->m_voiceManager->PlayVoice(this, 0x386, -1, 0, -1, -1);
        }
    }

    CWapX::ApplyAnimation(found, 0);
    i32 frame =
        static_cast<CAniRecordView*>(m_wwdObject->m_animationCursor.m_animation->AtChecked(0))
            ->m_param;
    SetImageFrameByName("GRUNTZ_EXITZ", frame);
    return 0;
}

// @early-stop
RVA(0x00064540, 0x11c)
i32 CGrunt::StepWarpExit() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, g_engineFrameDelta)
    if (IsAniCursorComplete(sub)) {
        if (m_deathType == GRUNT_DEATH_WARPOUT) {
            CState* st = g_gameReg->m_curState;
            i32 lvl = st->m_levelIndex + 0x64;
            CString s;
            s.Format("WORLDZ\\LEVEL%i", lvl);
            if (st->m_levelResources->GetRezFromPath(static_cast<LPCTSTR>(s), REZ_TAG_WWD)) {
                PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_WORLD), lvl);
            }
        }
        if (m_cellRemovalNotified == false) {
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 1);
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    return 0;
}

// @early-stop
RVA(0x000646b0, 0x9c8)
i32 CGrunt::StepCombatReaction(
    PickupType attackKind,
    i32 struckPose,
    i32 srcPlayerIndex,
    i32 srcUnitIndex,
    i32 srcPxX,
    i32 srcPxY,
    i32 fromProjectile,
    PickupType attackerGruntKind
) {
    if (m_entranceCommitted == false || m_entranceDropActive != false) {
        return 0;
    }
    {
        CWwdSpriteObject* h = m_object;
        i32 v = h->m_screenPosition.m_y + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(h, v)
    }

    bool ne;
    bool eq;
    ne = ANIMATION_ACT_DIFFERS("A");
    if (!ne) {
        goto tail;
    }
    ne = ANIMATION_ACT_DIFFERS("D");
    if (!ne) {
        goto tail;
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
        goto tail;
    }
    eq = ANIMATION_ACT_EQUALS("G");
    if (!eq) {
        eq = ANIMATION_ACT_EQUALS("L");
        if (!eq) {
            eq = ANIMATION_ACT_EQUALS("P");
            if (!eq) {
                eq = ANIMATION_ACT_EQUALS("O");
                if (eq) {
                    SnapToLastTile(1);
                    m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
                    goto tail;
                }
                eq = ANIMATION_ACT_EQUALS("Q");
                if (eq) {
                    m_triggerMgr
                        ->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_SHATTER, srcPlayerIndex);
                    return 0;
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
                        char* cn = EntranceCell()->WalkName().GetBuffer(0);
                        SetImageSetByName(cn);
                    } else {
                        ResetEntranceAnimation(1, 0, 0);
                    }
                    PickupType mode = m_entrancePickup;
                    if (mode >= PICKUP_POWERUPZ_FIRST) {
                        LoadGruntTypeTable(mode, 1, 0, 1);
                        m_entrancePickup = PICKUP_INVALID;
                        m_helpCueId = 0;
                    } else if (mode >= PICKUP_BRICKZ_FIRST) {
                        m_brickPickupType = mode;
                        m_entrancePickup = PICKUP_INVALID;
                    } else if (mode >= PICKUP_TOYZ_FIRST) {
                        LoadVehicleGruntSprites(mode);
                    } else {
                        LoadGruntTypeTable(mode, 1, 0, 1);
                        m_entrancePickup = PICKUP_INVALID;
                    }
                    goto tail;
                }
                eq = ANIMATION_ACT_EQUALS("N");
                if (eq) {
                    CWwdSpriteObject* h = m_object;
                    Coord snapped = h->ScreenPos();
                    SnapTileCenter(&snapped);
                    i32 flag = 1;
                    if (snapped != m_lastTilePx) {
                        if (IsDropReady(1)) {
                            m_coordToggle = (m_coordToggle == false) ? 1 : 0;
                            flag = 0;
                        }
                    }
                    SnapToLastTile(1);
                    if (flag != 0) {
                        SET_ANIMATION_ACT("D");
                        SetupTubeAnim(m_coordToggle);
                    }
                }
                goto tail;
            }
        }
    }
    if (m_entranceReason == PICKUP_SCROLL) {
        g_gameReg->m_voiceManager->StopVoice(m_object->m_objectId);
    }
    LoadGruntTypeTable(m_toolId, 1, 0, 1);
    {
        CWwdSpriteObject* h = m_object;
        i32 v = h->m_screenPosition.m_y + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(h, v)
    }
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    m_toyTime = 0;
    StopVehicleLoopSound();

tail:
    CreateHealthSprite();
    ArmGruntCombatTimeout(this);
    if (GRUNT_NOT_AT_SAVED_SCREEN_POS(this)) {
        ConsiderArrival(1);
    }
    if (LoadGruntCombatAnimations(
            attackKind,
            struckPose,
            srcPlayerIndex,
            srcUnitIndex,
            srcPxX,
            srcPxY,
            fromProjectile,
            attackerGruntKind
        )
        == 0) {
        return 0;
    }

    {
        CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        eq = (strcmp(*rec, "F") == 0);
        if (eq) {
            if (m_entranceCommitted != false) {
                return 0;
            }
        }
    }
    m_entranceActive = true;
    {
        CString* rec = g_typeColl.ScratchResolve(m_logicRecord->m_eventCode);
        ActNameConstructGrownSlots();
        ne = (strcmp(*rec, "O") != 0);
        if (ne) {
            SET_ANIMATION_ACT("H");
            CGrunt* cellObj =
                m_triggerMgr->m_units[srcPlayerIndex * TM_UNITS_PER_PLAYER + srcUnitIndex];
            if (cellObj != NULL) {
                CGameObject* oh = cellObj->m_object;
                Coord position = oh->ScreenPos();
                Coord lastTile = m_lastTilePx;
                ScreenTile(&lastTile);
                if (m_neighborScanEnabled != false && m_entranceCommitted != false
                    && RectContains(position.m_x, position.m_y)) {
                    if (!(g_gameReg->m_tileGrid->CellFlagsAt(lastTile.m_x, lastTile.m_y)
                          & IDX(CELL_FLAG_ARROW))) {
                        CommitNeighbor(srcPlayerIndex, srcUnitIndex, position.m_x, position.m_y);
                    }
                }
            }
        }
    }

    m_combatActive = false;
    CAniElement* pose = m_poseStruck[struckPose];
    SwitchAnimation(pose);
    i32 frame;
    {
        CAniElement* desc = m_wwdObject->m_animationCursor.m_animation;
        CAniRecordView* elem = static_cast<CAniRecordView*>(GetAniElementAt(desc, 0));
        frame = elem->m_param;
    }
    {
        char* cn = EntranceCell()->StruckName().GetBuffer(0);
        SetImageFrameByName(cn, frame);
    }
    {
        CWwdSpriteObject* h = m_object;
        Coord position = h->ScreenPos();
        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(rect, position.m_x, position.m_y)) {
            g_gameReg->m_voiceManager->PlayGruntVoiceCue(this, 7, -1, -1, -1);
        }
    }
    return 0;
}

RVA(0x00065300, 0x148)
i32 CGrunt::FinishStruckAnimation() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, static_cast<u32>(g_engineFrameDelta))
    if (!IsAniCursorComplete(sub)) {
        return 0;
    }
    if (m_health <= 0) {
        m_entranceCommitted = false;
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, m_killerPlayerIndex);
        return 0;
    }
    m_entranceActive = false;

    CMapMgr* grid = g_gameReg->m_tileGrid;
    Coord tile = m_lastTilePx;
    ScreenTile(&tile);
    i32 flags = grid->CellFlagsAt(tile.m_x, tile.m_y);
    if (flags & IDX(CELL_FLAG_ARROW)) {
        SetEntrancePos(1, 1);
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        return 0;
    }
    if (m_neighborScanEnabled == false && m_tileMoveCommitted != false) {
        StepArrivalDrop(m_commitPx.m_x, m_commitPx.m_y, 0, -1, 1, 0);
        return 0;
    }
    if (m_entranceReason == PICKUP_WARPSTONE) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

RVA(0x000654b0, 0x130)
i32 CGrunt::FinishKnockbackAnimation() {

    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, static_cast<u32>(g_engineFrameDelta))
    if (!IsAniCursorComplete(sub)) {
        return 0;
    }
    m_entranceActive = false;
    SnapToLastTile(1);
    SetEntrancePos(1, 1);

    m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    if (m_health <= 0) {
        m_entranceCommitted = false;
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, m_killerPlayerIndex);
        return 0;
    }
    CGruntzMgr* g = g_gameReg;
    CMapMgr* grid = g->m_tileGrid;
    Coord tile = m_lastTilePx;
    ScreenTile(&tile);
    i32 flags = grid->CellFlagsAt(tile.m_x, tile.m_y);
    if (flags & IDX(CELL_FLAG_ARROW)) {
        return 0;
    }
    if (m_neighborScanEnabled == false && m_tileMoveCommitted != false) {
        StepArrivalDrop(m_commitPx.m_x, m_commitPx.m_y, 0, -1, 1, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// @early-stop
RVA(0x00065630, 0x34b)
i32 CGrunt::RunMoveConfig(i32 tileX, i32 tileY) {
    bool eq = ANIMATION_ACT_EQUALS("I");
    if (eq) {
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            WWDDRAW_NO_ANIMATION
        );
    } else {
        CWwdSpriteObject* h = m_object;
        CGruntzMgr* g = g_gameReg;
        const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (CGameLevel::PointInBounds(bounds, h->m_screenPosition.m_x, h->m_screenPosition.m_y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 8, -1, -1, -1);
        }
    }

    FaceTowardTile(tileX, tileY);
    m_moveTile.Set(tileX, tileY);
    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }

    i32 poseIdx = 0;
    if (m_entranceReason == PICKUP_BOMB) {
        SET_ANIMATION_ACT("M");
        m_object->m_stateFlags &= ~SPRITE_STATE_FLASHING;
        m_timePerTile = g_buteMgr.GetDword("BOMBGRUNT", "RunningTimePerTile", 0x64);
        m_entranceActive = true;
        m_bombRunActive = true;
        SetEntrancePos(1, 1);
    } else if (m_entranceReason == PICKUP_TOOB) {
        m_entranceActive = true;
        SET_ANIMATION_ACT("N");
        m_coordToggle = (m_coordToggle == false);
    } else if (m_entranceReason == PICKUP_WAND) {
        i32 base;
        if (IsRandomChance(80)) {
            poseIdx = 1;
            base = 0x41a;
        } else {
            poseIdx = 0;
            base = 0x424;
        }

        i32 variant = m_moveVariantOverride;
        m_moveVariant = variant;
        if (variant == 0) {
            i32 n = 3;
            if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                n = 6;
            }
            m_moveVariant = GetRandom(1, n);
        }

        i32 cueId = base + m_moveVariant - 1;
        CWwdSpriteObject* h = m_object;
        CGruntzMgr* g = g_gameReg;
        Coord position = h->ScreenPos();
        const RECT& rect = g->m_world->m_level->m_mainPlane->m_planeViewRect;
        if (::PtInRect(&rect, position.m_x, position.m_y)) {
            g->m_voiceManager->PlayVoice(this, cueId, -1, 0, -1, -1);
        }

        SET_ANIMATION_ACT("I");
        m_entranceActive = true;
        SetEntrancePos(1, 1);
    } else {
        SET_ANIMATION_ACT("I");
        SetEntrancePos(1, 1);
    }

    SwitchAnimation(m_poseItem[poseIdx]);

    char* name = EntranceCell()->ItemName().GetBuffer(0);
    SetImageSetByName(name);
    return 0;
}

// @early-stop
RVA(0x00065a60, 0x159)
i32 CGrunt::LoadWandGruntItemConfig() {
    i32 advanced = m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    if (advanced > 0) {
        WwdAniDrawValue cue = static_cast<WwdAniDrawValue>(advanced);
        if (cue == WWDDRAW_TOOL_APPLIES) {
            m_entranceActive = true;
            u32 downtime =
                g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), "ItemDowntime");
            if (m_gruntKind == GRUNT_ROIDZ) {
                downtime = 0;
            }
            m_attackDowntimeLo = downtime;
            m_attackDowntimeHi = 0;
            m_attackClockLo = g_frameTime;
            m_attackClockHi = 0;
            m_lowStaminaCued = false;
            m_stamina = 0;
            if (m_healthSprite != NULL) {
                CreateStaminaSprite();
            }
            if (m_entranceReason == PICKUP_WAND) {
                LoadGruntAbilityTuning(m_moveVariant);
                i32 hp = m_health - g_buteMgr.GetInt("WANDGRUNT", "HealthLoss", 0x19);
                m_health = hp < 0 ? 0 : hp;
                if (m_health <= 0) {
                    m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, -1);
                }
            }
        }
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            cue
        );
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_animationCursor;
    if (IsAniCursorComplete(sub)) {
        m_entranceActive = false;
        ResetEntranceAnimation(1, 0, 0);
    }
    return 0;
}

// @early-stop
RVA(0x00065c20, 0x1d5)
i32 CGrunt::FinishToobMoveAnimation() {
    i32 advanced = m_wwdObject->m_animationCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    if (advanced > 0) {
        WwdAniDrawValue cue = static_cast<WwdAniDrawValue>(advanced);
        m_triggerMgr->LoadTileArrivalFx(
            m_playerIndex,
            m_unitIndex,
            m_moveTile.m_x,
            m_moveTile.m_y,
            m_entranceReason,
            cue
        );
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_animationCursor;
    if (!IsAniCursorComplete(sub)) {
        return 0;
    }
    m_entranceActive = false;
    if (m_arrived != false) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }
    SET_ANIMATION_ACT("D");
    SetupTubeAnim(m_coordToggle);
    CGruntzMgr* g = g_gameReg;
    CMapMgr* grid = g->m_tileGrid;
    Coord tile = m_lastTilePx;
    ScreenTile(&tile);
    i32 f1 = grid->CellFlagsAt(tile.m_x, tile.m_y);
    if (f1 & IDX(CELL_FLAG_TOOB_SPIKE)) {
        BuildGruntLoseItemAnimation();
        g = g_gameReg;
    }
    grid = g->m_tileGrid;
    char* cellObj;
    if (static_cast<u32>(tile.m_x) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(tile.m_y) >= static_cast<u32>(grid->m_height)) {
        cellObj = NULL;
    } else {

        AddrWord<char> slot;
        slot.m_word = grid->m_rows[tile.m_y][tile.m_x].m_objectId;
        cellObj = slot.m_addr;
    }
    if (cellObj == NULL) {
        return 0;
    }
    CGameObject* found = NULL;
    if (MapLookup(
            g->m_world->m_childGroup->m_registeredGameObjectsById,
            static_cast<void*>(cellObj),
            found
        )
        == false) {
        found = NULL;
    }
    if (found == NULL) {
        grid = g_gameReg->m_tileGrid;
        if (static_cast<u32>(tile.m_x) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tile.m_y) < static_cast<u32>(grid->m_height)) {
            BrickzCell& cell = grid->m_rows[tile.m_y][tile.m_x];
            cell.m_objectId = 0;
            cell.m_flags &= ~IDX(CELL_FLAG_IN_GAME_ICON);
        }
        return 0;
    }
    CInGameIcon* icon = static_cast<CInGameIcon*>(found->m_logicRecord->m_userLogic);
    icon->PlaceAt(m_playerIndex, m_unitIndex);
    return 0;
}

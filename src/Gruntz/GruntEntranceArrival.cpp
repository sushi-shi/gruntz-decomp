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
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
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

    GruntDirectionCell cell = m_entranceCell;
    i32 row = cell.row;
    i32 column = cell.column;
    i32 index = 3 * row + column;

    const char* name = m_cells[index].AttackName().GetBuffer(0);
    ApplyLookupSprite(name, frame);

    SET_ANIMATION_ACT("E");
    return 0;
}

RVA(0x000617c0, 0x127)
i32 CGrunt::UpdateGruntStatus() {
    if (m_poweredUp == 0) {
        ResetEntranceAnimation(1, 0, 0);
        return 0;
    }

    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (m_stamina >= STAMINA_FULL) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            CGrunt* n =
                m_triggerMgr
                    ->m_units[m_neighborPlayerIndex * TM_UNITS_PER_PLAYER + m_neighborUnitIndex];
            if (n != NULL && n->m_entranceCommitted != 0) {
                if (RectContains(n->m_object->m_screenX, n->m_object->m_screenY)) {
                    CommitNeighbor(
                        m_neighborPlayerIndex,
                        m_neighborUnitIndex,
                        n->m_object->m_screenX,
                        n->m_object->m_screenY
                    );
                }
            }
        }
    } else if (m_stamina > STAMINA_HALF) {
        if (m_lowStaminaCued == 0) {
            CGruntzMgr* g = g_gameReg;
            i32 y = m_object->m_screenY;
            i32 x = m_object->m_screenX;
            const RECT& vr = g->m_world->m_level->m_mainPlane->m_viewRect;
            if (CGameLevel::PointInRect(&vr, x, y)) {
                g->m_voiceManager->PlayGruntVoiceCue(this, 2, -1, -1, -1);
            }
            m_lowStaminaCued = 1;
        }
    }
    return 0;
}

// @early-stop
// cl hoists `idx = 1` out of the two switch arms and then re-uses that ebx as the
// function's constant-1 register (m_combatActive, m_entranceActive, the rand()%2
// mask all read ebx where retail uses immediates); retail's constant register holds
// 0 instead, shared by the two i64 high-dword stores. 9 arm/timer spellings and 13
// graded declaration counts measured, all 88.50.
RVA(0x00061940, 0x200)
i32 CGrunt::RearmAttackAnim(i32 targetPlayerIndex, i32 targetUnitIndex) {
    if (m_entranceReason >= PICKUP_TOYZ_FIRST) {
        return 0;
    }

    m_neighborPlayerIndex = targetPlayerIndex;
    m_neighborUnitIndex = targetUnitIndex;
    SET_ANIMATION_ACT("F");

    m_combatActive = 1;

    i32 idx;
    switch (m_entranceReason) {
        case PICKUP_BOOMERANG:
            if (m_arrivalState != AI_NONE) {
                m_entranceActive = 1;
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
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 yy = h->m_screenY;
        i32 xx = h->m_screenX;
        const RECT* rect = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInRect(rect, xx, yy)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 1, -1, -1, -1);
        }
    }

    {
        CWwdGameObjectA* h = m_object;
        i32 z = h->m_screenY + 0x186c1;
        SET_SORT_KEY_IF_CHANGED(h, z)
    }

    CWwdGameObjectA* p = m_wwdObject;
    m_value = p->m_animCursor.m_animation;
    p->m_animCursor.Setup(m_poseAttack[idx]);

    DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)

    GruntDirectionCell cell = m_entranceCell;
    i32 cellRow = cell.row;
    i32 cellColumn = cell.column;
    i32 base = cellRow + (cellColumn + 2 * cellRow);
    char* buf = m_cells[base].AttackName().GetBuffer(0);
    ApplyLookupSprite(buf, frame);
    m_struckPose = 1;
    return 0;
}

// @early-stop
RVA(0x00061bc0, 0xb2)
i32 CGrunt::RearmAttackAnim2() {
    SET_ANIMATION_ACT("F");

    CWwdGameObjectA* p = m_wwdObject;
    m_value = p->m_animCursor.m_animation;
    p->m_animCursor.Setup(AT(m_poseAttack, GRUNT_ATTACK2));

    DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)

    GruntDirectionCell cell = m_entranceCell;
    i32 row = cell.row;
    i32 column = cell.column;
    i32 base = row + (column + 2 * row);
    char* buf = m_cells[base].AttackName().GetBuffer(0);
    ApplyLookupSprite(buf, frame);
    m_struckPose = 1;
    return 0;
}

// @early-stop
RVA(0x00061cb0, 0x380)
i32 CGrunt::StepAttackFire() {
    i32 advanced = m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    i32 flag = 0;
    if (advanced == WWDDRAW_EFFECT_FRAME) {

        switch (m_entranceReason) {
            case GRUNT_GUNHAT:
            case GRUNT_NERFGUN:
            case GRUNT_ROCK:
            case GRUNT_WELDER:
            case GRUNT_WINGZ: {
                CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_logicRecord->m_dispatch(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_logicRecord->m_userLogic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_playerIndex,
                        m_unitIndex,
                        m_attackTargetPx.m_x,
                        m_attackTargetPx.m_y,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->SetObjectFlags(0x10000);
                }
                break;
            }
            case GRUNT_BOOMERANG: {
                CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Boomerang",
                    0x40003
                );
                spr->m_logicRecord->m_dispatch(spr);
                CProjectile* s = static_cast<CProjectile*>(spr->m_logicRecord->m_userLogic);
                if (s->LoadProjectileSprites(
                        m_entranceReason,
                        m_playerIndex,
                        m_unitIndex,
                        m_attackTargetPx.m_x,
                        m_attackTargetPx.m_y,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->SetObjectFlags(0x10000);
                }
                break;
            }
            case GRUNT_TIMEBOMB: {
                i32 pos[2];
                EntranceTileOffset(pos);
                CGameObject* spr = g_gameReg->m_world->m_childGroup
                                       ->CreateSprite(0, pos[0], pos[1], 0xf, "TimeBomb", 0x40003);
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
                        m_object->m_screenX,
                        m_object->m_screenY,
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

        m_entranceActive = 1;
        i32 dt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), "AttackDowntime");
        if (m_gruntKind == GRUNT_ROIDZ) {
            dt = 0;
        }
        m_attackDowntimeLo = dt;
        m_attackDowntimeHi = 0;
        m_attackClockLo = static_cast<i32>(g_frameTime);
        m_attackClockHi = 0;
        m_lowStaminaCued = 0;
        m_stamina = 0;
        if (m_healthSprite != NULL) {
            CreateStaminaSprite();
        }
        m_combatActive = 0;
    }

    CAniAdvanceCursor* cur = &m_wwdObject->m_animCursor;
    if ((cur->m_finished == 0 || cur->m_frameTicksLeft != 0) && flag == 0) {
        return 0;
    }
    if (m_entranceReason == GRUNT_BOOMERANG) {
        LoadGruntTypeTable(PICKUP_NONE, 1, 0, 0);
    }
    CWwdGameObjectA* h = m_object;
    i32 zkey = h->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(h, zkey)
    i32 v220 = m_poweredUp;
    m_entranceActive = 0;
    if (v220 != 0) {
        ResetGeometry();
        return 0;
    }
    ResetEntranceAnimation(1, 0, 0);
    return 0;
}

// @early-stop
// The `sel` decision is a FLAT four-arm else-if chain over (d0!=0, d1!=0), not a
// nested ?: - retail's 0x625d4/0x625d8 lattice is cl threading the known-value
// edges of that chain, its blocks are jumped ONTO past their own first test, and
// the last arm's compare is UNSIGNED (`cmp edi,eax / sbb edi,edi / inc edi`).
// Branch counts now agree 47/47.
// Residue: the pose test. Retail compares the member in place (`cmp DWORD PTR
// [edx+0x1b4],ecx`) and then RE-LOADS m_animation inside the if body for m_value,
// where cl materialises the load once for the compare and reuses it.
RVA(0x00062110, 0x5bc)
i32 CGrunt::UpdateArrival(i32 walking, i32 commit) {
    if (commit != 0) {
        StopVehicleLoopSound();
        if (m_arrivalPhase == ARRIVAL_TAG_TRIGGER_B && m_arrivalActive != 0) {
            CGrunt* occ =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (occ != NULL) {
                CGameObject* inner = occ->m_object;
                i32 innerY = inner->m_screenY;
                i32 innerX = inner->m_screenX;
                i32 xMasked = (innerX & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 yMasked = (innerY & ~TILE_MASK_PX) + TILE_HALF_PX;
                if (RectContainsGated(xMasked, yMasked) != 0) {
                    m_triggerMgr->ApplyTriggerB(m_playerIndex, m_unitIndex, innerX, innerY);
                }
            }
        }

        if (m_poweredUp != 0 && m_neighborValid == 0) {
            RESET_GRUNT_POWERED_STATE(this)
        }
        m_entranceActive = 1;
        SetEntrancePos(1, 1);

        if (CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS(this)
        }

        m_entranceStamped = 0;
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)

        if (m_entranceReason == PICKUP_SCROLL) {
            SET_ANIMATION_ACT("P");
            i32 toyIdx = rand() % 2;
            SwitchGeometryDirect(m_poseToy[toyIdx], 0);

            DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)
            char* buf = (&m_frameSetName)->GetBuffer(0);
            ApplyLookupSprite(buf, frame);

            i32 cueTier = ((toyIdx != 0) ? 0xa : 0) + 0x406;
            i32 m380 = m_moveVariant;
            if (m380 != 0) {
                i32 tier = cueTier + m380 - 1;
                CGruntzMgr* g = g_gameReg;
                const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(bounds, m_object->m_screenX, m_object->m_screenY)
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
                const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(bounds, m_object->m_screenX, m_object->m_screenY)
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
        if (m_poweredUp != 0 && m_neighborValid == 0) {
            RESET_GRUNT_POWERED_STATE(this)
        }
        SET_ANIMATION_ACT("L");
        SwitchAnimation(m_poseWalk);
        GruntDirectionCell cell = m_entranceCell;
        i32 colv = cell.column + cell.row * 2;
        i32 basev = cell.row + colv;
        char* nm = m_cells[basev].WalkName().GetBuffer(0);
        ApplyName(nm);

        DWORD tt = g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), s_ToyTime);
        m_idleDelay = tt >> 1;
        m_idleAnchor = g_frameTime;
        return 0;
    }

    SET_ANIMATION_ACT("G");

    CWwdGameObjectA* h = m_object;
    i32 z = h->m_screenY + 0xc3500;
    SET_SORT_KEY_IF_CHANGED(h, z)

    i32 t0 = AT(m_poseToy, GRUNT_TOY1)->m_total;
    i32 t1 = AT(m_poseToy, GRUNT_TOY2)->m_total;
    // Retail 0x62568: (m_toyDuration - now) + m_toyClock - the toy-time REMAINING
    // until the deadline, not the elapsed time (which is always <= 0 here).
    i64 remaining = m_toyDuration - static_cast<i64>(g_frameTime) + m_toyClock;
    i32 cap = static_cast<i32>(remaining);
    if (remaining < 0) {
        cap = 0;
    }
    i32 d0 = 0;
    if (static_cast<u32>(t0) > static_cast<u32>(cap)) {
        d0 = t0 - cap;
    }
    i32 d1 = 0;
    if (static_cast<u32>(t1) > static_cast<u32>(cap)) {
        d1 = t1 - cap;
    }
    i32 sel;
    if (d0 == 0 && d1 == 0) {
        i32 r = rand() % 0x64 + 1;
        sel = (r >= m_toyBlendPct) ? 1 : 0;
    } else if (d0 != 0 && d1 == 0) {
        sel = 0;
    } else if (d1 != 0 && d0 == 0) {
        sel = 1;
    } else {
        sel = (static_cast<u32>(d0) < static_cast<u32>(d1)) ? 0 : 1;
    }

    CAniElement* want = m_poseToy[sel];
    if (m_wwdObject->m_animCursor.m_animation != want) {
        SwitchAnimation(want);
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, el)
        char* buf = (&m_frameSetName)->GetBuffer(0);
        ApplyLookupSprite(buf, frame);
    }

    CWwdGameObjectA* object = m_object;
    CGruntzMgr* g = g_gameReg;
    i32 yy = object->m_screenY;
    i32 xx = object->m_screenX;

    if (sel == 0) {
        if (CGameLevel::PointInRect(&g->m_world->m_level->m_mainPlane->m_viewRect, xx, yy)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xa, -1, -1, -1);
        }
    } else {
        if (CGameLevel::PointInRect(&g->m_world->m_level->m_mainPlane->m_viewRect, xx, yy)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xb, -1, -1, -1);
        }
    }
    return 0;
}

RVA(0x00062840, 0x25d)
i32 CGrunt::StepEntranceRelatchA() {
    i32 ready = m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (IsAniCursorComplete(sub)) {
        if (m_arrived != 0) {
            CreateHealthSprite();
            CreateStaminaSprite();
            CreateToySprite();
        }
        SET_ANIMATION_ACT("A");
        LoadGruntTypeTable(m_toolId, 1, 0, 0);
        m_entranceActive = 0;
        CGruntzMgr* g = g_gameReg;
        CMapMgr* grid = g->m_tileGrid;
        i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 flags = grid->CellFlagsAt(tx, ty);
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
            return 0;
        }
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(h, v)
        return 0;
    }

    i64 diff = static_cast<i64>(g_frameTime) - m_toyClock;
    if (diff >= m_toyDuration && m_entranceStamped == 0 && ready == 1) {
        HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
        SwitchAnimation(AT(m_poseToy, GRUNT_TOY_BREAK));
        DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
        char* nm = (&m_frameSetName)->GetBuffer(0);
        ApplyLookupSprite(nm, frame);
        m_entranceStamped = 1;
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 y = h->m_screenY;
        i32 x = h->m_screenX;
        const RECT& r = g->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInRect(&r, x, y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xc, -1, -1, -1);
        }
        return 0;
    }
    StopVehicleLoopSound();
    if (ready == 1) {
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
// Regalloc only: retail spills the RECT* param and reloads it for the x-edge
// blocks (frame 0x10), cl keeps it in edx and recycles the arg slot (frame 0xc).
RVA(0x00062b70, 0x205)
i32 CGrunt::RectSegProbe(RECT* p, POINT* e1, POINT* e2) {
    i32 e1y = e1->y;
    i32 e2y = e2->y;

    i32 py = p->top;
    if ((e1y < py) != (e2y < py)) {
        float t = static_cast<float>((py - e1y)) / static_cast<float>((e2y - e1y));
        float ix = static_cast<float>(e1->x) + t * static_cast<float>((e2->x - e1->x));

        if (ix >= static_cast<float>(p->left) && ix <= static_cast<float>(p->right)) {
            return 1;
        }
    }

    i32 pyc = p->bottom;
    if ((e1y < pyc) != (e2y < pyc)) {
        float t = static_cast<float>((pyc - e1y)) / static_cast<float>((e2y - e1y));
        float ix = static_cast<float>(e1->x) + t * static_cast<float>((e2->x - e1->x));

        if (ix >= static_cast<float>(p->left) && ix <= static_cast<float>(p->right)) {
            return 1;
        }
    }

    i32 e1x = e1->x;
    i32 e2x = e2->x;
    i32 px = p->left;
    if ((e1x > px) != (e2x > px)) {

        float t = static_cast<float>((px - e1x)) / static_cast<float>((e2x - e1x));
        float iy = static_cast<float>(e1y) + t * static_cast<float>((e2y - e1y));

        if (iy < static_cast<float>(p->bottom) && iy > static_cast<float>(p->top)) {
            return 1;
        }
    }

    i32 pxr = p->right;
    if ((e1x > pxr) != (e2x > pxr)) {

        float t = static_cast<float>((pxr - e1x)) / static_cast<float>((e2x - e1x));
        float iy = static_cast<float>(e1y) + t * static_cast<float>((e2y - e1y));

        if (iy < static_cast<float>(p->bottom) && iy > static_cast<float>(p->top)) {
            return 1;
        }
    }

    return 0;
}

// @early-stop
// g_gameReg is loaded once per PointInBounds BLOCK (the bounds test and the
// PlayGruntVoiceCue call in each block share ONE load; retail 0x6300d, 0x63041)
// - `reloc_multiset` counts four loads in retail and six with a per-use spelling.
// Two observed residues, both small and both regalloc-shaped:
//  1. GetRandom(1, count)'s degenerate arm.  Retail selects with a branch between
//     the literal lo and the register holding hi (0x62f72 `test al,1 / je / mov
//     edi,1`); our cl additionally proves `count == 0` from `n == 0`, folds hi to
//     the literal 0, and collapses `(rand()&1) ? 1 : 0` to `and edi,ebx` - two
//     instructions shorter, so no source spelling of the shared <GameRand.h>
//     inline reaches it.
//  2. retail parks g_gameReg in EBP inside each PointInBounds block (0x6300d,
//     0x63041) so the shared PlayVoice tail is 3 instructions; ours holds
//     0 in EBP for the whole function and reloads ds:0x64556c per site, making
//     that tail 8.
// Block skeleton is otherwise aligned 59/59.
RVA(0x00062e10, 0x4a0)
void CGrunt::ResetEntranceAnimation(i32 refreshFrame, i32 chooseIdleVariant, i32 playVoiceCue) {
    m_resetApplied = 0;

    bool notIdle = ANIMATION_ACT_DIFFERS("A");
    i32 applied = 0;

    if (notIdle && chooseIdleVariant == 0) {

        SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE1));
        m_idleWindow = static_cast<u32>(0x3a98);
        m_idleTimer = g_frameTime;
        i32 d = static_cast<i32>(g_buteMgr.GetDwordDef("Grunt", "IdleDelay", 0x7530));
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
                i32 focused = (m_playerIndex == g_curPlayer);
                if (focused && idx > 0x5a) {
                    CGruntzMgr* g = g_gameReg;
                    if (CGameLevel::PointInBounds(
                            &g->m_world->m_level->m_mainPlane->m_viewRect,
                            m_object->m_screenX,
                            m_object->m_screenY
                        )) {

                        g->m_voiceManager->PlayGruntVoiceCue(this, 4, -1, -1, -1);
                    }
                } else if (focused || m_entranceReason != PICKUP_NONE) {
                    switch (idx) {
                        case GRUNT_IDLE_VARIANT_PRIMARY: {
                            CGruntzMgr* g = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &g->m_world->m_level->m_mainPlane->m_viewRect,
                                    m_object->m_screenX,
                                    m_object->m_screenY
                                )) {

                                g->m_voiceManager->PlayGruntVoiceCue(this, 5, -1, -1, -1);
                            }
                            break;
                        }
                        case GRUNT_IDLE_VARIANT_SECONDARY: {
                            CGruntzMgr* g = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &g->m_world->m_level->m_mainPlane->m_viewRect,
                                    m_object->m_screenX,
                                    m_object->m_screenY
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
            m_resetApplied = 1;
            applied = 1;
        } else {

            if (m_wwdObject->m_animCursor.m_animation == AT(m_poseIdle, GRUNT_IDLE1)) {
                goto latch;
            }
            SwitchAnimation(AT(m_poseIdle, GRUNT_IDLE1));
            {
                i32 d = static_cast<i32>(g_buteMgr.GetDwordDef("Grunt", "IdleDelay", 0x7530));
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
    if (m_wwdObject->m_animCursor.m_animation != AT(m_poseIdle, GRUNT_IDLE1)) {
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
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        CString key = m_cells[base].IdleName();

        APPLY_CURRENT_ANIMATION_FRAME_SPRITE(key, desc, elem)
    }
}

// @early-stop
RVA(0x000633e0, 0x2f1)
i32 CGrunt::ResolveEntranceArrival() {
    if (m_entranceActive != 0 && GRUNT_AT_SAVED_SCREEN_POS(this)) {
        CGruntzMgr* g = g_gameReg;
        CMapMgr* grid = g->m_tileGrid;
        i32 tx = m_object->m_screenX >> TILE_SHIFT_PX;
        i32 ty = m_object->m_screenY >> TILE_SHIFT_PX;
        i32 flags = grid->CellFlagsAt(tx, ty);
        if (!(flags & 0x80)) {
            m_entranceActive = 0;
        }
    }

    i32 ready = m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));

    if (static_cast<i64>(g_frameTime) - m_idleTimer >= m_idleWindow) {
        CGruntzMgr* g = g_gameReg;
        GameModeId mode = g->m_gameMode;
        if (mode != GAMEMODE_QUESTZ) {
            GruntzPlayer* slot = &g->m_players[m_playerIndex];
            if (slot != NULL && slot->m_humanControlled != 0) {
                if (m_tileClaimed == 0 && m_arrivalNotified == 0 && mode == GAMEMODE_MULTIPLAYER
                    && g_curPlayer == m_playerIndex && m_arrived == 0) {
                    m_triggerMgr->EnqueueGuardBegin(m_playerIndex, m_unitIndex);
                    m_arrivalNotified = 1;
                    goto tail;
                }
                if (mode != GAMEMODE_MULTIPLAYER && g_curPlayer == m_playerIndex && m_arrived == 0
                    && m_tileClaimed != 1) {
                    m_arrivalRerollLo = 0;
                    m_arrivalRerollWindowLo = 0;
                    m_arrivalRerollHi = 0;
                    m_arrivalRerollWindowHi = 0;
                    m_defenderPx = m_lastTilePx;
                    m_tileClaimed = 1;
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
                                g_buteMgr.GetIntDef("Grunt", "PlayerDefenderRadius", 3) + 1;
                            break;
                    }
                    m_arrivalCell.m_x = -1;
                    m_arrivalCell.m_y = -1;
                    m_arrivalState = AI_DEFENDER;
                    m_defenderState = AISTATE_SEEK;
                    m_arrivalActive = 0;
                    m_arrivalFlags |= 0x18040402;
                    m_object->m_extent.left = 0;
                    m_object->m_extent.right = 0;
                    m_object->m_extent.top = 0;
                    m_object->m_extent.bottom = 0;
                    SetEntrancePos(0, 0);
                }
            }
        }
    }

tail:
    if (m_wwdObject->m_animCursor.m_animation != AT(m_poseIdle, GRUNT_IDLE1)) {

        if (IsAniCursorComplete(&m_wwdObject->m_animCursor)) {
            ResetEntranceAnimation(0, 0, 0);
        }
        return 0;
    }
    if (static_cast<i64>(g_frameTime) - m_idleAnchor >= m_idleDelay && ready == 1) {
        ResetEntranceAnimation(0, 1, 1);
    }
    return 0;
}

// @early-stop
// The two flag arms and their join (B34/B40/B41) now carry retail's shape: the
// tail is duplicated into both arms rather than sunk, because each arm's LAST
// statement differs (see docs/patterns/trailing-statement-blocks-arm-tail-sink.md).
// Residue is +-1 instruction in B29/B31/B34/B35/B40 - CSE around the coord head
// read, not control flow.
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
    m_neighborScanEnabled = 0;

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
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    m_tileMoveCommitted = 0;
    if (CoordCount() == 0) {
        return 0;
    }

    Coord* targetCoord = static_cast<Coord*>(m_coordList.GetHead());
    CMapMgr* tileGrid = g_gameReg->m_tileGrid;
    i32 targetCellFlags = tileGrid->CellFlagsAt(targetCoord->m_x, targetCoord->m_y);
    GruntDirectionCell cell;
    if (!(targetCellFlags & 0x20000000)) {
        SET_ANIMATION_ACT("D");
        SwitchAnimation(m_poseWalk);
        cell = m_entranceCell;
    } else {

        i32 currentTileX = m_object->m_screenX >> TILE_SHIFT_PX;
        i32 currentTileY = m_object->m_screenY >> TILE_SHIFT_PX;
        i32 currentCellFlags = tileGrid->CellFlagsAt(currentTileX, currentTileY);
        if (!(currentCellFlags & 0x80)) {
            return 0;
        }
        SET_ANIMATION_ACT("D");
        SwitchAnimation(m_poseWalk);
        cell = m_entranceCell;
        m_entranceActive = 1;
    }
    i32 col = cell.column + cell.row * 2;
    i32 base = cell.row + col;

    char* walkAnimationName = m_cells[base].WalkName().GetBuffer(0);
    ApplyName(walkAnimationName);
    return 0;
}

// @early-stop
// Register-coloring wall: retail colours `this` ebx and the timeGetTime import
// pointer ebp; cl swaps the pair. Size, relocs and instruction selection match.
RVA(0x00063b60, 0x1cf)
i32 CGrunt::StepArrivalReroll() {
    m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
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
    if (elapsed % 1000 != 0) {
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
    CWwdGameObjectA* h = m_object;
    i32 y = h->m_screenY;
    i32 xp = h->m_screenX;
    if (pick > 0x19) {
        if (CGameLevel::PointInRect(&g_gameReg->m_world->m_level->m_mainPlane->m_viewRect, xp, y)) {
            g_gameReg->m_voiceManager->PlayVoice(this, 0x15d, -1, 0, -1, -1);
        }
    } else {
        if (CGameLevel::PointInRect(&g_gameReg->m_world->m_level->m_mainPlane->m_viewRect, xp, y)) {
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
        m_entranceActive = 0;

        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
        i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
        i32 flags = grid->CellFlagsAt(tx, ty);
        if (flags & 0x80) {
            SetEntrancePos(1, 1);
            m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        }
        return 0;
    }

    i64 elapsed = static_cast<i64>(g_frameTime) - m_toyClock;
    if (elapsed >= m_toyDuration) {
        if (m_entranceStamped == 0 && GRUNT_AT_SAVED_SCREEN_POS(this)) {
            HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
            SetEntrancePos(1, 1);
            m_entranceStamped = 1;
            SwitchGeometryDirect(AT(m_poseToy, GRUNT_TOY_BREAK), 0);

            DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)
            char* buf = (&m_frameSetName)->GetBuffer(0);
            ApplyLookupSprite(buf, frame);

            CWwdGameObjectA* h = m_object;
            CGruntzMgr* g = g_gameReg;
            i32 y = h->m_screenY;
            i32 x = h->m_screenX;
            const RECT& rect = g->m_world->m_level->m_mainPlane->m_viewRect;
            if (CGameLevel::PointInRect(&rect, x, y)) {
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
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 y = h->m_screenY;
        i32 x = h->m_screenX;
        const RECT& rect = g->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInRect(&rect, x, y)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 0xd, -1, -1, -1);
        }
    }

    CWwdGameObjectA* h2 = m_object;
    CGruntzMgr* g2 = g_gameReg;
    i32 hy = h2->m_screenY;
    i32 hx = h2->m_screenX;
    if (CGameLevel::PointInRect(&g2->m_viewBounds, hx, hy)) {
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

// Retail keeps all three bounds tests below as out-of-line calls (0x641b0 has three
// `call` sites and no expansion), so they take CGameLevel::PointInBounds rather than
// the inline PointInRect sibling.
RVA(0x000641b0, 0x2c1)
i32 CGrunt::BuildGruntExitAnimation() {
    if (m_deathAnimStarted != 0) {
        return 0;
    }

    FinishActiveAction();
    STOP_GRUNT_LOOP_SOUNDS;

    m_object->m_stateFlags &= ~SPRITE_STATE_FLASHING;
    m_entranceCommitted = 0;
    m_deathAnimStarted = 1;

    HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_powerupSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_selectedSprite)

    m_gruntKind = GRUNT_NORMAL;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
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
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_voiceManager->PlayVoice(this, 0x384, -1, 0, -1, -1);
        }
    } else if (r > 0xa0) {
        found = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_GRUNTZ_EXITZ_TWO);
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_voiceManager->PlayVoice(this, 0x385, -1, 0, -1, -1);
        }
    } else {
        found = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_GRUNTZ_EXITZ_THREE);
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_voiceManager->PlayVoice(this, 0x386, -1, 0, -1, -1);
        }
    }

    CWapX::ApplyAnimation(found, 0);
    i32 frame =
        static_cast<CAniRecordView*>(m_wwdObject->m_animCursor.m_animation->AtChecked(0))->m_param;
    ApplyLookupSprite("GRUNTZ_EXITZ", frame);
    return 0;
}

// @early-stop
// instruction stream is identical; retail holds `this` in edi and the sunk save
// in esi, cl picks them the other way round - callee-saved allocation order only.
RVA(0x00064540, 0x11c)
i32 CGrunt::StepWarpExit() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, g_engineFrameDelta)
    if (IsAniCursorComplete(sub)) {
        if (m_deathType == GRUNT_DEATH_WARPOUT) {
            CState* st = g_gameReg->m_curState;
            i32 lvl = st->m_levelIndex + 0x64;
            CString s;
            s.Format("WORLDZ\\LEVEL%i", lvl);
            if (st->m_levelResources->FindEntryByPath(static_cast<LPCTSTR>(s), REZ_TAG_WWD)) {
                PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_WORLD), lvl);
            }
        }
        if (m_cellRemovalNotified == 0) {
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 1);
        }
        SetObjectFlags(0x10000);
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
    if (m_entranceCommitted == 0 || m_entranceDropActive != 0) {
        return 0;
    }
    {
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
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
                    m_entranceActive = 0;
                    eq = (strcmp(*g_typeColl.GetNameRecord(m_previousAnimationActId), "D") == 0);
                    if (eq) {
                        if (m_poweredUp != 0 && m_neighborValid == 0) {
                            RESET_GRUNT_POWERED_STATE(this)
                        }
                        m_tileMoveCommitted = 0;
                        SET_ANIMATION_ACT("D");
                        SwitchAnimation(m_poseWalk);
                        GruntDirectionCell cell = m_entranceCell;
                        i32 col = cell.column + cell.row * 2;
                        i32 base = cell.row + col;
                        char* cn = m_cells[base].WalkName().GetBuffer(0);
                        ApplyName(cn);
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
                    CWwdGameObjectA* h = m_object;
                    DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(h, hx, hy)
                    i32 flag = 1;
                    if (PIXEL_PAIR_NOT_AT_POSITION(hx, hy, m_lastTilePx.m_x, m_lastTilePx.m_y)) {
                        if (IsDropReady(1)) {
                            m_coordToggle = (m_coordToggle == 0) ? 1 : 0;
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
        CWwdGameObjectA* h = m_object;
        i32 v = h->m_screenY + 0x186a0;
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
            if (m_entranceCommitted != 0) {
                return 0;
            }
        }
    }
    m_entranceActive = 1;
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
                i32 cx = oh->m_screenX;
                i32 cy = oh->m_screenY;
                if (m_neighborScanEnabled != 0 && m_entranceCommitted != 0
                    && RectContains(cx, cy)) {
                    if (!(g_gameReg->m_tileGrid->CellFlagsAt(
                              m_lastTilePx.m_x >> TILE_SHIFT_PX,
                              m_lastTilePx.m_y >> TILE_SHIFT_PX
                          )
                          & 0x80)) {
                        CommitNeighbor(srcPlayerIndex, srcUnitIndex, cx, cy);
                    }
                }
            }
        }
    }

    m_combatActive = 0;
    CAniElement* pose = m_poseStruck[struckPose];
    SwitchAnimation(pose);
    i32 frame;
    {
        CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
        CAniRecordView* elem = static_cast<CAniRecordView*>(GetAniElementAt(desc, 0));
        frame = elem->m_param;
    }
    {
        GruntDirectionCell cell = m_entranceCell;
        i32 col = cell.column + cell.row * 2;
        i32 base = cell.row + col;
        char* cn = m_cells[base].StruckName().GetBuffer(0);
        ApplyLookupSprite(cn, frame);
    }
    {
        CWwdGameObjectA* h = m_object;
        i32 vx = h->m_screenX;
        i32 vy = h->m_screenY;
        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInRect(rect, vx, vy)) {
            g_gameReg->m_voiceManager->PlayGruntVoiceCue(this, 7, -1, -1, -1);
        }
    }
    return 0;
}

RVA(0x00065300, 0x148)
i32 CGrunt::StepArrivalCommitA() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, static_cast<u32>(g_engineFrameDelta))
    if (!IsAniCursorComplete(sub)) {
        return 0;
    }
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, m_killerPlayerIndex);
        return 0;
    }
    m_entranceActive = 0;

    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 flags = grid->CellFlagsAt(tx, ty);
    if (flags & 0x80) {
        SetEntrancePos(1, 1);
        m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
        return 0;
    }
    if (m_neighborScanEnabled == 0 && m_tileMoveCommitted != 0) {
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
i32 CGrunt::StepArrivalCommitB() {

    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, static_cast<u32>(g_engineFrameDelta))
    if (!IsAniCursorComplete(sub)) {
        return 0;
    }
    m_entranceActive = 0;
    SnapToLastTile(1);
    SetEntrancePos(1, 1);

    m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_x, m_lastTilePx.m_y);
    if (m_health <= 0) {
        m_entranceCommitted = 0;
        m_triggerMgr->StartUnitDeath(m_playerIndex, m_unitIndex, DEATH_NORMAL, m_killerPlayerIndex);
        return 0;
    }
    CGruntzMgr* g = g_gameReg;
    CMapMgr* grid = g->m_tileGrid;
    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 flags = grid->CellFlagsAt(tx, ty);
    if (flags & 0x80) {
        return 0;
    }
    if (m_neighborScanEnabled == 0 && m_tileMoveCommitted != 0) {
        StepArrivalDrop(m_commitPx.m_x, m_commitPx.m_y, 0, -1, 1, 0);
        return 0;
    }
    ResetGeometry();
    return 0;
}

// @early-stop
// Two observed residues.  (1) Retail parks the constant 1 in edi across the whole
// tail - both arms of the powered-up reset write `mov edi,1` (0x6572a, 0x65752)
// and the PICKUP_BOMB compare, m_entranceActive and m_bombRunActive all read it;
// cl spells each as an immediate, so its version of that block is 9 instructions
// against retail's 11 and there is no separate merge block.  (2) Retail moves the
// `rand() % 100 < 80` ELSE arm to the end of the function (0x6596f) and keeps the
// THEN arm as the fall-through; cl lays them adjacent.
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
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        const LevelCoordRect* bounds = &g->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInBounds(bounds, h->m_screenX, h->m_screenY)) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 8, -1, -1, -1);
        }
    }

    FaceTowardTile(tileX, tileY);
    m_moveTile.m_x = tileX;
    m_moveTile.m_y = tileY;
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        RESET_GRUNT_POWERED_STATE(this)
    }

    i32 poseIdx = 0;
    if (m_entranceReason == PICKUP_BOMB) {
        SET_ANIMATION_ACT("M");
        m_object->m_stateFlags &= ~SPRITE_STATE_FLASHING;
        m_timePerTile = g_buteMgr.GetDwordDef("BOMBGRUNT", "RunningTimePerTile", 0x64);
        m_entranceActive = 1;
        m_bombRunActive = 1;
        SetEntrancePos(1, 1);
    } else if (m_entranceReason == PICKUP_TOOB) {
        m_entranceActive = 1;
        SET_ANIMATION_ACT("N");
        m_coordToggle = (m_coordToggle == 0);
    } else if (m_entranceReason == PICKUP_WAND) {
        i32 base;
        if (rand() % 0x64 < 0x50) {
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
        CWwdGameObjectA* h = m_object;
        CGruntzMgr* g = g_gameReg;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        const RECT& rect = g->m_world->m_level->m_mainPlane->m_viewRect;
        if (CGameLevel::PointInRect(&rect, x, y)) {
            g->m_voiceManager->PlayVoice(this, cueId, -1, 0, -1, -1);
        }

        SET_ANIMATION_ACT("I");
        m_entranceActive = 1;
        SetEntrancePos(1, 1);
    } else {
        SET_ANIMATION_ACT("I");
        SetEntrancePos(1, 1);
    }

    SwitchAnimation(m_poseItem[poseIdx]);

    GruntDirectionCell cell = m_entranceCell;
    i32 col = cell.column + cell.row * 2;
    i32 base = cell.row + col;
    char* name = m_cells[base].ItemName().GetBuffer(0);
    ApplyName(name);
    return 0;
}

// @early-stop
RVA(0x00065a60, 0x159)
i32 CGrunt::LoadWandGruntItemConfig() {
    i32 advanced = m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (advanced > 0) {
        WwdAniDrawValue cue = static_cast<WwdAniDrawValue>(advanced);
        if (cue == WWDDRAW_TOOL_APPLIES) {
            m_entranceActive = 1;
            u32 downtime =
                g_buteMgr.GetDword(static_cast<const char*>(m_animSetName), "ItemDowntime");
            if (m_gruntKind == GRUNT_ROIDZ) {
                downtime = 0;
            }
            m_attackDowntimeLo = downtime;
            m_attackDowntimeHi = 0;
            m_attackClockLo = g_frameTime;
            m_attackClockHi = 0;
            m_lowStaminaCued = 0;
            m_stamina = 0;
            if (m_healthSprite != NULL) {
                CreateStaminaSprite();
            }
            if (m_entranceReason == PICKUP_WAND) {
                LoadGruntAbilityTuning(m_moveVariant);
                i32 hp = m_health - g_buteMgr.GetIntDef("WANDGRUNT", "HealthLoss", 0x19);
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
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (IsAniCursorComplete(sub)) {
        m_entranceActive = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    return 0;
}

// @early-stop
// One branch differs (#10, the m_registeredGameObjectsById lookup at base +0x160). Retail leaves the
// failure value in the BOOL return - `test eax,eax / je 0x65d86 / mov eax,[esp+0xc]`
// - so `found` never touches memory on that path; passing `found` itself as the
// out-reference forces the extra `mov [esp+0xc],eax` store cl emits.  Splitting it
// into a separate out variable plus a register `found` is what retail's shape
// implies but scores WORSE here (98.22 -> 95.86), so the spelling is not the lever.
RVA(0x00065c20, 0x1d5)
i32 CGrunt::StepEntranceRelatchB() {
    i32 advanced = m_wwdObject->m_animCursor.Advance(static_cast<u32>(g_engineFrameDelta));
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
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (!IsAniCursorComplete(sub)) {
        return 0;
    }
    m_entranceActive = 0;
    if (m_arrived != 0) {
        CreateHealthSprite();
        CreateStaminaSprite();
        CreateToySprite();
    }
    SET_ANIMATION_ACT("D");
    SetupTubeAnim(m_coordToggle);
    CGruntzMgr* g = g_gameReg;
    CMapMgr* grid = g->m_tileGrid;
    i32 tx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 ty = m_lastTilePx.m_y >> TILE_SHIFT_PX;
    i32 f1 = grid->CellFlagsAt(tx, ty);
    if (f1 & 0x2000000) {
        BuildGruntLoseItemAnimation();
        g = g_gameReg;
    }
    grid = g->m_tileGrid;
    char* cellObj;
    if (static_cast<u32>(tx) >= static_cast<u32>(grid->m_width)
        || static_cast<u32>(ty) >= static_cast<u32>(grid->m_height)) {
        cellObj = NULL;
    } else {

        AddrWord<char> slot;
        slot.m_word = ((grid->m_rowInts[ty]))[tx * 7 + 2];
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
        == 0) {
        found = NULL;
    }
    if (found == NULL) {
        grid = g_gameReg->m_tileGrid;
        if (static_cast<u32>(tx) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(ty) < static_cast<u32>(grid->m_height)) {
            ((grid->m_rowInts[ty]))[tx * 7 + 2] = 0;
            i32 flags = ((grid->m_rowInts[ty]))[tx * 7];
            flags &= ~0x40000;
            ((grid->m_rowInts[ty]))[tx * 7] = flags;
        }
        return 0;
    }
    CInGameIcon* icon = static_cast<CInGameIcon*>(found->m_logicRecord->m_userLogic);
    icon->PlaceAt(m_playerIndex, m_unitIndex);
    return 0;
}

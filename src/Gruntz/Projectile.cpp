#include <rva.h>

#include <Gruntz/Projectile.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Globals.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Boomerang.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialRefLookup.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/State.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <MakeRect.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Utils/Square.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/MoveMode.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

DATA(0x001eaad8)
const double g_boomerangMidpointScale = 0.5;
DATA(0x001eaae0)
const double g_boomerangPixelToTileScale = 0.03125;
DATA(0x001eaae8)
const double g_boomerangHalfTurnRadians = 3.1415927;
DATA(0x001eaaf0)
const double g_boomerangHoldScale = 0.0625;
DATA(0x001eaaf8)
const double g_boomerangHoldBiasMs = -500.0;
DATA(0x001eab00)
const double g_boomerangFullTurnRadians = 6.2831854;
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;
DATA(0x001f04e8)
const u32 g_defaultZ = 24;

RVA_DYNINIT(0x000df900, 0xa, CActRegPool<CProjectile>::s_table)
RVA_DYNINIT(0x000df920, 0x15, CActRegPool<CProjectile>::s_table)
RVA_DYNINIT(0x000df950, 0xe, CActRegPool<CProjectile>::s_table)
RVA_DYNINIT(0x000df970, 0x1f, CActRegPool<CProjectile>::s_table)
template<> DATA(0x0024c758)
CActReg CActRegPool<CProjectile>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000e1790, 0xa, CActRegPool<CTimeBomb>::s_table)
RVA_DYNINIT(0x000e17b0, 0x15, CActRegPool<CTimeBomb>::s_table)
RVA_DYNINIT(0x000e17e0, 0xe, CActRegPool<CTimeBomb>::s_table)
RVA_DYNINIT(0x000e1800, 0x1f, CActRegPool<CTimeBomb>::s_table)
template<> DATA(0x0024c780)
CActReg CActRegPool<CTimeBomb>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @interleaver ??_G/??1 COMDATs - retail's kept copies sit in serialobjectfactory.obj's
// contribution (reviewed ownership in config/retail/link_order.tsv).
RVA_COMPGEN(0x00012980, 0x1e, ??_GCProjectile@@UAEPAXI@Z)
RVA_COMPGEN(0x000129d0, 0x1e, ??_GCBoomerang@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a00, 0x5, ??1CBoomerang@@UAE@XZ)
RVA_COMPGEN(0x00012a40, 0x1e, ??_GCTimeBomb@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a70, 0x44, ??1CTimeBomb@@UAE@XZ)

RVA_COMPGEN(0x00058ba0, 0x1, ??1CMotionState@@QAE@XZ)

// @early-stop
RVA(0x000dec60, 0x255)
CProjectile::CProjectile(CGameObject* owner) : CMovingLogic(owner), CWapX(owner) {
    SET_OBJECT_FLAGS_AND_HIDE_INLINE(0x2000002)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR);
    memset(&m_frames[0], 0, 0x1c);
    m_sound = NULL;
    m_shadow = NULL;
}

RVA(0x000def60, 0xbc)
CProjectile::~CProjectile() {
    if (m_sound != NULL) {
        m_sound->StopAndRewind();
        m_sound = NULL;
    }
    for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {
        Coord* hitPoint = static_cast<Coord*>(m_hitList.GetNext(pos));
        if (hitPoint != NULL) {

            g_coordPool.Push(hitPoint);
        }
    }
    m_hitList.RemoveAll();
}

RVA(0x000df050, 0x6ed)
i32 CProjectile::LoadProjectileSprites(
    PickupType kind,
    i32 sourcePlayerIndex,
    i32 sourceUnitIndex,
    i32 targetPxX,
    i32 targetPxY,
    i32 sourcePxX,
    i32 sourcePxY
) {
    CString key;
    m_sourcePlayerIndex = sourcePlayerIndex;
    m_sourceUnitIndex = sourceUnitIndex;
    m_targetPx.m_x = (targetPxX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_targetPx.m_y = (targetPxY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_kind = kind;
    m_sourcePx.m_x = sourcePxX;
    m_sourcePx.m_y = sourcePxY;

    double dx = static_cast<double>(m_targetPx.m_x) - m_object->m_screenPosition.m_x;
    double dy = static_cast<double>(m_targetPx.m_y) - m_object->m_screenPosition.m_y;
    i32 count = 1;

    switch (kind) {
        case PICKUP_ROCK:
            key = "GRUNTZ_ROCKGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDword("Projectile", "RockProjectileTimePerTile", 0xbb8);
            m_isArcing = true;
            break;
        case PICKUP_GUNHAT:
            key = "GRUNTZ_GUNHATGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDword("Projectile", "GunhatProjectileTimePerTile", 0xbb8);
            m_isArcing = true;
            break;
        case PICKUP_BOOMERANG:
            key = "GRUNTZ_BOOMERANGGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDword("Projectile", "BoomerangProjectileTimePerTile", 0xbb8);
            m_isArcing = false;
            break;
        case PICKUP_NERFGUN:
            key = "GRUNTZ_NERFGUNGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDword("Projectile", "NerfGunProjectileTimePerTile", 0xbb8);
            m_isArcing = true;
            break;
        case PICKUP_WELDER:
            key = "GRUNTZ_WELDERGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDword("Projectile", "WelderProjectileTimePerTile", 0xbb8);
            m_isArcing = true;
            break;
        case PICKUP_WINGZ: {
            key = "GRUNTZ_WINGZGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDword("Projectile", "WingzProjectileTimePerTile", 0xbb8);
            LaunchSound("GRUNTZ_WINGZGRUNT_WINGZGRUNTLOOP");
            m_isArcing = false;
            i32 ddx =
                abs((m_targetPx.m_x >> TILE_SHIFT_PX)
                    - (m_object->m_screenPosition.m_x >> TILE_SHIFT_PX));
            i32 ddy =
                abs((m_targetPx.m_y >> TILE_SHIFT_PX)
                    - (m_object->m_screenPosition.m_y >> TILE_SHIFT_PX));
            count = Max(ddx, ddy);
            break;
        }
        default:
            return 0;
    }

    m_frames[0] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        key + DATA_COMPGEN(0x00213658, "1")
        );
    if (m_frames[0] == NULL) {
        return 0;
    }
    m_frames[1] =
        MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "2");
    m_frames[2] =
        MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "3");
    m_frames[3] =
        MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "4");
    m_frames[4] =
        MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "5");
    m_frames[PF_IMPACT] =
        MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "IMPACT");
    m_frames[PF_FALL] =
        MapFind<CAniElement>(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "FALL");

    SwitchAnimation(m_frames[0]);
    SetImageSetByName(key + "_OBJECT");

    u32 totalTime = static_cast<u32>((count * m_timePerTile));
    double len = sqrt(Sqr(dx) + Sqr(dy));
    double t = static_cast<double>(totalTime);
    double vx = dx / len;
    m_flightDist = len;
    m_velScale = len / t;
    m_position.m_x = m_object->m_screenPosition.m_x;
    m_position.m_y = m_object->m_screenPosition.m_y;
    m_velocity.m_x = vx;
    dy /= len;
    m_velocity.m_y = dy;

    VECTOR_COMPONENT_ROUND_BIAS(m_roundBias.m_x, vx);
    VECTOR_COMPONENT_ROUND_BIAS(m_roundBias.m_y, dy);
    m_flightDist = fabs(len);
    m_currentPx.m_x = m_object->m_screenPosition.m_x;
    m_currentPx.m_y = m_object->m_screenPosition.m_y;
    m_arrived = false;

    CDDrawChildGroup* factory = g_gameReg->m_world->m_childGroup;
    m_shadow = (factory->CreateSprite(
        0,
        m_object->m_screenPosition.m_x,
        m_object->m_screenPosition.m_y,
        SORTKEY_ACTOR_BEHIND,
        "LightFx",
        WWD_GAME_OBJECT_FLAGS_CULL_SOUND_WORLD_SPRITE
    ));
    if (m_shadow != NULL) {
        m_shadow->m_logicRecord->m_dispatch(m_shadow);
        (static_cast<CLightFx*>(m_shadow->m_logicRecord->m_userLogic))
            ->Activate(
                static_cast<const char*>(key + "_SHADOW"),
                static_cast<const char*>(key + "1"),
                5,
                true
            );
    }

    SET_ANIMATION_ACT("A");
    return 1;
}

RVA(0x000df9a0, 0x102)
void CProjectile::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x000dfb00, 0x18d)
void CProjectile::RegisterType() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CProjectile>::s_table[id] =
        static_cast<CActHandler>(&CProjectile::AdvanceAnimationAndDeleteWhenComplete);
}

RVA(0x000dfd00, 0x70c)
void CProjectile::AdvanceMotion() {
    if (m_arrived != false) {
        return;
    }

    if (m_kind == PICKUP_WINGZ) {
        CWwdSpriteObject* owner = m_object;
        CGruntzMgr* reg = g_gameReg;
        if (::PtInRect(
                &reg->m_viewBounds,
                owner->m_screenPosition.m_x,
                owner->m_screenPosition.m_y
            )) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_sound != NULL) {
            m_sound->StopAndRewind();
            m_sound = NULL;
        }
    }

    if (m_currentPx.m_x != m_targetPx.m_x || m_currentPx.m_y != m_targetPx.m_y) {

        if (m_kind == PICKUP_WINGZ) {
            ScanTargets(0);
        }
        m_position.m_x =
            m_position.m_x + static_cast<double>(g_frameDelta) * m_velocity.m_x * m_velScale;
        m_position.m_y =
            m_position.m_y + static_cast<double>(g_frameDelta) * m_velocity.m_y * m_velScale;
        i32 xRes = static_cast<i32>((m_roundBias.m_x + m_position.m_x));
        i32 localX = xRes;
        i32 yRes = static_cast<i32>((m_roundBias.m_y + m_position.m_y));
        if (m_velocity.m_x > 0.0) {
            xRes = Min(xRes, m_targetPx.m_x);
            localX = xRes;
        } else if (m_velocity.m_x < 0.0) {
            xRes = Max(xRes, m_targetPx.m_x);
            localX = xRes;
        }
        if (m_velocity.m_y > 0.0) {
            yRes = Min(yRes, m_targetPx.m_y);
        } else if (m_velocity.m_y < 0.0) {
            yRes = Max(yRes, m_targetPx.m_y);
        }
        m_currentPx.m_x = xRes;
        m_currentPx.m_y = yRes;
        i32 offX = 0;
        i32 offY = 0;
        if (m_isArcing != false) {
            double dx = fabs(static_cast<double>(m_targetPx.m_x) - m_position.m_x);
            double dy = fabs(static_cast<double>(m_targetPx.m_y) - m_position.m_y);
            double dist = sqrt(Sqr(dx) + Sqr(dy));
            if (dist >= m_flightDist * 0.9 || dist < m_flightDist * 0.1) {
                offX = 0x4;
                offY = -0x4;
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[0]) {
                    SwitchAnimation(m_frames[0]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[0]);
                    }
                }
            } else if (dist >= m_flightDist * 0.8 || dist < m_flightDist * 0.2) {
                offX = 0x8;
                offY = -0x8;
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[1]) {
                    SwitchAnimation(m_frames[1]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[1]);
                    }
                }
            } else if (dist >= m_flightDist * 0.7 || dist < m_flightDist * 0.3) {
                offX = 0xc;
                offY = -0xc;
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[2]) {
                    SwitchAnimation(m_frames[2]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[2]);
                    }
                }
            } else if (dist >= m_flightDist * 0.6 || dist < m_flightDist * 0.4) {
                offX = 0x10;
                offY = -0x10;
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[3]) {
                    SwitchAnimation(m_frames[3]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[3]);
                    }
                }
            } else {
                offX = 0x14;
                offY = -0x14;
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[4]) {
                    SwitchAnimation(m_frames[4]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[4]);
                    }
                }
            }
        }
        m_object->m_screenPosition.m_x = offX + m_currentPx.m_x;
        m_object->m_screenPosition.m_y = offY + m_currentPx.m_y;
        if (m_shadow != NULL) {
            m_shadow->m_screenPosition.m_x = localX;
            m_shadow->m_screenPosition.m_y = yRes;
        }
        return;
    }

    if (m_sound != NULL) {
        m_sound->StopAndRewind();
        m_sound = NULL;
    }
    ScanTargets(0);
    if (m_shadow != NULL) {
        m_shadow->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        m_shadow = NULL;
    }
    m_arrived = true;
    i32 tier = 0;
    if (m_kind != PICKUP_WINGZ) {
        CGruntzMgr* reg = g_gameReg;
        CMapMgr* plane = reg->m_tileGrid;
        i32 tileY = m_targetPx.m_y >> TILE_SHIFT_PX;
        i32 tileX = m_targetPx.m_x >> TILE_SHIFT_PX;
        u32 flags = plane->CellFlagsAt(tileX, tileY);
        if ((flags & 0x900) == 0) {
            if (flags & IDX(CELL_FLAG_SPECIAL)) {
                if (flags & IDX(CELL_FLAG_REVEALED_POWERUP)) {
                    tier = 1;
                } else {
                    switch (reg->m_curState->m_levelType) {
                        case AREA_HIGH_ON_SWEETZ:
                        case AREA_HIGH_ROLLERZ:
                        case AREA_GRUNTZ_IN_SPACE:
                            tier = 1;
                            break;
                        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
                            break;
                        default:

                            if (::PtInRect(&reg->m_viewBounds, m_targetPx.m_x, m_targetPx.m_y)) {
                                CWwdSpriteObject* fx = reg->m_world->m_childGroup->CreateSprite(
                                    0,
                                    m_targetPx.m_x,
                                    m_targetPx.m_y,
                                    SORTKEY_ACTOR_BEHIND,
                                    "Particlez",
                                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                );
                                if (fx != NULL) {
                                    fx->SetImageSetByName("LEVEL_DEATHSPLASH");
                                    fx->SetAnimationByName("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                            return;
                    }
                }
            }
        } else {
            if (::PtInRect(&reg->m_viewBounds, m_targetPx.m_x, m_targetPx.m_y)) {
                CWwdSpriteObject* fx = reg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_targetPx.m_x,
                    m_targetPx.m_y,
                    SORTKEY_ACTOR_BEHIND,
                    "Particlez",
                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                );
                if (fx != NULL) {
                    fx->SetImageSetByName("GAME_WATER");
                    fx->SetAnimationByName("GAME_WATER", 0);
                }
            }
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            return;
        }
    }
    CAniElement* sprite;
    if (tier != 0) {
        sprite = m_frames[PF_FALL];
        if (sprite != NULL) {
            goto animate;
        }
    } else {
        sprite = m_frames[PF_IMPACT];
        if (sprite != NULL) {
            goto animate;
        }
    }
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return;

animate:
    SwitchAnimation(sprite);
}

RVA(0x000e05e0, 0x4e)
i32 CProjectile::AdvanceAnimationAndDeleteWhenComplete() {
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;

    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* sprite = m_wwdObject;
    if (sprite->m_animationCursor.IsComplete()) {
        sprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    }
    return 0;
}

RVA(0x000e0650, 0x2b)
CBoomerang::CBoomerang(CGameObject* owner) : CProjectile(owner) {

    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
}

RVA(0x000e0690, 0x1a9)
i32 CBoomerang::LoadProjectileSprites(
    PickupType kind,
    i32 sourcePlayerIndex,
    i32 sourceUnitIndex,
    i32 targetPxX,
    i32 targetPxY,
    i32 sourcePxX,
    i32 sourcePxY
) {
    if (CProjectile::LoadProjectileSprites(
            kind,
            sourcePlayerIndex,
            sourceUnitIndex,
            targetPxX,
            targetPxY,
            sourcePxX,
            sourcePxY
        )
        == 0) {
        return 0;
    }
    double duration = static_cast<double>(static_cast<u32>(m_timePerTile));
    double d =
        g_boomerangHalfTurnRadians / (duration * (g_boomerangPixelToTileScale * m_flightDist));
    CWwdSpriteObject* owner = m_object;
    m_launchPosition.m_x = owner->m_screenPosition.m_x;
    m_launchPosition.m_y = owner->m_screenPosition.m_y;
    double originY =
        (static_cast<double>(m_targetPx.m_y) + static_cast<double>(owner->m_screenPosition.m_y))
        * g_boomerangMidpointScale;
    m_origin.m_x =
        (static_cast<double>(m_targetPx.m_x) + static_cast<double>(owner->m_screenPosition.m_x))
        * g_boomerangMidpointScale;
    m_origin.m_y = originY;
    m_direction.m_x = m_origin.m_x - static_cast<double>(m_launchPosition.m_x);
    m_direction.m_y = originY - static_cast<double>(m_launchPosition.m_y);
    m_phase = 0.0;
    m_velScale = d;
    CGrunt* g =
        g_gameReg->m_triggerMgr->m_units[TM_UNITS_PER_PLAYER * sourcePlayerIndex + sourceUnitIndex];
    if (g != NULL) {
        g->m_holdWindowLo = static_cast<i32>(
            (duration * m_flightDist * g_boomerangHoldScale - g_boomerangHoldBiasMs)
        );
        g->m_holdWindowHi = 0;
        g->m_holdAnchorLo = g_frameTime;
        g->m_holdAnchorHi = 0;
        if (g->CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS(g)
        }
    }
    m_launched = false;
    return 1;
}

RVA(0x000e08b0, 0x1de)
void CBoomerang::AdvanceMotion() {
    double s;
    double c;
    if (m_launched == false && m_phase > g_boomerangHalfTurnRadians) {
        SET_VECTOR2_COMPONENTS(m_object->m_screenPosition, m_targetPx.m_x, m_targetPx.m_y);
        if (m_shadow != NULL) {
            SET_VECTOR2_COMPONENTS(m_shadow->m_screenPosition, m_targetPx.m_x, m_targetPx.m_y);
        }
        m_launched = true;
    } else if (m_phase > g_boomerangFullTurnRadians && m_launched != false) {
        ScanTargets(1);
        if (m_shadow != NULL) {
            m_shadow->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_shadow = NULL;
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return;
    }
    ScanTargets(0);

    s = sin(m_phase);
    c = cos(m_phase);
    double vx = m_direction.m_x;
    double vy = -m_direction.m_y;
    double phaseDelta = static_cast<double>(g_frameDelta) * m_velScale;
    double xSinTerm = vy * s;
    double xCosTerm = vx * c;
    double ySinTerm = vx * s;
    double yCosTerm = vy * c;
    SET_VECTOR2_COMPONENTS(m_position, xSinTerm - xCosTerm, ySinTerm + yCosTerm);
    SET_VECTOR2_COMPONENTS(
        m_position,
        m_origin.m_x + m_position.m_x,
        m_origin.m_y + m_position.m_y
    );
    m_phase = phaseDelta + m_phase;
    SET_VECTOR2_COMPONENTS(
        m_object->m_screenPosition,
        static_cast<i32>(m_position.m_x),
        static_cast<i32>(m_position.m_y)
    );
    if (m_shadow != NULL) {
        SET_VECTOR2_COMPONENTS(
            m_shadow->m_screenPosition,
            static_cast<i32>(m_position.m_x),
            static_cast<i32>(m_position.m_y)
        );
    }
}

RVA(0x000e0b10, 0x1bd)
void CProjectile::ScanTargets(i32 impact) {
    i32 playerIndex = 0;
    RECT box;
    SET_RECT_XY_EXTENTS(
        box,
        m_object->m_screenPosition.m_x - 0x10,
        box.left + 0x20,
        m_object->m_screenPosition.m_y - 0x10,
        box.top + 0x20
    );
    i32 playerBase = 0;
    i32 gridIndex;
    i32 unitIndex;
    while (playerBase < 0x3c) {
        unitIndex = 0;
        gridIndex = playerBase;
        for (; unitIndex < 0xf; unitIndex++, gridIndex++) {
            CGrunt* g = g_gameReg->m_triggerMgr->m_units[gridIndex];
            if (g == NULL) {
                continue;
            }
            if (g->m_entranceCommitted == false) {
                continue;
            }
            i32 gx = g->m_object->m_screenPosition.m_x - 7;
            i32 gy = g->m_object->m_screenPosition.m_y - 7;
            i32 gxhi = gx + 0xe;
            i32 gyhi = gy + 0xe;
            if (box.left > gxhi) {
                continue;
            }
            if (box.right < gx) {
                continue;
            }
            if (box.top > gyhi) {
                continue;
            }
            if (box.bottom < gy) {
                continue;
            }
            if (m_sourcePlayerIndex == playerIndex && m_sourceUnitIndex == unitIndex) {

                if (impact != 0 && g->m_entranceCommitted != false
                    && g->m_entranceReason == PICKUP_NONE) {
                    g->LoadGruntTypeTable(PICKUP_BOOMERANG, 1, 0, 0);
                }
                return;
            }

            i32 hitPlayerIndex = g->m_playerIndex;
            i32 hitUnitIndex = g->m_unitIndex;
            for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {

                Coord* k = static_cast<Coord*>(m_hitList.GetNext(pos));
                if (k->m_x == hitPlayerIndex && k->m_y == hitUnitIndex) {
                    return;
                }
            }

            Coord* slot = NULL;
            CoordPoolNode* p = g_coordPool.m_freeHead;
            if (p->m_next != NULL) {
                slot = &p->m_value;
                slot->m_x = hitPlayerIndex;
                slot->m_y = hitUnitIndex;
                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
            }
            m_hitList.AddTail(slot);
            g->StepCombatReaction(
                m_kind,
                1,
                m_sourcePlayerIndex,
                m_sourceUnitIndex,
                m_sourcePx.m_x,
                m_sourcePx.m_y,
                1,
                PICKUP_NONE
            );
        }
        playerIndex++;
        playerBase += TM_UNITS_PER_PLAYER;
    }
}

// @early-stop
RVA(0x000e0d40, 0x6c2)
i32 CProjectile::SerializeDispatch(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];

    switch (mode) {
        case SERIAL_LOAD: {
            m_sound = NULL;
            s->Read(&m_kind, sizeof(m_kind));
            s->Read(&m_sourcePlayerIndex, sizeof(m_sourcePlayerIndex));
            s->Read(&m_sourceUnitIndex, sizeof(m_sourceUnitIndex));
            s->Read(&m_targetPx.m_x, sizeof(m_targetPx.m_x));
            s->Read(&m_targetPx.m_y, sizeof(m_targetPx.m_y));
            s->Read(&m_flightDist, sizeof(m_flightDist));
            s->Read(&m_timePerTile, sizeof(m_timePerTile));
            s->Read(&m_velScale, sizeof(m_velScale));
            s->Read(&m_position.m_x, sizeof(m_position.m_x));
            s->Read(&m_position.m_y, sizeof(m_position.m_y));
            s->Read(&m_velocity.m_x, sizeof(m_velocity.m_x));
            s->Read(&m_velocity.m_y, sizeof(m_velocity.m_y));
            s->Read(&m_roundBias.m_x, sizeof(m_roundBias.m_x));
            s->Read(&m_roundBias.m_y, sizeof(m_roundBias.m_y));
            s->Read(&m_currentPx.m_x, sizeof(m_currentPx.m_x));
            s->Read(&m_currentPx.m_y, sizeof(m_currentPx.m_y));
            s->Read(&m_isArcing, sizeof(m_isArcing));
            s->Read(&m_arrived, sizeof(m_arrived));
            s->Read(&m_sourcePx.m_x, sizeof(m_sourcePx.m_x));
            s->Read(&m_sourcePx.m_y, sizeof(m_sourcePx.m_y));

            for (i32 ni = 0; ni < 7; ni++) {
                g_serialCounter++;
                s->Read(buf, SERIAL_NAME_LEN);
                if (strlen(buf) != 0) {
                    m_frames[ni] = MapFind<CAniElement>(reg->m_animRegistry->m_animations, buf);
                } else {
                    m_frames[ni] = NULL;
                }
            }

            g_serialCounter++;
            i32 count;
            s->Read(&count, sizeof(count));
            m_shadow = LookupSerialRef(reg->m_childGroup->m_registeredGameObjectsById, count);
            if (m_shadow == NULL && count != 0) {
                return 0;
            }

            s->Read(&count, sizeof(count));
            for (i32 ci = 0; ci < count; ci++) {
                CoordPoolNode* node = g_coordPool.m_freeHead;
                Coord* payload = NULL;
                if (node->m_next != NULL) {
                    payload = &node->m_value;
                    g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
                }
                s->Read(payload, 8);
                m_hitList.AddTail(payload);
            }
            break;
        }

        case SERIAL_SAVE: {
            s->Write(&m_kind, sizeof(m_kind));
            s->Write(&m_sourcePlayerIndex, sizeof(m_sourcePlayerIndex));
            s->Write(&m_sourceUnitIndex, sizeof(m_sourceUnitIndex));
            s->Write(&m_targetPx.m_x, sizeof(m_targetPx.m_x));
            s->Write(&m_targetPx.m_y, sizeof(m_targetPx.m_y));
            s->Write(&m_flightDist, sizeof(m_flightDist));
            s->Write(&m_timePerTile, sizeof(m_timePerTile));
            s->Write(&m_velScale, sizeof(m_velScale));
            s->Write(&m_position.m_x, sizeof(m_position.m_x));
            s->Write(&m_position.m_y, sizeof(m_position.m_y));
            s->Write(&m_velocity.m_x, sizeof(m_velocity.m_x));
            s->Write(&m_velocity.m_y, sizeof(m_velocity.m_y));
            s->Write(&m_roundBias.m_x, sizeof(m_roundBias.m_x));
            s->Write(&m_roundBias.m_y, sizeof(m_roundBias.m_y));
            s->Write(&m_currentPx.m_x, sizeof(m_currentPx.m_x));
            s->Write(&m_currentPx.m_y, sizeof(m_currentPx.m_y));
            s->Write(&m_isArcing, sizeof(m_isArcing));
            s->Write(&m_arrived, sizeof(m_arrived));
            s->Write(&m_sourcePx.m_x, sizeof(m_sourcePx.m_x));
            s->Write(&m_sourcePx.m_y, sizeof(m_sourcePx.m_y));

            CAniElement** fp = m_frames;
            for (i32 fi = 0; fi < 7; fi++) {
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (*fp != NULL) {
                    strcpy(buf, reg->m_animRegistry->FindAnimationKey(*fp));
                }
                s->Write(buf, SERIAL_NAME_LEN);
                fp++;
            }

            g_serialCounter++;
            i32 count = 0;
            if (m_shadow != NULL) {
                count = m_shadow->m_objectId;
            }
            s->Write(&count, sizeof(count));

            count = m_hitList.GetCount();
            s->Write(&count, sizeof(count));

            POSITION pos = m_hitList.GetHeadPosition();
            while (pos != NULL) {
                s->Write(m_hitList.GetNext(pos), 8);
            }
            break;
        }
    }

    i32 ok = CMovingLogic::SerializeDispatch(s, mode, typeId, object);
    if (ok == 0) {
        return ok;
    }
    if (s == NULL) {
        return 0;
    }

    switch (mode) {
        case SERIAL_LOAD: {
            s->Read(buf, SERIAL_NAME_LEN);
            s->Read(m_blob, 0x10);
            CGameObject* obj = object;
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdSpriteObject*>(obj);
            m_ownerLogicRecord = obj->m_logicRecord;
            if (strlen(buf) == 0) {
                m_value = NULL;
                return 1;
            }
            m_value = MapFind<CAniElement>(
                m_ownerLogicRecord->m_ownerCtx->m_animRegistry->m_animations,
                buf
            );
            return 1;
        }
        case SERIAL_SAVE: {
            char blob[SERIAL_NAME_LEN];
            memset(blob, 0, sizeof(blob));
            if (m_value != NULL) {
                strcpy(
                    blob,
                    m_ownerLogicRecord->m_ownerCtx->m_animRegistry->FindAnimationKey(m_value)
                );
            }
            s->Write(blob, SERIAL_NAME_LEN);
            s->Write(m_blob, 0x10);
            return 1;
        }
    }
    return 1;
}

RVA(0x000e15d0, 0x155)
i32 CBoomerang::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD:
            ar->Read(&m_launchPosition.m_x, sizeof(m_launchPosition.m_x));
            ar->Read(&m_launchPosition.m_y, sizeof(m_launchPosition.m_y));
            ar->Read(&m_direction.m_x, sizeof(m_direction.m_x));
            ar->Read(&m_direction.m_y, sizeof(m_direction.m_y));
            ar->Read(&m_origin.m_x, sizeof(m_origin.m_x));
            ar->Read(&m_origin.m_y, sizeof(m_origin.m_y));
            ar->Read(&m_phase, sizeof(m_phase));
            ar->Read(&m_launched, sizeof(m_launched));
            break;
        case SERIAL_SAVE:
            ar->Write(&m_launchPosition.m_x, sizeof(m_launchPosition.m_x));
            ar->Write(&m_launchPosition.m_y, sizeof(m_launchPosition.m_y));
            ar->Write(&m_direction.m_x, sizeof(m_direction.m_x));
            ar->Write(&m_direction.m_y, sizeof(m_direction.m_y));
            ar->Write(&m_origin.m_x, sizeof(m_origin.m_x));
            ar->Write(&m_origin.m_y, sizeof(m_origin.m_y));
            ar->Write(&m_phase, sizeof(m_phase));
            ar->Write(&m_launched, sizeof(m_launched));
            break;
    }
    return CProjectile::SerializeDispatch(ar, mode, typeId, object) ? 1 : 0;
}

RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CTimeBomb>::s_table[id] = static_cast<CActHandler>(&CTimeBomb::UpdateCountdown);
}

// @early-stop

RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj), m_startTime(0), m_duration(0) {
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_PROJECTILE);
    SetImageSetByName("GAME_TIMEBOMB");
    SET_ANIMATION_ACT("A");
    m_value = m_wwdObject->m_animationCursor.m_animation;
    if (m_object->m_damage > 0) {
        m_wwdObject->SetAnimationByName("GAME_TIMEBOMBFAST", 0);
        m_duration = static_cast<u32>(m_object->m_damage);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = true;
    } else {
        m_wwdObject->SetAnimationByName("GAME_TIMEBOMBSLOW", 0);
        m_duration = g_buteMgr.GetDword("Projectile", "TimeBombSlowTime", 0xfa0);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = false;
    }
    Coord tile;
    GetScreenTile(&tile);
    CMapMgr* g = g_gameReg->m_tileGrid;
    if (tile.m_x < g->m_width && tile.m_y < g->m_height) {
        g->m_rows[tile.m_y][tile.m_x].m_flags |= IDX(CELL_FLAG_TIME_BOMB);
    }
    m_object->m_smarts = -1;
}

// @early-stop
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::UpdateCountdown() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & BRICKZ_BLOCKED_MASK) || (cell & IDX(CELL_FLAG_SPECIAL))) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        TBombGridClear(m_object);
        return 0;
    }
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    if (static_cast<i64>(g_frameTime) - m_startTime >= m_duration) {
        if (m_fastPhase == false) {
            SwitchAnimationByName("GAME_TIMEBOMBFAST", 0);
            m_duration = g_buteMgr.GetDword("Projectile", "TimeBombFastTime", 0x3e8);
            m_startTime = g_frameTime;
            m_fastPhase = true;
        } else {
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            TBombGridClear(m_object);
            g_gameReg->m_triggerMgr->LoadExplosionSprites(
                m_object->m_screenPosition.m_x,
                m_object->m_screenPosition.m_y,
                m_object->m_smarts,
                1
            );
        }
    }
    return 0;
}

RVA(0x000e2080, 0xc1)
i32 CTimeBomb::SerializeDispatch(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
    SerBandPair(sa, mode, &m_timing);
    switch (mode) {
        case SERIAL_LOAD:
            sa->Read(&m_fastPhase, sizeof(m_fastPhase));
            break;
        case SERIAL_SAVE:
            sa->Write(&m_fastPhase, sizeof(m_fastPhase));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM(arc, sa, mode, typeId, object)
}

RVA(0x000e2190, 0x83)
i32 CProjectile::LaunchSound(const char* key) {
    CGruntzMgr* gameMgr;
    CDDrawSurfaceMgr* world;
    SoundCue* cue;
    if (m_sound != NULL) {
        goto fail;
    }
    gameMgr = g_gameReg;
    if (gameMgr->m_soundEnabled == false) {
        goto fail;
    }
    world = gameMgr->m_world;
    cue = NULL;
    MapLookup(world->m_soundRegistry->m_cues, key, cue);
    if (cue == NULL) {
        goto fail;
    }
    if (cue->m_sound == NULL) {
        goto fail;
    }

    m_sound = static_cast<SoundBuffer*>(cue->m_sound->AcquireInstance());
    if (m_sound != NULL) {
        m_sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
        return 1;
    }
fail:
    return 0;
}

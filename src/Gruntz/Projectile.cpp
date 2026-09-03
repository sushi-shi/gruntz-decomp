#include <rva.h>

#include <Gruntz/Projectile.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Boomerang.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
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
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/MoveMode.h>

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
// contribution (first obj on the link line realizing these vtables; docs/link-text-layout.md).
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
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR)
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

            PushFreeNode(&g_coordPool, hitPoint);
        }
    }
    m_hitList.RemoveAll();
}

static inline CAniElement* LookupAnim(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* found = NULL;
    MapLookup(map, name, found);
    return found;
}

static inline CWwdSpriteObject* LookupSerialRef(CMapPtrToPtr& byId, i32 id) {
    CGameObject* found = NULL;
    if (MapLookupById(byId, id, found) == false) {
        return NULL;
    }
    if (found == NULL) {
        return NULL;
    }
    return found->GetClassId() == CLASSID_SERIALREF ? static_cast<CWwdSpriteObject*>(found) : NULL;
}

// @early-stop
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
    m_targetPx.Set(targetPxX, targetPxY);
    SnapTileCenter(&m_targetPx);
    m_kind = kind;
    m_sourcePx.Set(sourcePxX, sourcePxY);

    Coord sourcePosition = m_object->ScreenPos();
    DoubleVector2 direction(m_targetPx - sourcePosition);
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
            Coord targetTile = m_targetPx;
            ScreenTile(&targetTile);
            Coord sourceTile = sourcePosition;
            ScreenTile(&sourceTile);
            Coord tileDistance = (targetTile - sourceTile).GetAbs();
            count = tileDistance.m_x;
            if (tileDistance.m_x <= tileDistance.m_y) {
                count = tileDistance.m_y;
            }
            break;
        }
        default:
            return 0;
    }

    m_frames[0] = LookupAnim(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        key + DATA_COMPGEN(0x00213658, "1")
        );
    if (m_frames[0] == NULL) {
        return 0;
    }
    m_frames[1] = LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "2");
    m_frames[2] = LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "3");
    m_frames[3] = LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "4");
    m_frames[4] = LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "5");
    m_frames[PF_IMPACT] =
        LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "IMPACT");
    m_frames[PF_FALL] =
        LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, key + "FALL");

    SwitchAnimation(m_frames[0]);
    SetImageSetByName(key + "_OBJECT");

    u32 totalTime = static_cast<u32>((count * m_timePerTile));
    double len = direction.Mag();
    double t = static_cast<double>(totalTime);
    m_flightDist = len;
    m_velScale = len / t;
    m_position.Init(sourcePosition);
    direction.Normalize();
    m_velocity = direction;
    m_roundBias = PixelRoundBias(direction);
    m_flightDist = fabs(len);
    m_currentPx = sourcePosition;
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

static inline CActHandler* ProjActLookup(i32 coord) {
    return (CActRegPool<CProjectile>::s_table.ResolveEntry(coord));
}

RVA(0x000df9a0, 0x102)
void CProjectile::FireActivation(i32 coord) {
    CActHandler* e = ProjActLookup(coord);
    if ((*e) != NULL) {
        (this->*((*ProjActLookup(coord))))();
    }
}

RVA(0x000dfb00, 0x18d)
void CProjectile::RegisterType() {
    ACT_NAME_ID(id, "A")
    *ProjActLookup(id) =
        static_cast<CActHandler>(&CProjectile::AdvanceAnimationAndDeleteWhenComplete);
}

// @early-stop
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

    if (m_currentPx != m_targetPx) {

        if (m_kind == PICKUP_WINGZ) {
            ScanTargets(0);
        }
        m_position += m_velocity * (static_cast<double>(g_frameDelta) * m_velScale);
        Coord nextPosition = (m_roundBias + m_position).ToCoord();
        if (m_velocity.x > 0.0) {
            if (nextPosition.m_x > m_targetPx.m_x) {
                nextPosition.m_x = m_targetPx.m_x;
            }
        } else if (m_velocity.x < 0.0) {
            if (nextPosition.m_x < m_targetPx.m_x) {
                nextPosition.m_x = m_targetPx.m_x;
            }
        }
        if (m_velocity.y > 0.0) {
            if (nextPosition.m_y > m_targetPx.m_y) {
                nextPosition.m_y = m_targetPx.m_y;
            }
        } else if (m_velocity.y < 0.0) {
            if (nextPosition.m_y < m_targetPx.m_y) {
                nextPosition.m_y = m_targetPx.m_y;
            }
        }
        m_currentPx = nextPosition;
        Coord arcOffset(0, 0);
        if (m_isArcing != false) {
            DoubleVector2 target(m_targetPx);
            double dist = m_position.Dist(target);
            if (dist >= m_flightDist * 0.9 || dist < m_flightDist * 0.1) {
                arcOffset.Set(0x4, -0x4);
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[0]) {
                    SwitchAnimation(m_frames[0]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[0]);
                    }
                }
            } else if (dist >= m_flightDist * 0.8 || dist < m_flightDist * 0.2) {
                arcOffset.Set(0x8, -0x8);
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[1]) {
                    SwitchAnimation(m_frames[1]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[1]);
                    }
                }
            } else if (dist >= m_flightDist * 0.7 || dist < m_flightDist * 0.3) {
                arcOffset.Set(0xc, -0xc);
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[2]) {
                    SwitchAnimation(m_frames[2]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[2]);
                    }
                }
            } else if (dist >= m_flightDist * 0.6 || dist < m_flightDist * 0.4) {
                arcOffset.Set(0x10, -0x10);
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[3]) {
                    SwitchAnimation(m_frames[3]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[3]);
                    }
                }
            } else {
                arcOffset.Set(0x14, -0x14);
                if (m_wwdObject->m_animationCursor.m_animation != m_frames[4]) {
                    SwitchAnimation(m_frames[4]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animationCursor.SetAnimation(m_frames[4]);
                    }
                }
            }
        }
        Coord displayPosition = m_currentPx;
        displayPosition += arcOffset;
        m_object->SetScreenPos(displayPosition);
        if (m_shadow != NULL) {
            m_shadow->SetScreenPos(m_currentPx);
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
        Coord targetTile = m_targetPx;
        ScreenTile(&targetTile);
        u32 flags = plane->CellFlagsAt(targetTile.m_x, targetTile.m_y);
        if ((flags & IDX(CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD)) == 0) {
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
    if (IsAniCursorComplete(&sprite->m_animationCursor)) {
        sprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    }
    return 0;
}

RVA(0x000e0650, 0x2b)
CBoomerang::CBoomerang(CGameObject* owner) : CProjectile(owner) {

    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
}

// @early-stop
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
    m_launchPosition = owner->ScreenPos();
    DoubleVector2 launchPosition(m_launchPosition);
    m_origin = (DoubleVector2(m_targetPx) + launchPosition) * g_boomerangMidpointScale;
    m_direction = m_origin - launchPosition;
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
            RECYCLE_GRUNT_COORDS_EXPANDED(g)
        }
    }
    m_launched = false;
    return 1;
}

RVA(0x000e08b0, 0x1de)
void CBoomerang::AdvanceMotion() {
    double s;
    if (m_launched == false && m_phase > g_boomerangHalfTurnRadians) {
        m_object->SetScreenPos(m_targetPx);
        if (m_shadow != NULL) {
            m_shadow->SetScreenPos(m_targetPx);
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
    double c = cos(m_phase);
    DoubleVector2 direction(m_direction.x, -m_direction.y);
    double phaseDelta = static_cast<double>(g_frameDelta) * m_velScale;
    DoubleVector2 rotated(direction.y * s - direction.x * c, direction.x * s + direction.y * c);
    m_position = m_origin;
    m_position += rotated;
    m_phase = phaseDelta + m_phase;
    Coord displayPosition = m_position.ToCoord();
    m_object->SetScreenPos(displayPosition);
    if (m_shadow != NULL) {
        m_shadow->SetScreenPos(displayPosition);
    }
}

RVA(0x000e0b10, 0x1bd)
void CProjectile::ScanTargets(i32 impact) {
    i32 playerIndex = 0;
    Coord position = m_object->ScreenPos();
    Coord halfExtent(TILE_HALF_PX, TILE_HALF_PX);
    Coord boxMin = position - halfExtent;
    Coord boxMax = position + halfExtent;
    CRect box = MakeRect(boxMin.m_x, boxMin.m_y, boxMax.m_x, boxMax.m_y);
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
            Coord gruntPosition = g->m_object->ScreenPos();
            CRect gruntBox(
                gruntPosition.m_x - 7,
                gruntPosition.m_y - 7,
                gruntPosition.m_x + 7,
                gruntPosition.m_y + 7
            );
            if (box.left > gruntBox.right) {
                continue;
            }
            if (box.right < gruntBox.left) {
                continue;
            }
            if (box.top > gruntBox.bottom) {
                continue;
            }
            if (box.bottom < gruntBox.top) {
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
            Coord hitIdentity(hitPlayerIndex, hitUnitIndex);
            for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {

                Coord* k = static_cast<Coord*>(m_hitList.GetNext(pos));
                if (*k == hitIdentity) {
                    return;
                }
            }

            Coord* slot = NULL;
            CoordPoolNode* p = g_coordPool.m_freeHead;
            if (p->m_next != NULL) {
                slot = &p->m_coord;
                slot->Set(hitPlayerIndex, hitUnitIndex);
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
            s->Read(&m_position.x, sizeof(m_position.x));
            s->Read(&m_position.y, sizeof(m_position.y));
            s->Read(&m_velocity.x, sizeof(m_velocity.x));
            s->Read(&m_velocity.y, sizeof(m_velocity.y));
            s->Read(&m_roundBias.x, sizeof(m_roundBias.x));
            s->Read(&m_roundBias.y, sizeof(m_roundBias.y));
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
                    m_frames[ni] = LookupAnim(reg->m_animRegistry->m_animations, buf);
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
                    payload = &node->m_coord;
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
            s->Write(&m_position.x, sizeof(m_position.x));
            s->Write(&m_position.y, sizeof(m_position.y));
            s->Write(&m_velocity.x, sizeof(m_velocity.x));
            s->Write(&m_velocity.y, sizeof(m_velocity.y));
            s->Write(&m_roundBias.x, sizeof(m_roundBias.x));
            s->Write(&m_roundBias.y, sizeof(m_roundBias.y));
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
            m_value = LookupAnim(m_ownerLogicRecord->m_ownerCtx->m_animRegistry->m_animations, buf);
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
            ar->Read(&m_direction.x, sizeof(m_direction.x));
            ar->Read(&m_direction.y, sizeof(m_direction.y));
            ar->Read(&m_origin.x, sizeof(m_origin.x));
            ar->Read(&m_origin.y, sizeof(m_origin.y));
            ar->Read(&m_phase, sizeof(m_phase));
            ar->Read(&m_launched, sizeof(m_launched));
            break;
        case SERIAL_SAVE:
            ar->Write(&m_launchPosition.m_x, sizeof(m_launchPosition.m_x));
            ar->Write(&m_launchPosition.m_y, sizeof(m_launchPosition.m_y));
            ar->Write(&m_direction.x, sizeof(m_direction.x));
            ar->Write(&m_direction.y, sizeof(m_direction.y));
            ar->Write(&m_origin.x, sizeof(m_origin.x));
            ar->Write(&m_origin.y, sizeof(m_origin.y));
            ar->Write(&m_phase, sizeof(m_phase));
            ar->Write(&m_launched, sizeof(m_launched));
            break;
    }
    return CProjectile::SerializeDispatch(ar, mode, typeId, object) ? 1 : 0;
}

static inline CActHandler* TBombLookup(i32 coord) {
    return (CActRegPool<CTimeBomb>::s_table.ResolveEntry(coord));
}

RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    CActHandler* e = TBombLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = TBombLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
    ACT_NAME_ID(id, "A")
    *(TBombLookup(id)) = static_cast<CActHandler>(&CTimeBomb::UpdateCountdown);
}

// @early-stop

RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj), m_startTime(0), m_duration(0) {
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_PROJECTILE)
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

static inline i32 TBombGridCell(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    Coord tile = obj->ScreenPos();
    ScreenTile(&tile);
    if (static_cast<u32>(tile.m_x) < static_cast<u32>(g->m_width)
        && static_cast<u32>(tile.m_y) < static_cast<u32>(g->m_height)) {
        BrickzCell* row = g->m_rows[tile.m_y];
        return row[tile.m_x].m_flags;
    }
    return 1;
}
static inline void TBombGridClear(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    Coord tile = obj->ScreenPos();
    ScreenTile(&tile);
    if (static_cast<u32>(tile.m_x) < static_cast<u32>(g->m_width)
        && static_cast<u32>(tile.m_y) < static_cast<u32>(g->m_height)) {
        g->m_rows[tile.m_y][tile.m_x].m_flags &= ~IDX(CELL_FLAG_TIME_BOMB);
    }
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

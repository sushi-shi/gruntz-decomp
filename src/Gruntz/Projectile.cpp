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
// retail 0x001f04e8 holds 0x18 in .rdata, so the default is 24 and const.
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

// The header-inline `~CMotionState() {}` (MotionState.h) emitted out of line: the
// unwind funclets of ??0CProjectile (this-0x18 +0x38) and ??0CGrunt take its
// address, so cl gives it a COMDAT. Retail keeps one 1-byte `ret` copy, isolated
// by 0xcc linker fill on both sides, reached through the ILT thunk at 0x00003819.
RVA_COMPGEN(0x00058ba0, 0x1, ??1CMotionState@@QAE@XZ)

// @early-stop
// residue: one instruction, the vptr stamp, which cl schedules after the first
// m_wwdObject load instead of before it.
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
    if (MapLookupById(byId, id, found) == 0) {
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
    m_targetPxX = (targetPxX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_targetPxY = (targetPxY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_kind = kind;
    m_sourcePxX = sourcePxX;
    m_sourcePxY = sourcePxY;

    double dx = static_cast<double>(m_targetPxX) - m_object->m_screenX;
    double dy = static_cast<double>(m_targetPxY) - m_object->m_screenY;
    i32 count = 1;

    switch (kind) {
        case PICKUP_ROCK:
            key = "GRUNTZ_ROCKGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDwordDef("Projectile", "RockProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PICKUP_GUNHAT:
            key = "GRUNTZ_GUNHATGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "GunhatProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PICKUP_BOOMERANG:
            key = "GRUNTZ_BOOMERANGGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "BoomerangProjectileTimePerTile", 0xbb8);
            m_isArcing = 0;
            break;
        case PICKUP_NERFGUN:
            key = "GRUNTZ_NERFGUNGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "NerfGunProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PICKUP_WELDER:
            key = "GRUNTZ_WELDERGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WelderProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PICKUP_WINGZ: {
            key = "GRUNTZ_WINGZGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WingzProjectileTimePerTile", 0xbb8);
            LaunchSound("GRUNTZ_WINGZGRUNT_WINGZGRUNTLOOP");
            m_isArcing = 0;
            i32 ddx = abs((m_targetPxX >> TILE_SHIFT_PX) - (m_object->m_screenX >> TILE_SHIFT_PX));
            i32 ddy = abs((m_targetPxY >> TILE_SHIFT_PX) - (m_object->m_screenY >> TILE_SHIFT_PX));
            count = ddx;
            if (ddx <= ddy) {
                count = ddy;
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
    double len = sqrt(dx * dx + dy * dy);
    double t = static_cast<double>(totalTime);
    double vx = dx / len;
    m_flightDist = len;
    m_velScale = len / t;
    m_posX = m_object->m_screenX;
    m_posY = m_object->m_screenY;
    m_velX = vx;
    dy /= len;
    m_velY = dy;

    if (vx > 0.0) {
        m_roundX = 0.5;
    } else if (vx < 0.0) {
        m_roundX = -0.5;
    } else {
        m_roundX = 0.0;
    }
    if (dy > 0.0) {
        m_roundY = 0.5;
    } else if (dy < 0.0) {
        m_roundY = -0.5;
    } else {
        m_roundY = 0.0;
    }
    m_flightDist = fabs(len);
    m_curX = m_object->m_screenX;
    m_curY = m_object->m_screenY;
    m_arrived = 0;

    CDDrawChildGroup* factory = g_gameReg->m_world->m_childGroup;
    m_shadow = (factory->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        SORTKEY_ACTOR_BEHIND,
        "LightFx",
        0x2040003
    ));
    if (m_shadow != NULL) {
        m_shadow->m_logicRecord->m_dispatch(m_shadow);
        (static_cast<CLightFx*>(m_shadow->m_logicRecord->m_userLogic))
            ->Activate(
                static_cast<const char*>(key + "_SHADOW"),
                static_cast<const char*>(key + "1"),
                5,
                1
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
    if (m_arrived != 0) {
        return;
    }

    if (m_kind == PICKUP_WINGZ) {
        CWwdSpriteObject* owner = m_object;
        CGruntzMgr* reg = g_gameReg;
        if (CGameLevel::PointInRect(&reg->m_viewBounds, owner->m_screenX, owner->m_screenY)) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_sound != NULL) {
            m_sound->StopAndRewind();
            m_sound = NULL;
        }
    }

    if (m_curX != m_targetPxX || m_curY != m_targetPxY) {

        if (m_kind == PICKUP_WINGZ) {
            ScanTargets(0);
        }
        m_posX = m_posX + static_cast<double>(g_frameDelta) * m_velX * m_velScale;
        m_posY = m_posY + static_cast<double>(g_frameDelta) * m_velY * m_velScale;
        i32 xRes = static_cast<i32>((m_roundX + m_posX));
        i32 yRes = static_cast<i32>((m_roundY + m_posY));
        i32 localX = xRes;
        if (m_velX > 0.0) {
            if (xRes > m_targetPxX) {
                localX = m_targetPxX;
                xRes = m_targetPxX;
            }
        } else if (m_velX < 0.0) {
            if (xRes < m_targetPxX) {
                localX = m_targetPxX;
                xRes = m_targetPxX;
            }
        }
        if (m_velY > 0.0) {
            if (yRes > m_targetPxY) {
                yRes = m_targetPxY;
            }
        } else if (m_velY < 0.0) {
            if (yRes < m_targetPxY) {
                yRes = m_targetPxY;
            }
        }
        m_curX = xRes;
        m_curY = yRes;
        i32 offX = 0;
        i32 offY = 0;
        if (m_isArcing != 0) {
            double dx = fabs(static_cast<double>(m_targetPxX) - m_posX);
            double dy = fabs(static_cast<double>(m_targetPxY) - m_posY);
            double dist = sqrt(dx * dx + dy * dy);
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
        m_object->m_screenX = offX + m_curX;
        m_object->m_screenY = offY + m_curY;
        if (m_shadow != NULL) {
            m_shadow->m_screenX = localX;
            m_shadow->m_screenY = yRes;
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
    m_arrived = 1;
    i32 tier = 0;
    if (m_kind != PICKUP_WINGZ) {
        CGruntzMgr* reg = g_gameReg;
        CMapMgr* plane = reg->m_tileGrid;
        i32 tileY = m_targetPxY >> TILE_SHIFT_PX;
        i32 tileX = m_targetPxX >> TILE_SHIFT_PX;
        u32 flags = plane->CellFlagsAt(tileX, tileY);
        if ((flags & 0x900) == 0) {
            if (flags & 0x2) {
                if (flags & 0x40) {
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

                            if (CGameLevel::PointInRect(
                                    &reg->m_viewBounds,
                                    m_targetPxX,
                                    m_targetPxY
                                )) {
                                CWwdSpriteObject* fx = reg->m_world->m_childGroup->CreateSprite(
                                    0,
                                    m_targetPxX,
                                    m_targetPxY,
                                    SORTKEY_ACTOR_BEHIND,
                                    "Particlez",
                                    0x40003
                                );
                                if (fx != NULL) {
                                    fx->SetImageSetByName("LEVEL_DEATHSPLASH");
                                    fx->SetAnimationByName("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            SetObjectFlags(0x10000);
                            return;
                    }
                }
            }
        } else {
            if (CGameLevel::PointInRect(&reg->m_viewBounds, m_targetPxX, m_targetPxY)) {
                CWwdSpriteObject* fx = reg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_targetPxX,
                    m_targetPxY,
                    SORTKEY_ACTOR_BEHIND,
                    "Particlez",
                    0x40003
                );
                if (fx != NULL) {
                    fx->SetImageSetByName("GAME_WATER");
                    fx->SetAnimationByName("GAME_WATER", 0);
                }
            }
            SetObjectFlags(0x10000);
            return;
        }
    }
    CAniElement* sprite = (tier != 0) ? m_frames[PF_FALL] : m_frames[PF_IMPACT];
    if (sprite != NULL) {
        SwitchAnimation(sprite);
        return;
    }
    SetObjectFlags(0x10000);
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

    SetObjectFlags(0x2000002);
}

// @early-stop
// Keep the converted duration live: it is reused for the Grunt hold window.
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
    m_launchX = owner->m_screenX;
    m_launchY = owner->m_screenY;
    double originY = (static_cast<double>(m_targetPxY) + static_cast<double>(owner->m_screenY))
                     * g_boomerangMidpointScale;
    m_originX = (static_cast<double>(m_targetPxX) + static_cast<double>(owner->m_screenX))
                * g_boomerangMidpointScale;
    m_originY = originY;
    m_dirX = m_originX - static_cast<double>(m_launchX);
    m_dirY = originY - static_cast<double>(m_launchY);
    m_phase = 0.0;
    m_velScale = d;
    CGrunt* g = g_gameReg->m_triggerMgr->m_units[15 * sourcePlayerIndex + sourceUnitIndex];
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
    m_launched = 0;
    return 1;
}

// @early-stop
// Retail keeps the two rotated components and the phase increment live in a
// 0x20-byte x87 frame; the equivalent local form below currently uses 0x18.
RVA(0x000e08b0, 0x1de)
void CBoomerang::AdvanceMotion() {
    if (m_launched == 0 && m_phase > g_boomerangHalfTurnRadians) {
        m_object->m_screenX = m_targetPxX;
        m_object->m_screenY = m_targetPxY;
        if (m_shadow != NULL) {
            m_shadow->m_screenX = m_targetPxX;
            m_shadow->m_screenY = m_targetPxY;
        }
        m_launched = 1;
    } else if (m_phase > g_boomerangFullTurnRadians && m_launched != 0) {
        ScanTargets(1);
        if (m_shadow != NULL) {
            m_shadow->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_shadow = NULL;
        }
        SetObjectFlags(0x10000);
        return;
    }
    ScanTargets(0);

    double s = sin(m_phase);
    double c = cos(m_phase);
    double amp = static_cast<double>(g_frameDelta);
    double vx = m_dirX;
    double vy = -m_dirY;
    m_phase += amp * m_velScale;
    m_posX = m_originX + vy * s - vx * c;
    m_posY = m_originY + vx * s + vy * c;
    m_object->m_screenX = static_cast<i32>(m_posX);
    m_object->m_screenY = static_cast<i32>(m_posY);
    if (m_shadow != NULL) {
        m_shadow->m_screenX = static_cast<i32>(m_posX);
        m_shadow->m_screenY = static_cast<i32>(m_posY);
    }
}

RVA(0x000e0b10, 0x1bd)
void CProjectile::ScanTargets(i32 impact) {
    i32 playerIndex = 0;
    RECT box;
    box.left = m_object->m_screenX - 0x10;
    box.right = box.left + 0x20;
    box.top = m_object->m_screenY - 0x10;
    box.bottom = box.top + 0x20;
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
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            i32 gx = g->m_object->m_screenX - 7;
            i32 gy = g->m_object->m_screenY - 7;
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

                if (impact != 0 && g->m_entranceCommitted != 0
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
                slot = &p->m_coord;
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
                m_sourcePxX,
                m_sourcePxY,
                1,
                PICKUP_NONE
            );
        }
        playerIndex++;
        playerBase += 15;
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
            s->Read(&m_targetPxX, sizeof(m_targetPxX));
            s->Read(&m_targetPxY, sizeof(m_targetPxY));
            s->Read(&m_flightDist, sizeof(m_flightDist));
            s->Read(&m_timePerTile, sizeof(m_timePerTile));
            s->Read(&m_velScale, sizeof(m_velScale));
            s->Read(&m_posX, sizeof(m_posX));
            s->Read(&m_posY, sizeof(m_posY));
            s->Read(&m_velX, sizeof(m_velX));
            s->Read(&m_velY, sizeof(m_velY));
            s->Read(&m_roundX, sizeof(m_roundX));
            s->Read(&m_roundY, sizeof(m_roundY));
            s->Read(&m_curX, sizeof(m_curX));
            s->Read(&m_curY, sizeof(m_curY));
            s->Read(&m_isArcing, sizeof(m_isArcing));
            s->Read(&m_arrived, sizeof(m_arrived));
            s->Read(&m_sourcePxX, sizeof(m_sourcePxX));
            s->Read(&m_sourcePxY, sizeof(m_sourcePxY));

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
            s->Write(&m_targetPxX, sizeof(m_targetPxX));
            s->Write(&m_targetPxY, sizeof(m_targetPxY));
            s->Write(&m_flightDist, sizeof(m_flightDist));
            s->Write(&m_timePerTile, sizeof(m_timePerTile));
            s->Write(&m_velScale, sizeof(m_velScale));
            s->Write(&m_posX, sizeof(m_posX));
            s->Write(&m_posY, sizeof(m_posY));
            s->Write(&m_velX, sizeof(m_velX));
            s->Write(&m_velY, sizeof(m_velY));
            s->Write(&m_roundX, sizeof(m_roundX));
            s->Write(&m_roundY, sizeof(m_roundY));
            s->Write(&m_curX, sizeof(m_curX));
            s->Write(&m_curY, sizeof(m_curY));
            s->Write(&m_isArcing, sizeof(m_isArcing));
            s->Write(&m_arrived, sizeof(m_arrived));
            s->Write(&m_sourcePxX, sizeof(m_sourcePxX));
            s->Write(&m_sourcePxY, sizeof(m_sourcePxY));

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
            ar->Read(&m_launchX, sizeof(m_launchX));
            ar->Read(&m_launchY, sizeof(m_launchY));
            ar->Read(&m_dirX, sizeof(m_dirX));
            ar->Read(&m_dirY, sizeof(m_dirY));
            ar->Read(&m_originX, sizeof(m_originX));
            ar->Read(&m_originY, sizeof(m_originY));
            ar->Read(&m_phase, sizeof(m_phase));
            ar->Read(&m_launched, sizeof(m_launched));
            break;
        case SERIAL_SAVE:
            ar->Write(&m_launchX, sizeof(m_launchX));
            ar->Write(&m_launchY, sizeof(m_launchY));
            ar->Write(&m_dirX, sizeof(m_dirX));
            ar->Write(&m_dirY, sizeof(m_dirY));
            ar->Write(&m_originX, sizeof(m_originX));
            ar->Write(&m_originY, sizeof(m_originY));
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
    SetObjectFlags(0x2000002);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_PROJECTILE)
    SetImageSetByName("GAME_TIMEBOMB");
    SET_ANIMATION_ACT("A");
    m_value = m_wwdObject->m_animationCursor.m_animation;
    if (m_object->m_damage > 0) {
        m_wwdObject->SetAnimationByName("GAME_TIMEBOMBFAST", 0);
        m_duration = static_cast<u32>(m_object->m_damage);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 1;
    } else {
        m_wwdObject->SetAnimationByName("GAME_TIMEBOMBSLOW", 0);
        m_duration = g_buteMgr.GetDwordDef("Projectile", "TimeBombSlowTime", 0xfa0);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 0;
    }
    i32 cx = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 cy = m_object->m_screenY >> TILE_SHIFT_PX;
    CMapMgr* g = g_gameReg->m_tileGrid;
    if (cx < g->m_width && cy < g->m_height) {
        g->m_rowInts[cy][cx * 7] |= 0x1000000;
    }
    m_object->m_smarts = -1;
}

static inline i32 TBombGridCell(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> TILE_SHIFT_PX;
    i32 cy = obj->m_screenY >> TILE_SHIFT_PX;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        BrickzCell* row = g->m_rows[cy];
        return row[cx].m_flags;
    }
    return 1;
}
static inline void TBombGridClear(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> TILE_SHIFT_PX;
    i32 cy = obj->m_screenY >> TILE_SHIFT_PX;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        g->m_rowInts[cy][cx * 7] &= ~0x1000000;
    }
}

// @early-stop
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::UpdateCountdown() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & BRICKZ_BLOCKED_MASK) || (cell & 2)) {
        SetObjectFlags(0x10000);
        TBombGridClear(m_object);
        return 0;
    }
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    if (static_cast<i64>(g_frameTime) - m_startTime >= m_duration) {
        if (m_fastPhase == 0) {
            SwitchAnimationByName("GAME_TIMEBOMBFAST", 0);
            m_duration = g_buteMgr.GetDwordDef("Projectile", "TimeBombFastTime", 0x3e8);
            m_startTime = g_frameTime;
            m_fastPhase = 1;
        } else {
            SetObjectFlags(0x10000);
            TBombGridClear(m_object);
            g_gameReg->m_triggerMgr->LoadExplosionSprites(
                m_object->m_screenX,
                m_object->m_screenY,
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
    if (gameMgr->m_soundEnabled == 0) {
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
        m_sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
        return 1;
    }
fail:
    return 0;
}

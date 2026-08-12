#include <rva.h>

#include <Gruntz/Projectile.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/Boomerang.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionStateSetZInline.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
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

DATA(0x001eaa88)
const double g_motionZScale = 0.001;
DATA(0x001eaad8)
const double g_boomHalf = 0.5;
DATA(0x001eaae0)
const double g_boomTimeScale = 0.03125;
DATA(0x001eaae8)
const double g_projPhase0 = 3.1415927;
DATA(0x001eaaf0)
const double g_boomRetC3 = 0.0625;
DATA(0x001eaaf8)
const double g_boomRetC4 = -500.0;
DATA(0x001eab00)
const double g_projPhase1 = 6.2831854;
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;
DATA(0x001f04e8)
// retail 0x001f04e8 holds 0x18 in .rdata, so the default is 24 and const.
const u32 g_defaultZ = 24;

template<> DATA(0x0024c758)
CActReg CActRegPool<CProjectile>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x0024c780)
CActReg CActRegPool<CTimeBomb>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @interleaver CTimeBomb::~CTimeBomb emitted in the 0x12xxx destructor COMDAT pool.
RVA_COMPGEN(0x00012980, 0x1e, ??_GCProjectile@@UAEPAXI@Z)
RVA_COMPGEN(0x000129d0, 0x1e, ??_GCBoomerang@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a00, 0x5, ??1CBoomerang@@UAE@XZ)
RVA_COMPGEN(0x00012a40, 0x1e, ??_GCTimeBomb@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a70, 0x44, ??1CTimeBomb@@UAE@XZ)

// @interleaver FinalizeStep - 71 B, sits in this class's destructor-COMDAT pool at
// 0x13c70 rather than in the TU's own .text block.
RVA(0x00013c70, 0x47)
void CMovingLogic::FinalizeStep(char*) {
    if (m_deferredCallback != 0) {
        if (m_gatedCallback != 0 && m_objAux->ActKey() == m_gatedActKey) {
            (this->*m_gatedCallback)();
            m_gatedCallback = 0;
        }
        (this->*m_deferredCallback)();
        m_deferredCallback = 0;
        m_gatedActKey = IDX(ACT_NONE);
    }
    AdvanceMotion();
}

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
    m_wwdObject->m_flags |= 0x2000002;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_ACTOR) {
        o->m_sortKey = SORTKEY_ACTOR;
        o->m_flags |= 0x20000;
    }
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
        void* data = m_hitList.GetNext(pos);
        if (data != NULL) {

            CoordPoolNode* node = g_coordPool.NodeOf(data);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_hitList.RemoveAll();
}

// @early-stop
RVA(0x000df050, 0x6ed)
i32 CProjectile::LoadProjectileSprites(
    PickupType kind,
    i32 a,
    i32 b,
    i32 sx,
    i32 sy,
    i32 t0,
    i32 t1
) {
    CString key;
    m_srcRow = a;
    m_targetX = (sx & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_srcCol = b;
    m_kind = kind;
    m_targetY = (sy & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_targetId = t0;
    m_ownerId = t1;

    double dx = static_cast<double>(m_targetX) - m_object->m_screenX;
    double dy = static_cast<double>(m_targetY) - m_object->m_screenY;
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
            i32 ddx = abs((m_targetX >> TILE_SHIFT_PX) - (m_object->m_screenX >> TILE_SHIFT_PX));
            i32 ddy = abs((m_targetY >> TILE_SHIFT_PX) - (m_object->m_screenY >> TILE_SHIFT_PX));
            count = ddx;
            if (ddx <= ddy) {
                count = ddy;
            }
            break;
        }
        default:
            return 0;
    }

    void* out;
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(
        key + DATA_COMPGEN(0x00213658, "1"), out
        );
    m_frames[0] = static_cast<CAniElement*>(out);
    if (m_frames[0] == NULL) {
        return 0;
    }
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(key + "2", out);
    m_frames[1] = static_cast<CAniElement*>(out);
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(key + "3", out);
    m_frames[2] = static_cast<CAniElement*>(out);
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(key + "4", out);
    m_frames[3] = static_cast<CAniElement*>(out);
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(key + "5", out);
    m_frames[4] = static_cast<CAniElement*>(out);
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(key + "IMPACT", out);
    m_frames[PF_IMPACT] = static_cast<CAniElement*>(out);
    out = NULL;
    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(key + "FALL", out);
    m_frames[PF_FALL] = static_cast<CAniElement*>(out);

    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_frames[0]);
    m_wwdObject->ApplyName(key + "_OBJECT");

    u32 totalTime = static_cast<u32>((count * m_timePerTile));
    double len = sqrt(dx * dx + dy * dy);
    double t = static_cast<double>(totalTime);
    double vx = dx / len;
    m_flightDist = len;
    m_velScale = len / t;
    m_posX = vx;
    m_posY = dy / len;
    m_velX = vx;
    m_velY = dy / len;

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
        m_shadow->m_animWorker->m_notify(m_shadow);
        (static_cast<CLightFx*>(m_shadow->m_animWorker->m_logic))
            ->Activate(
                static_cast<const char*>(key + "_SHADOW"),
                static_cast<const char*>(key + "1"),
                5,
                1
            );
    }

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    return 1;
}

static inline CActHandler* ProjActLookup(i32 coord) {
    return (CActRegPool<CProjectile>::s_table.ResolveEntry(coord));
}

RVA(0x000df9a0, 0x102)
void CProjectile::FireActivation(i32 coord) {
    CActHandler* e = ProjActLookup(coord);
    if ((*e) != 0) {
        (this->*((*ProjActLookup(coord))))();
    }
}

RVA(0x000dfb00, 0x18d)
void CProjectile::RegisterType() {
    ACT_NAME_ID(id, "A")
    *ProjActLookup(id) = static_cast<CActHandler>(&CProjectile::DetachRenderObj);
}

// @early-stop
RVA(0x000dfd00, 0x70c)
void CProjectile::AdvanceMotion() {
    if (m_arrived != 0) {
        return;
    }

    if (m_kind == PICKUP_WINGZ) {
        CWwdGameObjectA* owner = m_object;
        CGruntzMgr* reg = g_gameReg;
        if (CGameLevel::PointInRect(&reg->m_viewBounds, owner->m_screenX, owner->m_screenY)) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_sound != NULL) {
            m_sound->StopAndRewind();
            m_sound = NULL;
        }
    }

    if (m_curX != m_targetX || m_curY != m_targetY) {

        if (m_kind == PICKUP_WINGZ) {
            ScanTargets(0);
        }
        m_posX = m_posX + static_cast<double>(g_frameDelta) * m_velX * m_velScale;
        m_posY = m_posY + static_cast<double>(g_frameDelta) * m_velY * m_velScale;
        i32 xRes = static_cast<i32>((m_roundX + m_posX));
        i32 yRes = static_cast<i32>((m_roundY + m_posY));
        i32 localX = xRes;
        if (m_velX > DATA_COMPGEN(0x001eaa90, 0.0)) {
            if (xRes > m_targetX) {
                localX = m_targetX;
                xRes = m_targetX;
            }
        } else if (m_velX < 0.0) {
            if (xRes < m_targetX) {
                localX = m_targetX;
                xRes = m_targetX;
            }
        }
        if (m_velY > 0.0) {
            if (yRes > m_targetY) {
                yRes = m_targetY;
            }
        } else if (m_velY < 0.0) {
            if (yRes < m_targetY) {
                yRes = m_targetY;
            }
        }
        m_curX = xRes;
        m_curY = yRes;
        i32 offX = 0;
        i32 offY = 0;
        if (m_isArcing != 0) {
            double dx = fabs(static_cast<double>(m_targetX) - m_posX);
            double dy = fabs(static_cast<double>(m_targetY) - m_posY);
            double dist = sqrt(dx * dx + dy * dy);
            if (dist
                >= m_flightDist
                       * DATA_COMPGEN(0x001eaa98, 0.9) || dist < m_flightDist * DATA_COMPGEN(0x001eaaa0, 0.1)) {
                offX = 0x4;
                offY = -0x4;
                if (m_wwdObject->m_animCursor.m_animation != m_frames[0]) {
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->m_animCursor.Setup(m_frames[0]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animCursor.Setup(m_frames[0]);
                    }
                }
            } else if (
                dist
                >= m_flightDist
                       * DATA_COMPGEN(0x001eaaa8, 0.8) || dist < m_flightDist * DATA_COMPGEN(0x001eaab0, 0.2)) {
                offX = 0x8;
                offY = -0x8;
                if (m_wwdObject->m_animCursor.m_animation != m_frames[1]) {
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->m_animCursor.Setup(m_frames[1]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animCursor.Setup(m_frames[1]);
                    }
                }
            } else if (
                dist
                >= m_flightDist
                       * DATA_COMPGEN(0x001eaab8, 0.7) || dist < m_flightDist * DATA_COMPGEN(0x001eaac0, 0.3)) {
                offX = 0xc;
                offY = -0xc;
                if (m_wwdObject->m_animCursor.m_animation != m_frames[2]) {
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->m_animCursor.Setup(m_frames[2]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animCursor.Setup(m_frames[2]);
                    }
                }
            } else if (
                dist
                >= m_flightDist
                       * DATA_COMPGEN(0x001eaac8, 0.6) || dist < m_flightDist * DATA_COMPGEN(0x001eaad0, 0.4)) {
                offX = 0x10;
                offY = -0x10;
                if (m_wwdObject->m_animCursor.m_animation != m_frames[3]) {
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->m_animCursor.Setup(m_frames[3]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animCursor.Setup(m_frames[3]);
                    }
                }
            } else {
                offX = 0x14;
                offY = -0x14;
                if (m_wwdObject->m_animCursor.m_animation != m_frames[4]) {
                    m_value = m_wwdObject->m_animCursor.m_animation;
                    m_wwdObject->m_animCursor.Setup(m_frames[4]);
                    if (m_shadow != NULL) {
                        m_shadow->m_animCursor.Setup(m_frames[4]);
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
        m_shadow->m_flags |= 0x10000;
        m_shadow = NULL;
    }
    m_arrived = 1;
    i32 tier = 0;
    if (m_kind != PICKUP_WINGZ) {
        CGruntzMgr* reg = g_gameReg;
        CMapMgr* plane = reg->m_tileGrid;
        i32 tileX = m_targetX >> TILE_SHIFT_PX;
        i32 tileY = m_targetY >> TILE_SHIFT_PX;
        u32 flags;
        if (static_cast<u32>(tileX) >= static_cast<u32>(plane->m_width)
            || static_cast<u32>(tileY) >= static_cast<u32>(plane->m_height)) {
            flags = 1;
        } else {
            flags = plane->m_rowInts[tileY][tileX * 7];
        }
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

                            if (CGameLevel::PointInRect(&reg->m_viewBounds, m_targetX, m_targetY)) {
                                CWwdGameObjectA* fx = reg->m_world->m_childGroup->CreateSprite(
                                    0,
                                    m_targetX,
                                    m_targetY,
                                    SORTKEY_ACTOR_BEHIND,
                                    "Particlez",
                                    0x40003
                                );
                                if (fx != NULL) {
                                    fx->ApplyName("LEVEL_DEATHSPLASH");
                                    fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            m_wwdObject->m_flags |= 0x10000;
                            return;
                    }
                }
            }
        } else {
            if (CGameLevel::PointInRect(&reg->m_viewBounds, m_targetX, m_targetY)) {
                CWwdGameObjectA* fx = reg->m_world->m_childGroup->CreateSprite(
                    0,
                    m_targetX,
                    m_targetY,
                    SORTKEY_ACTOR_BEHIND,
                    "Particlez",
                    0x40003
                );
                if (fx != NULL) {
                    fx->ApplyName("GAME_WATER");
                    fx->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
            m_wwdObject->m_flags |= 0x10000;
            return;
        }
    }
    CAniElement* sprite;
    if (tier != 0) {
        sprite = m_frames[PF_FALL];
        if (sprite == NULL) {
            goto noSprite;
        }
        goto setupSprite;
    }
    sprite = m_frames[PF_IMPACT];
    if (sprite == NULL) {
        goto noSprite;
    }
setupSprite:
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(sprite);
    return;
noSprite:
    m_wwdObject->m_flags |= 0x10000;
}

RVA(0x000e05e0, 0x4e)
i32 CProjectile::DetachRenderObj() {
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;

    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CWwdGameObjectA* r = m_wwdObject;
    if (r->m_animCursor.m_finished != 0 && r->m_animCursor.m_frameTicksLeft == 0) {
        r->m_flags |= 0x10000;
    }
    return 0;
}

// @early-stop
RVA(0x000e0650, 0x2b)
CBoomerang::CBoomerang(CGameObject* owner) : CProjectile(owner) {

    m_wwdObject->m_flags |= 0x2000002;
}

// @early-stop
// x87 scheduling residue: duration must remain live for the hold-window calculation.
// Retail duplicates it before scaling; cl instead loads the constants into deeper stack
// slots, while preserving the same six-branch CFG and the retail 0x8-byte frame.
RVA(0x000e0690, 0x1a9)
i32 CBoomerang::LoadProjectileSprites(
    PickupType kind,
    i32 a,
    i32 b,
    i32 sx,
    i32 sy,
    i32 t0,
    i32 t1
) {
    if (CProjectile::LoadProjectileSprites(kind, a, b, sx, sy, t0, t1) == 0) {
        return 0;
    }
    double duration = static_cast<double>(static_cast<u32>(m_timePerTile));
    double d = g_projPhase0 / (duration * g_boomTimeScale * m_flightDist);
    CWwdGameObjectA* owner = m_object;
    m_launchX = owner->m_screenX;
    m_launchY = owner->m_screenY;
    m_originX =
        (static_cast<double>(m_targetX) + static_cast<double>(owner->m_screenX)) * g_boomHalf;
    m_originY =
        (static_cast<double>(m_targetY) + static_cast<double>(owner->m_screenY)) * g_boomHalf;
    m_dirX = m_originX - static_cast<double>(m_launchX);
    m_dirY = m_originY - static_cast<double>(m_launchY);
    m_phase = 0.0;
    m_velScale = d;
    CGrunt* g = g_gameReg->m_cmdGrid->m_grid[15 * a + b];
    if (g != NULL) {
        g->m_holdWindowLo = static_cast<i32>((duration * m_flightDist * g_boomRetC3 - g_boomRetC4));
        g->m_holdWindowHi = 0;
        g->m_holdAnchorLo = g_frameTime;
        g->m_holdAnchorHi = 0;
        if (g->CoordCount() != 0) {
            POSITION pos = g->m_coordList.GetHeadPosition();
            while (pos != NULL) {
                void* data = g->m_coordList.GetNext(pos);
                if (data != NULL) {
                    CoordPoolNode* p = g_coordPool.NodeOf(data);
                    p->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = p;
                }
            }
            g->m_coordList.RemoveAll();
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
    if (m_launched == 0 && m_phase > g_projPhase0) {
        m_object->m_screenX = m_targetX;
        m_object->m_screenY = m_targetY;
        if (m_shadow != NULL) {
            m_shadow->m_screenX = m_targetX;
            m_shadow->m_screenY = m_targetY;
        }
        m_launched = 1;
    } else if (m_phase > g_projPhase1 && m_launched != 0) {
        ScanTargets(1);
        if (m_shadow != NULL) {
            m_shadow->m_flags |= 0x10000;
            m_shadow = NULL;
        }
        m_wwdObject->m_flags |= 0x10000;
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
    i32 tileY = 0;
    RECT box;
    box.left = m_object->m_screenX - 0x10;
    box.right = box.left + 0x20;
    box.top = m_object->m_screenY - 0x10;
    box.bottom = box.top + 0x20;
    i32 rowBase = 0;
    i32 colOff;
    i32 col;
    while (rowBase < 0x3c) {
        col = 0;
        colOff = rowBase;
        for (; col < 0xf; col++, colOff++) {
            CGrunt* g = g_gameReg->m_cmdGrid->m_grid[colOff];
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
            if (m_srcRow == tileY && m_srcCol == col) {

                if (impact != 0 && g->m_entranceCommitted != 0
                    && g->m_entranceReason == PICKUP_NONE) {
                    g->LoadGruntTypeTable(PICKUP_BOOMERANG, 1, 0, 0);
                }
                return;
            }

            i32 keyX = g->m_tileOwnerHi;
            i32 keyY = g->m_tileOwnerLo;
            for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {

                Coord* k = static_cast<Coord*>(m_hitList.GetNext(pos));
                if (k->m_x == keyX && k->m_y == keyY) {
                    return;
                }
            }

            Coord* slot = 0;
            CoordPoolNode* p = g_coordPool.m_freeHead;
            if (p->m_next != NULL) {
                slot = &p->m_coord;
                slot->m_x = keyX;
                slot->m_y = keyY;
                g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
            }
            m_hitList.AddTail(slot);
            g->StepCombatReaction(
                m_kind,
                1,
                m_srcRow,
                m_srcCol,
                m_targetId,
                m_ownerId,
                1,
                PICKUP_NONE
            );
        }
        tileY++;
        rowBase += 15;
    }
}

// @early-stop
RVA(0x000e0d40, 0x6c2)
i32 CProjectile::SerializeMove(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
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
            s->Read(&m_srcRow, sizeof(m_srcRow));
            s->Read(&m_srcCol, sizeof(m_srcCol));
            s->Read(&m_targetX, sizeof(m_targetX));
            s->Read(&m_targetY, sizeof(m_targetY));
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
            s->Read(&m_targetId, sizeof(m_targetId));
            s->Read(&m_ownerId, sizeof(m_ownerId));

            void* out;
            for (i32 ni = 0; ni < 7; ni++) {
                g_serialCounter++;
                s->Read(buf, SERIAL_NAME_LEN);
                if (strlen(buf) != 0) {
                    out = NULL;
                    reg->m_animRegistry->m_animations.Lookup(buf, out);
                    m_frames[ni] = static_cast<CAniElement*>(out);
                } else {
                    m_frames[ni] = NULL;
                }
            }

            g_serialCounter++;
            i32 count;
            s->Read(&count, sizeof(count));
            out = NULL;
            CGameObject* r;
            if (MapLookupById(reg->m_childGroup->m_map48, count, out) == 0) {
                r = NULL;
            } else if (out == NULL) {
                r = NULL;
            } else {

                r = (static_cast<CGameObject*>(out)->GetClassId() == CLASSID_SERIALREF)
                        ? static_cast<CGameObject*>(out)
                        : 0;
            }
            m_shadow = static_cast<CWwdGameObjectA*>(r);
            if (m_shadow == NULL && count != 0) {
                return 0;
            }

            s->Read(&count, sizeof(count));
            for (i32 ci = 0; ci < count; ci++) {
                CoordPoolNode* node = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                void* payload = 0;
                if (node->m_next != NULL) {
                    g_coordPool.m_freeHead = node->m_next;
                    payload = &node->m_coord;
                }
                s->Read(payload, 8);
                m_hitList.AddTail(payload);
            }
            break;
        }

        case SERIAL_SAVE: {
            s->Write(&m_kind, sizeof(m_kind));
            s->Write(&m_srcRow, sizeof(m_srcRow));
            s->Write(&m_srcCol, sizeof(m_srcCol));
            s->Write(&m_targetX, sizeof(m_targetX));
            s->Write(&m_targetY, sizeof(m_targetY));
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
            s->Write(&m_targetId, sizeof(m_targetId));
            s->Write(&m_ownerId, sizeof(m_ownerId));

            CAniElement** fp = m_frames;
            for (i32 fi = 0; fi < 7; fi++) {
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (*fp != NULL) {
                    strcpy(buf, reg->m_animRegistry->KeyOfValue(*fp));
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

    if (CMovingLogic::SerializeMove(s, mode, typeId, pObj) == 0) {
        goto fail;
    }
    if (s == NULL) {
        goto fail;
    }

    switch (mode) {
        case SERIAL_LOAD: {
            s->Read(buf, SERIAL_NAME_LEN);
            s->Read(m_blob, 0x10);
            CGameObject* obj = pObj;
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
            m_animWorker = obj->m_animWorker;
            if (strlen(buf) == 0) {
                m_value = NULL;
                return 1;
            }
            void* out = 0;
            m_animWorker->m_ownerCtx->m_animRegistry->m_animations.Lookup(buf, out);
            m_value = static_cast<CAniElement*>(out);
            return 1;
        }
        case SERIAL_SAVE: {
            char blob[SERIAL_NAME_LEN];
            memset(blob, 0, sizeof(blob));
            if (m_value != NULL) {
                strcpy(blob, m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value));
            }
            s->Write(blob, SERIAL_NAME_LEN);
            s->Write(m_blob, 0x10);
            return 1;
        }
    }
    return 1;
fail:
    return 0;
}

RVA(0x000e15d0, 0x155)
i32 CBoomerang::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
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
    return CProjectile::SerializeMove(ar, mode, typeId, pObj) ? 1 : 0;
}

static inline CActHandler* TBombLookup(i32 coord) {
    return (CActRegPool<CTimeBomb>::s_table.ResolveEntry(coord));
}

RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    CActHandler* e = TBombLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = TBombLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
    ACT_NAME_ID(id, "A")
    *(TBombLookup(id)) = static_cast<CActHandler>(&CTimeBomb::LoadAttributes);
}

// @early-stop

RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj), m_startTime(0), m_duration(0) {
    m_wwdObject->m_flags |= 0x2000002;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_PROJECTILE) {
        o->m_sortKey = SORTKEY_PROJECTILE;
        o->m_flags |= 0x20000;
    }
    m_wwdObject->ApplyName("GAME_TIMEBOMB");
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_value = m_wwdObject->m_animCursor.m_animation;
    if (m_object->m_damage > 0) {
        m_wwdObject->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_duration = static_cast<u32>(m_object->m_damage);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 1;
    } else {
        m_wwdObject->ApplyLookupGeometry("GAME_TIMEBOMBSLOW", 0);
        m_duration = g_buteMgr.GetDwordDef("Projectile", "TimeBombSlowTime", 0xfa0);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 0;
    }
    i32 cx = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 cy = m_object->m_screenY >> TILE_SHIFT_PX;
    CMapMgr* g = g_gameReg->m_tileGrid;
    if (cx < g->m_width && cy < g->m_height) {
        BrickzCell* row = g->m_rows[cy];
        row[cx].m_flags |= 0x1000000;
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
        BrickzCell* row = g->m_rows[cy];
        row[cx].m_flags &= ~0x1000000;
    }
}

// @early-stop
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::LoadAttributes() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & BRICKZ_BLOCKED_MASK) || (cell & 2)) {
        m_wwdObject->m_flags |= 0x10000;
        TBombGridClear(m_object);
        return 0;
    }
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    if (static_cast<i64>(g_frameTime) - m_startTime >= m_duration) {
        if (m_fastPhase == 0) {
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
            m_duration = g_buteMgr.GetDwordDef("Projectile", "TimeBombFastTime", 0x3e8);
            m_startTime = g_frameTime;
            m_fastPhase = 1;
        } else {
            m_wwdObject->m_flags |= 0x10000;
            TBombGridClear(m_object);
            g_gameReg->m_cmdGrid->LoadExplosionSprites(
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
i32 CTimeBomb::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
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
    if (!CUserLogic::SerializeMove(arc, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(sa, mode, typeId, pObj) ? 1 : 0;
}

RVA(0x000e2190, 0x83)
i32 CProjectile::LaunchSound(const char* key) {
    CGruntzMgr* reg;
    CDDrawSurfaceMgr* world;
    void* entry_ob;
    LeafCue* entry;
    if (m_sound != NULL) {
        goto fail;
    }
    reg = g_gameReg;
    if (reg->m_soundEnabled == 0) {
        goto fail;
    }
    world = reg->m_world;
    entry_ob = NULL;
    world->m_soundRegistry->m_cues.Lookup(key, entry_ob);
    entry = static_cast<LeafCue*>(entry_ob);
    if (entry == NULL) {
        goto fail;
    }
    if (entry->m_sound == NULL) {
        goto fail;
    }

    m_sound = static_cast<DirectSoundMgr*>(entry->m_sound->GetItem());
    if (m_sound != NULL) {
        m_sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
        return 1;
    }
fail:
    return 0;
}

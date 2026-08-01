#define CMOVINGLOGIC_INLINE_DTOR

#include <Mfc.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/Boomerang.h>
#include <Gruntz/LightFx.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/State.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActReg.h>
#include <Bute/ButeMgr.h>
#include <math.h>
#include <string.h>
#include <Gruntz/Brickz.h>
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/StatusBarUpdatersViews.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/AniElement.h>
#include <Wap32/zBitVec.h>
#include <Utils/MapTyped.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TypeKeyColl.h>

VTBL(CTimeBomb, 0x001e771c);
VTBL(CProjectile, 0x001e798c);
DATA(0x001f04b0)
const double g_movingLogicMin = -2147483647.0;
DATA(0x001f04b8)
const double g_movingLogicMax = 2147483646.0;

DATA(0x001eaa88)
const double g_motionZScale = 0.0;
DATA(0x001eab00)
const double g_projPhase1 = 6.2831854;
DATA(0x001f04e8)
u32 g_defaultZ = 0;
template<> DATA(0x0024c758)
CActReg CActRegPool<CProjectile>::s_table(2000, 2010);
template<> DATA(0x0024c780)
CActReg CActRegPool<CTimeBomb>::s_table(2000, 2010);

RVA(0x000126e0, 0x1fc)
CProjectile::CProjectile() {}

// @interleaver CTimeBomb::~CTimeBomb emitted in the 0x12xxx destructor COMDAT pool.
RVA_COMPGEN(0x00012980, 0x1e, ??_GCProjectile@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a40, 0x1e, ??_GCTimeBomb@@UAEPAXI@Z)
RVA_COMPGEN(0x00012a70, 0x44, ??1CTimeBomb@@UAE@XZ)

RVA(0x00013c70, 0x47)
void CMovingLogic::FinalizeStep(char*) {
    if (m_deferredCallback != 0) {
        if (m_gatedCallback != 0 && m_objAux->ActKey() == m_28) {
            (this->*m_gatedCallback)();
            m_gatedCallback = 0;
        }
        (this->*m_deferredCallback)();
        m_deferredCallback = 0;
        m_28 = 0x3e9;
    }
    AdvanceMotion();
}

// @early-stop
RVA(0x000dec60, 0x255)
CProjectile::CProjectile(CGameObject* owner) : CMovingLogic(owner) {

    i32 lo0 = m_objAux->m_2c;
    if (lo0 == 0) {
        Motion()->m_70 = g_movingLogicMin;
    } else {
        Motion()->m_70 = static_cast<double>(lo0);
    }
    i32 lo1 = m_objAux->m_34;
    if (lo1 == 0) {
        Motion()->m_78 = g_movingLogicMin;
    } else {
        Motion()->m_78 = static_cast<double>(lo1);
    }
    i32 hi0 = m_objAux->m_30;
    if (hi0 == 0) {
        Motion()->m_88 = g_movingLogicMax;
    } else {
        Motion()->m_88 = static_cast<double>(hi0);
    }
    i32 hi1 = m_objAux->m_38;
    if (hi1 == 0) {
        Motion()->m_90 = g_movingLogicMax;
    } else {
        Motion()->m_90 = static_cast<double>(hi1);
    }
    Motion()->SetParams(
        static_cast<double>(m_object->m_screenX),
        static_cast<double>(m_object->m_screenY),
        0.0,
        static_cast<double>(m_object->m_164),
        static_cast<double>(m_object->m_168),
        0.0,
        0.0,
        0.0,
        0.0,
        static_cast<double>(g_frameTime) * g_motionZScale,
        0.0
    );

    CMotionState* m = Motion();
    double z = static_cast<double>(g_defaultZ);
    m->m_d8 = z;
    m->m_e0 = z;
    m->m_e8 = z;
    m_148 = 0;
    m_14c = 0;
    m_object->m_moveMode = 7;

    CMovingLogic::AdvanceMotion();

    m_34 = owner;
    m_38 = static_cast<CWwdGameObjectA*>(owner);
    m_3c = owner->m_animWorker;
    m_38->m_flags |= 0x2000002;
    m_38->m_stateFlags |= 1;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xcf850) {
        o->m_sortKey = 0xcf850;
        o->m_flags |= 0x20000;
    }
    memset(&m_frames[0], 0, 0x1c);
    m_sound = 0;
    m_shadow = 0;
}

RVA(0x000def60, 0xbc)
CProjectile::~CProjectile() {
    if (m_sound != 0) {
        m_sound->StopAndRewind();
        m_sound = 0;
    }
    for (POSITION pos = m_hitList.GetHeadPosition(); pos != NULL;) {
        void* data = m_hitList.GetNext(pos);
        if (data != 0) {

            CoordPoolNode* node = g_coordPool.NodeOf(data);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_hitList.RemoveAll();
}

enum ProjectileKind {
    PROJ_BOOMERANG = 2,
    PROJ_GUNHAT = 9,
    PROJ_NERFGUN = 10,
    PROJ_ROCK = 11,
    PROJ_WELDER = 21,
    PROJ_WINGZ = 22,
};

// @early-stop
RVA(0x000df050, 0x6ed)
i32 CProjectile::LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1) {
    CString key;
    m_srcRow = a;
    m_targetX = (sx & ~0x1f) + 0x10;
    m_srcCol = b;
    m_kind = kind;
    m_targetY = (sy & ~0x1f) + 0x10;
    m_targetId = t0;
    m_ownerId = t1;

    CWwdGameObjectA* owner = m_object;
    double dx = static_cast<double>((m_targetX - owner->m_screenX));
    double dy = static_cast<double>((m_targetY - owner->m_screenY));
    i32 count = 1;

    switch (kind) {
        case PROJ_ROCK:
            key = "GRUNTZ_ROCKGRUNT_PROJECTILE";
            m_timePerTile = g_buteMgr.GetDwordDef("Projectile", "RockProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_GUNHAT:
            key = "GRUNTZ_GUNHATGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "GunhatProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_BOOMERANG:
            key = "GRUNTZ_BOOMERANGGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "BoomerangProjectileTimePerTile", 0xbb8);
            m_isArcing = 0;
            break;
        case PROJ_NERFGUN:
            key = "GRUNTZ_NERFGUNGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "NerfGunProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_WELDER:
            key = "GRUNTZ_WELDERGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WelderProjectileTimePerTile", 0xbb8);
            m_isArcing = 1;
            break;
        case PROJ_WINGZ: {
            key = "GRUNTZ_WINGZGRUNT_PROJECTILE";
            m_timePerTile =
                g_buteMgr.GetDwordDef("Projectile", "WingzProjectileTimePerTile", 0xbb8);
            LaunchSound("GRUNTZ_WINGZGRUNT_WINGZGRUNTLOOP");
            m_isArcing = 0;
            i32 ddx = (m_targetX >> 5) - (owner->m_screenX >> 5);
            if (ddx < 0) {
                ddx = -ddx;
            }
            i32 ddy = (m_targetY >> 5) - (owner->m_screenY >> 5);
            if (ddy < 0) {
                ddy = -ddy;
            }
            count = ddx;
            if (ddx <= ddy) {
                count = ddy;
            }
            break;
        }
        default:
            return 0;
    }

    CMapStringToPtr& map = m_38->OwnerMgr()->m_animRegistry->m_10;
    void* out;
    out = 0;
    map.Lookup(key + "1", out);
    m_frames[0] = static_cast<CAniElement*>(out);
    if (m_frames[0] == 0) {
        return 0;
    }
    out = 0;
    map.Lookup(key + "2", out);
    m_frames[1] = static_cast<CAniElement*>(out);
    out = 0;
    map.Lookup(key + "3", out);
    m_frames[2] = static_cast<CAniElement*>(out);
    out = 0;
    map.Lookup(key + "4", out);
    m_frames[3] = static_cast<CAniElement*>(out);
    out = 0;
    map.Lookup(key + "5", out);
    m_frames[4] = static_cast<CAniElement*>(out);
    out = 0;
    map.Lookup(key + "IMPACT", out);
    m_frames[PF_IMPACT] = static_cast<CAniElement*>(out);
    out = 0;
    map.Lookup(key + "FALL", out);
    m_frames[PF_FALL] = static_cast<CAniElement*>(out);

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_frames[0]);
    m_38->ApplyName(key + "_OBJECT");

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
    m_flightDist = len < 0.0 ? -len : len;
    m_curX = owner->m_screenX;
    m_curY = owner->m_screenY;
    m_arrived = 0;

    CDDrawChildGroup* factory = g_gameReg->m_world->m_childGroup;
    m_shadow =
        (factory
             ->CreateSprite(0, owner->m_screenX, owner->m_screenY, 0xcf84f, "LightFx", 0x2040003));
    if (m_shadow != 0) {
        m_shadow->m_animWorker->m_notify(m_shadow);
        (static_cast<CLightFx*>(m_shadow->m_animWorker->m_logic))
            ->Activate(
                static_cast<const char*>(key + "_SHADOW"),
                static_cast<const char*>(key + "1"),
                5,
                1
            );
    }

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    return 1;
}

static inline CActHandler* ProjActLookup(i32 coord) {
    return (CActRegPool<CProjectile>::s_table.ResolveEntry(coord));
}

static inline CString* ProjTypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return g_typeColl.Elem(key);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0) != 0) {
        return g_typeColl.Elem(key);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
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
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ProjTypeLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != 0) {
                nodes->~CString();
            }
            nodes++;
        }
        (*slot) = "A";
        g_typeCounter++;
    }

    *ProjActLookup(id) = static_cast<CActHandler>(&CProjectile::DetachRenderObj);
}

// @early-stop
RVA(0x000dfd00, 0x70c)
void CProjectile::AdvanceMotion() {
    if (m_arrived != 0) {
        return;
    }

    if (m_kind == 0x16) {
        CWwdGameObjectA* owner = m_object;
        CGruntzMgr* reg = g_gameReg;
        if (owner->m_screenX < reg->m_viewBounds.right && owner->m_screenX >= reg->m_viewBounds.left
            && owner->m_screenY < reg->m_viewBounds.bottom
            && owner->m_screenY >= reg->m_viewBounds.top) {
            LaunchSound("GRUNTZ_WINGZGRUNT_PROJECTILELOOP");
        } else if (m_sound != 0) {
            m_sound->StopAndRewind();
            m_sound = 0;
        }
    }

    if (m_curX != m_targetX || m_curY != m_targetY) {

        if (m_kind == 0x16) {
            ScanTargets(0);
        }
        m_posX = m_posX + static_cast<double>(g_frameDelta) * m_velX * m_velScale;
        m_posY = m_posY + static_cast<double>(g_frameDelta) * m_velY * m_velScale;
        i32 xRes = static_cast<i32>((m_roundX + m_posX));
        i32 yRes = static_cast<i32>((m_roundY + m_posY));
        i32 localX = xRes;
        if (m_velX > 0.0) {
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
            double mag = m_flightDist;
            if (dist >= mag * 0.9 || dist < mag * 0.1) {
                offX = 0x4;
                offY = -0x4;
                if (m_38->m_1a0.m_14 != m_frames[0]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[0]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[0]);
                    }
                }
            } else if (dist >= mag * 0.8 || dist < mag * 0.2) {
                offX = 0x8;
                offY = -0x8;
                if (m_38->m_1a0.m_14 != m_frames[1]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[1]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[1]);
                    }
                }
            } else if (dist >= mag * 0.7 || dist < mag * 0.3) {
                offX = 0xc;
                offY = -0xc;
                if (m_38->m_1a0.m_14 != m_frames[2]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[2]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[2]);
                    }
                }
            } else if (dist >= mag * 0.6 || dist < mag * 0.4) {
                offX = 0x10;
                offY = -0x10;
                if (m_38->m_1a0.m_14 != m_frames[3]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[3]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[3]);
                    }
                }
            } else {
                offX = 0x14;
                offY = -0x14;
                if (m_38->m_1a0.m_14 != m_frames[4]) {
                    m_value = m_38->m_1a0.m_14;
                    m_38->m_1a0.Setup(m_frames[4]);
                    if (m_shadow != 0) {
                        m_shadow->m_1a0.Setup(m_frames[4]);
                    }
                }
            }
        }
        m_object->m_screenX = offX + m_curX;
        m_object->m_screenY = offY + m_curY;
        if (m_shadow != 0) {
            m_shadow->m_screenX = localX;
            m_shadow->m_screenY = yRes;
        }
        return;
    }

    if (m_sound != 0) {
        m_sound->StopAndRewind();
        m_sound = 0;
    }
    ScanTargets(0);
    if (m_shadow != 0) {
        m_shadow->m_flags |= 0x10000;
        m_shadow = 0;
    }
    m_arrived = 1;
    i32 tier = 0;
    if (m_kind != 0x16) {
        CGruntzMgr* reg = g_gameReg;
        CMapMgr* plane = reg->m_tileGrid;
        i32 tileX = m_targetX >> 5;
        i32 tileY = m_targetY >> 5;
        u32 flags;
        if (static_cast<u32>(tileX) >= static_cast<u32>(plane->m_width)
            || static_cast<u32>(tileY) >= static_cast<u32>(plane->m_height)) {
            flags = 1;
        } else {
            flags = plane->m_rowInts[tileY][tileX * 7];
        }
        if (flags & 0x900) {

            if (m_targetX < reg->m_viewBounds.right && m_targetX >= reg->m_viewBounds.left
                && m_targetY < reg->m_viewBounds.bottom && m_targetY >= reg->m_viewBounds.top) {
                CWwdGameObjectA* fx =
                    reg->m_world->m_childGroup
                        ->CreateSprite(0, m_targetX, m_targetY, 0xcf84f, "Particlez", 0x40003);
                if (fx != 0) {
                    fx->ApplyName("GAME_WATER");
                    fx->ApplyLookupGeometry("GAME_WATER", 0);
                }
            }
            m_38->m_flags |= 0x10000;
            return;
        }
        if (flags & 0x2) {
            if (flags & 0x40) {
                tier = 1;
            } else {
                switch (reg->m_curState->m_levelType) {
                    case 4:
                    case 5:
                    case 8:
                        tier = 1;
                        break;
                    case 6:
                        break;
                    default:

                        if (m_targetX < reg->m_viewBounds.right
                            && m_targetX >= reg->m_viewBounds.left
                            && m_targetY < reg->m_viewBounds.bottom
                            && m_targetY >= reg->m_viewBounds.top) {
                            CWwdGameObjectA* fx = reg->m_world->m_childGroup->CreateSprite(
                                0,
                                m_targetX,
                                m_targetY,
                                0xcf84f,
                                "Particlez",
                                0x40003
                            );
                            if (fx != 0) {
                                fx->ApplyName("LEVEL_DEATHSPLASH");
                                fx->ApplyLookupGeometry("LEVEL_DEATHSPLASH", 0);
                            }
                        }
                        m_38->m_flags |= 0x10000;
                        return;
                }
            }
        }
    }
    CAniElement* sprite = (tier != 0) ? m_frames[PF_FALL] : m_frames[PF_IMPACT];
    if (sprite == 0) {
        m_38->m_flags |= 0x10000;
        return;
    }
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(sprite);
}

RVA(0x000e05e0, 0x4e)
i32 CProjectile::DetachRenderObj() {
    m_38->m_stateFlags &= ~1u;

    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* r = m_38;
    if (r->m_1a0.m_finished != 0 && r->m_1a0.m_frameTicksLeft == 0) {
        r->m_flags |= 0x10000;
    }
    return 0;
}

// @early-stop
RVA(0x000e08b0, 0x1de)
void CBoomerang::AdvanceMotion() {
    i32 impact = 0;
    if (m_launched == 0) {
        if (m_phase > g_projPhase0) {

            m_object->m_screenX = m_targetX;
            m_object->m_screenY = m_targetY;
            if (m_shadow != 0) {
                m_shadow->m_screenX = m_targetX;
                m_shadow->m_screenY = m_targetY;
            }
            m_launched = 1;
            goto step;
        }
    } else if (m_phase > g_projPhase1) {

        ScanTargets(1);
        if (m_shadow != 0) {
            m_shadow->m_flags |= 0x10000;
            m_shadow = 0;
        }
        m_38->m_flags |= 0x10000;
        return;
    }
step:
    ScanTargets(impact);

    double s = sin(m_phase);
    double c = cos(m_phase);
    double amp = static_cast<double>(g_frameDelta);
    double vx = -m_dirX;
    double vy = m_dirY;
    double px = m_originX + vy * m_velScale * s - vx * amp * c + m_phase;
    double py = m_originY + vy * amp * c + vx * m_velScale * s;
    m_posX = px;
    m_posY = py;
    m_phase = px;
    m_object->m_screenX = static_cast<i32>(m_posX);
    m_object->m_screenY = static_cast<i32>(m_posY);
    if (m_shadow != 0) {
        m_shadow->m_screenX = static_cast<i32>(m_posX);
        m_shadow->m_screenY = static_cast<i32>(m_posY);
    }
}

// @early-stop
RVA(0x000e0b10, 0x1bd)
void CProjectile::ScanTargets(i32 impact) {
    i32 tileY = 0;
    i32 projXlo = m_object->m_screenX - 0x10;
    i32 projXhi = projXlo + 0x20;
    i32 projYlo = m_object->m_screenY - 0x10;
    i32 projYhi = projYlo + 0x20;
    i32 rowBase = 0;
    i32 colOff;
    i32 col;
    do {
        col = 0;
        colOff = rowBase;
        for (; col < 0xf; col++, colOff++) {
            CGrunt* g = g_gameReg->m_cmdGrid->m_grid[colOff];
            if (g == 0) {
                continue;
            }
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            i32 gx = g->m_object->m_screenX - 7;
            i32 gy = g->m_object->m_screenY - 7;
            i32 gxhi = gx + 0xe;
            i32 gyhi = gy + 0xe;
            if (projXlo > gxhi) {
                continue;
            }
            if (projXhi < gx) {
                continue;
            }
            if (projYlo > gyhi) {
                continue;
            }
            if (projYhi < gy) {
                continue;
            }
            if (m_srcRow == tileY && m_srcCol == col) {

                if (impact != 0 && g->m_entranceCommitted != 0 && g->m_entranceReason == 0) {
                    g->LoadGruntTypeTable(2, 1, 0, 0);
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
            CoordPoolNode* p = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
            if (p->m_next != 0) {
                slot = &p->m_coord;
                slot->m_x = keyX;
                slot->m_y = keyY;
                g_coordPool.m_freeHead = p->m_next;
            }
            m_hitList.AddTail(slot);
            g->StepCombatReaction(m_kind, 1, m_srcRow, m_srcCol, m_targetId, m_ownerId, 1, 0);
        }
        rowBase += 15;
        tileY++;
    } while (rowBase < 0x3c);
}

// @early-stop
RVA(0x000e0d40, 0x6c2)
i32 CProjectile::SerializeMove(CFileMemBase* s, i32 mode, i32 typeId, CGameObject* pObj) {
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 7: {
            m_sound = 0;
            s->Read(&m_kind, 4);
            s->Read(&m_srcRow, 4);
            s->Read(&m_srcCol, 4);
            s->Read(&m_targetX, 4);
            s->Read(&m_targetY, 4);
            s->Read(&m_flightDist, 8);
            s->Read(&m_timePerTile, 4);
            s->Read(&m_velScale, 8);
            s->Read(&m_posX, 8);
            s->Read(&m_posY, 8);
            s->Read(&m_velX, 8);
            s->Read(&m_velY, 8);
            s->Read(&m_roundX, 8);
            s->Read(&m_roundY, 8);
            s->Read(&m_curX, 4);
            s->Read(&m_curY, 4);
            s->Read(&m_isArcing, 4);
            s->Read(&m_arrived, 4);
            s->Read(&m_targetId, 4);
            s->Read(&m_ownerId, 4);

            void* out;
            for (i32 ni = 0; ni < 7; ni++) {
                g_serialCounter++;
                s->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    out = 0;
                    reg->m_animRegistry->m_10.Lookup(buf, out);
                    m_frames[ni] = static_cast<CAniElement*>(out);
                } else {
                    m_frames[ni] = 0;
                }
            }

            g_serialCounter++;
            i32 key;
            s->Read(&key, 4);
            out = 0;
            CGameObject* r;
            if (MapLookupById(reg->m_childGroup->m_map48, key, out) == 0) {
                r = 0;
            } else if (out == 0) {
                r = 0;
            } else {

                r = (static_cast<CGameObject*>(out)->GetClassId() == CLASSID_SERIALREF)
                        ? static_cast<CGameObject*>(out)
                        : 0;
            }
            m_shadow = static_cast<CWwdGameObjectA*>(r);
            if (m_shadow == 0 && key != 0) {
                return 0;
            }

            i32 cnt;
            s->Read(&cnt, 4);
            for (i32 ci = 0; ci < cnt; ci++) {
                CoordPoolNode* node = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                void* payload = 0;
                if (node->m_next != 0) {
                    g_coordPool.m_freeHead = node->m_next;
                    payload = &node->m_coord;
                }
                s->Read(payload, 8);
                m_hitList.AddTail(payload);
            }
            break;
        }

        case 4: {
            s->Write(&m_kind, 4);
            s->Write(&m_srcRow, 4);
            s->Write(&m_srcCol, 4);
            s->Write(&m_targetX, 4);
            s->Write(&m_targetY, 4);
            s->Write(&m_flightDist, 8);
            s->Write(&m_timePerTile, 4);
            s->Write(&m_velScale, 8);
            s->Write(&m_posX, 8);
            s->Write(&m_posY, 8);
            s->Write(&m_velX, 8);
            s->Write(&m_velY, 8);
            s->Write(&m_roundX, 8);
            s->Write(&m_roundY, 8);
            s->Write(&m_curX, 4);
            s->Write(&m_curY, 4);
            s->Write(&m_isArcing, 4);
            s->Write(&m_arrived, 4);
            s->Write(&m_targetId, 4);
            s->Write(&m_ownerId, 4);

            CAniElement** fp = m_frames;
            for (i32 fi = 0; fi < 7; fi++) {
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (*fp != 0) {
                    strcpy(buf, reg->m_animRegistry->KeyOfValue(*fp));
                }
                s->Write(buf, 0x80);
                fp++;
            }

            g_serialCounter++;
            i32 n = 0;
            if (m_shadow != 0) {
                n = m_shadow->m_188;
            }
            s->Write(&n, 4);

            i32 v2 = m_hitList.GetCount();
            s->Write(&v2, 4);

            POSITION pos = m_hitList.GetHeadPosition();
            while (pos != 0) {
                s->Write(m_hitList.GetNext(pos), 8);
            }
            break;
        }
    }

    if (CMovingLogic::SerializeMove(s, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (s == 0) {
        return 0;
    }

    switch (mode) {
        case 7: {
            s->Read(buf, 0x80);
            s->Read(m_blob, 0x10);
            CGameObject* obj = pObj;
            m_34 = obj;
            m_38 = static_cast<CWwdGameObjectA*>(obj);
            m_3c = obj->m_animWorker;
            if (strlen(buf) == 0) {
                m_value = 0;
                return 1;
            }
            void* out = 0;
            m_3c->m_ownerCtx->m_animRegistry->m_10.Lookup(buf, out);
            m_value = static_cast<CAniElement*>(out);
            return 1;
        }
        case 4: {
            char blob[0x80];
            memset(blob, 0, sizeof(blob));
            if (m_value != 0) {
                strcpy(blob, m_3c->m_ownerCtx->m_animRegistry->KeyOfValue(m_value));
            }
            s->Write(blob, 0x80);
            s->Write(m_blob, 0x10);
            return 1;
        }
    }
    return 1;
}

static inline CActHandler* TBombLookup(i32 coord) {
    return (CActRegPool<CTimeBomb>::s_table.ResolveEntry(coord));
}

static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

static inline CString* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return g_typeColl.Elem(id);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0) != 0) {
        return g_typeColl.Elem(id);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
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
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    *(TBombLookup(id)) = static_cast<CActHandler>(&CTimeBomb::LoadAttributes);
}

// @early-stop

RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj)
    : CUserLogic(obj), CWapX(obj), m_startTime(0), m_duration(0) {
    m_38->m_flags |= 0x2000002;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xf) {
        o->m_sortKey = 0xf;
        o->m_flags |= 0x20000;
    }
    m_38->ApplyName("GAME_TIMEBOMB");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_value = m_38->m_1a0.m_14;
    if (m_object->m_120 > 0) {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_duration = static_cast<u32>(m_object->m_120);
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 1;
    } else {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBSLOW", 0);
        m_duration = static_cast<u32>(
            static_cast<i32>(g_buteMgr.GetDwordDef("Projectile", "TimeBombSlowTime", 0xfa0))
        );
        m_startTime = static_cast<u32>(g_frameTime);
        m_fastPhase = 0;
    }
    i32 cx = m_object->m_screenX >> 5;
    i32 cy = m_object->m_screenY >> 5;
    CMapMgr* g = g_gameReg->m_tileGrid;
    if (cx < g->m_width && cy < g->m_height) {
        BrickzCell* row = g->m_rows[cy];
        row[cx].m_0 |= 0x1000000;
    }
    m_object->m_124 = -1;
}

static inline i32 TBombGridCell(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        BrickzCell* row = g->m_rows[cy];
        return row[cx].m_0;
    }
    return 1;
}
static inline void TBombGridClear(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        BrickzCell* row = g->m_rows[cy];
        row[cx].m_0 &= ~0x1000000;
    }
}

// @early-stop
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::LoadAttributes() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & 0x939) || (cell & 2)) {
        m_38->m_flags |= 0x10000;
        TBombGridClear(m_object);
        return 0;
    }
    m_38->m_1a0.Advance(g_engineFrameDelta);

    if (static_cast<i64>(g_frameTime) - m_startTime >= m_duration) {
        if (m_fastPhase == 0) {
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
            m_duration = static_cast<u32>(
                static_cast<i32>(g_buteMgr.GetDwordDef("Projectile", "TimeBombFastTime", 0x3e8))
            );
            m_startTime = static_cast<u32>(static_cast<i32>(g_frameTime));
            m_fastPhase = 1;
        } else {
            m_38->m_flags |= 0x10000;
            TBombGridClear(m_object);
            g_gameReg->m_cmdGrid->LoadExplosionSprites(
                m_object->m_screenX,
                m_object->m_screenY,
                m_object->m_124,
                1
            );
        }
    }
    return 0;
}

RVA(0x000e2080, 0xc1)
i32 CTimeBomb::SerializeMove(CFileMemBase* arc, i32 mode, i32 typeId, CGameObject* pObj) {
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);
    i64* clock = &m_startTime;
    switch (mode) {
        case 7:
            sa->Read(clock, 8);
            clock++;
            sa->Read(clock, 8);
            break;
        case 4:
            sa->Write(clock, 8);
            clock++;
            sa->Write(clock, 8);
            break;
    }
    switch (mode) {
        case 7:
            sa->Read(&m_fastPhase, 4);
            break;
        case 4:
            sa->Write(&m_fastPhase, 4);
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
    if (m_sound != 0) {
        goto fail;
    }
    reg = g_gameReg;
    if (reg->m_soundEnabled == 0) {
        goto fail;
    }
    world = reg->m_world;
    entry_ob = 0;
    world->m_soundRegistry->m_10.Lookup(key, entry_ob);
    entry = static_cast<LeafCue*>(entry_ob);
    if (entry == 0) {
        goto fail;
    }
    if (entry->m_10 == 0) {
        goto fail;
    }

    m_sound = static_cast<DirectSoundMgr*>(entry->m_10->GetItem());
    if (m_sound != 0) {
        m_sound->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
        return 1;
    }
fail:
    return 0;
}

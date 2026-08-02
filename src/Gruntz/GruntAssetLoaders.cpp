#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>

#include <string.h>

static void GruntScratchTeardown() {
    CString* slot = (g_typeColl.Slots());
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
}

static const char s_dAnimKeyC[] = "C";
DATA(0x0020e158)
static const char s_DEATHZ_SQUASH[] = "GRUNTZ_DEATHZ_SQUASH";
DATA(0x0020e140)
static const char s_DEATHZ_SINK[] = "GRUNTZ_DEATHZ_SINK";
DATA(0x0020e128)
static const char s_DEATHZ_HOLE[] = "GRUNTZ_DEATHZ_HOLE";
DATA(0x0020e10c)
static const char s_DEATHZ_SHATTER[] = "GRUNTZ_DEATHZ_SHATTER";
static const char s_DEATHZ_FREEZE[] = "GRUNTZ_DEATHZ_FREEZE";
DATA(0x0020e0d8)
static const char s_DEATHZ_BURN[] = "GRUNTZ_DEATHZ_BURN";
DATA(0x0020e0bc)
static const char s_DEATHZ_QUICKFALL[] = "GRUNTZ_DEATHZ_QUICKFALL";
DATA(0x0020e0a4)
static const char s_DEATHZ_FALL[] = "GRUNTZ_DEATHZ_FALL";
DATA(0x0020e08c)
static const char s_DEATHZ_FALL2[] = "GRUNTZ_DEATHZ_FALL2";
DATA(0x0020e06c)
static const char s_DEATHZ_QUICKFALL2[] = "GRUNTZ_DEATHZ_QUICKFALL2";
DATA(0x0020e04c)
static const char s_DEATHZ_ELECTROCUTE[] = "GRUNTZ_DEATHZ_ELECTROCUTE";
static const char s_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";
DATA(0x0020e018)
static const char s_DEATHZ_KAROKE[] = "GRUNTZ_DEATHZ_KAROKE";
DATA(0x0020dffc)
static const char s_DEATHZ_EXPLODE[] = "GRUNTZ_DEATHZ_EXPLODE";
DATA(0x0020dfe4)
static const char s_EXITZ_DRAIN[] = "GRUNTZ_EXITZ_DRAIN";
static const char s_dEXITZ[] = "GRUNTZ_EXITZ";
static const char s_dExitKeyB[] = "B";
DATA(0x0020bcf4)
static const char s_NORMALGRUNT_DEATH[] = "GRUNTZ_NORMALGRUNT_DEATH";

enum GruntDeathType {
    DEATH_DROP = 0,
    DEATH_NORMAL = 1,
    DEATH_SQUASH = 2,
    DEATH_HOLE = 3,
    DEATH_SINK = 4,
    DEATH_MELT = 5,
    DEATH_SHATTER = 6,
    DEATH_BURN = 7,
    DEATH_FALL = 8,
    DEATH_ELECTROCUTE = 9,
    DEATH_KAROKE = 10,
    DEATH_EXPLODE = 11,
    DEATH_DRAIN = 12,
    DEATH_FALL2 = 14,
    DEATH_QUICKFALL = 15,
};

#define DEATH_FRAME()                                                                              \
    (m_wwdObject->m_animCursor.m_animation->m_records.GetSize() > 0                                \
         ? static_cast<CAniRecordView*>(m_wwdObject->m_animCursor.m_animation->m_records.GetAt(0)) \
               ->m_param                                                                           \
         : static_cast<CAniRecordView*>(0)->m_param)

#define DEATH_CUE(tag)                                                                             \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (CGameLevel::PointInBounds(                                                             \
                &_g->m_world->m_level->m_mainPlane->m_viewRect,                                    \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->m_cueSink->SpawnVoiceDriver(this, (tag), -1, 0, -1, -1);                           \
        }                                                                                          \
    } while (0)

// @early-stop
RVA(0x00060150, 0xdd0)
i32 CGrunt::LoadGruntDeathAnimations(i32 deathType, i32 killerSlot) {
    if (m_deathAnimStarted != 0) {
        return 0;
    }

    FinishActiveAction();
    StopStruckSlotSound();
    StopStruckVoiceSound();

    m_object->m_stateFlags &= ~8;
    m_deathAnimStarted = 1;
    m_health = 0;
    m_entranceCommitted = 0;

    if (m_healthSprite) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite) {
        m_powerupSprite->m_flags |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = 0;
    }

    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1);

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_dAnimKeyC);

    m_wwdObject->m_flags |= 1;
    if (m_object->m_sortKey != 0x15f90) {
        m_object->m_sortKey = 0x15f90;
        m_object->m_flags |= 0x20000;
    }

    if (killerSlot != -1) {
        m_killerSlot = killerSlot;
        g_gameReg->m_scoreHud->BumpWin(killerSlot, m_tileOwnerHi);
    }

    switch (deathType) {
        case DEATH_SQUASH:
            if (m_entranceReason == 1) {
                m_value = m_wwdObject->m_animCursor.m_animation;
                m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
                goto pathA;
            }
            m_poseDeath = static_cast<CAniElement*>(
                m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_SQUASH)
            );
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_SQUASH, DEATH_FRAME());
            DEATH_CUE(0x35b);
            goto finalize;

        case DEATH_DROP:
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            m_wwdObject->m_flags |= 0x10000;
            goto tail;

        case DEATH_SINK:
            m_poseDeath = static_cast<CAniElement*>(
                m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_SINK)
            );
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_SINK, DEATH_FRAME());
            DEATH_CUE(0x35a);
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            goto tail;

        case DEATH_HOLE:
            m_poseDeath = static_cast<CAniElement*>(
                m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_HOLE)
            );
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_HOLE, DEATH_FRAME());
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_SHATTER:
            m_poseDeath = static_cast<CAniElement*>(
                m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_SHATTER)
            );
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_FREEZE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;

        case DEATH_BURN:
            m_poseDeath = static_cast<CAniElement*>(
                m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_BURN)
            );
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_BURN, DEATH_FRAME());
            DEATH_CUE(0x352);
            goto finalize;

        case DEATH_QUICKFALL:
            m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
            m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
            m_poseDeath = static_cast<CAniElement*>(
                m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_QUICKFALL)
            );
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            if (m_object->m_sortKey != -1) {
                m_object->m_sortKey = -1;
                m_object->m_flags |= 0x20000;
            }
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_FALL: {
            CMapMgr* grid = g_gameReg->m_tileGrid;
            i32 attr =
                ((grid->m_rowInts[m_object->m_screenY >> 5]))[(m_object->m_screenX >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = static_cast<CAniElement*>(
                    m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_QUICKFALL)
                );
                tag = 0x357;
                if (m_object->m_sortKey != -1) {
                    m_object->m_sortKey = -1;
                    m_object->m_flags |= 0x20000;
                }
                m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
                m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
            } else {
                m_poseDeath = static_cast<CAniElement*>(
                    m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_FALL)
                );
            }
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            goto tail;
        }

        case DEATH_FALL2: {
            CMapMgr* grid = g_gameReg->m_tileGrid;
            i32 attr =
                ((grid->m_rowInts[m_object->m_screenY >> 5]))[(m_object->m_screenX >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = static_cast<CAniElement*>(
                    m_wwdObject->OwnerMgr()->m_animRegistry->LookupValue(s_DEATHZ_QUICKFALL2)
                );
                tag = 0x357;
                if (m_object->m_sortKey != -1) {
                    m_object->m_sortKey = -1;
                    m_object->m_flags |= 0x20000;
                }
                m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
                m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
            } else {
                void* out_ob = 0;
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(
                    s_DEATHZ_FALL2,
                    out_ob
                );
                m_poseDeath = static_cast<CAniElement*>(out_ob);
            }
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            goto tail;
        }

        case DEATH_ELECTROCUTE: {
            void* out_ob = 0;
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(
                s_DEATHZ_ELECTROCUTE,
                out_ob
            );
            m_poseDeath = static_cast<CAniElement*>(out_ob);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_ELECTROCUTE, DEATH_FRAME());
            DEATH_CUE(0x353);
            goto finalize;
        }

        case DEATH_MELT: {
            SnapToLastTile(1);
            void* out_ob = 0;
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(s_DEATHZ_MELT, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_MELT, DEATH_FRAME());
            DEATH_CUE(0x359);
            goto finalize;
        }

        case DEATH_KAROKE: {
            void* out_ob = 0;
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(s_DEATHZ_KAROKE, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->ApplyGeometryDirect(m_poseDeath, 0);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_KAROKE, DEATH_FRAME());
            DEATH_CUE(0x358);
            goto tail;
        }

        case DEATH_EXPLODE: {
            if (m_entranceReason == 1) {
                m_value = m_wwdObject->m_animCursor.m_animation;
                m_wwdObject->m_animCursor.Setup(m_poseDeath);
                goto pathA;
            }
            void* out_ob = 0;
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(s_DEATHZ_EXPLODE, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_poseDeath);
            m_wwdObject->ApplyLookupSprite(s_DEATHZ_EXPLODE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;
        }

        case DEATH_DRAIN: {
            void* out_ob = 0;
            m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(s_EXITZ_DRAIN, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob);
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_poseDeath);
            m_wwdObject->ApplyLookupSprite(s_dEXITZ, DEATH_FRAME());
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId(s_dExitKeyB);
            goto tail;
        }

        default:
            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_poseDeath);
            m_wwdObject->ApplyName(static_cast<const char*>(m_deathFrameSetName));
            {
                CGruntzMgr* g = g_gameReg;
                CCueRect* r = &g->m_world->m_level->m_mainPlane->m_viewRect;
                i32 x = m_object->m_screenX;
                i32 y = m_object->m_screenY;
                if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
                    g->m_cueSink->LoadGruntSpawnConfig(this, 3, -1, -1, -1);
                }
            }

            if (m_entranceReason == 0x14 && g_gameReg->m_gameMode != 1) {
                m_wwdObject->ApplyLookupGeometry(s_NORMALGRUNT_DEATH, 0);
                m_wwdObject->ApplyName(s_NORMALGRUNT_DEATH);
            }
            goto tail;
    }

pathA:
    m_wwdObject->ApplyName(static_cast<const char*>(m_deathFrameSetName));
    {
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_viewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_cueSink->LoadGruntSpawnConfig(this, 3, -1, -1, -1);
        }
    }
    deathType = DEATH_NORMAL;
    goto tail;

finalize:
    m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);

tail:

    if (m_entranceReason == 0x14 && g_gameReg->m_gameMode != 1) {
        SpawnTileFx(m_object->m_screenX, m_object->m_screenY, m_warpstoneAnchorIndex);
    }
    if (m_arrivalState == 0xd) {
        TryPowerupAtTile();
    }
    m_gruntKind = 0;
    m_deathType = deathType;
    return 0;
}

#undef DEATH_FRAME
#undef DEATH_CUE

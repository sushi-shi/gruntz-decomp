#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>

#include <new>
#include <string.h>

DATA(0x0020dfe4)
static char s_exitzDrain[] = "GRUNTZ_EXITZ_DRAIN";
DATA(0x0020dffc)
static char s_deathzExplode[] = "GRUNTZ_DEATHZ_EXPLODE";
DATA(0x0020e018)
static char s_deathzKaroke[] = "GRUNTZ_DEATHZ_KAROKE";
DATA(0x0020e04c)
static char s_deathzElectrocute[] = "GRUNTZ_DEATHZ_ELECTROCUTE";
DATA(0x0020e06c)
static char s_deathzQuickfall2[] = "GRUNTZ_DEATHZ_QUICKFALL2";
DATA(0x0020e08c)
static char s_deathzFall2[] = "GRUNTZ_DEATHZ_FALL2";
DATA(0x0020e0a4)
static char s_deathzFall[] = "GRUNTZ_DEATHZ_FALL";
DATA(0x0020e0bc)
static char s_deathzQuickfall[] = "GRUNTZ_DEATHZ_QUICKFALL";
DATA(0x0020e0d8)
static char s_deathzBurn[] = "GRUNTZ_DEATHZ_BURN";
DATA(0x0020e10c)
static char s_deathzShatter[] = "GRUNTZ_DEATHZ_SHATTER";
DATA(0x0020e128)
static char s_deathzHole[] = "GRUNTZ_DEATHZ_HOLE";
DATA(0x0020e140)
static char s_deathzSink[] = "GRUNTZ_DEATHZ_SINK";
DATA(0x0020e158)
static char s_deathzSquash[] = "GRUNTZ_DEATHZ_SQUASH";
static const char s_normalgruntDeath[] = "GRUNTZ_NORMALGRUNT_DEATH";

#define DEATH_FRAME()                                                                              \
    (static_cast<CAniRecordView*>(                                                                 \
         m_wwdObject->m_animationCursor.m_animation->m_records.GetSize() > 0                       \
             ? m_wwdObject->m_animationCursor.m_animation->m_records.GetAt(0)                      \
             : NULL                                                                                \
    )                                                                                              \
         ->m_param)

#define DEATH_CUE(tag)                                                                             \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (CGameLevel::PointInBounds(                                                             \
                &_g->m_world->m_level->m_mainPlane->m_planeViewRect,                               \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->m_voiceManager->PlayVoice(this, (tag), -1, 0, -1, -1);                             \
        }                                                                                          \
    } while (0)

static inline CAniElement* LookupAnim(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* found = NULL;
    MapLookup(map, name, found);
    return found;
}

// @early-stop
RVA(0x00060150, 0xdd0)
i32 CGrunt::LoadGruntDeathAnimations(GruntDeathType deathType, i32 killerPlayerIndex) {
    if (m_deathAnimStarted != false) {
        return 0;
    }

    FinishActiveAction();
    STOP_GRUNT_LOOP_SOUNDS;

    m_object->m_stateFlags &= ~SPRITE_STATE_FLASHING;
    m_deathAnimStarted = true;
    m_health = 0;
    m_entranceCommitted = false;

    HIDE_AND_CLEAR_GRUNT_SPRITE(m_healthSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_staminaSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toySprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_toyTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_wingzTimeSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_powerupSprite)
    HIDE_AND_CLEAR_GRUNT_SPRITE(m_selectedSprite)

    if (m_poweredUp != false && m_neighborValid == false) {
        RESET_GRUNT_POWERED_STATE(this)
    }
    m_triggerMgr->RemoveCellRecord(m_playerIndex, m_unitIndex, 1);

    SET_ANIMATION_ACT(DATA_COMPGEN(0x0020cc90, "C"));

    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    {
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_DEATH)
    }

    if (killerPlayerIndex != -1) {
        m_killerPlayerIndex = killerPlayerIndex;
        g_gameReg->m_gameStats->RecordKill(killerPlayerIndex, m_playerIndex);
    }

    switch (deathType) {
        case DEATH_SQUASH:
            if (m_entranceReason == PICKUP_BOMB) {
                SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
                goto pathA;
            }
            m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzSquash);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzSquash, DEATH_FRAME());
            DEATH_CUE(0x35b);
            goto finalize;

        case DEATH_DROP:
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            goto tail;

        case DEATH_SINK:
            m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzSink);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzSink, DEATH_FRAME());
            DEATH_CUE(0x35a);
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
            LoadGruntMovingDeathConfig();
            goto tail;

        case DEATH_HOLE:
            m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzHole);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzHole, DEATH_FRAME());
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_SHATTER:
            m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzShatter);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE("GRUNTZ_DEATHZ_FREEZE", DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;

        case DEATH_BURN:
            m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzBurn);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzBurn, DEATH_FRAME());
            DEATH_CUE(0x352);
            goto finalize;

        case DEATH_QUICKFALL:
            SNAP_OBJECT_TO_TILE_CENTER(m_object)
            m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzQuickfall);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzFall, DEATH_FRAME());
            {
                CWwdSpriteObject* o = m_object;
                SET_SORT_KEY_IF_CHANGED(o, -1)
            }
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_FALL: {
            CMapMgr* grid = g_gameReg->m_tileGrid;
            TileCollisionKind attr = static_cast<TileCollisionKind>((
                (grid->m_rowInts[m_object->m_screenY >> TILE_SHIFT_PX])
            )[(m_object->m_screenX >> TILE_SHIFT_PX) * 7 + 4]);
            i32 tag = 0x355;
            if (attr == TILEKIND_DEATHBRIDGE_UP || attr == TILEKIND_TOGGLEDEATHBRIDGE_UP) {
                m_poseDeath =
                    m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzQuickfall);
                tag = 0x357;
                {
                    CWwdSpriteObject* o = m_object;
                    SET_SORT_KEY_IF_CHANGED(o, -1)
                }
                SNAP_OBJECT_TO_TILE_CENTER(m_object)
            } else {
                m_poseDeath = m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzFall);
            }
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzFall, DEATH_FRAME());
            DEATH_CUE(tag);
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
            LoadGruntMovingDeathConfig();
            goto tail;
        }

        case DEATH_FALL2: {
            CMapMgr* grid = g_gameReg->m_tileGrid;
            TileCollisionKind attr = static_cast<TileCollisionKind>((
                (grid->m_rowInts[m_object->m_screenY >> TILE_SHIFT_PX])
            )[(m_object->m_screenX >> TILE_SHIFT_PX) * 7 + 4]);
            i32 tag = 0x355;
            if (attr == TILEKIND_DEATHBRIDGE_UP || attr == TILEKIND_TOGGLEDEATHBRIDGE_UP) {
                m_poseDeath =
                    m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(s_deathzQuickfall2);
                tag = 0x357;
                {
                    CWwdSpriteObject* o = m_object;
                    SET_SORT_KEY_IF_CHANGED(o, -1)
                }
                SNAP_OBJECT_TO_TILE_CENTER(m_object)
            } else {
                m_poseDeath = LookupAnim(
                    m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                    s_deathzFall2
                );
            }
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzFall, DEATH_FRAME());
            DEATH_CUE(tag);
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
            LoadGruntMovingDeathConfig();
            goto tail;
        }

        case DEATH_ELECTROCUTE: {
            m_poseDeath = LookupAnim(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                s_deathzElectrocute
            );
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzElectrocute, DEATH_FRAME());
            DEATH_CUE(0x353);
            goto finalize;
        }

        case DEATH_MELT: {
            SnapToLastTile(1);
            m_poseDeath = LookupAnim(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                "GRUNTZ_DEATHZ_MELT"
            );
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE("GRUNTZ_DEATHZ_MELT", DEATH_FRAME());
            DEATH_CUE(0x359);
            goto finalize;
        }

        case DEATH_KAROKE: {
            m_poseDeath =
                LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_deathzKaroke);
            SwitchAnimationAndMaybeAdvance(m_poseDeath, 0);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzKaroke, DEATH_FRAME());
            DEATH_CUE(0x358);
            goto tail;
        }

        case DEATH_EXPLODE: {
            if (m_entranceReason == PICKUP_BOMB) {
                SwitchAnimation(m_poseDeath);
                goto pathA;
            }
            m_poseDeath =
                LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_deathzExplode);
            SwitchAnimation(m_poseDeath);
            APPLY_LOOKUP_SPRITE_INLINE(s_deathzExplode, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;
        }

        case DEATH_DRAIN: {
            m_poseDeath =
                LookupAnim(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, s_exitzDrain);
            SwitchAnimation(m_poseDeath);
            APPLY_LOOKUP_SPRITE_INLINE("GRUNTZ_EXITZ", DEATH_FRAME());
            SET_ANIMATION_ACT("B");
            goto tail;
        }

        default:
            SwitchAnimation(m_poseDeath);
            APPLY_NAME_INLINE(static_cast<const char*>(m_deathFrameSetName));
            {
                CGruntzMgr* g = g_gameReg;
                CCueRect* r = &g->m_world->m_level->m_mainPlane->m_planeViewRect;
                i32 x = m_object->m_screenX;
                i32 y = m_object->m_screenY;
                if (::PtInRect(r, x, y)) {
                    g->m_voiceManager->PlayGruntVoiceCue(this, 3, -1, -1, -1);
                }
            }

            if (m_entranceReason == PICKUP_WARPSTONE && g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                SwitchAnimationByName("GRUNTZ_NORMALGRUNT_DEATH", 0);
                APPLY_NAME_INLINE("GRUNTZ_NORMALGRUNT_DEATH");
            }
            goto tail;
    }

pathA:
    APPLY_NAME_INLINE(static_cast<const char*>(m_deathFrameSetName));
    {
        CGruntzMgr* g = g_gameReg;
        if (CGameLevel::PointInBounds(
                &g->m_world->m_level->m_mainPlane->m_planeViewRect,
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_voiceManager->PlayGruntVoiceCue(this, 3, -1, -1, -1);
        }
    }
    deathType = DEATH_NORMAL;
    goto tail;

finalize:
    m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);

tail:

    if (m_entranceReason == PICKUP_WARPSTONE && g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        m_triggerMgr->SpawnTileFx(m_object->m_screenX, m_object->m_screenY, m_warpstoneAnchorIndex);
    }
    if (m_arrivalState == AI_TOOLTHIEF) {
        TryPowerupAtTile();
    }
    m_gruntKind = GRUNT_NORMAL;
    m_deathType = deathType;
    return 0;
}

#undef DEATH_FRAME
#undef DEATH_CUE

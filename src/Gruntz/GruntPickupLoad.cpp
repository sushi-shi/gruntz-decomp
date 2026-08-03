#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Utils/MapTyped.h>
#include <Wap32/ZVec.h>

#include <string.h>

#define PICKUP(key, idv)                                                                           \
    do {                                                                                           \
        geo = 0;                                                                                   \
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, (key), geo);              \
        id = (idv);                                                                                \
        m_pickupGeoSrc = geo;                                                                      \
    } while (0)

RVA(0x00065e80, 0x12b8)
i32 CGrunt::LoadPickupSprites(
    PickupType type,
    i32 forced,
    i32 helpCueId,
    i32 unused,
    i32 countStats
) {
    CAniElement* geo;
    if (m_gruntKind == GRUNT_CONVERSION || m_gruntKind == GRUNT_DEATHTOUCH) {
        return 0;
    }
    if (forced == 0) {
        if (m_entranceActive != 0) {
            return 0;
        }

        if (strcmp(*g_typeColl.GetNameRecord(m_objAux->ActKey()), "A") != 0
            && strcmp(*g_typeColl.GetNameRecord(m_objAux->ActKey()), s_codeD) != 0
            && strcmp(*g_typeColl.GetNameRecord(m_objAux->ActKey()), "E") != 0) {
            return 0;
        }
    }
    FinishActiveAction();
    if (m_entranceActive != 0) {
        return 0;
    }
    if (type >= PICKUP_COLORBRICK_FIRST && type <= PICKUP_BRICKZ_LAST) {
        PickupType st = m_entranceReason;
        if (st > PICKUP_EQUIPPABLE_LAST) {
            st = m_toolId;
        }
        if (st != PICKUP_BRICK) {
            return 0;
        }
    }
    if (m_coordToggle != 0) {
        return 0;
    }
    if (m_wingzEnabled != 0) {
        return 0;
    }
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
    }
    if (m_entranceReason == PICKUP_WARPSTONE) {
        if (type >= PICKUP_EQUIPPABLE_FIRST && type <= PICKUP_EQUIPPABLE_LAST) {
            return 0;
        }
        if (type >= PICKUP_TIMEDPOWERUP_FIRST && type <= PICKUP_TIMEDPOWERUP_LAST) {
            return 0;
        }
    }
    if (countStats != 0) {
        if (type >= PICKUP_EQUIPPABLE_FIRST && type <= PICKUP_EQUIPPABLE_LAST
            && type != PICKUP_WARPSTONE) {
            g_gameReg->m_scoreHud->m_toolzCount++;
            g_gameReg->m_scoreHud
                ->m_weaponPickupz[IDX(type) - IDX(PICKUP_BOMB) + 22 * m_tileOwnerHi]++;
        } else if (type >= PICKUP_TOYZ_FIRST && type <= PICKUP_TOYZ_LAST) {
            g_gameReg->m_scoreHud->m_toyzCount++;
            g_gameReg->m_scoreHud
                ->m_toyPickupz[IDX(type) - IDX(PICKUP_BABYWALKER) + 10 * m_tileOwnerHi]++;
        } else if (type >= PICKUP_TIMEDPOWERUP_FIRST && type <= PICKUP_TIMEDPOWERUP_LAST) {
            g_gameReg->m_scoreHud->m_powerupCount++;
            g_gameReg->m_scoreHud
                ->m_powerupPickupz[IDX(type) - IDX(PICKUP_GHOST) + 7 * m_tileOwnerHi]++;
        } else if (type >= PICKUP_CURSEZ_FIRST && type <= PICKUP_CURSEZ_LAST) {
            g_gameReg->m_scoreHud
                ->m_miscPickupz[IDX(type) - IDX(PICKUP_RANDOMCOLORZ) + 4 * m_tileOwnerHi]++;
        }
    }

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("J");

    i32 id = 0;
    forced = 0;
    switch (type) {
        case PICKUP_NONE:
            return 1;
        case PICKUP_BOMB:
            PICKUP("GRUNTZ_PICKUPS_BOMB", 0x3c3);
            break;
        case PICKUP_BOOMERANG:
            PICKUP("GRUNTZ_PICKUPS_BOOMERANG", 0x3c4);
            break;
        case PICKUP_BRICK:
            PICKUP("GRUNTZ_PICKUPS_BRICK", 0x3c5);
            break;
        case PICKUP_CLUB:
            PICKUP("GRUNTZ_PICKUPS_CLUB", 0x3c6);
            break;
        case PICKUP_GAUNTLETZ:
            PICKUP("GRUNTZ_PICKUPS_GAUNTLETZ", 0x3c7);
            break;
        case PICKUP_GLOVEZ:
            PICKUP("GRUNTZ_PICKUPS_GLOVEZ", 0x3c8);
            break;
        case PICKUP_GOOBER:
            PICKUP("GRUNTZ_PICKUPS_GOOBER", 0x3ca);
            break;
        case PICKUP_GRAVITYBOOTZ:
            PICKUP("GRUNTZ_PICKUPS_GRAVITYBOOTZ", 0x3cb);
            break;
        case PICKUP_GUNHAT:
            PICKUP("GRUNTZ_PICKUPS_GUNHAT", 0x3cc);
            break;
        case PICKUP_NERFGUN:
            PICKUP("GRUNTZ_PICKUPS_NERFGUN", 0x3cf);
            break;
        case PICKUP_ROCK:
            PICKUP("GRUNTZ_PICKUPS_ROCK", 0x3d1);
            break;
        case PICKUP_SHIELD:
            PICKUP("GRUNTZ_PICKUPS_SHIELD", 0x3d3);
            break;
        case PICKUP_SHOVEL:
            PICKUP("GRUNTZ_PICKUPS_SHOVEL", 0x3d4);
            break;
        case PICKUP_SPRING:
            PICKUP("GRUNTZ_PICKUPS_SPRING", 0x3d5);
            break;
        case PICKUP_SPY:
            PICKUP("GRUNTZ_PICKUPS_SPY", 0x3d6);
            break;
        case PICKUP_SWORD:
            PICKUP("GRUNTZ_PICKUPS_SWORD", 0x3d8);
            break;
        case PICKUP_TIMEBOMB:
            PICKUP("GRUNTZ_PICKUPS_TIMEBOMB", 0x3d9);
            break;
        case PICKUP_TOOB:
            PICKUP("GRUNTZ_PICKUPS_TOOB", 0x3da);
            break;
        case PICKUP_WAND:
            PICKUP("GRUNTZ_PICKUPS_WAND", 0x3db);
            break;
        case PICKUP_WARPSTONE:
            PICKUP("GRUNTZ_PICKUPS_WARPSTONE", 0x3dc);
            break;
        case PICKUP_WELDER:
            PICKUP("GRUNTZ_PICKUPS_WELDER", 0x3dd);
            break;
        case PICKUP_WINGZ:
            PICKUP("GRUNTZ_PICKUPS_WINGZ", 0x3de);
            break;
        case PICKUP_BABYWALKER:
            PICKUP("GRUNTZ_PICKUPS_BABYWALKER", 0x3c0);
            break;
        case PICKUP_BEACHBALL:
            PICKUP("GRUNTZ_PICKUPS_BEACHBALL", 0x3c1);
            break;
        case PICKUP_BIGWHEEL:
            PICKUP("GRUNTZ_PICKUPS_BIGWHEEL", 0x3c2);
            break;
        case PICKUP_GOKART:
            PICKUP("GRUNTZ_PICKUPS_GOKART", 0x3c9);
            break;
        case PICKUP_JACKINTHEBOX:
            PICKUP("GRUNTZ_PICKUPS_JACKINTHEBOX", 0x3cd);
            break;
        case PICKUP_JUMPROPE:
            PICKUP("GRUNTZ_PICKUPS_JUMPROPE", 0x3ce);
            break;
        case PICKUP_POGOSTICK:
            PICKUP("GRUNTZ_PICKUPS_POGOSTICK", 0x3d0);
            break;
        case PICKUP_SCROLL:
            PICKUP("GRUNTZ_PICKUPS_SCROLL", 0x3d2);
            break;
        case PICKUP_SQUEAKTOY:
            PICKUP("GRUNTZ_PICKUPS_SQUEAKTOY", 0x3d7);
            break;
        case PICKUP_YOYO:
            PICKUP("GRUNTZ_PICKUPS_YOYO", 0x3df);
            break;
        case PICKUP_REDBRICK:
            PICKUP("GRUNTZ_PICKUPS_REDBRICK", 0x3e3);
            break;
        case PICKUP_BLUEBRICK:
            PICKUP("GRUNTZ_PICKUPS_BLUEBRICK", 0x3e1);
            break;
        case PICKUP_GOLDBRICK:
            PICKUP("GRUNTZ_PICKUPS_GOLDBRICK", 0x3e2);
            break;
        case PICKUP_BLACKBRICK:
            PICKUP("GRUNTZ_PICKUPS_BLACKBRICK", 0x3e0);
            break;
        case PICKUP_MEGAPHONE: {
            CPlay* play = static_cast<CPlay*>(g_gameReg->m_curState);
            geo = 0;
            MapLookup(
                m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
                "GRUNTZ_PICKUPS_MEGAPHONE",
                geo
            );
            m_pickupGeoSrc = geo;
            PickupType n = static_cast<PickupType>(play->m_guts->GetActiveValue());
            if (countStats != 0) {
                if (n >= PICKUP_EQUIPPABLE_FIRST && n <= PICKUP_EQUIPPABLE_LAST
                    && n != PICKUP_WARPSTONE) {
                    g_gameReg->m_scoreHud->m_toolzCount++;
                    g_gameReg->m_scoreHud
                        ->m_weaponPickupz[IDX(n) - IDX(PICKUP_BOMB) + 22 * m_tileOwnerHi]++;
                } else if (n >= PICKUP_TOYZ_FIRST && n <= PICKUP_TOYZ_LAST) {
                    g_gameReg->m_scoreHud->m_toyzCount++;
                    g_gameReg->m_scoreHud
                        ->m_toyPickupz[IDX(n) - IDX(PICKUP_BABYWALKER) + 10 * m_tileOwnerHi]++;
                }
            }
            switch (n) {
                case PICKUP_BOMB:
                    id = 0x39b;
                    break;
                case PICKUP_BOOMERANG:
                    id = 0x39c;
                    break;
                case PICKUP_BRICK:
                    id = 0x39d;
                    break;
                case PICKUP_CLUB:
                    id = 0x39e;
                    break;
                case PICKUP_GAUNTLETZ:
                    id = 0x39f;
                    break;
                case PICKUP_GLOVEZ:
                    id = 0x3a0;
                    break;
                case PICKUP_GOOBER:
                    id = 0x3a2;
                    break;
                case PICKUP_GRAVITYBOOTZ:
                    id = 0x3a3;
                    break;
                case PICKUP_GUNHAT:
                    id = 0x3a4;
                    break;
                case PICKUP_NERFGUN:
                    id = 0x3a7;
                    break;
                case PICKUP_ROCK:
                    id = 0x3a9;
                    break;
                case PICKUP_SHIELD:
                    id = 0x3ab;
                    break;
                case PICKUP_SHOVEL:
                    id = 0x3ac;
                    break;
                case PICKUP_SPRING:
                    id = 0x3ad;
                    break;
                case PICKUP_SPY:
                    id = 0x3ae;
                    break;
                case PICKUP_SWORD:
                    id = 0x3b0;
                    break;
                case PICKUP_TIMEBOMB:
                    id = 0x3b1;
                    break;
                case PICKUP_TOOB:
                    id = 0x3b2;
                    break;
                case PICKUP_WAND:
                    id = 0x3b3;
                    break;
                case PICKUP_WARPSTONE:
                    id = 0x3b4;
                    break;
                case PICKUP_WELDER:
                    id = 0x3b5;
                    break;
                case PICKUP_WINGZ:
                    id = 0x3b6;
                    break;
                case PICKUP_BABYWALKER:
                    id = 0x398;
                    break;
                case PICKUP_BEACHBALL:
                    id = 0x399;
                    break;
                case PICKUP_BIGWHEEL:
                    id = 0x39a;
                    break;
                case PICKUP_GOKART:
                    id = 0x3a1;
                    break;
                case PICKUP_JACKINTHEBOX:
                    id = 0x3a5;
                    break;
                case PICKUP_JUMPROPE:
                    id = 0x3a6;
                    break;
                case PICKUP_POGOSTICK:
                    id = 0x3a8;
                    break;
                case PICKUP_SCROLL:
                    id = 0x3aa;
                    break;
                case PICKUP_SQUEAKTOY:
                    id = 0x3af;
                    break;
                case PICKUP_YOYO:
                    id = 0x3b7;
                    break;
                case PICKUP_REDBRICK:
                    id = 0x3bb;
                    break;
                case PICKUP_BLUEBRICK:
                    id = 0x3b9;
                    break;
                case PICKUP_GOLDBRICK:
                    id = 0x3ba;
                    break;
                case PICKUP_BLACKBRICK:
                    id = 0x3b8;
                    break;
                default:
                    break;
            }
            break;
        }
        case PICKUP_HEALTH1:
            PICKUP("GRUNTZ_PICKUPS_HEALTH1", 0x3e4);
            break;
        case PICKUP_HEALTH2:
            PICKUP("GRUNTZ_PICKUPS_HEALTH2", 0x3e5);
            break;
        case PICKUP_HEALTH3:
            PICKUP("GRUNTZ_PICKUPS_HEALTH3", 0x3e6);
            break;
        case PICKUP_GHOST:
            PICKUP("GRUNTZ_PICKUPS_GHOST", 0x3ed);
            break;
        case PICKUP_SUPERSPEED:
            PICKUP("GRUNTZ_PICKUPS_SUPERSPEED", 0x3e9);
            break;
        case PICKUP_INVULNERABILITY:
            PICKUP("GRUNTZ_PICKUPS_INVULNERABILITY", 0x3ec);
            break;
        case PICKUP_CONVERSION:
            PICKUP("GRUNTZ_PICKUPS_CONVERSION", 0x3e7);
            break;
        case PICKUP_DEATHTOUCH:
            PICKUP("GRUNTZ_PICKUPS_DEATHTOUCH", 0x3e8);
            break;
        case PICKUP_ROIDZ:
            PICKUP("GRUNTZ_PICKUPS_ROIDZ", 0x3ea);
            break;
        case PICKUP_REACTIVEARMOR:
            PICKUP("GRUNTZ_PICKUPS_REACTIVEARMOR", 0x3eb);
            break;
        case PICKUP_RANDOMCOLORZ:
            PICKUP("GRUNTZ_PICKUPS_RANDOMCOLORZ", 0x3f1);
            forced = 1;
            break;
        case PICKUP_SCREENSHAKE:
            PICKUP("GRUNTZ_PICKUPS_SCREENSHAKE", 0x3f0);
            forced = 1;
            break;
        case PICKUP_BLACKSCREEN:
            PICKUP("GRUNTZ_PICKUPS_BLACKSCREEN", 0x3ef);
            forced = 1;
            break;
        case PICKUP_MINICAM:
            PICKUP("GRUNTZ_PICKUPS_MINICAM", 0x3ee);
            forced = 1;
            break;
        case PICKUP_STOPWATCH:
            PICKUP("GRUNTZ_PICKUPS_STOPWATCH", 0x3bf);
            break;
        case PICKUP_COIN:
            PICKUP("GRUNTZ_PICKUPS_COIN", 0x3bf);
            break;
        case PICKUP_W:
            PICKUP("GRUNTZ_PICKUPS_W", 0x3bf);
            break;
        case PICKUP_A:
            PICKUP("GRUNTZ_PICKUPS_A", 0x3bf);
            break;
        case PICKUP_R:
            PICKUP("GRUNTZ_PICKUPS_R", 0x3bf);
            break;
        case PICKUP_P:
            PICKUP("GRUNTZ_PICKUPS_P", 0x3bf);
            break;
        case PICKUP_HELPBOX:
            PICKUP("GRUNTZ_PICKUPS_HELPBOX", 0x3be);
            break;
        default:
            return 0;
    }

    if (m_pickupGeoSrc == 0) {
        return 0;
    }
    if (id != 0) {
        CWwdGameObjectA* hud = m_object;
        CGruntzMgr* g = g_gameReg;
        if ((hud->m_screenX < g->m_viewBounds.right && hud->m_screenX >= g->m_viewBounds.left
             && hud->m_screenY < g->m_viewBounds.bottom && hud->m_screenY >= g->m_viewBounds.top)
            || forced != 0) {
            g->m_cueSink->SpawnVoiceDriver(this, id, -1, 0, -1, -1);
        }
    }
    m_helpCueId = helpCueId;
    m_entranceActive = 1;
    m_entrancePickup = type;
    if (m_healthSprite != 0) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite != 0) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite != 0) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite != 0) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_pickupGeoSrc);
    m_wwdObject->ApplyName("GRUNTZ_PICKUPS");
    return 1;
}

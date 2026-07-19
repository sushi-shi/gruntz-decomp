// GruntPickupLoad.cpp - CGrunt::LoadPickupSprites (@0x65e80), re-homed from
// src/Stub/ApiCallers.cpp. The pickup/powerup entrance-sprite loader: gate on the
// grunt-kind + entrance state, bump the per-owner pickup stat counters (when
// counting is requested), re-latch the "J" anim-set node, then a ~90-way switch on
// the pickup type resolves the matching GRUNTZ_PICKUPS_* sprite (the MEGAPHONE case
// runs a 2nd unit-type switch) and fires the on-screen entrance cue. Class-split
// into its own TU (matching-neutral); only OFFSETS + code bytes are load-bearing.
#include <Gruntz/Grunt.h>
#include <Gruntz/WwdGameRegPtr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Wap32/ZVec.h>
#include <Gruntz/TypeKeyColl.h>   // the shared CTypeKeyColl (g_typeColl @0x6bf650)
#include <rva.h>
#include <string.h>
#include <Bute/ButeMgr.h>      // CButeTree g_buteTree (Find)
#include <Gruntz/PickupType.h> // the shared object/pickup/grunt-kind type id space
#include <Globals.h>
// The entrance player's name/geometry setters are folded onto CEntranceAnimPlayer /
// CEntranceAnimSub (<Gruntz/Grunt.h>), reached as m_154->CacheFirstFrame /
// m_154->m_1a0.SetGeometry; the former per-TU CDDrawBlitParam / CAniAdvanceCursor /
// CGruntSprite facet views are gone.

// The id->name-slot type registry @0x6bf650: Resolve(id) returns a slot whose +0 is
// the interned anim-code name string (reloc-masked; Resolve is thunk 0x437c).
// CTypeKeyColl is the shared <Gruntz/TypeKeyColl.h> shape.

// The single-char anim-code key strings (reloc-masked .rodata).

// The per-owner pickup-stat block is the real CBattlezData (g_gameReg->m_scoreHud,
// +0x7c): the stat bands are its m_*Pickupz arrays (the PickupType id base folds into
// the retail displacement; see BattlezData.h). Re-read the member each use (matching
// retail's reload of g_gameReg + [+0x7c]). The former GruntPickupStats view is GONE.
#include <Gruntz/BattlezData.h>
#include <Gruntz/GruntPickupStats.h> // MegaHolder / MegaCounter (the MEGAPHONE count path)

// The looked-up sprite handle lands in the (otherwise dead) arg4 slot: taking its
// address pins a4 to its incoming stack slot, exactly as retail reuses [esp+0x20].
#define PICKUP(key, idv)                                                                           \
    do {                                                                                           \
        a4 = 0;                                                                                    \
        m_38->m_0c->m_animRegistry->m_10.Lookup((key), reinterpret_cast<void*&>(a4));                               \
        id = (idv);                                                                                \
        m_pickupGeoSrc = a4;                                                                       \
    } while (0)

// The object-type id (LoadPickupSprites `type` / the megaphone-announce unit type)
// that selects the GRUNTZ_PICKUPS_<NAME> entrance sprite is PickupType, the shared
// object/pickup/grunt-kind id space in <Gruntz/PickupType.h>. Values are byte-verified
// from the matched dispatch switch; each name is confirmed by its case's sprite
// string. Same immediates as the bare labels -> naming is matching-neutral.

// @early-stop
// Lookup out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md),
// amplified across the ~90 switch cases: prologue/switch-jump-table/case CFG/tail are all
// byte-correct in shape, but retail SINKS each case's `out=0` store + the `m_c` load PAST the
// two arg pushes (store lands at [esp+0x28], ours hoists it to [esp+0x20]) — an identical
// instruction multiset, 2-3 instrs permuted per Lookup. Source-invariant under /O2 (the
// documented Lookup-family coin-flip); counter-block regalloc (edx<->eax) is the same wall's
// tail. Logic complete (guard, stats counters, "J" relatch, 90-way pickup switch + MEGAPHONE
// inner unit-type switch, cue-rect gate, sprite retire, geometry apply). ~47%; final sweep.
RVA(0x00065e80, 0x12b8)
i32 CGrunt::LoadPickupSprites(i32 type, i32 a2, i32 a3, i32 a4, i32 a5) {
    if (m_gruntKind == 0x39 || m_gruntKind == 0x3a) {
        return 0;
    }
    if (a2 == 0) {
        if (m_entranceActive != 0) {
            return 0;
        }
        if (strcmp(*reinterpret_cast<const char**>((static_cast<zDArray*>(&g_typeColl))->IndexToPtr(reinterpret_cast<i32>(m_14->m_1c))), "A")
                != 0
            && strcmp(*reinterpret_cast<const char**>((static_cast<zDArray*>(&g_typeColl))->IndexToPtr(reinterpret_cast<i32>(m_14->m_1c))), s_codeD)
                   != 0
            && strcmp(*reinterpret_cast<const char**>((static_cast<zDArray*>(&g_typeColl))->IndexToPtr(reinterpret_cast<i32>(m_14->m_1c))), "E")
                   != 0) {
            return 0;
        }
    }
    PickupResetA();
    if (m_entranceActive != 0) {
        return 0;
    }
    if (type >= PICKUP_REDBRICK && type <= PICKUP_BLACKBRICK) {
        i32 st = m_entranceReason;
        if (st > 0x16) {
            st = m_19c;
        }
        if (st != 3) {
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
        PickupResetB(1, 0, 0);
    }
    if (m_entranceReason == 0x14) {
        if (type >= PICKUP_BOMB && type <= PICKUP_WINGZ) {
            return 0;
        }
        if (type >= PICKUP_GHOST && type <= PICKUP_REACTIVEARMOR) {
            return 0;
        }
    }
    if (a5 != 0) {
        if (type >= PICKUP_BOMB && type <= PICKUP_WINGZ && type != PICKUP_WARPSTONE) {
            g_gameReg->m_scoreHud->m_weaponCount++;
            g_gameReg->m_scoreHud->m_weaponPickupz[type - PICKUP_BOMB + 22 * m_tileOwnerHi]++;
        } else if (type >= PICKUP_BABYWALKER && type <= PICKUP_YOYO) {
            g_gameReg->m_scoreHud->m_toyzCount++;
            g_gameReg->m_scoreHud->m_toyPickupz[type - PICKUP_BABYWALKER + 10 * m_tileOwnerHi]++;
        } else if (type >= PICKUP_GHOST && type <= PICKUP_REACTIVEARMOR) {
            g_gameReg->m_scoreHud->m_powerupCount++;
            g_gameReg->m_scoreHud->m_powerupPickupz[type - PICKUP_GHOST + 7 * m_tileOwnerHi]++;
        } else if (type >= PICKUP_RANDOMCOLORZ && type <= PICKUP_MINICAM) {
            g_gameReg->m_scoreHud->m_miscPickupz[type - PICKUP_RANDOMCOLORZ + 4 * m_tileOwnerHi]++;
        }
    }

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("J");

    i32 id = 0;
    a2 = 0; // force-cue local (reuses the consumed arg2 slot)
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
            MegaHolder* mh = reinterpret_cast<MegaHolder*>(g_gameReg->m_2c);
            a4 = 0;
            m_38->m_0c->m_animRegistry->m_10.Lookup("GRUNTZ_PICKUPS_MEGAPHONE", reinterpret_cast<void*&>(a4));
            m_pickupGeoSrc = a4;
            i32 n = mh->m_2dc->M();
            if (a5 != 0) {
                if (n >= PICKUP_BOMB && n <= PICKUP_WINGZ && n != PICKUP_WARPSTONE) {
                    g_gameReg->m_scoreHud->m_weaponCount++;
                    g_gameReg->m_scoreHud->m_weaponPickupz[n - PICKUP_BOMB + 22 * m_tileOwnerHi]++;
                } else if (n >= PICKUP_BABYWALKER && n <= PICKUP_YOYO) {
                    g_gameReg->m_scoreHud->m_toyzCount++;
                    g_gameReg->m_scoreHud
                        ->m_toyPickupz[n - PICKUP_BABYWALKER + 10 * m_tileOwnerHi]++;
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
            a2 = 1;
            break;
        case PICKUP_SCREENSHAKE:
            PICKUP("GRUNTZ_PICKUPS_SCREENSHAKE", 0x3f0);
            a2 = 1;
            break;
        case PICKUP_BLACKSCREEN:
            PICKUP("GRUNTZ_PICKUPS_BLACKSCREEN", 0x3ef);
            a2 = 1;
            break;
        case PICKUP_MINICAM:
            PICKUP("GRUNTZ_PICKUPS_MINICAM", 0x3ee);
            a2 = 1;
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

    // shared tail
    if (m_pickupGeoSrc == 0) {
        return 0;
    }
    if (id != 0) {
        CGameObject* hud = m_10;
        WwdGameReg* g = g_gameReg;
        if ((hud->m_screenX < g->m_viewOriginR && hud->m_screenX >= g->m_viewOriginL
             && hud->m_screenY < g->m_viewOriginB && hud->m_screenY >= g->m_viewOriginT)
            || a2 != 0) {
            g->m_cueSink->CueA(this, id, -1, 0, -1, -1);
        }
    }
    m_1a4 = a3;
    m_entranceActive = 1;
    m_moveMode = type;
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
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(reinterpret_cast<CAniElement*>(m_pickupGeoSrc));
    m_38->ApplyName("GRUNTZ_PICKUPS");
    return 1;
}

SIZE_UNKNOWN(CTypeKeyColl);

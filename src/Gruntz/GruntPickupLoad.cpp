// GruntPickupLoad.cpp - CGrunt::LoadPickupSprites (@0x65e80), re-homed from
// src/Stub/ApiCallers.cpp. The pickup/powerup entrance-sprite loader: gate on the
// grunt-kind + entrance state, bump the per-owner pickup stat counters (when
// counting is requested), re-latch the "J" anim-set node, then a ~90-way switch on
// the pickup type resolves the matching GRUNTZ_PICKUPS_* sprite (the MEGAPHONE case
// runs a 2nd unit-type switch) and fires the on-screen entrance cue. Class-split
// into its own TU (matching-neutral); only OFFSETS + code bytes are load-bearing.
#include <Gruntz/Grunt.h>
#include <rva.h>
#include <string.h>
#include <Bute/ButeMgr.h> // CButeTree g_buteTree (Find)
#include <Globals.h>

extern CButeTree g_buteTree; // ?g_buteTree@@3VCButeTree@@A @0x6bf620

// The id->name-slot type registry @0x6bf650: Resolve(id) returns a slot whose +0 is
// the interned anim-code name string (reloc-masked; Resolve is thunk 0x437c).
struct CTypeKeyColl {
    void* Resolve(void* id); // thunk 0x437c (__thiscall ret 4)
};
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // ?g_typeColl@@3UCTypeKeyColl@@A

// The single-char anim-code key strings (reloc-masked .rodata).
DATA(0x0020a454)
extern char s_codeA[]; // "A"

// The per-owner pickup-stat block hung off g_gameReg+0x7c (reloc-masked). Reloaded
// through the global each use (matches retail: it reloads g_gameReg + [+0x7c]).
struct GruntPickupStats {
    char m_pad0[0x14];
    i32 m_14; // toyz counter
    i32 m_18; // weapon counter
    char m_pad1c[0x24 - 0x1c];
    i32 m_24; // powerup counter
};
#define STATS (*(GruntPickupStats**)((char*)g_gameReg + 0x7c))

// The MEGAPHONE announce path resolves a unit-type count through g_gameReg->m_2c
// (+0x2dc), a __thiscall counter (thunk 0x15fa). Reloc-masked.
struct MegaCounter {
    i32 Count(); // thunk 0x15fa (__thiscall ret 0)
};
struct MegaHolder {
    char m_pad0[0x2dc];
    MegaCounter* m_2dc; // +0x2dc
};

#define AT_I32(o) (*(i32*)((char*)this + (o)))
#define GREG_I32(g, o) (*(i32*)((char*)(g) + (o)))
// The looked-up sprite handle lands in the (otherwise dead) arg4 slot: taking its
// address pins a4 to its incoming stack slot, exactly as retail reuses [esp+0x20].
#define PICKUP(key, idv)                                                                           \
    do {                                                                                           \
        a4 = 0;                                                                                    \
        m_154->m_c->m_2c->m_10map.Lookup((key), (CSprite**)&a4);                                   \
        id = (idv);                                                                                \
        AT_I32(0x3d8) = a4;                                                                        \
    } while (0)

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
        if (strcmp(*(const char**)g_typeColl.Resolve(m_14->m_1c), s_codeA) != 0
            && strcmp(*(const char**)g_typeColl.Resolve(m_14->m_1c), s_codeD) != 0
            && strcmp(*(const char**)g_typeColl.Resolve(m_14->m_1c), s_codeE) != 0) {
            return 0;
        }
    }
    PickupResetA();
    if (m_entranceActive != 0) {
        return 0;
    }
    if (type >= 0x23 && type <= 0x26) {
        i32 st = m_entranceReason;
        if (st > 0x16) {
            st = AT_I32(0x19c);
        }
        if (st != 3) {
            return 0;
        }
    }
    if (AT_I32(0x234) != 0) {
        return 0;
    }
    if (m_wingzEnabled != 0) {
        return 0;
    }
    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        AT_I32(0x218) = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        PickupResetB(1, 0, 0);
    }
    if (m_entranceReason == 0x14) {
        if (type >= 1 && type <= 0x16) {
            return 0;
        }
        if (type >= 0x36 && type <= 0x3c) {
            return 0;
        }
    }
    if (a5 != 0) {
        if (type >= 1 && type <= 0x16 && type != 0x14) {
            STATS->m_18++;
            ((i32*)((char*)STATS + 0xd4))[type + 22 * m_tileOwnerHi]++;
        } else if (type >= 0x17 && type <= 0x20) {
            STATS->m_14++;
            ((i32*)((char*)STATS + 0x1dc))[type + 10 * m_tileOwnerHi]++;
        } else if (type >= 0x36 && type <= 0x3c) {
            STATS->m_24++;
            ((i32*)((char*)STATS + 0x200))[type + 7 * m_tileOwnerHi]++;
        } else if (type >= 0x3d && type <= 0x40) {
            ((i32*)((char*)STATS + 0x254))[type + 4 * m_tileOwnerHi]++;
        }
    }

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_codeJ);

    i32 id = 0;
    a2 = 0; // force-cue local (reuses the consumed arg2 slot)
    switch (type) {
        case 0:
            return 1;
        case 1:
            PICKUP("GRUNTZ_PICKUPS_BOMB", 0x3c3);
            break;
        case 2:
            PICKUP("GRUNTZ_PICKUPS_BOOMERANG", 0x3c4);
            break;
        case 3:
            PICKUP("GRUNTZ_PICKUPS_BRICK", 0x3c5);
            break;
        case 4:
            PICKUP("GRUNTZ_PICKUPS_CLUB", 0x3c6);
            break;
        case 5:
            PICKUP("GRUNTZ_PICKUPS_GAUNTLETZ", 0x3c7);
            break;
        case 6:
            PICKUP("GRUNTZ_PICKUPS_GLOVEZ", 0x3c8);
            break;
        case 7:
            PICKUP("GRUNTZ_PICKUPS_GOOBER", 0x3ca);
            break;
        case 8:
            PICKUP("GRUNTZ_PICKUPS_GRAVITYBOOTZ", 0x3cb);
            break;
        case 9:
            PICKUP("GRUNTZ_PICKUPS_GUNHAT", 0x3cc);
            break;
        case 10:
            PICKUP("GRUNTZ_PICKUPS_NERFGUN", 0x3cf);
            break;
        case 11:
            PICKUP("GRUNTZ_PICKUPS_ROCK", 0x3d1);
            break;
        case 12:
            PICKUP("GRUNTZ_PICKUPS_SHIELD", 0x3d3);
            break;
        case 13:
            PICKUP("GRUNTZ_PICKUPS_SHOVEL", 0x3d4);
            break;
        case 14:
            PICKUP("GRUNTZ_PICKUPS_SPRING", 0x3d5);
            break;
        case 15:
            PICKUP("GRUNTZ_PICKUPS_SPY", 0x3d6);
            break;
        case 16:
            PICKUP("GRUNTZ_PICKUPS_SWORD", 0x3d8);
            break;
        case 17:
            PICKUP("GRUNTZ_PICKUPS_TIMEBOMB", 0x3d9);
            break;
        case 18:
            PICKUP("GRUNTZ_PICKUPS_TOOB", 0x3da);
            break;
        case 19:
            PICKUP("GRUNTZ_PICKUPS_WAND", 0x3db);
            break;
        case 20:
            PICKUP("GRUNTZ_PICKUPS_WARPSTONE", 0x3dc);
            break;
        case 21:
            PICKUP("GRUNTZ_PICKUPS_WELDER", 0x3dd);
            break;
        case 22:
            PICKUP("GRUNTZ_PICKUPS_WINGZ", 0x3de);
            break;
        case 23:
            PICKUP("GRUNTZ_PICKUPS_BABYWALKER", 0x3c0);
            break;
        case 24:
            PICKUP("GRUNTZ_PICKUPS_BEACHBALL", 0x3c1);
            break;
        case 25:
            PICKUP("GRUNTZ_PICKUPS_BIGWHEEL", 0x3c2);
            break;
        case 26:
            PICKUP("GRUNTZ_PICKUPS_GOKART", 0x3c9);
            break;
        case 27:
            PICKUP("GRUNTZ_PICKUPS_JACKINTHEBOX", 0x3cd);
            break;
        case 28:
            PICKUP("GRUNTZ_PICKUPS_JUMPROPE", 0x3ce);
            break;
        case 29:
            PICKUP("GRUNTZ_PICKUPS_POGOSTICK", 0x3d0);
            break;
        case 30:
            PICKUP("GRUNTZ_PICKUPS_SCROLL", 0x3d2);
            break;
        case 31:
            PICKUP("GRUNTZ_PICKUPS_SQUEAKTOY", 0x3d7);
            break;
        case 32:
            PICKUP("GRUNTZ_PICKUPS_YOYO", 0x3df);
            break;
        case 0x23:
            PICKUP("GRUNTZ_PICKUPS_REDBRICK", 0x3e3);
            break;
        case 0x24:
            PICKUP("GRUNTZ_PICKUPS_BLUEBRICK", 0x3e1);
            break;
        case 0x25:
            PICKUP("GRUNTZ_PICKUPS_GOLDBRICK", 0x3e2);
            break;
        case 0x26:
            PICKUP("GRUNTZ_PICKUPS_BLACKBRICK", 0x3e0);
            break;
        case 0x32: {
            MegaHolder* mh = *(MegaHolder**)((char*)g_gameReg + 0x2c);
            a4 = 0;
            m_154->m_c->m_2c->m_10map.Lookup("GRUNTZ_PICKUPS_MEGAPHONE", (CSprite**)&a4);
            AT_I32(0x3d8) = a4;
            i32 n = mh->m_2dc->Count();
            if (a5 != 0) {
                if (n >= 1 && n <= 0x16 && n != 0x14) {
                    STATS->m_18++;
                    ((i32*)((char*)STATS + 0xd4))[n + 22 * m_tileOwnerHi]++;
                } else if (n >= 0x17 && n <= 0x20) {
                    STATS->m_14++;
                    ((i32*)((char*)STATS + 0x1dc))[n + 10 * m_tileOwnerHi]++;
                }
            }
            switch (n) {
                case 1:
                    id = 0x39b;
                    break;
                case 2:
                    id = 0x39c;
                    break;
                case 3:
                    id = 0x39d;
                    break;
                case 4:
                    id = 0x39e;
                    break;
                case 5:
                    id = 0x39f;
                    break;
                case 6:
                    id = 0x3a0;
                    break;
                case 7:
                    id = 0x3a2;
                    break;
                case 8:
                    id = 0x3a3;
                    break;
                case 9:
                    id = 0x3a4;
                    break;
                case 10:
                    id = 0x3a7;
                    break;
                case 11:
                    id = 0x3a9;
                    break;
                case 12:
                    id = 0x3ab;
                    break;
                case 13:
                    id = 0x3ac;
                    break;
                case 14:
                    id = 0x3ad;
                    break;
                case 15:
                    id = 0x3ae;
                    break;
                case 16:
                    id = 0x3b0;
                    break;
                case 17:
                    id = 0x3b1;
                    break;
                case 18:
                    id = 0x3b2;
                    break;
                case 19:
                    id = 0x3b3;
                    break;
                case 20:
                    id = 0x3b4;
                    break;
                case 21:
                    id = 0x3b5;
                    break;
                case 22:
                    id = 0x3b6;
                    break;
                case 23:
                    id = 0x398;
                    break;
                case 24:
                    id = 0x399;
                    break;
                case 25:
                    id = 0x39a;
                    break;
                case 26:
                    id = 0x3a1;
                    break;
                case 27:
                    id = 0x3a5;
                    break;
                case 28:
                    id = 0x3a6;
                    break;
                case 29:
                    id = 0x3a8;
                    break;
                case 30:
                    id = 0x3aa;
                    break;
                case 31:
                    id = 0x3af;
                    break;
                case 32:
                    id = 0x3b7;
                    break;
                case 35:
                    id = 0x3bb;
                    break;
                case 36:
                    id = 0x3b9;
                    break;
                case 37:
                    id = 0x3ba;
                    break;
                case 38:
                    id = 0x3b8;
                    break;
                default:
                    break;
            }
            break;
        }
        case 0x33:
            PICKUP("GRUNTZ_PICKUPS_HEALTH1", 0x3e4);
            break;
        case 0x34:
            PICKUP("GRUNTZ_PICKUPS_HEALTH2", 0x3e5);
            break;
        case 0x35:
            PICKUP("GRUNTZ_PICKUPS_HEALTH3", 0x3e6);
            break;
        case 0x36:
            PICKUP("GRUNTZ_PICKUPS_GHOST", 0x3ed);
            break;
        case 0x37:
            PICKUP("GRUNTZ_PICKUPS_SUPERSPEED", 0x3e9);
            break;
        case 0x38:
            PICKUP("GRUNTZ_PICKUPS_REACTIVEARMOR", 0x3eb);
            break;
        case 0x39:
            PICKUP("GRUNTZ_PICKUPS_CONVERSION", 0x3e7);
            break;
        case 0x3a:
            PICKUP("GRUNTZ_PICKUPS_DEATHTOUCH", 0x3e8);
            break;
        case 0x3b:
            PICKUP("GRUNTZ_PICKUPS_ROIDZ", 0x3ea);
            break;
        case 0x3c:
            PICKUP("GRUNTZ_PICKUPS_INVULNERABILITY", 0x3ec);
            break;
        case 0x3d:
            PICKUP("GRUNTZ_PICKUPS_RANDOMCOLORZ", 0x3f1);
            a2 = 1;
            break;
        case 0x3e:
            PICKUP("GRUNTZ_PICKUPS_SCREENSHAKE", 0x3f0);
            a2 = 1;
            break;
        case 0x3f:
            PICKUP("GRUNTZ_PICKUPS_BLACKSCREEN", 0x3ef);
            a2 = 1;
            break;
        case 0x40:
            PICKUP("GRUNTZ_PICKUPS_MINICAM", 0x3ee);
            a2 = 1;
            break;
        case 0x4b:
            PICKUP("GRUNTZ_PICKUPS_STOPWATCH", 0x3bf);
            break;
        case 0x50:
            PICKUP("GRUNTZ_PICKUPS_COIN", 0x3bf);
            break;
        case 0x5a:
            PICKUP("GRUNTZ_PICKUPS_W", 0x3bf);
            break;
        case 0x5b:
            PICKUP("GRUNTZ_PICKUPS_A", 0x3bf);
            break;
        case 0x5c:
            PICKUP("GRUNTZ_PICKUPS_R", 0x3bf);
            break;
        case 0x5d:
            PICKUP("GRUNTZ_PICKUPS_P", 0x3bf);
            break;
        case 0x5e:
            PICKUP("GRUNTZ_PICKUPS_HELPBOX", 0x3be);
            break;
        default:
            return 0;
    }

    // shared tail
    if (AT_I32(0x3d8) == 0) {
        return 0;
    }
    if (id != 0) {
        CGruntHud* hud = m_10;
        WwdGameReg* g = g_gameReg;
        if ((hud->m_5c < GREG_I32(g, 0x144) && hud->m_5c >= GREG_I32(g, 0x13c)
             && hud->m_60 < GREG_I32(g, 0x148) && hud->m_60 >= GREG_I32(g, 0x140))
            || a2 != 0) {
            g->m_60->CueA(this, id, -1, 0, -1, -1);
        }
    }
    AT_I32(0x1a4) = a3;
    m_entranceActive = 1;
    AT_I32(0x1a0) = type;
    if (m_healthSprite != 0) {
        m_healthSprite->m_8 |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite != 0) {
        m_staminaSprite->m_8 |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite != 0) {
        m_toySprite->m_8 |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite != 0) {
        m_toyTimeSprite->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite != 0) {
        m_wingzTimeSprite->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    m_prevEntranceDesc = (i32)m_154->m_1b4;
    m_154->m_1a0.SetGeometry(AT_I32(0x3d8));
    m_154->SetAnimName("GRUNTZ_PICKUPS");
    return 1;
}

SIZE_UNKNOWN(CTypeKeyColl);
SIZE_UNKNOWN(GruntPickupStats);
SIZE_UNKNOWN(MegaCounter);
SIZE_UNKNOWN(MegaHolder);

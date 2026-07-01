// GruntCombatAnim.cpp - CGrunt::LoadGruntCombatAnimations (0x597a0), re-homed from
// src/Stub/ApiCallers.cpp. The 8-arg combat-hit reaction dispatch (__thiscall, ret 0x20):
//   * kind/reason guard (m_gruntKind 0x38 / reason 1);
//   * a7==0x39 conversion path: bump the struck enemy's health + fire GAME_CONVERSIONHIT;
//   * a hit-type byte-table lookup (g_hitTable[reason*23 + a0]) with an optional
//     handicap halving, then a duration scale for kind 0x3c (death-touch damage);
//   * self health decrement + kill dispatch (ApplyCellEffect);
//   * on-screen visibility gate then a hit/block sprite resolve: a7==0x3a fires
//     GAME_DEATHTOUCHHIT, else a reason(m_170)-switch over 9/0xc/0xe/0x12 plus an
//     a0-switch resolving the 22 GRUNTZ_NORMALGRUNT_IMPACT*/BLOCK* / SPRING / TOOB
//     sound cues into distinct out slots, then a kill-clock-gated ConfigureItem cue;
//   * the block path (a0 in {6,0xa,0x16}): the g_typeColl/g_typeNodes/g_typeCount
//     name-registry free-list rebuild + an "O" type-name gate, then an x87 angle-octant
//     direction resolver copying g_dirVec[] triples into CGrunt+0x43c, a tile-to-tile
//     occupancy + diagonal-corner move check, the arrival commit, and the knockback
//     trajectory (bute KnockBackTimePerTile) + coord free-list recycle tail.
// The CGrunt/tile-mgr field bag is addressed by raw offset (naming-independent-codegen
// exception); all engine callees + globals are external (reloc-masked).
//
// @early-stop
// Logic reconstructed in full - verified vs `llvm-objdump -dr` the prologue/guards, the
// a7==0x39 conversion path (grid index leal(edx,edx,2)+0x1c(ecx,eax,4), the
// m_158->m_c->m_28->m_30 chain, CheckSpawn, the +0x19/clamp-0x64 heal) are byte-identical.
// Frame-size packing wall: cl lays the ~38 distinct Lookup out-slots into a 0xac frame,
// retail into 0xb0 (one extra 4-byte slot). That 4-byte delta shifts every arg
// ([esp+0xdc] vs [esp+0xe0] ...) + out-slot displacement; cl also hoists `mov eax,1`
// into the prologue (immediate `cmp ...,1` in retail). Both are cl slot-allocator /
// constant-hoist choices structured C++ can't force; the switch tail-merge + FP octant
// block layout compound it. Same family as LoadGruntDeathAnimations. Final-sweep candidate.
#include <Bute/ButeMgr.h>  // canonical CButeMgr (one shape)
#include <Bute/ButeTree.h> // canonical CButeTree (one shape)
#include <Ints.h>
#include <string.h> // inline strcmp of the type name

#include <rva.h>

extern "C" int rand(); // 0x11fee0 the engine LCG rand()
extern "C" double sqrt(double);

#pragma intrinsic(strcmp, sqrt)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

struct CGruntCombat;

// The struck enemy's launch-sound cue (Lookup out-param): m_10 owner runs ConfigureItem,
// m_14 last-fire clock, m_18 the cooldown window.
struct CombatItemOwner {
    void ConfigureItem(i32 tag, i32 a, i32 b, i32 c); // 0x1360d0 (__thiscall, 4 args)
};
struct CombatCue {
    char m_pad0[0x10];
    CombatItemOwner* m_10; // +0x10
    i32 m_14;              // +0x14 last-fire clock
    i32 m_18;              // +0x18 cooldown window
};
struct CombatCueMap {
    i32 Lookup(const char* key, CombatCue** out); // 0x1b8438 (__thiscall, ret 8)
};
struct CombatSprInner {
    char m_pad0[0x10];
    CombatCueMap m_10; // +0x10 the launch-sound lookup map
};
struct CombatSprCat {
    char m_pad0[0x28];
    CombatSprInner* m_28; // +0x28
};

// The board tile grid (g->m_70): row table at +0x8, dims at +0xc/+0x10. Each cell is
// 7 dwords (28 bytes); cell[0] is the occupancy word, byte cell+3 the 0x20 flag, cell+4
// the owner id.
struct CombatGrid {
    char m_pad0[0x8];
    i32** m_8; // +0x08 row table (m_8[y] -> row of cells)
    i32 m_c;   // +0x0c width
    i32 m_10;  // +0x10 height
};

// The manager singleton (0x64556c): sprite-cue category, board grid, handicap gate,
// visible-bounds rect.
struct CombatReg {
    char m_pad0[0x30];
    CombatSprCat* m_30; // +0x30
    char m_pad34[0x70 - 0x34];
    CombatGrid* m_70; // +0x70 the tile grid
    char m_pad74[0x118 - 0x74];
    i32 m_118; // +0x118 handicap enable
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134 handicap side
    char m_pad138[0x13c - 0x138];
    i32 m_13c; // +0x13c view left
    i32 m_140; // +0x140 view top
    i32 m_144; // +0x144 view right
    i32 m_148; // +0x148 view bottom
};
extern "C" CombatReg* g_mgrSettings; // _g_mgrSettings @0x64556c
extern "C" i32 g_644c54;             // _g_644c54 handicap owner id

// The tile-mgr grunt board (CGrunt+0x260): 4x15 grunt pointer grid at +0x1c + the
// per-cell engine ops (all __thiscall, reloc-masked).
struct CombatTileMgr {
    i32 CheckSpawn(i32 ownerHi, i32 ownerLo, i32 tile, i32 icon); // 0x4014a1
    void ApplyCellEffect(i32 i, i32 j, i32 k, i32 flag);          // 0x402e96 (ret 0x10)
    void ApplySwitch(CGruntCombat* g, i32 x, i32 y);              // 0x406d300 -> thunk 0x26df
    char m_pad0[0x1c];
    CGruntCombat* m_grid[4][15]; // +0x1c
};

// The GAME_CONVERSIONHIT cue: __cdecl lookup (0x2cca) then a __thiscall play (0x25fe).
struct CombatConvCue {
    void PlayIfElapsed(i32 tag, i32 a, i32 b, i32 c); // 0x2025fe (__thiscall, 4 args)
};
extern "C" CombatConvCue* CombatConvLookup(const char* key); // 0x2cca (__cdecl, 1 arg)

// The active-anim-set type-name registry: g_typeColl.IndexToPtr(node) -> record whose
// first field is the name string; g_typeNodes[0..g_typeCount) each get Reset.
struct CombatTypeNode {
    void Reset(); // 0x1b9b93 (__thiscall, 0 args)
};
struct CombatTypeColl {
    char** IndexToPtr(void* node); // 0x403864 -> thunk 0x3864 (__thiscall, ret 4)
};
extern CombatTypeColl g_typeColl; // ?g_typeColl@@3UCTypeKeyColl@@A @0x6bf650
extern "C" char* g_typeNodes;     // ?g_typeNodes@@3PAXA @0x6bf66c
extern "C" i32 g_typeCount;       // ?g_typeCount@@3HA @0x6bf670

// The keyed config tree (canonical CButeTree, include/Bute/ButeTree.h): Find
// (0x16d190) is reloc-masked __thiscall.
extern CButeTree g_buteTree; // ?g_buteTree@@3VCButeTree@@A @0x6bf620
// The bute-config manager (canonical CButeMgr): GetDwordDef (0x1721e0) is
// reloc-masked __thiscall.
extern CButeMgr g_buteMgr; // ?g_buteMgr@@3VCButeMgr@@A @0x6453d8

// The occupied-coord recycle list at CGrunt+0x31c (AddHead/RemoveAll) + the shared
// coord node free-list (head @0x645544, bias @0x64554c).
struct CombatCoordList {
    void AddHead(void* node); // 0x1b4967
    void RemoveAll();         // 0x1b48a6
};
extern "C" void* g_freeList;   // ?g_freeList@@3PAXA @0x645544
extern "C" i32 g_freeListBias; // ?g_freeListNodeBias@@3HA @0x64554c

// The kill-clock + sound-enable + cue-tag globals.
extern "C" i32 g_killCueClock; // _g_killCueClock @0x6bf3c0
extern "C" i32 g_sndEnabled;   // ?g_sndEnabled@@3HA @0x61ab20
extern "C" i32 g_sndCueTag;    // ?g_sndCueTag@@3HA @0x61ab24

// The 8 octant direction-vector triples (16-byte stride) copied into CGrunt+0x43c.
extern "C" i32 g_dirVec[9][4]; // DAT_00644970

// The hit-type byte table [reason][a0] (row stride 23) + the octant tangent thresholds.
extern "C" unsigned char g_hitTable[]; // DAT_005e9788
extern "C" float g_dtScale;            // DAT_005e999c death-touch duration scale
extern "C" float g_tanC0;              // DAT_005e99a0
extern "C" float g_tanC1;              // DAT_005e99a4
extern "C" double g_tanC2;             // DAT_005e99a8
extern "C" double g_tanC3;             // DAT_005e99b0

// The impact/block sound-cue keys (literal .rodata; reloc-masked).
static const char s_CONVERSIONHIT[] = "GAME_CONVERSIONHIT";
static const char s_DEATHTOUCHHIT[] = "GAME_DEATHTOUCHHIT";
static const char s_IMPACTMM1[] = "GRUNTZ_NORMALGRUNT_IMPACTMM1";
static const char s_IMPACTMM2[] = "GRUNTZ_NORMALGRUNT_IMPACTMM2";
static const char s_IMPACTMM3[] = "GRUNTZ_NORMALGRUNT_IMPACTMM3";
static const char s_IMPACTMM4[] = "GRUNTZ_NORMALGRUNT_IMPACTMM4";
static const char s_IMPACTWM1[] = "GRUNTZ_NORMALGRUNT_IMPACTWM1";
static const char s_IMPACTWM2[] = "GRUNTZ_NORMALGRUNT_IMPACTWM2";
static const char s_IMPACTWM3[] = "GRUNTZ_NORMALGRUNT_IMPACTWM3";
static const char s_BLOCKBODY1[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY1";
static const char s_BLOCKBODY2[] = "GRUNTZ_NORMALGRUNT_BLOCKBODY2";
static const char s_BLOCKMETAL1[] = "GRUNTZ_NORMALGRUNT_BLOCKMETAL1";
static const char s_SPRING2[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS2S1";
static const char s_SPRING1[] = "GRUNTZ_SPRINGGRUNT_SPRINGGRUNTS1S1";
static const char s_TOOBZ[] = "GRUNTZ_TOOBGRUNT_TOOBZGRUNTUI1B";
static const char s_typeO[] = "O";
static const char s_knockKey[] = "KnockBackTimePerTile";
static const char s_gruntSec[] = "Grunt";

// Resolve a launch-sound cue by key into a fresh out slot.
#define LK(key)                                                                                    \
    do {                                                                                           \
        CombatCue* out = 0;                                                                        \
        reg->m_30->m_28->m_10.Lookup((key), &out);                                                 \
        cue = out;                                                                                 \
    } while (0)

// Copy octant direction-vector triple k into CGrunt+0x43c; set the target tile pixel.
#define SETDIR(k, nx, ny)                                                                          \
    do {                                                                                           \
        F(this, 0x43c) = g_dirVec[k][0];                                                           \
        F(this, 0x440) = g_dirVec[k][1];                                                           \
        F(this, 0x444) = g_dirVec[k][2];                                                           \
        newX = (nx);                                                                               \
        newY = (ny);                                                                               \
    } while (0)

struct CGruntCombat {
    i32 LoadGruntCombatAnimations(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
};

RVA(0x000597a0, 0x1345)
i32 CGruntCombat::LoadGruntCombatAnimations(
    i32 a0,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7
) {
    if (F(this, 0x258) == 0x38 && F(this, 0x170) != 1) {
        return 1;
    }

    // a7 == 0x39: conversion hit - heal the struck enemy, fire GAME_CONVERSIONHIT.
    if (a7 == 0x39) {
        CGruntCombat* enemy = ((CombatTileMgr*)P(this, 0x260))->m_grid[a2][a3];
        if (enemy != 0
            && ((CombatTileMgr*)P(this, 0x260))
                       ->CheckSpawn(F(this, 0x1ec), F(this, 0x1f0), a2, F(enemy, 0x1f4))
                   != 0) {
            i32 h = F(enemy, 0x3ec) + 0x19;
            if (h >= 0x64) {
                h = 0x64;
            }
            F(enemy, 0x3ec) = h;
            if (F(P(P(P(this, 0x158), 0xc), 0x28), 0x30) == 0) {
                CombatConvCue* cc = CombatConvLookup(s_CONVERSIONHIT);
                if (cc != 0) {
                    cc->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
            return 0;
        }
    }

    // Hit-type byte-table lookup + optional handicap halving.
    i32 hit = g_hitTable[F(this, 0x170) * 23 + a0];
    CombatReg* reg = g_mgrSettings;
    if (reg->m_118 != 0 && reg->m_134 == 1 && F(this, 0x1ec) == g_644c54) {
        i32 t = hit / 2;
        hit = t + t % 5;
    }

    // Duration scale (kind 0x3c death-touch): scale by g_dtScale, then damage the enemy.
    if (a7 == 0x3a) {
        hit = 0x64;
    } else if (F(this, 0x258) == 0x3c) {
        hit = (i32)((float)hit * g_dtScale);
        if (a6 == 0) {
            CGruntCombat* enemy = ((CombatTileMgr*)P(this, 0x260))->m_grid[a2][a3];
            if (enemy != 0 && F(enemy, 0x1fc) != 0) {
                i32 nh = F(enemy, 0x3ec) - hit * 3;
                if (nh < 0) {
                    nh = 0;
                }
                F(enemy, 0x3ec) = nh;
                if (nh <= 0) {
                    ((CombatTileMgr*)P(this, 0x260))->ApplyCellEffect(a2, a3, 1, -1);
                }
            }
        }
    }

    // Self health decrement + reason-1 kill dispatch.
    i32 nh = F(this, 0x3ec) - hit;
    if (nh < 0) {
        nh = 0;
    }
    F(this, 0x3ec) = nh;
    if (F(this, 0x170) == 1) {
        ((CombatTileMgr*)P(this, 0x260))->ApplyCellEffect(F(this, 0x1ec), F(this, 0x1f0), 1, a2);
        return 0;
    }
    if (nh <= 0) {
        F(this, 0x1fc) = 0;
        F(this, 0x370) = a2;
    }

    // On-screen visibility gate, then the hit/block sound-cue resolve.
    CombatCue* cue = 0;
    i32 vx = F(P(this, 0x10), 0x5c);
    i32 vy = F(P(this, 0x10), 0x60);
    if (vx < reg->m_144 && vx >= reg->m_13c && vy < reg->m_148 && vy >= reg->m_140) {
        if (a7 == 0x3a) {
            LK(s_DEATHTOUCHHIT);
            goto L_cue;
        }
        if (a0 == 6 || a0 == 0xa || a0 == 0x16) {
            if (F(this, 0x170) == 8) {
                LK(s_BLOCKBODY2);
            } else {
                LK(s_IMPACTMM2);
            }
            goto L_cue;
        }
        if (F(this, 0x170) == 9) {
            if (a0 == 5 || a0 == 0xd || a0 == 0xe || a0 == 4) {
                LK(s_IMPACTMM4);
            } else {
                LK(s_IMPACTMM3);
            }
            goto L_cue;
        }
        if (F(this, 0x170) == 0xc) {
            LK(s_BLOCKMETAL1);
            goto L_cue;
        }
        if (F(this, 0x170) == 0xe) {
            if (a1 == 1) {
                LK(s_SPRING2);
            } else {
                LK(s_SPRING1);
            }
            goto L_cue;
        }
        if (F(this, 0x170) == 0x12 && F(this, 0x234) != 0) {
            LK(s_TOOBZ);
            goto L_cue;
        }
        switch (a0) {
            case 0:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 2:
                LK(s_IMPACTMM1);
                break;
            case 3:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 4:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 5:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case 7:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM1);
                }
                break;
            case 8:
                if (a1 == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 9:
                LK(s_IMPACTWM2);
                break;
            case 0xb:
                LK(s_IMPACTMM2);
                break;
            case 0xc:
                if (a1 == 0) {
                    LK(s_BLOCKBODY1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 0xd:
                if (a1 == 0) {
                    LK(s_BLOCKMETAL1);
                } else {
                    LK(s_IMPACTMM4);
                }
                break;
            case 0xe:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTWM3);
                }
                break;
            case 0xf:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x10:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM3);
                }
                break;
            case 0x12:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x13:
                if (a1 == 0) {
                    LK(s_BLOCKBODY2);
                } else {
                    LK(s_IMPACTMM1);
                }
                break;
            case 0x14:
                LK(s_IMPACTWM2);
                break;
            case 0x15:
                LK(s_IMPACTWM2);
                break;
            default:
                LK(s_IMPACTMM3);
                break;
        }

    L_cue:
        // Kill-clock-gated launch cue.
        if (cue != 0 && g_sndEnabled != 0) {
            i32 clk = g_killCueClock;
            if ((u32)(clk - cue->m_14) >= (u32)cue->m_18) {
                cue->m_14 = clk;
                cue->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
            }
        }
    }

    // Block path (a0 in {6,0xa,0x16}); otherwise reason 0x15 kills, else return.
    if (!(a0 == 6 || a0 == 0xa || a0 == 0x16)) {
        if (a0 != 0x15) {
            return 1;
        }
        if (F(this, 0x3ec) > 0) {
            return 1;
        }
        ((CombatTileMgr*)P(this, 0x260))->ApplyCellEffect(F(this, 0x1ec), F(this, 0x1f0), 7, a2);
        return 0;
    }

    if (F(this, 0x170) == 8) {
        return 1;
    }

    // Rebuild the active-anim-set type-name registry free list.
    char** typeRec = g_typeColl.IndexToPtr(*(void**)(P(this, 0x14) + 0x1c));
    if (g_typeCount != 0) {
        char* p = g_typeNodes;
        i32 n = g_typeCount;
        do {
            if (p != 0) {
                ((CombatTypeNode*)p)->Reset();
            }
            p += 4;
        } while (--n != 0);
    }
    if (strcmp(*typeRec, s_typeO) == 0) {
        return 1;
    }

    // x87 angle-octant direction resolver: copy the matching g_dirVec triple into
    // CGrunt+0x43c and set the target tile pixel (newX/newY).
    i32 dy = a5 - F(P(this, 0x10), 0x60);
    i32 dx = a4 - F(P(this, 0x10), 0x5c);
    i32 newX;
    i32 newY;
    if (a0 == 0x16) {
        switch (rand() % 8 - 1) {
            case 0:
                SETDIR(8, F(this, 0x17c) + 0x20, F(this, 0x180) - 0x20);
                break;
            case 1:
                SETDIR(3, F(this, 0x17c) + 0x20, F(this, 0x180));
                break;
            case 2:
                SETDIR(5, F(this, 0x17c) + 0x20, F(this, 0x180) + 0x20);
                break;
            case 3:
                SETDIR(1, F(this, 0x17c), F(this, 0x180) + 0x20);
                break;
            case 4:
                SETDIR(4, F(this, 0x17c) - 0x20, F(this, 0x180) + 0x20);
                break;
            case 5:
                SETDIR(0, F(this, 0x17c) - 0x20, F(this, 0x180));
                break;
            case 6:
                SETDIR(6, F(this, 0x17c) - 0x20, F(this, 0x180) - 0x20);
                break;
            default:
                SETDIR(2, F(this, 0x17c), F(this, 0x180) - 0x20);
                break;
        }
    } else if (dx == 0) {
        if (a5 > F(P(this, 0x10), 0x60)) {
            SETDIR(2, F(this, 0x17c), F(this, 0x180) - 0x20);
        } else if (a5 < F(P(this, 0x10), 0x60)) {
            SETDIR(1, F(this, 0x17c), F(this, 0x180) + 0x20);
        } else {
            goto L_moveDone;
        }
    } else {
        float slope = (float)dy / dx;
        if (slope > g_tanC0 || slope < g_tanC1) {
            if (a5 > F(P(this, 0x10), 0x60)) {
                SETDIR(2, F(this, 0x17c), F(this, 0x180) - 0x20);
            } else {
                SETDIR(1, F(this, 0x17c), F(this, 0x180) + 0x20);
            }
        } else if (slope > g_tanC2 || slope < g_tanC3) {
            if (slope > g_tanC2) {
                if (a4 > F(P(this, 0x10), 0x5c)) {
                    SETDIR(6, F(this, 0x17c) - 0x20, F(this, 0x180) - 0x20);
                } else {
                    SETDIR(5, F(this, 0x17c) + 0x20, F(this, 0x180) + 0x20);
                }
            } else if (slope < g_tanC3) {
                if (a4 > F(P(this, 0x10), 0x5c)) {
                    SETDIR(4, F(this, 0x17c) - 0x20, F(this, 0x180) + 0x20);
                } else {
                    SETDIR(8, F(this, 0x17c) + 0x20, F(this, 0x180) - 0x20);
                }
            } else {
                goto L_moveDone;
            }
        } else {
            if (a4 > F(P(this, 0x10), 0x5c)) {
                SETDIR(0, F(this, 0x17c) - 0x20, F(this, 0x180));
            } else {
                SETDIR(3, F(this, 0x17c) + 0x20, F(this, 0x180));
            }
        }
    }

    // Tile-to-tile occupancy + diagonal-corner move check.
    {
        i32 flags = F(this, 0x248) | 0x20000000;
        CombatGrid* grid = g_mgrSettings->m_70;
        i32 nyt = newY >> 5;
        i32 nxt = newX >> 5;
        i32 oxt = F(this, 0x17c) >> 5;
        i32 oyt = F(this, 0x180) >> 5;
        if (!(oxt == nxt && oyt == nyt)) {
            if ((u32)nxt >= (u32)grid->m_c) {
                return 1;
            }
            if ((u32)nyt >= (u32)grid->m_10) {
                return 1;
            }
            i32* cell = grid->m_8[nyt] + nxt * 7;
            i32 t = flags & cell[0];
            if (t & 0x20000000) {
                return 1;
            }
            if (t != 0 && (cell[0] & (F(this, 0x24c) | 0x18000482)) == 0) {
                return 1;
            }
            i32* ocell = grid->m_8[oyt] + oxt * 7;
            i32 dxt = nxt - oxt;
            i32 dyt = nyt - oyt;
            if (dxt != 0 && dyt != 0) {
                i32 rb = grid->m_c * 7 * 4;
                if (dxt > 0) {
                    if (dyt > 0) {
                        if ((*(i32*)((char*)ocell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell + rb) & 0x2000)
                            || (*(i32*)((char*)cell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell - rb) & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if ((*(i32*)((char*)ocell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell - rb) & 0x2000)
                            || (*(i32*)((char*)cell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell + rb) & 0x2000)) {
                            return 1;
                        }
                    }
                } else {
                    if (dyt > 0) {
                        if ((*(i32*)((char*)ocell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell + rb) & 0x2000)
                            || (*(i32*)((char*)cell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell - rb) & 0x2000)) {
                            return 1;
                        }
                    } else {
                        if ((*(i32*)((char*)ocell - 0x1c) & 0x2000)
                            || (*(i32*)((char*)ocell - rb) & 0x2000)
                            || (*(i32*)((char*)cell + 0x1c) & 0x2000)
                            || (*(i32*)((char*)cell + rb) & 0x2000)) {
                            return 1;
                        }
                    }
                }
            }
        }

        // Arrival commit + occupancy re-stamp + knockback trajectory tail.
        if (F(this, 0x1e8) == 0) {
            ((CombatTileMgr*)P(this, 0x260))->ApplySwitch(this, F(this, 0x17c), F(this, 0x180));
        }
        CombatGrid* g2 = g_mgrSettings->m_70;
        i32 ox = F(this, 0x17c) >> 5;
        i32 oy = F(this, 0x180) >> 5;
        i32* oc = g2->m_8[oy] + ox * 7;
        *((unsigned char*)oc + 3) &= 0xdf;
        i32* oc2 = g2->m_8[oy] + ox * 7;
        oc2[1] = -1;
        i32* nc = g2->m_8[nyt] + nxt * 7;
        *((unsigned char*)nc + 3) |= 0x20;
        i32* nc2 = g2->m_8[nyt] + nxt * 7;
        nc2[1] = (F(this, 0x1ec) << 8) | F(this, 0x1f0);

        if (F(this, 0x328) != 0) {
            i32* node = 0;
            i32 rx = F(this, 0x17c) >> 5;
            i32 ry = F(this, 0x180) >> 5;
            if (*(void**)g_freeList != 0) {
                node = (i32*)((char*)g_freeList + 4);
                node[0] = rx;
                node[1] = ry;
                g_freeList = *(void**)g_freeList;
            }
            ((CombatCoordList*)((char*)this + 0x31c))->AddHead(node);
        }

        F(this, 0x17c) = newX;
        F(this, 0x180) = newY;
        F(this, 0x30) = F(P(this, 0x14), 0x1c);
        F(P(this, 0x14), 0x1c) = (i32)g_buteTree.Find(s_typeO);
        double ddx = (double)newX - F(P(this, 0x10), 0x5c);
        double ddy = (double)newY - F(P(this, 0x10), 0x60);
        double dist = sqrt(ddx * ddx + ddy * ddy);
        u32 kb = g_buteMgr.GetDwordDef((char*)s_gruntSec, (char*)s_knockKey, 200);
        *(double*)((char*)this + 0x400) = dist / (double)kb;
        *(double*)((char*)this + 0x408) = (double)F(P(this, 0x10), 0x5c);
        *(double*)((char*)this + 0x410) = (double)F(P(this, 0x10), 0x60);

        if (F(this, 0x328) != 0) {
            void** node = (void**)P(this, 0x320);
            if (node != 0) {
                void* fl = g_freeList;
                do {
                    void** cur = node;
                    node = (void**)*node;
                    void* data = *(void**)((char*)cur + 8);
                    if (data != 0) {
                        void** slot = (void**)((char*)data - g_freeListBias);
                        *slot = fl;
                        fl = slot;
                        g_freeList = fl;
                    }
                } while (node != 0);
            }
            ((CombatCoordList*)((char*)this + 0x31c))->RemoveAll();
        }
        F(this, 0x1e8) = 0;
    }

L_moveDone:
    return 1;
}

SIZE_UNKNOWN(CGruntCombat);
SIZE_UNKNOWN(CombatConvCue);
SIZE_UNKNOWN(CombatCoordList);
SIZE_UNKNOWN(CombatCue);
SIZE_UNKNOWN(CombatCueMap);
SIZE_UNKNOWN(CombatGrid);
SIZE_UNKNOWN(CombatItemOwner);
SIZE_UNKNOWN(CombatReg);
SIZE_UNKNOWN(CombatSprCat);
SIZE_UNKNOWN(CombatSprInner);
SIZE_UNKNOWN(CombatTileMgr);
SIZE_UNKNOWN(CombatTypeColl);
SIZE_UNKNOWN(CombatTypeNode);

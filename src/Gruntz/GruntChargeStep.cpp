// GruntChargeStep.cpp - CGrunt::ChargeStep (0x0ef6b0) re-homed from
// src/Stub/Discovered.cpp. The per-frame "pursue / charge the target grunt"
// behavior step: a three-state machine (m_defenderState = scan / move / arrived) over the
// grunt manager's tile table (m_tileMgr), issuing move/attack commands and re-arming
// a random-wander fallback. /base - no destructible locals. Self-contained CGrunt
// view (own TU) so the shared Grunt.h/CGrunt model stays untouched; only offsets +
// the reloc-masked engine callees (ILT thunks) are load-bearing.
#include <Ints.h>
#include <rva.h>

#include <stdlib.h> // rand (0x11fee0)

// A grunt world-position sub-object (this->m_10 and target->m_10). +0x5c/+0x60 are
// the pixel coords; +0x134..+0x140 the random-wander bounding rect.
struct PosObj {
    char pad0[0x5c];
    i32 m_pixelX; // +0x5c  x
    i32 m_pixelY; // +0x60  y
    char pad64[0x134 - 0x64];
    i32 m_wanderMinX; // +0x134
    i32 m_wanderMinY; // +0x138
    i32 m_wanderMaxX; // +0x13c
    i32 m_wanderMaxY; // +0x140
};

struct Grunt;

// The grunt manager tile table at this->m_tileMgr: Find(grunt) returns the target and
// the +0x1c entry table is a 15-wide grid of grunt slots.
struct GruntTable {
    Grunt* Find(Grunt* g); // 0x40253b (thiscall, 1 arg)
    char pad0[0x1c];
    Grunt* entries[1]; // +0x1c (indexed [m_arrivalRow + m_arrivalCol*15])
};

// The g_64556c game-manager singleton chain this step walks.
struct MgrGrid {
    char pad0[0x30];
    i32 m_30; // +0x30 (grid dim base, +0x40'd)
};
struct MgrSubB {
    char pad0[0x5c];
    MgrGrid* m_grid; // +0x5c
};
struct MgrSubA {
    char pad0[0x24];
    MgrSubB* m_24; // +0x24
};
struct MgrBounds {
    char pad0[0xc];
    u32 m_c;  // +0x0c  world width
    u32 m_10; // +0x10  world height
};
struct MgrNotifySink {
    // 0x4039f4 (thiscall): the "grunt engaged" notify (id 0x366).
    void Notify(Grunt* g, i32 id, i32 a, i32 b, i32 c, i32 d);
};
struct GameMgr {
    char pad0[0x30];
    MgrSubA* m_world; // +0x30
    char pad34[0x60 - 0x34];
    MgrNotifySink* m_cueSink; // +0x60
    char pad64[0x70 - 0x64];
    MgrBounds* m_tileGrid; // +0x70
};
extern "C" {
    DATA(0x0024556c)
    extern GameMgr* g_64556c;
}

// __cdecl line-of-sight probe (0x401127 via ILT): (destTile, x, y) -> reachable.
i32 GruntLos1127(i32 dest, i32 x, i32 y);

struct Grunt {
    char pad0[0x10];
    PosObj* m_10; // +0x10
    char pad14[0x17c - 0x14];
    i32 m_lastTilePxX; // +0x17c  x
    i32 m_lastTilePxY; // +0x180  y
    char pad184[0x1e4 - 0x184];
    i32 m_entranceActive; // +0x1e4
    char pad1e8[0x1ec - 0x1e8];
    i32 m_tileOwnerHi; // +0x1ec  (target home x)
    i32 m_tileOwnerLo; // +0x1f0  (target home y)
    char pad1f4[0x1fc - 0x1f4];
    i32 m_entranceCommitted; // +0x1fc  (target alive flag)
    char pad200[0x218 - 0x200];
    i32 m_combatActive;  // +0x218
    i32 m_neighborValid; // +0x21c
    i32 m_poweredUp;     // +0x220
    char pad224[0x244 - 0x224];
    i32 m_resetApplied; // +0x244
    i32 m_arrivalFlags; // +0x248
    char pad24c[0x260 - 0x24c];
    GruntTable* m_tileMgr; // +0x260
    char pad264[0x2d4 - 0x264];
    i32 m_defenderState; // +0x2d4  behavior sub-state (here: 0=scan,1=move,2=arrived)
    char pad2d8[0x2ec - 0x2d8];
    i32 m_dwell;      // +0x2ec  step/dwell timer (ms)
    i32 m_arrivalCol; // +0x2f0  arrival grid col (index = 15*col+row)
    i32 m_arrivalRow; // +0x2f4  arrival grid row
    char pad2f8[0x300 - 0x2f8];
    i32 m_defenderX; // +0x300  saved x (= m_lastTilePxX)
    i32 m_defenderY; // +0x304  saved y (= m_lastTilePxY)
    char pad308[0x318 - 0x308];
    i32 m_318; // +0x318
    char pad31c[0x328 - 0x31c];
    i32 m_coordCount; // +0x328  occupied-coord count (here gates a wander-span snap)
    char pad32c[0x3f0 - 0x32c];
    i32 m_stamina; // +0x3f0  stamina (>=100 gate)

    // reloc-masked engine callees (ILT thunks; thiscall on this):
    i32 AtTile(i32 x, i32 y);                               // 0x403c4c
    i32 IsBusy(i32 a);                                      // 0x403d5a
    void StopMove(i32 a, i32 b, i32 c);                     // 0x40136b
    i32 CanReach(i32 x, i32 y);                             // 0x401014
    void FaceTarget(i32 a, i32 b, i32 c, i32 d);            // 0x40302b
    void MoveTo6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x4014e2
    i32 Attack(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);   // 0x401640
    void Snap(i32 a, i32 b);                                // 0x401401

    i32 ChargeStep(); // 0x0ef6b0
};

// ---------------------------------------------------------------------------
// @early-stop
// CRACKED 21%->57.6% (2026-07-05). The state machine is a `switch (m_defenderState)`
// (NOT if/else) - that alone produced retail's `sub eax; je state0; dec; je state1; dec;
// jne return; [state2 fall-through]` dispatch and the state2/state1/state0 reverse layout
// (+30% in one build). Also fixed: the powered-up (m_220) block flattened to retail's
// guard-gotos with the >=100 stamina path as fall-through; a real bug - the state 0/1/2
// AtTile args were reversed (must be AtTile(m_5c, m_60) like the top hitGate); state1's
// dwell compare is unsigned (jbe). Residual ~42% is three genuine walls (final sweep):
//   1. State-2 powered-up recheck (0x198, 57 insns) is DEAD in-source (the switch is only
//      reached with m_220==0, verified: 0x157 has one pred = the m_220==0 guard) so THIS
//      cl dead-code-eliminates it; retail's cl emits it. Pure DCE-behavior artifact - no
//      clean C spelling forces the dead block without a goto/opaque hack.
//   2. Zero-register swap cascade: retail pins the ubiquitous 0 constant in ebp and hitGate
//      in ebx; this cl swaps them (ebx=0, ebp=hitGate), flipping every `cmp <zero>,x`.
//   3. Shared FaceTarget(0x40302b) tail-merge (states 0 and 2 share one FaceTarget;return 1
//      tail via `sub esp,0xc` coord spills) + the surrounding regalloc permutations.
//   All member offsets + reloc-masked engine callees are modeled.
RVA(0x000ef6b0, 0x61d)
i32 Grunt::ChargeStep() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    Grunt* g = m_tileMgr->Find(this);
    i32 hitGate = 0;
    if (g != 0) {
        PosObj* gp = g->m_10;
        if (gp->m_pixelX == g->m_lastTilePxX && gp->m_pixelY == g->m_lastTilePxY
            && AtTile(gp->m_pixelX, gp->m_pixelY)) {
            hitGate = 1;
        }
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina >= 100) {
            if (IsBusy(1) != 0) {
                return 1;
            }
            if (hitGate != 0 && g == 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            StopMove(1, 0, 0);
            return 1;
        }
        if (hitGate != 0) {
            return 1;
        }
        if (m_poweredUp == 0) {
            return 1;
        }
        if (m_neighborValid != 0) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        StopMove(1, 0, 0);
        return 1;
    }

    // ---- m_poweredUp == 0: the charge state machine (switch -> sub/dec/dec dispatch;
    // states 0 and 2 both end with FaceTarget(...);return 1 and MSVC tail-merges them). ----
    switch (m_defenderState) {
        case 0: {
            // scan for a target on the wander tile
            if (g != 0) {
                if (hitGate != 0 && m_stamina >= 100) {
                    PosObj* gp = g->m_10;
                    if (gp->m_pixelX == g->m_lastTilePxX && gp->m_pixelY == g->m_lastTilePxY
                        && AtTile(gp->m_pixelX, gp->m_pixelY)) {
                        FaceTarget(
                            g->m_tileOwnerHi,
                            g->m_tileOwnerLo,
                            g->m_lastTilePxX,
                            g->m_lastTilePxY
                        );
                        return 1;
                    }
                }
                if (m_dwell > 500) {
                    if (CanReach(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
                        return 1;
                    }
                    if (Attack(g->m_10->m_pixelX >> 5, g->m_10->m_pixelY >> 5, 0, m_arrivalFlags, 1, 0)
                        != 0) {
                        Snap(1, 1);
                        m_arrivalCol = g->m_tileOwnerHi;
                        m_arrivalRow = g->m_tileOwnerLo;
                        m_defenderState = 1;
                        PosObj* mp = m_10;
                        GameMgr* mgr = g_64556c;
                        i32 los = GruntLos1127(
                            mgr->m_world->m_24->m_grid->m_30 + 0x40,
                            mp->m_pixelX,
                            mp->m_pixelY
                        );
                        if (los != 0) {
                            mgr->m_cueSink->Notify(this, 0x366, -1, 0, -1, -1);
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            if (m_resetApplied == 0 && m_318 != 0 && m_dwell > 3000) {
                PosObj* mp = m_10;
                i32 baseX = mp->m_wanderMinX;
                i32 spanX = mp->m_wanderMaxX - baseX;
                spanX = spanX < 0 ? -spanX : spanX;
                i32 baseY = mp->m_wanderMinY;
                i32 spanY = mp->m_wanderMaxY - baseY;
                spanY = spanY < 0 ? -spanY : spanY;
                if (spanX != 0) {
                    baseX += rand() % spanX;
                }
                if (spanY != 0) {
                    baseY += rand() % spanY;
                }
                GameMgr* mgr = g_64556c;
                if ((u32)baseX < mgr->m_tileGrid->m_c && (u32)baseY < mgr->m_tileGrid->m_10) {
                    Attack(baseX, baseY, 0, m_arrivalFlags, 1, 0);
                }
                if (m_coordCount != 0) {
                    if (spanX <= spanY) {
                        spanX = spanY;
                    }
                    if (spanX < m_coordCount) {
                        Snap(1, 1);
                    }
                }
                m_dwell = 0;
            }
            break;
        }
        case 1: {
            // moving to the arrival tile
            Grunt* t = m_tileMgr->entries[m_arrivalRow + m_arrivalCol * 0xf];
            Grunt* cur = m_tileMgr->Find(this);
            if (cur != 0 && cur != t) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
                return 1;
            }
            if (t == 0 || t->m_entranceCommitted == 0
                || CanReach(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if ((u32)m_dwell > 500) {
                MoveTo6(t->m_lastTilePxX, t->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp == 0 && m_stamina >= 100 && AtTile(t->m_10->m_pixelX, t->m_10->m_pixelY) != 0
                && t->m_10->m_pixelX == t->m_lastTilePxX && t->m_10->m_pixelY == t->m_lastTilePxY) {
                FaceTarget(t->m_tileOwnerHi, t->m_tileOwnerLo, t->m_lastTilePxX, t->m_lastTilePxY);
                m_defenderState = 2;
                return 1;
            }
            break;
        }
        case 2: {
            // arrived: re-check target then hold
            if (m_poweredUp != 0) {
                Grunt* t = m_tileMgr->entries[m_arrivalRow + m_arrivalCol * 0xf];
                if (t == 0 || CanReach(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0
                    || t->m_entranceCommitted == 0) {
                    m_defenderState = 1;
                    m_dwell = 0x1f4;
                    return 1;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < 100) {
                    return 1;
                }
                if (AtTile(t->m_10->m_pixelX, t->m_10->m_pixelY) == 0 || t->m_10->m_pixelX != t->m_lastTilePxX
                    || t->m_10->m_pixelY != t->m_lastTilePxY) {
                    m_defenderState = 1;
                    m_dwell = 0x1f4;
                    return 1;
                }
                FaceTarget(t->m_tileOwnerHi, t->m_tileOwnerLo, t->m_lastTilePxX, t->m_lastTilePxY);
                return 1;
            }
            m_defenderState = 1;
            m_dwell = 0x1f4;
            return 1;
        }
    }
    return 1;
}

SIZE_UNKNOWN(GameMgr);
SIZE_UNKNOWN(Grunt);
SIZE_UNKNOWN(GruntTable);
SIZE_UNKNOWN(MgrBounds);
SIZE_UNKNOWN(MgrGrid);
SIZE_UNKNOWN(MgrNotifySink);
SIZE_UNKNOWN(MgrSubA);
SIZE_UNKNOWN(MgrSubB);
SIZE_UNKNOWN(PosObj);

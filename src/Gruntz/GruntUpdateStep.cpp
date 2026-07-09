// GruntUpdateStep.cpp - two big CGrunt per-tick arrival/move update steps that sit
// right after CGrunt_0ef6b0 in retail (0xf0130 + 0xf71c0). Both are __thiscall, no
// explicit args, return 1. They drive the grunt's tile-to-tile move: resolve the
// grunt under the HUD center (m_tileMgr->GetOccupant), gate on the powered-up /
// arrival state words, and on commit fire the on-screen grunt cue (m_cueSink->CueA
// 0x366) and re-stamp the move. 0xf71c0 is the search/seek variant (scan the 15 grid
// slots for the nearest live target).
//
// FOLDED onto the canonical CGrunt (<Gruntz/Grunt.h>): the local CGruntStep this-alias
// + the local CGruntTileMgr/MgrGrid/GruntCue views are gone. These are real CGrunt
// methods; the tile-mgr grid is the canonical CGruntTileMgr::m_grid (this+0x260); the
// cue is m_cueSink->CueA (the retail 0x39f4 thiscall - the prior __stdcall free-fn model
// was wrong). Every CGrunt/CGruntHud field access is the NAMED member (m_dwell,
// m_poweredUp, m_10->m_5c, ...); only the registry-chain reads (g_pGameRegistry->
// m_cmdGrid/m_tileGrid + the board rect) stay F()/P() pending typed manager sub-objects.
#include <Gruntz/Grunt.h> // canonical CGrunt / CGruntTileMgr / CGruntCueSink / CGameRegistry
#include <Wap32/ZVec.h>

#include <rva.h>
#include <string.h> // inline strcmp of the grunt type name
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/TypeColl.h>

#pragma intrinsic(strcmp)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

// The shared game-manager singleton (*0x64556c); reached typed as CGameRegistry.
extern CGameRegistry* g_pGameRegistry; // ?g_gameReg@@3PAUWwdGameReg@@A (0x64556c)

// Type-name collection (g_typeColl @0x6bf650): Lookup(key)->node, node->m_0 = name.
// (DATA-bound here; GruntArrivalScan.cpp references it unbound.)
DATA(0x006bf650)
extern CTypeColl g_typeColl;

DATA(0x00645588)
extern u32 g_clock; // game clock (g_645588)
DATA(0x00645544)
extern void* g_freeList;
DATA(0x0064554c)
extern i32 g_freeListBias;

DATA(0x00645540)
extern FreeNodePool g_dropList;

// ===========================================================================
// @early-stop
// 23%->34% (2026-07-05): the phase dispatch is a `switch (m_defenderState)`, not
// if/else - that produced retail's `sub eax; je phase0; dec; je phase1; dec; jne tail;
// [phase2 fall-through]` ladder and the phase2/phase1/phase0 reverse layout, and the
// `goto tail`s became `break`s. Residual walls (final sweep):
//   * phase-2 powered-up recheck (~78 insns, retail 0x1cb-0x24d) is DEAD in-source (the
//     switch runs only with m_220==0, which the top powered-up early-out proves) so THIS
//     cl dead-code-eliminates it while retail emits it - the same DCE artifact as
//     GruntChargeStep's state-2 recheck; no clean C spelling forces the dead block.
//   * `if (!strcmp(name,"I"))` sete-bool vs my `!=0` branch, and the deep regalloc across
//     the grunt-under-HUD pointer / clock / grid bases.
RVA(0x000f0130, 0x7c0)
i32 CGrunt::UpdateArrival() {
    char* name = ((CTypeNode*)((zDArray*)&g_typeColl)->IndexToPtr((i32)m_14->m_1c))->m_0;
    if (strcmp(name, "I") != 0) {
        return 1;
    }
    this->m_defenderX = this->m_lastTilePxX;
    this->m_defenderY = this->m_lastTilePxY;
    CGrunt* g = m_tileMgr->GetOccupant(this);
    bool atTarget = false;
    if (g != 0) {
        i32 x = g->m_10->m_5c;
        if (x == g->m_lastTilePxX && g->m_10->m_60 == g->m_lastTilePxY
            && g->RectContains(x, g->m_10->m_60) != 0) {
            atTarget = true;
        }
    }

    if (this->m_poweredUp != 0) {
        if (this->m_neighborValid != 0) {
            this->m_neighborValid = 0;
            return 1;
        }
        if (this->m_combatActive != 0) {
            return 1;
        }
        if (this->m_stamina < 100) {
            if (atTarget) {
                return 1;
            }
            if (this->m_poweredUp == 0) {
                return 1;
            }
            this->m_entranceActive = 0;
            this->m_combatActive = 0;
            this->m_neighborValid = 0;
            this->m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        if (FindGridNeighbor(1) != 0) {
            return 1;
        }
        if (atTarget && g == 0) {
            return 1;
        }
        if (this->m_poweredUp == 0) {
            return 1;
        }
        if (this->m_neighborValid != 0) {
            return 1;
        }
        this->m_entranceActive = 0;
        this->m_combatActive = 0;
        this->m_neighborValid = 0;
        this->m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    switch (this->m_defenderState) {
        case 0:
            if (g != 0) {
                if (this->m_stamina > 99) {
                    i32 x = g->m_10->m_5c;
                    if (x == g->m_lastTilePxX && g->m_10->m_60 == g->m_lastTilePxY
                        && g->RectContains(x, g->m_10->m_60) != 0) {
                        CommitNeighbor(
                            g->m_tileOwnerHi,
                            g->m_tileOwnerLo,
                            g->m_lastTilePxX,
                            g->m_lastTilePxY
                        );
                        break;
                    }
                }
                if (g != 0 && (u32)this->m_dwell > 1000) {
                    if (g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        i32 c[4];
                        GetScreenPos((GruntTilePos*)c);
                        if (TileSwitch6(c[1] >> 5, c[0] >> 5, 0, this->m_arrivalFlags, 0, 0x20)
                            != 0) {
                            SetEntrancePos(1, 1);
                            this->m_arrivalCol = g->m_tileOwnerHi;
                            this->m_arrivalRow = g->m_tileOwnerLo;
                            this->m_defenderState = 1;
                            i32 r = GruntPointVisible(
                                F(P(g_pGameRegistry->m_world->m_24, 0x5c), 0) + 0x40,
                                this->m_10->m_5c,
                                this->m_10->m_60
                            );
                            if (r != 0) {
                                g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    this->m_dwell = 0;
                    break;
                }
            }
            if (this->m_resetApplied == 0 && this->m_318 != 0 && (u32)this->m_dwell > 3000) {
                i32 cmp =
                    -(i32)((u32)g_clock < (u32)this->m_arrivalRerollLo) - this->m_arrivalRerollHi;
                if (this->m_arrivalRerollWindowHi < cmp
                    || (this->m_arrivalRerollWindowHi <= cmp
                        && (u32)this->m_arrivalRerollWindowLo
                               <= g_clock - (u32)this->m_arrivalRerollLo)) {
                    ResetEntranceAnimation(1, 1, 0);
                    this->m_arrivalRerollLo = 0;
                    this->m_arrivalRerollWindowLo = 0;
                    this->m_arrivalRerollHi = 0;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollWindowLo = GruntRand() % 30000 + 30000;
                    this->m_arrivalRerollWindowHi = 0;
                    this->m_arrivalRerollLo = (i32)g_clock;
                    this->m_arrivalRerollHi = 0;
                } else {
                    CGruntHud* base = this->m_10;
                    u32 lo = base->m_134;
                    i32 dx = base->m_13c - (i32)lo;
                    i32 ax = (dx ^ (dx >> 31)) - (dx >> 31);
                    u32 lo2 = base->m_138;
                    i32 dy = base->m_140 - (i32)lo2;
                    i32 ay = (dy ^ (dy >> 31)) - (dy >> 31);
                    if (ax != 0) {
                        lo = lo + GruntRand() % ax;
                    }
                    if (ay != 0) {
                        lo2 = lo2 + GruntRand() % ay;
                    }
                    if (lo < (u32)F(g_pGameRegistry->m_tileGrid, 0xc)
                        && lo2 < (u32)F(g_pGameRegistry->m_tileGrid, 0x10)) {
                        TileSwitch6((i32)lo, (i32)lo2, 0, this->m_arrivalFlags, 1, 0);
                    }
                    if (this->m_coordCount != 0) {
                        if (ax <= ay) {
                            ax = ay;
                        }
                        if (ax < this->m_coordCount) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
                this->m_dwell = 0;
            }
            break;
        case 1: {
            CGrunt* slot = m_tileMgr->m_grid[this->m_arrivalCol][this->m_arrivalRow];
            i32 cur = m_tileMgr->GetOccupant(this) ? 1 : 0;
            CGrunt* found = m_tileMgr->GetOccupant(this);
            (void)cur;
            if (found == 0 || found == slot) {
                if (slot == 0 || slot->m_entranceCommitted == 0
                    || slot->GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
                    this->m_defenderState = 0;
                } else {
                    StepArrivalDrop(
                        slot->m_lastTilePxX,
                        slot->m_lastTilePxY,
                        0,
                        this->m_arrivalFlags,
                        0,
                        0x20
                    );
                    if (this->m_poweredUp == 0 && this->m_stamina > 99
                        && slot->RectContains(slot->m_10->m_5c, slot->m_10->m_60) != 0
                        && slot->m_10->m_5c == slot->m_lastTilePxX
                        && slot->m_10->m_60 == slot->m_lastTilePxY) {
                        CommitNeighbor(
                            slot->m_tileOwnerHi,
                            slot->m_tileOwnerLo,
                            slot->m_lastTilePxX,
                            slot->m_lastTilePxY
                        );
                        this->m_defenderState = 2;
                    }
                }
            } else {
                this->m_arrivalCol = -1;
                this->m_defenderState = 0;
                this->m_arrivalRow = -1;
            }
            break;
        }
        case 2:
            this->m_defenderState = 1;
            break;
    }

    if (this->m_coordCount != 0) {
        // The active-move cell: (head node)->link is a [col,row]; gate on the grid
        // cell's flag byte (&0x20).
        i32* cell = (i32*)this->m_320->m_coord;
        u8* flags = (u8*)(F(F(g_pGameRegistry->m_tileGrid, 0x8) + cell[1] * 4, 0) + cell[0] * 0x1c);
        if ((flags[0] & 0x20) != 0) {
            SetEntrancePos(1, 1);
            if (this->m_coordCount != 0) {
                void* p = (void*)this->m_320;
                void* prev = g_freeList;
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_freeList = (void*)(*link - g_freeListBias);
                        *(void**)g_freeList = prev;
                        prev = g_freeList;
                    }
                }
                m_31c.RemoveAll();
            }
            SetEntrancePos(cell[0] * 0x20 + 0x10, cell[1] * 0x20 + 0x10);
        }
    }
    return 1;
}

// ===========================================================================
// @early-stop
// Seek variant of UpdateArrival (scan the 15 grid slots for the nearest live
// target, drop the queued move nodes, re-probe). Same reloc-masked engine-call set +
// the deep regalloc / cold-block wall. Logic reconstructed faithfully. Final-sweep
// candidate.
RVA(0x000f71c0, 0x721)
i32 CGrunt::SeekTarget() {
    this->m_defenderX = this->m_lastTilePxX;
    this->m_defenderY = this->m_lastTilePxY;
    if (this->m_coordCount != 0
        && F(F(g_pGameRegistry->m_cmdGrid, 0x1c) + this->m_arrivalCol * 4, 0) == 0) {
        void* p = (void*)this->m_320;
        while (p != 0) {
            void* next = *(void**)p;
            i32* link = (i32*)((char*)p + 8);
            p = next;
            if (*link != 0) {
                g_dropList.Push((void*)(*link));
            }
        }
        m_31c.RemoveAll();
        this->m_arrivalCol = 0;
    }

    i32 reason = this->m_entranceReason;
    if (reason > 0x16) {
        reason = this->m_19c;
    }
    if (reason == 0 && (reason = this->m_arrivalCol, reason >= 0) && reason < 0xf) {
        CGrunt* slot = (CGrunt*)F(F(g_pGameRegistry->m_cmdGrid, 0x1c) + reason * 4, 0);
        if (slot == 0 || slot->m_entranceCommitted == 0) {
            if (this->m_coordCount != 0) {
                void* p = (void*)this->m_320;
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_dropList.Push((void*)(*link));
                    }
                }
                m_31c.RemoveAll();
            }
            this->m_arrivalCol = -1;
            return 1;
        }
        // Adjacency probe: read this grunt's HUD center + the slot's, in tile units,
        // and require both axis deltas < 2 (the slot is the immediate neighbor).
        i32 c0[4];
        GetScreenPos((GruntTilePos*)c0);
        i32 cy = c0[1] >> 5;
        i32 d0[4];
        GetScreenPos((GruntTilePos*)d0);
        i32 e0[4];
        GetScreenPos((GruntTilePos*)e0);
        i32 f0[4];
        GetScreenPos((GruntTilePos*)f0);
        i32 dx = (f0[1] >> 5) - (f0[3] >> 5);
        i32 dy = cy - (e0[3] >> 5);
        if (((dy ^ (dy >> 31)) - (dy >> 31)) < 2 && ((dx ^ (dx >> 31)) - (dx >> 31)) < 2) {
            i32 r2 = slot->m_entranceReason;
            if (r2 > 0x16) {
                r2 = slot->m_19c;
            }
            if (r2 != 0x14 && r2 != 1) {
                slot->LoadGruntTypeTable(r2, 1, 0, 0);
                slot->LoadGruntTypeTable(0, 1, 0, 0);
                this->m_defenderState = 4;
                if (this->m_coordCount == 0) {
                    return 1;
                }
                void* p = (void*)this->m_320;
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_dropList.Push((void*)(*link));
                    }
                }
                m_31c.RemoveAll();
                return 1;
            }
        }
    }

    reason = this->m_entranceReason;
    if (reason > 0x16) {
        reason = this->m_19c;
    }
    if (reason == 0) {
        if (this->m_coordCount == 0) {
            if (this->m_defenderState != 0) {
                return 1;
            }
            i32 best = 0x7fffffff;
            i32 bestIdx = -1;
            CGrunt** slots = (CGrunt**)((char*)0 + F(g_pGameRegistry->m_cmdGrid, 0x1c));
            i32 i = 0;
            do {
                CGrunt* sv = slots[i];
                if (sv != 0 && sv->m_entranceCommitted != 0) {
                    i32 k = sv->m_entranceReason;
                    i32 kk = k;
                    if (k > 0x16) {
                        kk = sv->m_19c;
                    }
                    if (kk != 0 && kk != 0x14 && kk != 1
                        && !(k > 0x16 ? (sv->m_19c == 0x14) : false) && sv->m_gruntKind != 0x36) {
                        i32 ex = sv->m_10->m_5c >> 5;
                        i32 ddx = ex - (this->m_10->m_5c >> 5);
                        i32 ey = (sv->m_10->m_60 >> 5) - (this->m_10->m_60 >> 5);
                        i32 dist = ddx * ddx + ey * ey;
                        if (dist < best
                            && dist <= this->m_defenderRadius * this->m_defenderRadius) {
                            best = dist;
                            bestIdx = i;
                        }
                    }
                }
                i++;
            } while (i < 0xf);
            if (bestIdx != -1) {
                this->m_arrivalCol = bestIdx;
                CGruntHud* base = slots[bestIdx]->m_10;
                if (TileSwitch6(base->m_5c >> 5, base->m_60 >> 5, 0, this->m_arrivalFlags, 1, 0)
                    != 0) {
                    i32 by = this->m_10->m_60;
                    i32 bx = this->m_10->m_5c;
                    i32 board = F(P(g_pGameRegistry->m_world->m_24, 0x5c), 0);
                    if (bx < F(board, 0x48) && F(board, 0x40) <= bx && by < F(board, 0x4c)
                        && F(board, 0x44) <= by) {
                        g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                    }
                }
            }
            this->m_dwell = 0;
            return 1;
        }
        if (this->m_defenderState != 0) {
            return 1;
        }
        if ((u32)this->m_dwell < 0x3e9) {
            return 1;
        }
        CGruntHud* base =
            ((CGrunt*)F(F(g_pGameRegistry->m_cmdGrid, 0x1c) + this->m_arrivalCol * 4, 0))->m_10;
        TileSwitch6(base->m_5c >> 5, base->m_60 >> 5, 0, this->m_arrivalFlags, 1, 0);
    } else {
        CGrunt* g = m_tileMgr->GetOccupant(this);
        bool atTarget = false;
        if (g != 0) {
            i32 x = g->m_10->m_5c;
            if (x == g->m_lastTilePxX && g->m_10->m_60 == g->m_lastTilePxY
                && g->RectContains(x, (i32)g->m_10) != 0) {
                atTarget = true;
            }
        }
        if (this->m_poweredUp != 0) {
            if (this->m_neighborValid != 0) {
                this->m_neighborValid = 0;
                return 1;
            }
            if (this->m_combatActive != 0) {
                return 1;
            }
            if (this->m_stamina < 100) {
                if (atTarget) {
                    return 1;
                }
                if (this->m_poweredUp == 0) {
                    return 1;
                }
                this->m_entranceActive = 0;
                this->m_combatActive = 0;
                this->m_neighborValid = 0;
                this->m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
                return 1;
            }
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (this->m_poweredUp == 0) {
                return 1;
            }
            if (this->m_neighborValid != 0) {
                return 1;
            }
            this->m_entranceActive = 0;
            this->m_combatActive = 0;
            this->m_neighborValid = 0;
            this->m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        this->m_defenderX = this->m_lastTilePxX;
        this->m_defenderY = this->m_lastTilePxY;
        if (g == 0 || g->GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
            this->m_390 = 0;
            return 1;
        }
        if (this->m_poweredUp == 0 && this->m_stamina > 99) {
            i32 x = g->m_10->m_5c;
            if (x == g->m_lastTilePxX && g->m_10->m_60 == g->m_lastTilePxY
                && g->RectContains(x, (i32)g->m_10) != 0) {
                CommitNeighbor(
                    g->m_tileOwnerHi,
                    g->m_tileOwnerLo,
                    g->m_lastTilePxX,
                    g->m_lastTilePxY
                );
            }
        }
        if ((u32)this->m_dwell < 0x1f5) {
            return 1;
        }
        if (TileSwitch6(g->m_10->m_5c >> 5, g->m_10->m_60 >> 5, 0, this->m_arrivalFlags, 1, 0)
            == 0) {
            return 1;
        }
        if (this->m_390 != 0) {
            i32 r = GruntPointVisible(
                F(P(g_pGameRegistry->m_world->m_24, 0x5c), 0) + 0x40,
                this->m_10->m_5c,
                this->m_10->m_60
            );
            if (r != 0) {
                g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
            }
            this->m_390 = 0;
            this->m_dwell = 0;
            return 1;
        }
    }
    this->m_dwell = 0;
    return 1;
}

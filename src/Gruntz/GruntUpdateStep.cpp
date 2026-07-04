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
// was wrong). The CGrunt field bag stays raw F()/P() offset (naming the ~40 touched
// members is codegen-neutral but noisy; the raw offsets are the load-bearing shape).
#include <Gruntz/Grunt.h> // canonical CGrunt / CGruntTileMgr / CGruntCueSink / CGameRegistry

#include <rva.h>
#include <string.h> // inline strcmp of the grunt type name
#include <Gruntz/StepList2.h>
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
extern CStepList2 g_dropList;

// ===========================================================================
// @early-stop
// Deep per-tick AI step: ~18 reloc-masked engine calls (mixed __thiscall receivers
// on this/other-grunt/grid + the on-screen cue + __cdecl frees), the manager-grid
// chain and the free-list splice. Logic reconstructed from the decomp faithfully;
// MSVC5's register allocation across the long live ranges (the grunt-under-HUD
// pointer, the clock, the grid bases) and the cold-block placement do not reproduce
// instruction-for-instruction. Final-sweep candidate (deep-regalloc + branch-placement wall).
RVA(0x000f0130, 0x7c0)
i32 CGrunt::UpdateArrival() {
    char* name = g_typeColl.Lookup((i32)m_14->m_1c)->m_0;
    if (strcmp(name, "I") != 0) {
        return 1;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    CGrunt* g = m_tileMgr->GetOccupant(this);
    bool atTarget = false;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && g->RectContains(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = true;
        }
    }

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) < 100) {
            if (atTarget) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
            F(this, 0x1e4) = 0;
            F(this, 0x218) = 0;
            F(this, 0x21c) = 0;
            F(this, 0x220) = 0;
            Stub_062e10(1, 0, 0);
            return 1;
        }
        if (FindGridNeighbor(1) != 0) {
            return 1;
        }
        if (atTarget && g == 0) {
            return 1;
        }
        if (F(this, 0x220) == 0) {
            return 1;
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        Stub_062e10(1, 0, 0);
        return 1;
    }

    i32 phase = F(this, 0x2d4);
    if (phase == 0) {
        if (g != 0) {
            if (F(this, 0x3f0) > 99) {
                i32 x = F(P(g, 0x10), 0x5c);
                if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                    && g->RectContains(x, F(P(g, 0x10), 0x60)) != 0) {
                    CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
                    goto tail;
                }
            }
            if (g != 0 && (u32)F(this, 0x2ec) > 1000) {
                if (g->GruntInRadius(F(g, 0x1ec), F(g, 0x1f0)) != 0) {
                    i32 c[4];
                    GetScreenPos((GruntTilePos*)c);
                    if (TileSwitch6(c[1] >> 5, c[0] >> 5, 0, F(this, 0x248), 0, 0x20) != 0) {
                        SetEntrancePos(1, 1);
                        F(this, 0x2f0) = F(g, 0x1ec);
                        F(this, 0x2f4) = F(g, 0x1f0);
                        F(this, 0x2d4) = 1;
                        i32 r = GruntPointVisible(
                            F(P(g_pGameRegistry->m_world->m_24, 0x5c), 0) + 0x40,
                            F(P(this, 0x10), 0x5c),
                            F(P(this, 0x10), 0x60)
                        );
                        if (r != 0) {
                            g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                        }
                    }
                }
                F(this, 0x2ec) = 0;
                goto tail;
            }
        }
        if (F(this, 0x244) == 0 && F(this, 0x318) != 0 && (u32)F(this, 0x2ec) > 3000) {
            i32 cmp = -(i32)((u32)g_clock < (u32)F(this, 0x308)) - F(this, 0x30c);
            if (F(this, 0x314) < cmp
                || (F(this, 0x314) <= cmp
                    && (u32)F(this, 0x310) <= g_clock - (u32)F(this, 0x308))) {
                Stub_062e10(1, 1, 0);
                F(this, 0x308) = 0;
                F(this, 0x310) = 0;
                F(this, 0x30c) = 0;
                F(this, 0x314) = 0;
                F(this, 0x310) = GruntRand() % 30000 + 30000;
                F(this, 0x314) = 0;
                F(this, 0x308) = (i32)g_clock;
                F(this, 0x30c) = 0;
            } else {
                i32 base = F(this, 0x10);
                u32 lo = F(base, 0x134);
                i32 dx = F(base, 0x13c) - (i32)lo;
                i32 ax = (dx ^ (dx >> 31)) - (dx >> 31);
                u32 lo2 = F(base, 0x138);
                i32 dy = F(base, 0x140) - (i32)lo2;
                i32 ay = (dy ^ (dy >> 31)) - (dy >> 31);
                if (ax != 0) {
                    lo = lo + GruntRand() % ax;
                }
                if (ay != 0) {
                    lo2 = lo2 + GruntRand() % ay;
                }
                if (lo < (u32)F(g_pGameRegistry->m_tileGrid, 0xc)
                    && lo2 < (u32)F(g_pGameRegistry->m_tileGrid, 0x10)) {
                    TileSwitch6((i32)lo, (i32)lo2, 0, F(this, 0x248), 1, 0);
                }
                if (F(this, 0x328) != 0) {
                    if (ax <= ay) {
                        ax = ay;
                    }
                    if (ax < F(this, 0x328)) {
                        SetEntrancePos(1, 1);
                    }
                }
            }
            F(this, 0x2ec) = 0;
        }
    } else if (phase == 1) {
        CGrunt* slot = m_tileMgr->m_grid[F(this, 0x2f0)][F(this, 0x2f4)];
        i32 cur = m_tileMgr->GetOccupant(this) ? 1 : 0;
        CGrunt* found = m_tileMgr->GetOccupant(this);
        (void)cur;
        if (found == 0 || found == slot) {
            if (slot == 0 || F(slot, 0x1fc) == 0
                || slot->GruntInRadius(F(slot, 0x1ec), F(slot, 0x1f0)) == 0) {
                F(this, 0x2d4) = 0;
            } else {
                StepArrivalDrop(F(slot, 0x17c), F(slot, 0x180), 0, F(this, 0x248), 0, 0x20);
                if (F(this, 0x220) == 0 && F(this, 0x3f0) > 99
                    && slot->RectContains(F(P(slot, 0x10), 0x5c), F(P(slot, 0x10), 0x60)) != 0
                    && F(P(slot, 0x10), 0x5c) == F(slot, 0x17c)
                    && F(P(slot, 0x10), 0x60) == F(slot, 0x180)) {
                    CommitNeighbor(F(slot, 0x1ec), F(slot, 0x1f0), F(slot, 0x17c), F(slot, 0x180));
                    F(this, 0x2d4) = 2;
                }
            }
        } else {
            F(this, 0x2f0) = -1;
            F(this, 0x2d4) = 0;
            F(this, 0x2f4) = -1;
        }
    } else if (phase == 2) {
        F(this, 0x2d4) = 1;
    }

tail:
    if (F(this, 0x328) != 0) {
        // The active-move cell: (head node)->link is a [col,row]; gate on the grid
        // cell's flag byte (&0x20).
        i32* cell = (i32*)P(P(this, 0x320), 0x8);
        u8* flags = (u8*)(F(F(g_pGameRegistry->m_tileGrid, 0x8) + cell[1] * 4, 0) + cell[0] * 0x1c);
        if ((flags[0] & 0x20) != 0) {
            SetEntrancePos(1, 1);
            if (F(this, 0x328) != 0) {
                void* p = (void*)P(this, 0x320);
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
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    if (F(this, 0x328) != 0 && F(F(g_pGameRegistry->m_cmdGrid, 0x1c) + F(this, 0x2f0) * 4, 0) == 0) {
        void* p = (void*)P(this, 0x320);
        while (p != 0) {
            void* next = *(void**)p;
            i32* link = (i32*)((char*)p + 8);
            p = next;
            if (*link != 0) {
                g_dropList.Drop(*link);
            }
        }
        m_31c.RemoveAll();
        F(this, 0x2f0) = 0;
    }

    i32 reason = F(this, 0x170);
    if (reason > 0x16) {
        reason = F(this, 0x19c);
    }
    if (reason == 0 && (reason = F(this, 0x2f0), reason >= 0) && reason < 0xf) {
        CGrunt* slot = (CGrunt*)F(F(g_pGameRegistry->m_cmdGrid, 0x1c) + reason * 4, 0);
        if (slot == 0 || F(slot, 0x1fc) == 0) {
            if (F(this, 0x328) != 0) {
                void* p = (void*)P(this, 0x320);
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_dropList.Drop(*link);
                    }
                }
                m_31c.RemoveAll();
            }
            F(this, 0x2f0) = -1;
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
            i32 r2 = F(slot, 0x170);
            if (r2 > 0x16) {
                r2 = F(slot, 0x19c);
            }
            if (r2 != 0x14 && r2 != 1) {
                slot->LoadGruntTypeTable(r2, 1, 0, 0);
                slot->LoadGruntTypeTable(0, 1, 0, 0);
                F(this, 0x2d4) = 4;
                if (F(this, 0x328) == 0) {
                    return 1;
                }
                void* p = (void*)P(this, 0x320);
                while (p != 0) {
                    void* next = *(void**)p;
                    i32* link = (i32*)((char*)p + 8);
                    p = next;
                    if (*link != 0) {
                        g_dropList.Drop(*link);
                    }
                }
                m_31c.RemoveAll();
                return 1;
            }
        }
    }

    reason = F(this, 0x170);
    if (reason > 0x16) {
        reason = F(this, 0x19c);
    }
    if (reason == 0) {
        if (F(this, 0x328) == 0) {
            if (F(this, 0x2d4) != 0) {
                return 1;
            }
            i32 best = 0x7fffffff;
            i32 bestIdx = -1;
            i32* slots = (i32*)(F(g_pGameRegistry->m_cmdGrid, 0x1c) + (char*)0);
            i32 i = 0;
            do {
                i32 sv = slots[i];
                if (sv != 0 && F(sv, 0x1fc) != 0) {
                    i32 k = F(sv, 0x170);
                    i32 kk = k;
                    if (k > 0x16) {
                        kk = F(sv, 0x19c);
                    }
                    if (kk != 0 && kk != 0x14 && kk != 1
                        && !(k > 0x16 ? (F(sv, 0x19c) == 0x14) : false) && F(sv, 0x258) != 0x36) {
                        i32 ex = F(P(sv, 0x10), 0x5c) >> 5;
                        i32 ddx = ex - (F(P(this, 0x10), 0x5c) >> 5);
                        i32 ey = (F(P(sv, 0x10), 0x60) >> 5) - (F(P(this, 0x10), 0x60) >> 5);
                        i32 dist = ddx * ddx + ey * ey;
                        if (dist < best && dist <= F(this, 0x2dc) * F(this, 0x2dc)) {
                            best = dist;
                            bestIdx = i;
                        }
                    }
                }
                i++;
            } while (i < 0xf);
            if (bestIdx != -1) {
                F(this, 0x2f0) = bestIdx;
                i32 base = F((i32)slots[bestIdx], 0x10);
                if (TileSwitch6(F(base, 0x5c) >> 5, F(base, 0x60) >> 5, 0, F(this, 0x248), 1, 0)
                    != 0) {
                    i32 by = F(P(this, 0x10), 0x60);
                    i32 bx = F(P(this, 0x10), 0x5c);
                    i32 board = F(P(g_pGameRegistry->m_world->m_24, 0x5c), 0);
                    if (bx < F(board, 0x48) && F(board, 0x40) <= bx && by < F(board, 0x4c)
                        && F(board, 0x44) <= by) {
                        g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
                    }
                }
            }
            F(this, 0x2ec) = 0;
            return 1;
        }
        if (F(this, 0x2d4) != 0) {
            return 1;
        }
        if ((u32)F(this, 0x2ec) < 0x3e9) {
            return 1;
        }
        i32 base = F((i32)F(F(g_pGameRegistry->m_cmdGrid, 0x1c) + F(this, 0x2f0) * 4, 0), 0x10);
        TileSwitch6(F(base, 0x5c) >> 5, F(base, 0x60) >> 5, 0, F(this, 0x248), 1, 0);
    } else {
        CGrunt* g = m_tileMgr->GetOccupant(this);
        bool atTarget = false;
        if (g != 0) {
            i32 x = F(P(g, 0x10), 0x5c);
            if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                && g->RectContains(x, F(g, 0x10)) != 0) {
                atTarget = true;
            }
        }
        if (F(this, 0x220) != 0) {
            if (F(this, 0x21c) != 0) {
                F(this, 0x21c) = 0;
                return 1;
            }
            if (F(this, 0x218) != 0) {
                return 1;
            }
            if (F(this, 0x3f0) < 100) {
                if (atTarget) {
                    return 1;
                }
                if (F(this, 0x220) == 0) {
                    return 1;
                }
                F(this, 0x1e4) = 0;
                F(this, 0x218) = 0;
                F(this, 0x21c) = 0;
                F(this, 0x220) = 0;
                Stub_062e10(1, 0, 0);
                return 1;
            }
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
            if (F(this, 0x21c) != 0) {
                return 1;
            }
            F(this, 0x1e4) = 0;
            F(this, 0x218) = 0;
            F(this, 0x21c) = 0;
            F(this, 0x220) = 0;
            Stub_062e10(1, 0, 0);
            return 1;
        }
        F(this, 0x300) = F(this, 0x17c);
        F(this, 0x304) = F(this, 0x180);
        if (g == 0 || g->GruntInRadius(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
            F(this, 0x390) = 0;
            return 1;
        }
        if (F(this, 0x220) == 0 && F(this, 0x3f0) > 99) {
            i32 x = F(P(g, 0x10), 0x5c);
            if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                && g->RectContains(x, F(g, 0x10)) != 0) {
                CommitNeighbor(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
            }
        }
        if ((u32)F(this, 0x2ec) < 0x1f5) {
            return 1;
        }
        if (TileSwitch6(F(P(g, 0x10), 0x5c) >> 5, F(P(g, 0x10), 0x60) >> 5, 0, F(this, 0x248), 1, 0)
            == 0) {
            return 1;
        }
        if (F(this, 0x390) != 0) {
            i32 r = GruntPointVisible(
                F(P(g_pGameRegistry->m_world->m_24, 0x5c), 0) + 0x40,
                F(P(this, 0x10), 0x5c),
                F(P(this, 0x10), 0x60)
            );
            if (r != 0) {
                g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
            }
            F(this, 0x390) = 0;
            F(this, 0x2ec) = 0;
            return 1;
        }
    }
    F(this, 0x2ec) = 0;
    return 1;
}

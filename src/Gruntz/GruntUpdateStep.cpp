// GruntUpdateStep.cpp - two big CGrunt per-tick arrival/move update steps that sit
// right after CGrunt_0ef6b0 in retail (0xf0130 + 0xf71c0). Both are __thiscall, no
// explicit args, return 1. They drive the grunt's tile-to-tile move: resolve the
// grunt under the HUD center (m_tileMgr->FindGrunt), gate on the powered-up / arrival
// state words, and on commit fire the matching grunt cue (Func39f4 0x366/0x366) and
// re-stamp the move. 0xf71c0 is the search/seek variant (scan the 15 grid slots for
// the nearest live target). All engine helpers + the manager/grid globals are
// external (reloc-masked); the CGrunt field bag is addressed by raw offset exactly as
// retail does (naming the ~40 touched members buys nothing and would perturb the
// shared header).
#include <Ints.h>
#include <string.h> // inline strcmp of the grunt type name

#include <rva.h>

#pragma intrinsic(strcmp)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

struct CGruntStep;

// Type-name collection (g_typeColl @0x6bf650): Lookup(key)->node, node->m_0 = name.
struct CTypeNode {
    char* m_0;
};
struct CTypeColl {
    CTypeNode* Lookup(i32 key); // 0x40437c (__thiscall)
};
DATA(0x006bf650)
extern CTypeColl g_typeColl;

// Per-grunt move/path sub-manager (CGrunt+0x260): FindGrunt(this)->grunt-under-HUD.
struct CGruntTileMgr {
    CGruntStep* FindGrunt(CGruntStep* g); // 0x40253b (__thiscall)
};

// A small owned collection at CGrunt+0x31c that gets RemoveAll'd on commit.
struct CStepList {
    void RemoveAll(); // 0x5b48a6 (__thiscall)
};

// The manager singleton (g_mgrSettings @0x64556c): m_30->m_24->m_5c board base,
// m_68 grid (slot[] at +0x1c), m_70 dims (m_c/m_10), plus a +0x68 sub the
// scatter helper (0x14bf) is a __thiscall on.
struct MgrSub24 {
    char pad[0x5c];
    i32 m_5c;
};
struct MgrSub30 {
    char pad[0x24];
    MgrSub24* m_24;
};
struct MgrGrid {
    char pad[0x1c];
    CGruntStep* slot[15];                    // +0x1c
    i32 Scatter(i32 a, i32 b, i32 c, i32 d); // 0x4014bf (__thiscall on this grid)
};
struct MgrDims {
    char pad[0x8];
    i32 m_8;
    i32 m_c;
    i32 m_10;
};
struct MgrSettings {
    char pad30[0x30];
    MgrSub30* m_30; // +0x30
    char pad34[0x68 - 0x34];
    MgrGrid* m_68; // +0x68 (grid; slot[] at +0x1c)
    char pad6c[0x70 - 0x6c];
    MgrDims* m_70; // +0x70
};
DATA(0x0064556c)
extern MgrSettings* g_mgrSettings;

DATA(0x00645588)
extern u32 g_clock; // game clock (g_645588)
DATA(0x00645544)
extern void* g_freeList;
DATA(0x0064554c)
extern i32 g_freeListBias;

// A second list the seek variant RemoveAll-equivalent walks (g_645540).
struct CStepList2 {
    void Drop(i32 node); // 0x40163b (__thiscall on g_645540)
};
DATA(0x00645540)
extern CStepList2 g_dropList;

// Engine helpers, all reloc-masked. The thiscall ones are modeled as CGruntStep
// methods (called on `this` or on another grunt); the free ones are __stdcall/__cdecl.
extern "C" {
    void __stdcall GruntCue(CGruntStep* g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x4039f4
    i32 BoardTest(i32 a, i32 b, i32 c); // 0x401127 (__cdecl)
    i32 GameRand();                     // 0x51fee0 (__cdecl)
}

struct CGruntStep {
    i32 UpdateArrival(); // 0xf0130
    i32 SeekTarget();    // 0xf71c0

    // reloc-masked CGrunt __thiscall helpers (called on this and on other grunts):
    i32 TileProbe(i32 x, i32 y);                              // 0x403c4c
    i32 RunGate(i32 a);                                       // 0x403d5a
    void ResetEntrance(i32 a, i32 b, i32 c);                  // 0x40136b
    i32 OwnsTile(i32 a, i32 b);                               // 0x401014
    void PlaceTile(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x4014e2
    void CommitMove(i32 a, i32 b, i32 c, i32 d);              // 0x40302b
    i32 ProbeMove(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);  // 0x401640
    void StampMove(i32 a, i32 b);                             // 0x401401
    void ReadCenter(void* out);                               // 0x4036c0
    void SelectIcon(i32 a, i32 b, i32 c, i32 d);              // 0x403bd9
};

// ===========================================================================
// @early-stop
// Deep per-tick AI step: ~18 reloc-masked engine calls (mixed __thiscall receivers
// on this/other-grunt/grid + __stdcall/__cdecl frees), the manager-grid chain and the
// free-list splice. Logic reconstructed from the decomp faithfully; MSVC5's register
// allocation across the long live ranges (the grunt-under-HUD pointer, the clock, the
// grid bases) and the cold-block placement do not reproduce instruction-for-
// instruction. Final-sweep candidate (deep-regalloc + branch-placement wall).
RVA(0x000f0130, 0x7c0)
i32 CGruntStep::UpdateArrival() {
    char* name = g_typeColl.Lookup(F(P(this, 0x14), 0x1c))->m_0;
    if (strcmp(name, "I") != 0) {
        return 1;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    CGruntStep* g = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
    bool atTarget = false;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && g->TileProbe(x, F(P(g, 0x10), 0x60)) != 0) {
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
            ResetEntrance(1, 0, 0);
            return 1;
        }
        if (RunGate(1) != 0) {
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
        ResetEntrance(1, 0, 0);
        return 1;
    }

    i32 phase = F(this, 0x2d4);
    if (phase == 0) {
        if (g != 0) {
            if (F(this, 0x3f0) > 99) {
                i32 x = F(P(g, 0x10), 0x5c);
                if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                    && g->TileProbe(x, F(P(g, 0x10), 0x60)) != 0) {
                    CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
                    goto tail;
                }
            }
            if (g != 0 && (u32)F(this, 0x2ec) > 1000) {
                if (g->OwnsTile(F(g, 0x1ec), F(g, 0x1f0)) != 0) {
                    i32 c[4];
                    ReadCenter(c);
                    if (ProbeMove(c[1] >> 5, c[0] >> 5, 0, F(this, 0x248), 0, 0x20) != 0) {
                        StampMove(1, 1);
                        F(this, 0x2f0) = F(g, 0x1ec);
                        F(this, 0x2f4) = F(g, 0x1f0);
                        F(this, 0x2d4) = 1;
                        i32 r = BoardTest(
                            F(P(g_mgrSettings->m_30->m_24, 0x5c), 0) + 0x40,
                            F(P(this, 0x10), 0x5c),
                            F(P(this, 0x10), 0x60)
                        );
                        if (r != 0) {
                            GruntCue(this, 0x366, -1, 0, -1, -1);
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
                ResetEntrance(1, 1, 0);
                F(this, 0x308) = 0;
                F(this, 0x310) = 0;
                F(this, 0x30c) = 0;
                F(this, 0x314) = 0;
                F(this, 0x310) = GameRand() % 30000 + 30000;
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
                    lo = lo + GameRand() % ax;
                }
                if (ay != 0) {
                    lo2 = lo2 + GameRand() % ay;
                }
                if (lo < (u32)F(g_mgrSettings->m_70, 0xc)
                    && lo2 < (u32)F(g_mgrSettings->m_70, 0x10)) {
                    ProbeMove((i32)lo, (i32)lo2, 0, F(this, 0x248), 1, 0);
                }
                if (F(this, 0x328) != 0) {
                    if (ax <= ay) {
                        ax = ay;
                    }
                    if (ax < F(this, 0x328)) {
                        StampMove(1, 1);
                    }
                }
            }
            F(this, 0x2ec) = 0;
        }
    } else if (phase == 1) {
        CGruntStep* slot = ((MgrGrid*)P(this, 0x260))->slot[F(this, 0x2f4) + F(this, 0x2f0) * 0xf];
        i32 cur = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this) ? 1 : 0;
        CGruntStep* found = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
        (void)cur;
        if (found == 0 || found == slot) {
            if (slot == 0 || F(slot, 0x1fc) == 0
                || slot->OwnsTile(F(slot, 0x1ec), F(slot, 0x1f0)) == 0) {
                F(this, 0x2d4) = 0;
            } else {
                PlaceTile(F(slot, 0x17c), F(slot, 0x180), 0, F(this, 0x248), 0, 0x20);
                if (F(this, 0x220) == 0 && F(this, 0x3f0) > 99
                    && slot->TileProbe(F(P(slot, 0x10), 0x5c), F(P(slot, 0x10), 0x60)) != 0
                    && F(P(slot, 0x10), 0x5c) == F(slot, 0x17c)
                    && F(P(slot, 0x10), 0x60) == F(slot, 0x180)) {
                    CommitMove(F(slot, 0x1ec), F(slot, 0x1f0), F(slot, 0x17c), F(slot, 0x180));
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
        u8* flags = (u8*)(F(F(g_mgrSettings->m_70, 0x8) + cell[1] * 4, 0) + cell[0] * 0x1c);
        if ((flags[0] & 0x20) != 0) {
            StampMove(1, 1);
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
                ((CStepList*)((char*)this + 0x31c))->RemoveAll();
            }
            StampMove(cell[0] * 0x20 + 0x10, cell[1] * 0x20 + 0x10);
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
i32 CGruntStep::SeekTarget() {
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    if (F(this, 0x328) != 0 && F(F(g_mgrSettings->m_68, 0x1c) + F(this, 0x2f0) * 4, 0) == 0) {
        void* p = (void*)P(this, 0x320);
        while (p != 0) {
            void* next = *(void**)p;
            i32* link = (i32*)((char*)p + 8);
            p = next;
            if (*link != 0) {
                g_dropList.Drop(*link);
            }
        }
        ((CStepList*)((char*)this + 0x31c))->RemoveAll();
        F(this, 0x2f0) = 0;
    }

    i32 reason = F(this, 0x170);
    if (reason > 0x16) {
        reason = F(this, 0x19c);
    }
    if (reason == 0 && (reason = F(this, 0x2f0), reason >= 0) && reason < 0xf) {
        CGruntStep* slot = (CGruntStep*)F(F(g_mgrSettings->m_68, 0x1c) + reason * 4, 0);
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
                ((CStepList*)((char*)this + 0x31c))->RemoveAll();
            }
            F(this, 0x2f0) = -1;
            return 1;
        }
        // Adjacency probe: read this grunt's HUD center + the slot's, in tile units,
        // and require both axis deltas < 2 (the slot is the immediate neighbor).
        i32 c0[4];
        ReadCenter(c0);
        i32 cy = c0[1] >> 5;
        i32 d0[4];
        ReadCenter(d0);
        i32 e0[4];
        ReadCenter(e0);
        i32 f0[4];
        ReadCenter(f0);
        i32 dx = (f0[1] >> 5) - (f0[3] >> 5);
        i32 dy = cy - (e0[3] >> 5);
        if (((dy ^ (dy >> 31)) - (dy >> 31)) < 2 && ((dx ^ (dx >> 31)) - (dx >> 31)) < 2) {
            i32 r2 = F(slot, 0x170);
            if (r2 > 0x16) {
                r2 = F(slot, 0x19c);
            }
            if (r2 != 0x14 && r2 != 1) {
                slot->SelectIcon(r2, 1, 0, 0);
                slot->SelectIcon(0, 1, 0, 0);
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
                ((CStepList*)((char*)this + 0x31c))->RemoveAll();
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
            i32* slots = (i32*)(F(g_mgrSettings->m_68, 0x1c) + (char*)0);
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
                if (ProbeMove(F(base, 0x5c) >> 5, F(base, 0x60) >> 5, 0, F(this, 0x248), 1, 0)
                    != 0) {
                    i32 by = F(P(this, 0x10), 0x60);
                    i32 bx = F(P(this, 0x10), 0x5c);
                    i32 board = F(P(g_mgrSettings->m_30->m_24, 0x5c), 0);
                    if (bx < F(board, 0x48) && F(board, 0x40) <= bx && by < F(board, 0x4c)
                        && F(board, 0x44) <= by) {
                        GruntCue(this, 0x366, -1, 0, -1, -1);
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
        i32 base = F((i32)F(F(g_mgrSettings->m_68, 0x1c) + F(this, 0x2f0) * 4, 0), 0x10);
        ProbeMove(F(base, 0x5c) >> 5, F(base, 0x60) >> 5, 0, F(this, 0x248), 1, 0);
    } else {
        CGruntStep* g = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
        bool atTarget = false;
        if (g != 0) {
            i32 x = F(P(g, 0x10), 0x5c);
            if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                && g->TileProbe(x, F(g, 0x10)) != 0) {
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
                ResetEntrance(1, 0, 0);
                return 1;
            }
            if (RunGate(1) != 0) {
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
            ResetEntrance(1, 0, 0);
            return 1;
        }
        F(this, 0x300) = F(this, 0x17c);
        F(this, 0x304) = F(this, 0x180);
        if (g == 0 || g->OwnsTile(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
            F(this, 0x390) = 0;
            return 1;
        }
        if (F(this, 0x220) == 0 && F(this, 0x3f0) > 99) {
            i32 x = F(P(g, 0x10), 0x5c);
            if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                && g->TileProbe(x, F(g, 0x10)) != 0) {
                CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
            }
        }
        if ((u32)F(this, 0x2ec) < 0x1f5) {
            return 1;
        }
        if (ProbeMove(F(P(g, 0x10), 0x5c) >> 5, F(P(g, 0x10), 0x60) >> 5, 0, F(this, 0x248), 1, 0)
            == 0) {
            return 1;
        }
        if (F(this, 0x390) != 0) {
            i32 r = BoardTest(
                F(P(g_mgrSettings->m_30->m_24, 0x5c), 0) + 0x40,
                F(P(this, 0x10), 0x5c),
                F(P(this, 0x10), 0x60)
            );
            if (r != 0) {
                GruntCue(this, 0x366, -1, 0, -1, -1);
            }
            F(this, 0x390) = 0;
            F(this, 0x2ec) = 0;
            return 1;
        }
    }
    F(this, 0x2ec) = 0;
    return 1;
}

SIZE_UNKNOWN(MgrDims);

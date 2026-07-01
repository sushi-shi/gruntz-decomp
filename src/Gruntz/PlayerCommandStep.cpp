// PlayerCommandStep.cpp - 0xd1b60, the player-command dispatch (CGruntzMgr-side).
// __thiscall(a2..a8) ret 0x1c, returns int. Switches on (a4 & 0xff) - the command
// code 0..10 - and applies it to the addressed grid grunt (m_4->m_68 slot grid,
// 15-wide rows): spawn-probe (0), move/attack/tool variants (2..5,9,10), select/
// deselect (6,7), and the conversion/pickup pre-pass (8). a2 = player id (== g_644c54
// is "local"), a3 = column, a5/a6 = pixel target or a second grid cell, a7/a8 spare.
// The grunt-state reset block (clear +0x308.. / +0x420 / mask +0x248) repeats across
// most cases. All engine helpers + the manager/registry globals are external
// (reloc-masked); the grunt/grid/this field bags are raw-offset addressed as retail.
#include <Ints.h>

#include <rva.h>

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

struct CGrid;
struct CGruntObj;

// The addressed-grid grunt: only its raw state offsets are touched here.
struct CGruntObj {
    void StampMove(i32 a, i32 b); // 0x401401 (__thiscall)
};

// The world grid (m_4->m_68): slot[] of grunt pointers at +0x1c plus the per-command
// reach/path probes the handler runs against it.
struct CGrid {
    char pad[0x1c];
    CGruntObj* slot[15]; // +0x1c (15-wide rows)
    i32 SpawnProbe(
        i32 player,
        i32 x,
        i32 y,
        i32 a,
        i32 b,
        void* ctx,
        i32 c,
        i32 d,
        i32 e,
        i32 f,
        i32 g,
        i32 h,
        i32 i
    );                                                        // 0x4040bb
    i32 MoveTo(i32 player, i32 col, i32 x, i32 y, i32 mode);  // 0x401e51
    i32 PathProbe(i32 x, i32 y, i32* outA, i32* outB, i32 n); // 0x403deb
    i32 Reach(i32 player, i32 a, i32 b, i32 c);               // 0x4014bf
    i32 ReachB(i32 player, i32 a, i32 b, i32 c);              // 0x403030
    void Refresh();                                           // 0x4036ed
    void Convert(i32 player, i32 col, i32 a, i32 b);          // 0x4029cd
};

// This handler object (CGruntzMgr): m_4 = world (->m_68 grid, +0xc gate), m_c chain.
struct CCmdHandler {
    i32 Dispatch(u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8); // 0xd1b60
    void SetTarget(i32 a, i32 b, i32 c, i32 d);                           // 0x4012a8
    void NotifySelect(i32 a);                                             // 0x4017a8
    void Defended(i32 a, i32 b);                                          // 0x40213f
};

// Bute-config manager (g_buteMgr @0x6453d8): read the defender-radius value.
struct CButeMgr {
    i32 ReadRadius(const char* sec, const char* key, i32 def); // 0x40171aa0->0x571aa0
};
DATA(0x006453d8)
extern CButeMgr g_buteMgr;

// Cue-tag holder (g_sndCueTag @0x61ab24): the missed-select complaint cue.
struct CCueTag {
    void Complain(i32 a, i32 b, i32 c); // 0x4025fe (__thiscall on the tag)
};
DATA(0x0061ab24)
extern CCueTag g_sndCueTag;

DATA(0x00644c54)
extern i32 g_localPlayer; // g_644c54
DATA(0x00644ca4)
extern void* g_renderCtx; // g_644ca4
DATA(0x0064556c)
extern char* g_mgrSettings; // ->m_134

// Free engine helpers (reloc-masked).
extern "C" {
    void __stdcall GruntCue(CGruntObj* g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x4039f4
    i32 BadSelect(const char* msg);                     // 0x402cca (__cdecl)
    i32 PickupCheck(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x403c6a (__cdecl)
}

// @early-stop
// 11-case command switch + the shared jump table (switchdataD_004d2790, a scoring
// artifact) + ~16 reloc-masked engine calls whose precise receivers/arg reuse
// (the &a4/&a8 path-probe outputs threaded into the reach calls, the per-case
// pixel-vs-grid arg overloading) MSVC5 schedules through registers the decomp can't
// fully attribute. The grunt-state reset blocks + slot lookups are faithful; the
// call arg-staging + cold-case placement park it. Final-sweep candidate.
RVA(0x000d1b60, 0xc2f)
i32 CCmdHandler::Dispatch(u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8) {
    i32 localP = g_localPlayer;
    char* world = P(this, 4);
    if (F(world, 0xc) != 0) {
        return 0;
    }
    CGrid* grid = (CGrid*)P(world, 0x68);
    i32 res;

    switch (a4 & 0xff) {
        default:
            return 1;

        case 0: {
            i32 r = grid->SpawnProbe(
                a2 & 0xff,
                a5 & 0xffff,
                a6 & 0xffff,
                100000,
                2,
                g_renderCtx,
                0,
                0,
                0,
                0,
                0,
                0,
                0
            );
            if (r != -1) {
                if ((a2 & 0xff) == (u32)localP) {
                    grid->Refresh();
                }
                return 1;
            }
            if (F(F(P(this, 0xc), 0x28), 0x30) == 0) {
                if (BadSelect((const char*)0x612c28) != 0) {
                    g_sndCueTag.Complain(0, 0, 0);
                }
            }
            return 0;
        }

        case 2: {
            a2 &= 0xff;
            CGruntObj* g = grid->slot[(a3 & 0xff) + a2 * 0xf];
            if (g != 0 && F(g, 0x1fc) != 0) {
                F(g, 0x230) = 0;
            }
            res = grid->MoveTo(a2, a3 & 0xff, a5 & 0xffff, a6 & 0xffff, 0);
            if (res != 0) {
                if (a2 != (u32)localP) {
                    return 1;
                }
                if (g != 0 && F(g, 0x1fc) != 0) {
                    GruntCue(g, 0x323, -1, 0, -1, -1);
                }
                return 1;
            }
            if (a2 != (u32)localP || g == 0 || F(g, 0x1fc) == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 3:
        case 4: {
            u32 player = a2 & 0xff;
            CGruntObj* g = grid->slot[(a3 & 0xff) + player * 0xf];
            if (g == 0 || F(g, 0x1fc) == 0) {
                return 0;
            }
            if ((a4 & 0xff) == 4 && F(g, 0x1e4) != 0) {
                return 0;
            }
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->StampMove(1, 1);
            }
            a6 &= 0xffff;
            a5 &= 0xffff;
            i32 oa = (i32)a4, ob = (i32)a8;
            char* node = (char*)grid->PathProbe(a5, a6, &oa, &ob, 5);
            if (node == 0 || F(g, 0x1e4) != 0) {
                F(g, 0x230) = 0;
            } else {
                SetTarget((i32)player, a5, F(P(node, 0x10), 0x5c), F(P(node, 0x10), 0x60));
            }
            res = ((a4 & 0xff) == 3) ? grid->Reach(player, oa, ob, 0)
                                     : grid->ReachB(player, oa, ob, 0);
            if (res != 0) {
                if (res != -1) {
                    if (player != (u32)localP) {
                        return 1;
                    }
                    if (F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x323, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = grid->MoveTo(player, oa, ob, 0, ((a4 & 0xff) == 3) ? 2 : 3);
                if (res != 0) {
                    if (player != (u32)localP) {
                        return 1;
                    }
                    if (F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x323, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (player != (u32)localP || F(g, 0x1fc) == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != (u32)localP) {
                return 0;
            }
            res = F(g, 0x1fc);
            if (res == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 5: {
            CGruntObj* g = grid->slot[(a2 & 0xff) * 0xf + (a3 & 0xff)];
            if (g == 0 || F(g, 0x1fc) == 0 || F(g, 0x1e4) != 0) {
                return 0;
            }
            g->StampMove(1, 1);
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->StampMove(1, 1);
            }
            return 1;
        }

        case 6: {
            CGruntObj* g = grid->slot[(a2 & 0xff) * 0xf + (a3 & 0xff)];
            if (g != 0) {
                if (F(g, 0x420) != 1) {
                    F(g, 0x308) = 0;
                    F(g, 0x310) = 0;
                    F(g, 0x30c) = 0;
                    F(g, 0x314) = 0;
                    F(g, 0x300) = F(g, 0x17c);
                    F(g, 0x420) = 1;
                    F(g, 0x304) = F(g, 0x180);
                    switch (F(g, 0x170)) {
                        case 2:
                        case 9:
                        case 10:
                        case 0xb:
                        case 0x15:
                        case 0x16:
                            F(g, 0x2dc) = 1;
                            break;
                        default:
                            F(g, 0x2dc) = g_buteMgr.ReadRadius(
                                              (const char*)0x60a9ec,
                                              (const char*)0x60e1ac,
                                              3
                                          )
                                          + 1;
                    }
                    F(g, 0x248) |= 0x18040402;
                    F(g, 0x2f0) = -1;
                    F(g, 0x2d0) = 4;
                    F(g, 0x2d4) = 0;
                    F(g, 0x2f4) = -1;
                    F(g, 0x230) = 0;
                    F(P(g, 0x10), 0x134) = 0;
                    F(P(g, 0x10), 0x13c) = 0;
                    F(P(g, 0x10), 0x138) = 0;
                    F(P(g, 0x10), 0x140) = 0;
                    g->StampMove(1, 1);
                }
                F(g, 0x464) = 0;
            }
            return 1;
        }

        case 7: {
            CGruntObj* g = grid->slot[(a2 & 0xff) * 0xf + (a3 & 0xff)];
            if (g == 0 || F(g, 0x420) == 0) {
                return 1;
            }
            F(g, 0x308) = 0;
            F(g, 0x310) = 0;
            F(g, 0x30c) = 0;
            F(g, 0x314) = 0;
            F(g, 0x420) = 0;
            F(g, 0x2d0) = 0;
            F(g, 0x248) &= 0xe7fbfbfd;
            g->StampMove(1, 1);
            return 1;
        }

        case 8: {
            a2 &= 0xff;
            if (a2 == (u32)localP) {
                F(this, 0x4f0) = 0;
            }
            i32 idx = (a3 & 0xff) + a2 * 0xf;
            CGruntObj* g = grid->slot[idx];
            if (g != 0 && F(g, 0x1fc) != 0 && F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->StampMove(1, 1);
            }
            CGruntObj* g2 = ((CGrid*)P(P(this, 4), 0x68))->slot[idx];
            i32 r;
            if (g2 == 0 || F(g2, 0x1fc) == 0) {
                r = 0;
            } else {
                r = PickupCheck(a7 & 0xff, 0, 0, 0, F(g_mgrSettings, 0x134) != 1);
            }
            i32 sel;
            if (r == 0) {
                sel = 0;
            } else {
                if (a2 == (u32)localP) {
                    grid->Convert(a2, a3 & 0xff, 0, 0);
                }
                sel = 1;
            }
            if (a2 == (u32)localP) {
                F(this, 0x36c) = 0;
                Defended(sel, F(this, 0x2f4));
                NotifySelect(0);
            }
            return r;
        }

        case 9: {
            u32 player = a2 & 0xff;
            CGruntObj* g = grid->slot[(a3 & 0xff) + player * 0xf];
            if (g == 0 || F(g, 0x1fc) == 0) {
                return 0;
            }
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->StampMove(1, 1);
            }
            u32 row = a5 & 0xffff, col = a6 & 0xffff;
            CGruntObj* g2 = ((CGrid*)P(P(this, 4), 0x68))->slot[col + row * 0xf];
            if (g2 == 0 || F(g, 0x1e4) != 0) {
                F(g, 0x230) = 0;
                return 0;
            }
            char* m10 = P(g2, 0x10);
            SetTarget(row, col, F(m10, 0x5c), F(m10, 0x60));
            res = grid->Reach(player, a7, row, 0);
            if (res != 0) {
                if (res == -1) {
                    res = grid->MoveTo(player, a8, a2, 0, 2);
                    if (res == 0) {
                        if (player != (u32)localP || F(g, 0x1fc) == 0) {
                            return 0;
                        }
                        GruntCue(g, 0x324, -1, 0, -1, -1);
                        return 0;
                    }
                    if ((a2 & 0xff) != (u32)localP) {
                        return 1;
                    }
                    if ((u32)localP != a4 && F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if ((a2 & 0xff) != (u32)localP) {
                    return 1;
                }
                if ((u32)localP != a8 && F(g, 0x1fc) != 0) {
                    GruntCue(g, 0x325, -1, 0, -1, -1);
                }
                return 1;
            }
            if (player != (u32)localP) {
                return 0;
            }
            res = F(g, 0x1fc);
            if (res == 0) {
                return 0;
            }
            GruntCue(g, 0x324, -1, 0, -1, -1);
            return 0;
        }

        case 10: {
            u32 player = a2 & 0xff;
            CGruntObj* g = grid->slot[(a3 & 0xff) + player * 0xf];
            if (g == 0 || F(g, 0x1fc) == 0 || F(g, 0x1e4) != 0) {
                return 0;
            }
            if (F(g, 0x420) != 0) {
                F(g, 0x308) = 0;
                F(g, 0x310) = 0;
                F(g, 0x30c) = 0;
                F(g, 0x314) = 0;
                F(g, 0x420) = 0;
                F(g, 0x2d0) = 0;
                F(g, 0x248) &= 0xe7fbfbfd;
                g->StampMove(1, 1);
            }
            u32 row = a5 & 0xffff, col = a6 & 0xffff;
            CGruntObj* g2 = ((CGrid*)P(P(this, 4), 0x68))->slot[col + row * 0xf];
            if (g2 == 0 || F(g, 0x1e4) != 0) {
                F(g, 0x230) = 0;
                return 0;
            }
            char* m10 = P(g2, 0x10);
            SetTarget(row, col, F(m10, 0x5c), F(m10, 0x60));
            res = grid->ReachB(player, a7, row, 0);
            if (res != 0) {
                if (res != -1) {
                    if ((a2 & 0xff) != (u32)localP) {
                        return 1;
                    }
                    if ((u32)localP != a8 && F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                res = grid->MoveTo(player, a8, a2, 0, 3);
                if (res != 0) {
                    if ((a2 & 0xff) != (u32)localP) {
                        return 1;
                    }
                    if ((u32)localP != a4 && F(g, 0x1fc) != 0) {
                        GruntCue(g, 0x325, -1, 0, -1, -1);
                    }
                    return 1;
                }
                if (player != (u32)localP || F(g, 0x1fc) == 0) {
                    return 0;
                }
                GruntCue(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != (u32)localP) {
                return 0;
            }
            res = F(g, 0x1fc);
            break;
        }
    }

    if (res == 0) {
        return 0;
    }
    return 0;
}
SIZE_UNKNOWN(CCmdHandler);
SIZE_UNKNOWN(CCueTag);
SIZE_UNKNOWN(CGrid);
SIZE_UNKNOWN(CGruntObj);

// WanderIdleStep.cpp - CGrunt idle/wander per-tick AI state machine (0xed9f0),
// re-homed from src/Stub/ApiCallers.cpp. A non-EH __thiscall step (returns 1) sitting
// in the same family as the GruntUpdateStep.cpp arrival/seek variants: it saves the
// grunt's current tile pixel (m_300/m_304), resolves the grunt under the HUD center
// (m_tileMgr->FindGrunt), gates on the powered-up / arrival state words, then runs a
// 6-way phase switch (m_2d4) that drives the tile-to-tile move / scatter / random
// wander (phases 0/1/2 commit or re-probe a queued move; phase 5 rolls a random
// neighbour tile, PtInRect-clips it against a nearby grunt's box, and probes it).
// All engine helpers + the manager/grid/free-list globals are external (reloc-masked);
// the CGrunt field bag is addressed by raw offset exactly as retail does
// (naming-independent-codegen exception, same as GruntUpdateStep.cpp).
#include <Ints.h>
#include <Win32.h> // RECT/POINT/PtInRect (game WinAPI table 0x6c456c, IAT call)

#include <rva.h>
#include <Gruntz/CGameRegistry.h>
#include <Gruntz/CStepList2.h>
#include <Gruntz/CStepList.h>

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

struct CGruntWander;

// Per-grunt move/path sub-manager (CGrunt+0x260): FindGrunt(this) + the 15-slot grid.
struct CGruntTileMgr {
    CGruntWander* FindGrunt(CGruntWander* g); // 0x40253b (__thiscall)
};

// A small owned collection at CGrunt+0x31c that gets RemoveAll'd on commit.

extern CStepList2 g_dropList;

// The tile-mgr grunt board (CGrunt+0x260) and the registry's +0x68 reused slot
// share this slot-table shape (slot[] at +0x1c).
struct MgrGrid {
    char pad[0x1c];
    CGruntWander* slot[15]; // +0x1c
};
// The +0x60 on-screen cue receiver (CGruntCueSink, forward-declared by
// CGameRegistry.h): its 6-arg grunt-cue is at 0x4039f4 (__thiscall). Completed
// here with just that method (the class is data-less, so this is layout-neutral).
SIZE_UNKNOWN(CGruntCueSink);
class CGruntCueSink {
public:
    void GruntCue(CGruntWander* g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x4039f4 (__thiscall)
};
// The game-manager singleton is the canonical CGameRegistry (*0x64556c): the board
// base via m_world->m_24->+0x5c, the +0x60 cue receiver (m_cueSink), the +0x70 tile
// grid (m_tileGrid; m_c/m_10 dims), and the +0x68 reused slot downcast to MgrGrid.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // 0x64556c
extern u32 g_clock;              // game clock (0x645588)
extern void* g_freeList;         // 0x645544
extern i32 g_freeListBias;       // 0x64554c

extern "C" {
    i32 BoardTest(i32 a, i32 b, i32 c); // 0x401127 (__cdecl)
    i32 GameRand();                     // 0x51fee0 (__cdecl)
}

SIZE_UNKNOWN(CGruntWander);
struct CGruntWander {
    i32 WanderStep(); // 0xed9f0

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
};

// ===========================================================================
// @early-stop
// Deep per-tick idle/wander AI step: ~18 reloc-masked engine calls (mixed __thiscall
// receivers on this/other-grunt + __cdecl frees), the manager-grid chain, the free-list
// splice/drop, a 6-way phase jump table and a PtInRect box-clip. Logic reconstructed in
// full from the disasm (jump-table cases + tail verified vs `llvm-objdump -dr`, incl. the
// case-5 PtInRect IAT call `call [0x6c456c]` and the manager-grid slot chain). Banked at
// ~74% fuzzy (stub was 28.6%). Two propagating codegen walls, same family as the
// GruntUpdateStep.cpp siblings:
//   (1) frame-size shift: cl reserves `sub esp,0x14`, retail `sub esp,0x18` (retail keeps
//       one extra 4-byte local slot for the CommitMove m_17c/m_180 spill), which offsets
//       every [esp+N] displacement + the epilogue `add esp,N`;
//   (2) prologue zero-register colouring swap: retail zeroes ebx (the CSE'd 0 used by all
//       the `cmp field,0` / `mov field,0`) before ebp (the atTarget flag); cl picks the
//       opposite, so every `cmp X,ebx` <-> `cmp X,ebp` flips downstream.
// Both are cl register-allocator / stack-slot tie-breaks structured C++ cannot force.
// Final-sweep candidate.
RVA(0x000ed9f0, 0x8dd)
i32 CGruntWander::WanderStep() {
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);

    i32 flag = 0;
    CGruntWander* g = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
    if (g != 0) {
        i32 gx = F(P(g, 0x10), 0x5c);
        if (gx == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && TileProbe(gx, F(P(g, 0x10), 0x60)) != 0) {
            flag = 1;
        }
    }

    // Powered-up / arrival gate: never returns except through RunGate; otherwise it
    // forces the phase to 5 and falls into the switch.
    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
        } else if (F(this, 0x218) == 0) {
            bool reset;
            if (F(this, 0x3f0) >= 0x64) {
                if (RunGate(1) != 0) {
                    F(this, 0x2d4) = 5;
                    return 1;
                }
                reset = !(flag != 0 && g == 0);
            } else {
                reset = (flag == 0);
            }
            if (reset) {
                F(this, 0x1e4) = 0;
                F(this, 0x218) = 0;
                F(this, 0x21c) = 0;
                F(this, 0x220) = 0;
                ResetEntrance(1, 0, 0);
            }
        }
        F(this, 0x2d4) = 5;
    }

    switch (F(this, 0x2d4)) {
        case 0:
            if (g != 0) {
                if (F(this, 0x220) == 0 && F(this, 0x3f0) >= 0x64
                    && F(P(g, 0x10), 0x5c) == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
                    && TileProbe(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
                    CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
                    F(this, 0x358) = 0;
                    if (F(this, 0x328) != 0) {
                        void* node = P(this, 0x320);
                        if (node != 0) {
                            do {
                                void* cur = node;
                                node = *(void**)node;
                                i32 data = *(i32*)((char*)cur + 8);
                                if (data != 0) {
                                    g_dropList.Drop(data);
                                }
                            } while (node != 0);
                        }
                        ((CStepList*)((char*)this + 0x31c))->RemoveAll();
                    }
                    F(this, 0x2d4) = 5;
                    return 1;
                }
                if ((u32)F(this, 0x2ec) > 0x3e8) {
                    if (OwnsTile(F(g, 0x1ec), F(g, 0x1f0)) != 0) {
                        i32 c[4];
                        g->ReadCenter(c);
                        if (ProbeMove(c[0] >> 5, c[1] >> 5, 0, F(this, 0x248), 1, 0) != 0) {
                            StampMove(1, 1);
                            F(this, 0x2f0) = F(g, 0x1ec);
                            F(this, 0x2f4) = F(g, 0x1f0);
                            F(this, 0x2d4) = 1;
                            if (BoardTest(
                                    F(g_gameReg->m_world->m_24, 0x5c) + 0x40,
                                    F(P(this, 0x10), 0x5c),
                                    F(P(this, 0x10), 0x60)
                                )
                                != 0) {
                                g_gameReg->m_cueSink->GruntCue(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    F(this, 0x2ec) = 0;
                    return 1;
                }
            }
            goto timeout;

        case 1: {
            CGruntWander* slot =
                ((MgrGrid*)P(this, 0x260))->slot[F(this, 0x2f4) + F(this, 0x2f0) * 0xf];
            CGruntWander* active = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
            if (active != 0 && active != slot) {
                F(this, 0x2f0) = -1;
                F(this, 0x2d4) = 0;
                F(this, 0x2f4) = -1;
                return 1;
            }
            if (slot == 0 || F(slot, 0x1fc) == 0 || OwnsTile(F(slot, 0x1ec), F(slot, 0x1f0)) == 0) {
                F(this, 0x2d4) = 0;
                return 1;
            }
            if ((u32)F(this, 0x2ec) > 0x1f4) {
                PlaceTile(F(slot, 0x17c), F(slot, 0x180), 0, F(this, 0x248), 1, 0);
                F(this, 0x2ec) = 0;
            }
            if (F(this, 0x220) != 0) {
                return 1;
            }
            if (F(this, 0x3f0) < 0x64) {
                return 1;
            }
            if (TileProbe(F(P(slot, 0x10), 0x5c), F(P(slot, 0x10), 0x60)) == 0) {
                return 1;
            }
            if (F(P(slot, 0x10), 0x5c) != F(slot, 0x17c)) {
                return 1;
            }
            if (F(P(slot, 0x10), 0x60) != F(slot, 0x180)) {
                return 1;
            }
            CommitMove(F(slot, 0x1ec), F(slot, 0x1f0), F(slot, 0x17c), F(slot, 0x180));
            F(this, 0x358) = 0;
            if (F(this, 0x328) != 0) {
                void* node = P(this, 0x320);
                if (node != 0) {
                    do {
                        void* cur = node;
                        node = *(void**)node;
                        i32 data = *(i32*)((char*)cur + 8);
                        if (data != 0) {
                            g_dropList.Drop(data);
                        }
                    } while (node != 0);
                }
                ((CStepList*)((char*)this + 0x31c))->RemoveAll();
            }
            F(this, 0x2d4) = 5;
            return 1;
        }

        case 2: {
            if (F(this, 0x220) == 0) {
                F(this, 0x2d4) = 0;
                return 1;
            }
            CGruntWander* slot =
                ((MgrGrid*)P(this, 0x260))->slot[F(this, 0x2f4) + F(this, 0x2f0) * 0xf];
            if (slot == 0 || OwnsTile(F(slot, 0x1ec), F(slot, 0x1f0)) == 0 || F(slot, 0x1fc) == 0) {
                goto ph1;
            }
            if (F(this, 0x21c) != 0) {
                return 1;
            }
            if (F(this, 0x218) != 0) {
                return 1;
            }
            if (F(this, 0x3f0) < 0x64) {
                return 1;
            }
            if (TileProbe(F(P(slot, 0x10), 0x5c), F(P(slot, 0x10), 0x60)) == 0) {
                goto ph1;
            }
            if (F(P(slot, 0x10), 0x5c) != F(slot, 0x17c)) {
                goto ph1;
            }
            if (F(P(slot, 0x10), 0x60) != F(slot, 0x180)) {
                goto ph1;
            }
            CommitMove(F(slot, 0x1ec), F(slot, 0x1f0), F(slot, 0x17c), F(slot, 0x180));
            F(this, 0x358) = 0;
            if (F(this, 0x328) != 0) {
                void* node = P(this, 0x320);
                if (node != 0) {
                    i32 prev = (i32)g_freeList;
                    do {
                        void* cur = node;
                        node = *(void**)node;
                        i32 data = *(i32*)((char*)cur + 8);
                        if (data != 0) {
                            i32* fslot = (i32*)(data - g_freeListBias);
                            *fslot = prev;
                            prev = (i32)fslot;
                            g_freeList = fslot;
                        }
                    } while (node != 0);
                }
                ((CStepList*)((char*)this + 0x31c))->RemoveAll();
            }
            F(this, 0x2d4) = 5;
            F(this, 0x2ec) = 0x1f4;
            return 1;
        ph1:
            F(this, 0x2d4) = 1;
            F(this, 0x2ec) = 0x1f4;
            return 1;
        }

        case 5: {
            if (F(this, 0x218) != 0) {
                return 1;
            }
            if (F(this, 0x3f0) >= 0x64) {
                F(this, 0x2d4) = 0;
                return 1;
            }
            if (F(this, 0x328) != 0) {
                return 1;
            }
            i32 base = F(this, 0x10);
            i32 clip = 1;
            i32 py = GameRand() % 4 + (F(base, 0x60) >> 5) - 2;
            i32 px = GameRand() % 4 + (F(base, 0x5c) >> 5) - 2;
            if ((u32)F(this, 0x2f0) < 4 && (u32)F(this, 0x2f4) < 0xf) {
                CGruntWander* entry =
                    ((MgrGrid*)g_gameReg->m_68)->slot[F(this, 0x2f0) * 0xf + F(this, 0x2f4)];
                if (entry != 0) {
                    i32 e10 = F(entry, 0x10);
                    RECT rc;
                    rc.left = (F(e10, 0x5c) >> 5) - 2;
                    rc.top = (F(e10, 0x60) >> 5) - 2;
                    rc.right = (F(e10, 0x5c) >> 5) + 3;
                    rc.bottom = (F(e10, 0x60) >> 5) + 3;
                    POINT pt;
                    pt.x = px;
                    pt.y = py;
                    if (PtInRect(&rc, pt)) {
                        clip = 0;
                    }
                }
            }
            if (clip == 0) {
                return 1;
            }
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if ((u32)px >= (u32)grid->m_c) {
                return 1;
            }
            if ((u32)py >= (u32)grid->m_10) {
                return 1;
            }
            ProbeMove(px, py, 0, F(this, 0x248), 1, 0);
            return 1;
        }

        default:
            return 1;
    }

timeout:
    if (F(this, 0x244) == 0 && F(this, 0x318) != 0 && (u32)F(this, 0x2ec) > 0xbb8) {
        i32 hi = -(i32)((u32)g_clock < (u32)F(this, 0x308)) - F(this, 0x30c);
        i32 lo = (i32)(g_clock - (u32)F(this, 0x308));
        if (F(this, 0x314) < hi || (F(this, 0x314) == hi && (u32)lo >= (u32)F(this, 0x310))) {
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
            u32 lx = (u32)F(base, 0x134);
            i32 dxr = F(base, 0x13c) - (i32)lx;
            i32 ax = (dxr ^ (dxr >> 31)) - (dxr >> 31);
            u32 ly = (u32)F(base, 0x138);
            i32 dyr = F(base, 0x140) - (i32)ly;
            i32 ay = (dyr ^ (dyr >> 31)) - (dyr >> 31);
            if (ax != 0) {
                lx += GameRand() % ax;
            }
            if (ay != 0) {
                ly += GameRand() % ay;
            }
            if (lx < (u32)F(g_gameReg->m_tileGrid, 0xc)
                && ly < (u32)F(g_gameReg->m_tileGrid, 0x10)) {
                ProbeMove((i32)lx, (i32)ly, 0, F(this, 0x248), 1, 0);
            }
            if (F(this, 0x328) != 0) {
                if (ax <= ay) {
                    ax = ay;
                }
                if (ax < F(this, 0x328)) {
                    StampMove(1, 1);
                    F(this, 0x2ec) = 0;
                    return 1;
                }
            }
        }
        F(this, 0x2ec) = 0;
    }
    return 1;
}

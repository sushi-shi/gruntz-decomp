// GruntTargetScan.cpp - CGrunt's per-tick nearest-enemy / arrival-target scan
// (0xf42f0), re-homed from src/Stub/ApiCallers.cpp. A direct sibling of
// GruntArrivalScan.cpp's ArrivalScanA/B/C (0xecc90/0xf0e20/0xf36a0) - it sits one
// slot after 0xf36a0 in retail and shares that family's whole idiom: a nested scan
// over the tile-mgr's 4x15 grunt board (g_gameReg->m_68->m_grid), a reason->priority
// switch (inlined 12x = 2 per compare site x 6 sites) that gates each candidate,
// squared-distance min tracking, a PtInRect box gate, the m_2d4 mode dispatch
// (0=wander/seek, 1=lock, 2=arrive), and a rand()-driven idle-wander tail
// (idiv 0x7530 window + idiv m_13c/m_140 nearby jitter). All engine helpers + the
// manager/grid globals are external (reloc-masked); the CGrunt field bag is addressed
// by raw offset exactly as retail does (naming-independent-codegen exception).
//
// @early-stop
// Logic reconstructed in full (every branch, the 12 inlined priority switches, the grid
// scan, the m_2d4 dispatch, the wander tail). Verified vs `llvm-objdump -dr`: the
// prologue, the nested grid scan (sar-5 tile coords, g->m_68 grid load, the
// reason->priority jump table with value-sorted blocks 2..0x17) match instruction-for-
// instruction; frame trimmed to 0x44 (retail 0x40). Residual is the same deep-regalloc +
// shared-return-tail-merge (f5476/f4a30/f4d19/f5309/f531c) + cold-block wall family as the
// three ArrivalScan siblings (all @early-stop): cl won't permute the tail merges or pin
// row/col/best/best-dist in retail's order (moving the frame 0x50->0x44 left % ~flat,
// proving the residual is block layout not displacement). Final-sweep candidate.
#include <Ints.h>
#include <string.h>

#include <Win32.h> // RECT / POINT / PtInRect
#include <rva.h>
#include <Gruntz/ScanGrid.h>
#include <stdlib.h> // engine rand (0x11fee0)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))

// The reason(m_170)->priority map, inlined at each of the 6 candidate-compare sites
// (12 jump tables). Case bodies emit the priority constants 2..0x17 in value order
// (retail's block layout); the jump table carries the reason permutation. reason 20
// and every out-of-range reason fall to the default 0x17.
#define PRIO(dst, r)                                                                               \
    switch (r) {                                                                                   \
        case 1:                                                                                    \
            dst = 2;                                                                               \
            break;                                                                                 \
        case 21:                                                                                   \
            dst = 3;                                                                               \
            break;                                                                                 \
        case 16:                                                                                   \
            dst = 4;                                                                               \
            break;                                                                                 \
        case 9:                                                                                    \
            dst = 5;                                                                               \
            break;                                                                                 \
        case 4:                                                                                    \
            dst = 6;                                                                               \
            break;                                                                                 \
        case 11:                                                                                   \
            dst = 7;                                                                               \
            break;                                                                                 \
        case 13:                                                                                   \
            dst = 8;                                                                               \
            break;                                                                                 \
        case 2:                                                                                    \
            dst = 9;                                                                               \
            break;                                                                                 \
        case 14:                                                                                   \
            dst = 10;                                                                              \
            break;                                                                                 \
        case 5:                                                                                    \
            dst = 11;                                                                              \
            break;                                                                                 \
        case 22:                                                                                   \
            dst = 12;                                                                              \
            break;                                                                                 \
        case 15:                                                                                   \
            dst = 13;                                                                              \
            break;                                                                                 \
        case 3:                                                                                    \
            dst = 14;                                                                              \
            break;                                                                                 \
        case 8:                                                                                    \
            dst = 15;                                                                              \
            break;                                                                                 \
        case 12:                                                                                   \
            dst = 16;                                                                              \
            break;                                                                                 \
        case 7:                                                                                    \
            dst = 17;                                                                              \
            break;                                                                                 \
        case 18:                                                                                   \
            dst = 18;                                                                              \
            break;                                                                                 \
        case 6:                                                                                    \
            dst = 19;                                                                              \
            break;                                                                                 \
        case 17:                                                                                   \
            dst = 20;                                                                              \
            break;                                                                                 \
        case 10:                                                                                   \
            dst = 21;                                                                              \
            break;                                                                                 \
        case 19:                                                                                   \
            dst = 22;                                                                              \
            break;                                                                                 \
        default:                                                                                   \
            dst = 23;                                                                              \
            break;                                                                                 \
    }

struct CGruntScan;

// The tile-mgr's 4x15 grunt board (g_gameReg->m_68, == CGrunt+0x260): a grid of grunt
// pointers at +0x1c (row stride 0x3c, col stride 4).
struct CScanTileMgr {
    char m_pad0[0x1c];
    CGruntScan* m_grid[4][15]; // +0x1c
};

// The board grid (g_gameReg->m_tileGrid): dims at +0xc/+0x10.

// The on-screen cue mgr (g_gameReg->m_cueSink): fires the grunt entrance cue (0x4039f4).
struct CScanCueMgr {
    void PlayCue(CGruntScan* g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x4039f4
};
struct CScanSub24 {
    char m_pad0[0x5c];
    char* m_5c; // +0x5c board base
};
struct CScanSub30 {
    char m_pad0[0x24];
    CScanSub24* m_24; // +0x24
};
struct CScanReg {
    char m_pad0[0x30];
    CScanSub30* m_world; // +0x30
    char m_pad34[0x60 - 0x34];
    CScanCueMgr* m_cueSink; // +0x60
    char m_pad64[0x68 - 0x64];
    CScanTileMgr* m_68; // +0x68 the tile-mgr grunt board
    char m_pad6c[0x70 - 0x6c];
    CScanGrid* m_tileGrid; // +0x70 the board grid (dims)
};
extern "C" CScanReg* g_mgrSettings; // _g_mgrSettings @0x64556c
extern "C" u32 g_clock;             // _g_645588 @0x645588 running game clock

// __cdecl board rect predicate (0x401127): point-in-board-rect.
extern "C" i32 BoardTest(char* board, i32 x, i32 y); // 0x401127


struct CGruntScan {
    i32 ScanNearestTarget(); // 0xf42f0

    // reloc-masked CGrunt __thiscall helpers (called on this and on other grunts).
    void ReadCenter(void* out);                                // 0x4036c0
    i32 TileProbe(i32 x, i32 y);                               // 0x403c4c
    i32 RunGate(i32 a);                                        // 0x403d5a
    void ResetEntrance(i32 a, i32 b, i32 c);                   // 0x40136b
    i32 OwnsTile(i32 a, i32 b);                                // 0x401014
    void CommitMove(i32 a, i32 b, i32 c, i32 d);               // 0x40302b
    i32 ProbeMove(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);   // 0x401640
    void ProbeMoveB(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x4014e2
    void StampMove(i32 a, i32 b);                              // 0x401401
};

RVA(0x000f42f0, 0x1193)
i32 CGruntScan::ScanNearestTarget() {
    i32 ownerHi = F(this, 0x1ec);
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    i32 cx = F(this, 0x17c) >> 5;
    i32 cy = F(this, 0x180) >> 5;

    // Scan the tile-mgr grunt board for the nearest higher-or-equal-priority target.
    CGruntScan* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 row = 0; row < 4; row++) {
        if (row == ownerHi) {
            continue;
        }
        CScanTileMgr* board = g_mgrSettings->m_68;
        for (i32 col = 0; col < 15; col++) {
            CGruntScan* cand = board->m_grid[row][col];
            if (cand != 0 && F(cand, 0x1fc) != 0 && F(cand, 0x258) != 0x36) {
                i32 pa;
                PRIO(pa, F(this, 0x170));
                i32 pb;
                PRIO(pb, F(cand, 0x170));
                if (pa <= pb) {
                    i32 dx = (F(P(cand, 0x10), 0x5c) >> 5) - cx;
                    i32 dy = (F(P(cand, 0x10), 0x60) >> 5) - cy;
                    i32 d = dx * dx + dy * dy;
                    if (d < bestDist) {
                        best = cand;
                        bestDist = d;
                    }
                }
            }
        }
    }

    // Recompute the scan box (center +- (m_2dc + m_298 + 1)) and reject `best` when its
    // center falls outside it.
    i32 halfBox = F(this, 0x2dc) + F(this, 0x298) + 1;
    i32 pt[2];
    ReadCenter(pt);
    i32 by = pt[1] >> 5;
    ReadCenter(pt);
    i32 bx = pt[0] >> 5;
    ReadCenter(pt);
    i32 t3y = pt[1] >> 5;
    ReadCenter(pt);
    i32 t4x = pt[0] >> 5;
    RECT box;
    box.left = t4x - halfBox;
    box.top = t3y - halfBox;
    box.right = bx + halfBox + 1;
    box.bottom = by + halfBox + 1;
    if (best != 0) {
        POINT pt;
        pt.x = F(best, 0x17c) >> 5;
        pt.y = F(best, 0x180) >> 5;
        if (!PtInRect(&box, pt)) {
            best = 0;
        }
    }

    // atTarget: `best` has reached its own last-tile pixel AND the tile probes free.
    i32 atTarget = 0;
    if (best != 0) {
        i32 x = F(P(best, 0x10), 0x5c);
        if (x == F(best, 0x17c) && F(P(best, 0x10), 0x60) == F(best, 0x180)
            && this->TileProbe(x, F(P(best, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    // Powered-up reset gate (identical to ArrivalScanB's m_220 path).
    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (RunGate(1) != 0) {
                return 1;
            }
            if (atTarget && best == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
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

    // m_2d4 mode dispatch.
    if (F(this, 0x2d4) == 2) {
        if (F(this, 0x220) != 0) {
            CGruntScan* sg =
                ((CScanTileMgr*)P(this, 0x260))->m_grid[F(this, 0x2f0)][F(this, 0x2f4)];
            if (sg == 0) {
                goto L_setLock;
            }
            i32 pa;
            PRIO(pa, F(this, 0x170));
            i32 pb;
            PRIO(pb, F(sg, 0x170));
            if (pa > pb) {
                goto L_setLock;
            }
            if (this->OwnsTile(F(sg, 0x1ec), F(sg, 0x1f0)) == 0) {
                goto L_setLock;
            }
            if (F(sg, 0x1fc) == 0) {
                goto L_setLock;
            }
            if (F(this, 0x21c) != 0 || F(this, 0x218) != 0 || F(this, 0x3f0) < 100) {
                return 1;
            }
            if (this->TileProbe(F(P(sg, 0x10), 0x5c), F(P(sg, 0x10), 0x60)) == 0) {
                goto L_setLock;
            }
            if (F(P(sg, 0x10), 0x5c) != F(sg, 0x17c) || F(P(sg, 0x10), 0x60) != F(sg, 0x180)) {
                goto L_setLock;
            }
            CommitMove(F(sg, 0x1ec), F(sg, 0x1f0), F(sg, 0x17c), F(sg, 0x180));
            F(this, 0x2d4) = 2;
            return 1;
        L_setLock:
            F(this, 0x2d4) = 1;
            F(this, 0x2ec) = 0x1f4;
            return 1;
        }
        F(this, 0x2d4) = 1;
        F(this, 0x2ec) = 0x1f4;
        return 1;
    }

    if (F(this, 0x2d4) == 1) {
        CGruntScan* sg = ((CScanTileMgr*)P(this, 0x260))->m_grid[F(this, 0x2f0)][F(this, 0x2f4)];
        if (best != 0 && best != sg) {
            F(this, 0x2f0) = -1;
            F(this, 0x2d4) = 0;
            F(this, 0x2f4) = -1;
            return 1;
        }
        if (sg == 0) {
            goto L_clearMode;
        }
        i32 pa;
        PRIO(pa, F(this, 0x170));
        i32 pb;
        PRIO(pb, F(sg, 0x170));
        if (pa > pb) {
            goto L_clearMode;
        }
        if (F(sg, 0x1fc) == 0) {
            goto L_clearMode;
        }
        if (this->OwnsTile(F(sg, 0x1ec), F(sg, 0x1f0)) == 0) {
            goto L_clearMode;
        }
        if ((u32)F(this, 0x2ec) > 0x1f4) {
            ProbeMoveB(F(sg, 0x17c), F(sg, 0x180), F(this, 0x248), 0, 1, 0);
            F(this, 0x2ec) = 0;
        }
        if (F(this, 0x220) != 0 || F(this, 0x3f0) < 100) {
            return 1;
        }
        if (this->TileProbe(F(P(sg, 0x10), 0x5c), F(P(sg, 0x10), 0x60)) == 0) {
            return 1;
        }
        if (F(P(sg, 0x10), 0x5c) != F(sg, 0x17c) || F(P(sg, 0x10), 0x60) != F(sg, 0x180)) {
            return 1;
        }
        CommitMove(F(sg, 0x1ec), F(sg, 0x1f0), F(sg, 0x17c), F(sg, 0x180));
        F(this, 0x2d4) = 2;
        return 1;
    L_clearMode:
        F(this, 0x2d4) = 0;
        return 1;
    }

    // m_2d4 == 0: seek / commit toward `best`, else idle wander.
    if (best == 0) {
        goto L_wander;
    }
    if (F(this, 0x220) == 0 && F(this, 0x3f0) >= 100 && F(P(best, 0x10), 0x5c) == F(best, 0x17c)
        && F(P(best, 0x10), 0x60) == F(best, 0x180)) {
        i32 pa;
        PRIO(pa, F(this, 0x170));
        i32 pb;
        PRIO(pb, F(best, 0x170));
        if (pa <= pb && this->TileProbe(F(P(best, 0x10), 0x5c), F(P(best, 0x10), 0x60)) != 0) {
            CommitMove(F(best, 0x1ec), F(best, 0x1f0), F(best, 0x17c), F(best, 0x180));
            return 1;
        }
    }

    // seek: probe-move toward best's center, stamp the move, fire the cue.
    if (best == 0) {
        goto L_wander;
    }
    {
        i32 pa;
        PRIO(pa, F(this, 0x170));
        i32 pb;
        PRIO(pb, F(best, 0x170));
        if (pa > pb) {
            goto L_wander;
        }
    }
    if ((u32)F(this, 0x2ec) <= 0x3e8) {
        goto L_wander;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    {
        i32 pa;
        PRIO(pa, F(this, 0x170));
        i32 pb;
        PRIO(pb, F(best, 0x170));
        if (pa > pb) {
            goto L_scanDone;
        }
    }
    if (this->OwnsTile(F(best, 0x1ec), F(best, 0x1f0)) == 0) {
        goto L_scanDone;
    }
    {
        i32 cc[4];
        best->ReadCenter(cc);
        if (this->ProbeMove(cc[0] >> 5, cc[1] >> 5, 0, F(this, 0x248), 1, 0) == 0) {
            goto L_scanDone;
        }
    }
    StampMove(1, 1);
    F(this, 0x2f0) = F(best, 0x1ec);
    F(this, 0x2f4) = F(best, 0x1f0);
    F(this, 0x2d4) = 1;
    {
        CScanReg* g = g_mgrSettings;
        if (BoardTest(g->m_world->m_24->m_5c + 0x40, F(P(this, 0x10), 0x5c), F(P(this, 0x10), 0x60))
            != 0) {
            g->m_cueSink->PlayCue(this, 0x366, -1, 0, -1, -1);
        }
    }
L_scanDone:
    F(this, 0x2ec) = 0;
    return 1;

L_wander:
    if (F(this, 0x244) != 0 || F(this, 0x318) == 0 || (u32)F(this, 0x2ec) <= 0xbb8) {
        return 1;
    }
    // 64-bit elapsed = g_clock - {m_308:m_30c}; compare with window {m_310:m_314}.
    {
        i32 lo = (i32)g_clock - F(this, 0x308);
        i32 hi = 0 - F(this, 0x30c) - ((u32)(i32)g_clock < (u32)F(this, 0x308) ? 1 : 0);
        i32 winHi = F(this, 0x314);
        if (hi > winHi || (hi == winHi && (u32)lo >= (u32)F(this, 0x310))) {
            // window elapsed: re-arm the idle timer with a fresh rand()%0x7530+0x7530.
            ResetEntrance(1, 1, 0);
            F(this, 0x308) = 0;
            F(this, 0x310) = 0;
            F(this, 0x30c) = 0;
            F(this, 0x314) = 0;
            F(this, 0x310) = rand() % 0x7530 + 0x7530;
            F(this, 0x314) = 0;
            F(this, 0x308) = (i32)g_clock;
            F(this, 0x30c) = 0;
        } else {
            // not elapsed: jitter to a random nearby board cell.
            char* hud = P(this, 0x10);
            i32 baseCol = F(hud, 0x134);
            i32 spanX = F(hud, 0x13c) - baseCol;
            i32 baseRow = F(hud, 0x138);
            spanX = (spanX ^ (spanX >> 31)) - (spanX >> 31);
            i32 spanY = F(hud, 0x140) - baseRow;
            spanY = (spanY ^ (spanY >> 31)) - (spanY >> 31);
            if (spanX != 0) {
                baseCol += rand() % spanX;
            }
            if (spanY != 0) {
                baseRow += rand() % spanY;
            }
            CScanGrid* grid = g_mgrSettings->m_tileGrid;
            if ((u32)baseCol < (u32)grid->m_c && (u32)baseRow < (u32)grid->m_10) {
                this->ProbeMove(baseCol, baseRow, 0, F(this, 0x248), 1, 0);
            }
            if (F(this, 0x328) != 0) {
                if (spanX > spanY) {
                    spanX = spanY;
                }
                if (F(this, 0x328) > spanX) {
                    StampMove(1, 1);
                }
            }
        }
    }
    F(this, 0x2ec) = 0;
    return 1;
}

SIZE_UNKNOWN(CGruntScan);
SIZE_UNKNOWN(CScanCueMgr);
SIZE_UNKNOWN(CScanReg);
SIZE_UNKNOWN(CScanSub24);
SIZE_UNKNOWN(CScanSub30);
SIZE_UNKNOWN(CScanTileMgr);

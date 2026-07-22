// Brickz.cpp - the scattered OUT-OF-LINE CBrickzGrid singletons (Tier B of the
// de-fragmentation assessment below). The Tier-A out-of-line pathfinding core
// (block F, 0x9ea60..0x9f7b7: AllocGrid/Search/Expand/Insert/PopFront/CellPush/
// Find/FindCellNode/Drain/Reset/Unlink/CellPop) is homed in src/Gruntz/MapMgr.cpp
// per docs/exe-map/interval-dossiers.md #10a (mapmgr + brickz-interval = ONE
// original TU; A-B-A-B weave + one bracketing init-frag run).
//
// Placeholder class name (see <Gruntz/Brickz.h>): these are __thiscall pointer-
// shuffle ops over a self-contained graph/grid container's intrusive node lists.
// They match by shape; field names are placeholders, offsets are load-bearing.
//
// ---------------------------------------------------------------------------
// DE-FRAGMENTATION ASSESSMENT (matcher-2, 2026-07-10; Tier A since re-homed):
// One dominant class, CBrickzGrid (non-polymorphic grid/pathfinding container,
// all QAE), + two tiny homed helpers (CLevelPlane::SetCell @0x77dc0,
// CTriggerMgr::FindNearestEnemy @0x77df0). Game object CBrickz is its own TU
// (src/Gruntz/CBrickz.cpp).
//
// TIER B - the scattered singletons here (Clip 0x2b340 | ComputeCellFlags 0x77790 |
//   SetCell 0x77dc0 | FindNearest 0x77df0 | SearchEdge/UpdDiag/Line 0x81e10.. |
//   IsCellClear 0x853f0 | Serialize 0x9356c) are OUT-OF-LINE functions that retail
//   CALLS: each has its own rva, reached from other TUs via incremental-link
//   thunks, and the delinked target references them as an `U <name>` extern (= a
//   linked CALL). They STAY OUT-OF-LINE here. DO NOT move them to Brickz.h.
//   PROVEN (2026-07-10, measured end-to-end): making ComputeCellFlags an `inline`
//   header member makes MSVC5 /O2 INLINE it into its callers (BuildRockBreakParticles
//   doubled 1008->2096B, the switch body folded in, 0 calls left) ->
//   RockBreakParticles 81->0, ApplyMove 70->0. The DELINKER IS NOT THE CAUSE -
//   re-attributing the rva alone (no recompile) craters nothing. Retail inlines
//   these SELECTIVELY: ComputeCellFlags is inlined ONLY into CBrickz::LoadAttributes
//   @0x810f0 (the switch consts appear at exactly 2 rvas: 0x77790 + 0x8150c) and
//   CALLED from the other 3 sites. We mirror that by hand: out-of-line member +
//   hand-written inlined body where retail inlined it (see BrickzLoad.cpp), NEVER
//   the `inline` keyword. IsCellClear is data-ref'd ONLY from vtable slots
//   ??_7CMapMgr@@6B@+0x14 & ??_7CGruntzMapMgr@@6B@+0x14 => an inline VIRTUAL of
//   CMapMgr (slot 5), mislabeled CBrickzGrid::IsCellClear (@owner-TODO). Virtuals
//   ARE safe as header-inline (vtable dispatch = `call *(%eax)`, never inlined at
//   the call site) - that is the ONLY safe header-inline case. See docs/patterns/
//   nonvirtual-inline-header-craters-delinker-packing.md (corrected: root cause is
//   MSVC inlining, not the delinker).
//
// Remaining work: per-method @early-stop residue (final sweep); (the ex-Grid_77df0
// via xref (@identity-TODO); IsCellClear -> CMapMgr slot 5 (real virtual,
// header-inline-safe); the CBrickzGrid<->CMapMgr identity reconcile (see MapMgr.cpp).
// ---------------------------------------------------------------------------
// CARVE (holding-TU drain, 2026-07-11): CBrickzGrid::Clip (0x02b340) and the 0x077790
// obj (ComputeCellFlags 0x077790 + CLevelPlane::SetCell 0x077dc0 + CTriggerMgr::
// FindNearest 0x077df0) were carved into src/Gruntz/BrickzClip_02b340.cpp and
// src/Gruntz/BrickzCellFlags_077790.cpp - each a distinct contiguous retail .text obj
// (CBrickzGrid methods compiled in 3 different objs). This file keeps ONLY the main
// pathfinding obj (SearchEdge/UpdateDiagonals/LineIsClear/IsCellClear @0x081e10..) +
// the pooled Serialize @0x09356c.
#include <Win32.h> // RECT - CMapMgr::m_bounds is a real RECT member now
#include <rva.h>
#include <Gruntz/BattlezData.h>
#include <stdlib.h> // abs (/Oi intrinsic: |goal-cur| lowers to cdq/xor/sub, not jns)

#include <Gruntz/Brickz.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// ---------------------------------------------------------------------------
// CBrickzGrid::SearchEdge (0x081e10) - run a Search between two adjacent cells with
// their edge state temporarily punched open, then restore. Bounds-check both
// cells (cols < m_c, rows < m_10); save cellA.m_0/m_4 + cellB.m_0/m_4 + cellB's
// 0x20000000 edge bit; clear that bit, set both cells' m_4 = -1 and (when
// clearFlag) m_0 = 0; set m_4c = maskA & 0x20000000; run Search(...,0x2000,...);
// then m_4c = 0 and restore every saved field. Returns Search's result.
// @early-stop
// regalloc wall (~82%): the body is byte-correct end to end - the combined bounds
// gate shares one return-0 tail, the cell save/punch/restore re-indexes m_8[row][col]
// per write (matching retail's factored col byte-offset), and the 8-arg Search call's
// interleaved push/reload schedule is byte-identical. The residual is that retail pins
// `this` in ebx (xA in edi) where MSVC5 here pins `this` in edi, plus a 1-slot frame
// delta (0x1c vs 0x18); the zero-pin/this-reg choice is not source-steerable.
RVA(0x00081e10, 0x1a7)
i32 CBrickzGrid::SearchEdge(
    i32 xA,
    i32 yA,
    i32 xB,
    i32 yB,
    void* list,
    i32 clearFlag,
    i32 maskA,
    i32 maskC
) {
    if (static_cast<u32>(xA) >= m_width || static_cast<u32>(yA) >= m_height || static_cast<u32>(xB) >= m_width || static_cast<u32>(yB) >= m_height) {
        return 0;
    }
    BrickzCell* cellB = &m_rows[yB][xB];
    BrickzCell* cellA = &m_rows[yA][xA];
    i32 savedB0 = cellB->m_0;
    i32 savedA4 = cellA->m_4;
    i32 savedA0 = cellA->m_0;
    i32 bBit29 = (static_cast<u32>(savedB0) >> 29) & 1;
    i32 savedB4 = cellB->m_4;
    if (bBit29 != 0) {
        cellB->m_0 = savedB0 & 0xdfffffff;
    }
    // Punch the edge open: re-index m_8[row][col] for each write (retail keeps the
    // col byte-offset factored and re-reads the row base rather than caching cell).
    m_rows[yA][xA].m_4 = -1;
    m_rows[yB][xB].m_4 = -1;
    m_edgeMask = maskA & 0x20000000;
    if (clearFlag != 0) {
        m_rows[yA][xA].m_0 = 0;
        m_rows[yB][xB].m_0 = 0;
    }
    i32 ret = Search(xA, yA, xB, yB, list, maskA, 0x2000, maskC);
    m_edgeMask = 0;
    m_rows[yA][xA].m_4 = savedA4;
    m_rows[yB][xB].m_4 = savedB4;
    if (clearFlag != 0) {
        m_rows[yA][xA].m_0 = savedA0;
        m_rows[yB][xB].m_0 = savedB0;
    }
    if (bBit29 != 0) {
        m_rows[yB][xB].m_0 |= 0x20000000;
    }
    return ret;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::UpdateDiagonals (0x082030) - when m_5c (dirty) is set, walk the whole
// flat cell pool and, for each cell with flag bit 0x100 set, clear its 0x1000 bit
// and re-set it if any opposite neighbour pair (UP/DOWN, RIGHT/LEFT, UR/DL, UL/DR)
// is both passable (no 0x939 bit). Clears m_5c and returns 1.
// @early-stop
// 8-neighbour spill-walk regalloc wall (~54%): logic byte-correct - the dirty gate,
// the per-cell `test ah,1` / `and ah,0xef` / `or ah,0x10` byte-flag ops, and the
// goto-cascade that sets the 0x1000 bit at one shared site all match retail. The
// residual is the shrink-wrapped callee-save push order (retail pins the cell walker
// in ebx by pushing ebx first; MSVC5 here picks ebp) and the 4 stack-slot diagonal
// neighbour layout; neither is source-steerable. Parked for the final sweep.
RVA(0x00082030, 0x1a1)
i32 CBrickzGrid::UpdateDiagonals(i32 unused) {
    BrickzCell* cell = m_cellPool;
    if (m_dirty == 0) {
        return 1;
    }
    for (u32 r = 0; r < m_height; r++) {
        for (u32 c = 0; c < m_width; c++) {
            i32 nf = cell->m_0;
            if ((nf & 0x100) != 0) {
                BrickzCell* up = (r != 0) ? cell - m_width : 0;
                BrickzCell* down = (r < m_height - 1) ? cell + m_width : 0;
                BrickzCell* right = (c < m_width - 1) ? cell + 1 : 0;
                BrickzCell* left = (c != 0) ? cell - 1 : 0;
                BrickzCell* ur = (up && right) ? up + 1 : 0;
                BrickzCell* dl = (down && left) ? down - 1 : 0;
                BrickzCell* ul = (up && left) ? up - 1 : 0;
                BrickzCell* dr = (down && right) ? down + 1 : 0;
                nf &= ~0x1000;
                cell->m_0 = nf;
                if (up && down && !(up->m_0 & 0x939) && !(down->m_0 & 0x939)) {
                    goto setbit;
                }
                if (right && left && !(right->m_0 & 0x939) && !(left->m_0 & 0x939)) {
                    goto setbit;
                }
                if (ur && dl && !(ur->m_0 & 0x939) && !(dl->m_0 & 0x939)) {
                    goto setbit;
                }
                if (ul && dr && !(ul->m_0 & 0x939) && !(dr->m_0 & 0x939)) {
                setbit:
                    cell->m_0 = nf | 0x1000;
                }
            }
            cell++;
        }
    }
    m_dirty = 0;
    return 1;
}

RVA(0x00082250, 0x17c)
i32 CBrickzGrid::LineIsClear(i32 x0, i32 y0, i32 x1, i32 y1) {
    if (x0 == x1 && y0 == y1) {
        return 1;
    }
    i32 dx = x1 - x0;
    i32 dy = y1 - y0;
    if (abs(dx) > abs(dy)) {
        i32 slope = (dy << 16) / dx;
        i32 yacc = y0 << 16;
        if (dx > 0) {
            for (i32 x = x0; x < x1; x++) {
                if (m_rows[yacc >> 16][x].m_0 != 0) {
                    return 0;
                }
                yacc += slope;
            }
        } else {
            for (i32 x = x0; x > x1; x--) {
                if (m_rows[yacc >> 16][x].m_0 != 0) {
                    return 0;
                }
                yacc += slope;
            }
        }
    } else {
        i32 slope = (dx << 16) / dy;
        i32 xacc = x0 << 16;
        if (dy > 0) {
            for (i32 y = y0; y < y1; y++) {
                if (m_rows[y][xacc >> 16].m_0 != 0) {
                    return 0;
                }
                xacc += slope;
            }
        } else {
            for (i32 y = y0; y > y1; y--) {
                if (m_rows[y][xacc >> 16].m_0 != 0) {
                    return 0;
                }
                xacc += slope;
            }
        }
    }
    return 1;
}

RVA(0x000853f0, 0x46)
i32 CBrickzGrid::IsCellClear(i32 x, i32 y) {
    i32 occ;
    if (static_cast<u32>(x) >= m_width || static_cast<u32>(y) >= m_height) {
        occ = 1;
    } else {
        occ = m_rows[y][x].m_0;
    }
    return occ == 0;
}

// ---------------------------------------------------------------------------
// DELETED (ML1, 2026-07-17): `CBrickzGrid::Serialize` @0x09356c was a PHANTOM - not a
// function at all, but the TAIL of CGruntzMgr::BroadcastCmd @0x093460, double-claimed.
// BroadcastCmd's RVA(0x00093460, 0x15c) covers 0x93460..0x935bc; this claim's
// 0x9356c..0x935a4 sat ENTIRELY INSIDE it - the same 56 bytes scored twice, under two
// names, in two units (brickz + gruntzmgr).
//
// The stack proves it, and it is just arithmetic. The block has FOUR pushes
// (ebx/ebp/esi/edi), cancelled by the `add esp,0x10` after its __cdecl call, then FIVE
// pops (edi/esi/ebp/ebx/ecx) before `ret 0x10`: it reaches its epilogue 0x14 - five
// dwords - short of what the ret needs. Impossible for a function entry. Assume instead
// a continuation of a function whose prologue pushed five registers and it closes
// exactly - and 0x93460 (the first byte after the `cc cc` int3 padding at
// 0x93440..0x9345f) is precisely `push ecx; push ebx; push ebp; push esi; push edi`.
// 0x93562 `jne 0x9356c` jumps INTO it over an early `ret 0x10` at 0x93569 (a rel8 jcc -
// invisible to a rel32 caller scan, which is why `sema xref` reported no caller at all;
// nothing in .rdata/.data points at it either, and no ILT thunk jmps to it).
//
// So the `@early-stop` "arg-forwarding-via-uninitialised-callee-saved-regs wall (~38%)"
// was a misdiagnosis, and docs/patterns/serialize-wrapper-reg-forward.md invented an ABI
// to explain it ("the forwarded values arrive already in the callee-saved registers";
// "no natural C++ signature reproduces the register-resident-args ABI"). MSVC5 invents
// no conventions: the args arrive in those registers because BROADCASTCMD'S OWN EARLIER
// BODY loads them - the body this fragment did not contain. Likewise its
// `mov eax,[esp+0x10]; mov ecx,[eax+0x7c]`, read as "arg3->m_7c, not `this`": under the
// real 5-push frame [esp+0x10] IS the saved ecx, i.e. plain `this->m_7c` (= BroadcastCmd's
// m_scoreHud, which GruntzMgr.cpp already models). That pattern doc is replaced by
// docs/patterns/unbalanced-stack-means-wrong-boundary.md.
//
// Nothing is lost: GruntzMgr.cpp already reconstructs these bytes as part of BroadcastCmd
// (the MapSerializeCurve gate, the m_scoreHud->Command forward, the neg/sbb/neg bool).
// The `extern "C" i32 __cdecl MapSerializeCurve(i32,i32,i32,i32)` this TU carried existed
// only to serve this phantom's call and dies with it - see <Gruntz/MapLogic.h>.


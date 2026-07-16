// TileScan.cpp - a 3x3 tile-region scan (orphan COMDAT @0x35f10). Gated on a
// per-frame threshold + a tile-state probe, it walks the 3x3 tile region around the
// argument's screen position (skipping the centre + out-of-bounds tiles), and on
// the first neighbour tile whose grid flags request it (the inlined grid lookup =
// the 0x75a40 archetype), notifies the argument (0x1640) and clears its latch.
// Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>
// Grunt.h pulls <Mfc.h> (the afx umbrella) - it MUST precede ScanGrid.h's <Win32.h>
// (bare windows.h), or afxv_w32.h trips C1189 (MFC-wall; see mfc-wall-is-breakable).
#include <Gruntz/Grunt.h>    // CGrunt (the scanned arg) + CGameRegistry/CFocusSlot (this->m_4)
#include <Gruntz/ScanGrid.h> // CScanGrid (this->m_c tile board)

// The tile-switch notify (0x1640 thunk -> 0x4b320) is CGrunt::TileSwitch - __thiscall
// (retail loads the grunt into ecx at every site; the convention conflation is settled).

// The scanned arg is a real CGrunt: m_2e8 the focus-slot id, m_dwell (+0x2ec) the
// dwell timer compared to the threshold, m_10 the bound HUD/object (screen x/y @
// CGrunt/CGruntHud - m_dwell @+0x2ec is the proven CGrunt signature.)
//
// CTileScan (the orphan-COMDAT scan owner, @identity-TODO) is declared in
// <Gruntz/ScanGrid.h> (included above) - its shape belongs in the shared scan header.

// The inlined grid lookup (the 0x75a40 archetype): the cell's first dword, or 1
// when out of bounds.
static inline i32 GridLookup(CScanGrid* g, i32 x, i32 y) {
    if (static_cast<u32>(x) < static_cast<u32>(g->m_c) && static_cast<u32>(y) < static_cast<u32>(g->m_10)) {
        return g->m_8[y][x].m_flags;
    }
    return 1;
}

// @early-stop
// 71%: the gate (threshold + tile-state probe via the +0x150 sub-base), the inlined
// grid lookup, the flag tests (0x939 / 2), and the notify+clear are byte-faithful in
// operations/offsets. The residual is a loop-induction regalloc wall: retail
// materializes each 3x3 bound via two independent `sar 5` copies + in-place dec/add
// and SPILLS three bounds to the 0x14-byte frame (sub esp,0x14), while cl shares one
// `sar` per centre + `lea` for the bounds and keeps them in registers (push ecx). A
// pure inner-loop register-pressure choice; no source reorder reproduced the spill.
RVA(0x00035f10, 0x155)
i32 CTileScan::Scan(CGrunt* arg) {
    if (arg->m_dwell <= m_c8) {
        return 1;
    }
    i32 v = arg->m_2e8; // the grunt's focus-slot id (index into the registry's m_focusSlots[])
    i32 ok = 0;
    if (v != -1) {
        CFocusSlot* fs = &m_4->m_focusSlots[v];
        if (fs->m_24 != 0) {
            ok = 1;
        } else if (fs->m_20 == 0) {
            ok = 1;
        }
    }
    if (ok == 0) {
        return 1;
    }

    CGameObject* p = arg->m_10;
    i32 v60 = p->m_screenY;
    i32 v5c = p->m_screenX;
    i32 a0 = v60 >> 5;
    i32 b0 = v5c >> 5;
    for (i32 a = a0 - 1; a < a0 + 2; a++) {
        for (i32 b = b0 - 1; b < b0 + 2; b++) {
            if (b == (v5c >> 5) && a == (v60 >> 5)) {
                continue;
            }
            CScanGrid* grid = m_c;
            if (static_cast<u32>(b) >= static_cast<u32>(grid->m_c) || static_cast<u32>(a) >= static_cast<u32>(grid->m_10)) {
                continue;
            }
            i32 flags = GridLookup(grid, b, a);
            if (flags & 0x939) {
                continue;
            }
            if ((flags & 2) == 0) {
                arg->TileSwitch(b, a, 0, 0xd87, 0, 0);
                arg->m_dwell = 0; // reset the dwell timer on the tile switch
                return 1;
            }
        }
    }
    return 1;
}

// (SIZE_UNKNOWN(CTileScan) now travels with the struct in <Gruntz/ScanGrid.h>.)

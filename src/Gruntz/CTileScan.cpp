// CTileScan.cpp - a 3x3 tile-region scan (orphan COMDAT @0x35f10). Gated on a
// per-frame threshold + a tile-state probe, it walks the 3x3 tile region around the
// argument's screen position (skipping the centre + out-of-bounds tiles), and on
// the first neighbour tile whose grid flags request it (the inlined grid lookup =
// the 0x75a40 archetype), notifies the argument (0x1640) and clears its latch.
// Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

struct CScanCell {
    i32 m_0;
    char _pad[0x1c - 4];
};
struct CScanGrid {
    char _00[8];
    CScanCell** m_8; // +0x08  rows
    i32 m_c;         // +0x0c  width
    i32 m_10;        // +0x10  height
};
struct CScanPos {
    char _00[0x5c];
    i32 m_5c; // +0x5c  pixel y
    i32 m_60; // +0x60  pixel x
};
struct CScanArg {
    char _00[0x10];
    CScanPos* m_10; // +0x10
    char _14[0x2e8 - 0x14];
    i32 m_2e8;                                             // +0x2e8
    i32 m_2ec;                                             // +0x2ec  latch
    void Notify(i32 x, i32 y, i32 a, i32 b, i32 c, i32 d); // 0x1640
};
struct CTileScan {
    char _00[4];
    char* m_4; // +0x04  tile array base (stride 568)
    char _08[0xc - 8];
    CScanGrid* m_c; // +0x0c  grid
    char _10[0xc8 - 0x10];
    i32 m_c8;                // +0xc8  threshold
    i32 Scan(CScanArg* arg); // 0x35f10
};

// The inlined grid lookup (the 0x75a40 archetype): the cell's first dword, or 1
// when out of bounds.
static inline i32 GridLookup(CScanGrid* g, i32 x, i32 y) {
    if ((u32)x < (u32)g->m_c && (u32)y < (u32)g->m_10) {
        return g->m_8[y][x].m_0;
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
i32 CTileScan::Scan(CScanArg* arg) {
    if (arg->m_2ec <= m_c8) {
        return 1;
    }
    i32 v = arg->m_2e8;
    i32 ok = 0;
    if (v != -1) {
        char* t = m_4 + v * 568;
        i32* s = (i32*)(t + 0x150);
        if (*(i32*)(t + 0x174) != 0) {
            ok = 1;
        } else if (s[0x20 / 4] == 0) {
            ok = 1;
        }
    }
    if (ok == 0) {
        return 1;
    }

    CScanPos* p = arg->m_10;
    i32 v60 = p->m_60;
    i32 v5c = p->m_5c;
    i32 a0 = v60 >> 5;
    i32 b0 = v5c >> 5;
    for (i32 a = a0 - 1; a < a0 + 2; a++) {
        for (i32 b = b0 - 1; b < b0 + 2; b++) {
            if (b == (v5c >> 5) && a == (v60 >> 5)) {
                continue;
            }
            CScanGrid* grid = m_c;
            if ((u32)b >= (u32)grid->m_c || (u32)a >= (u32)grid->m_10) {
                continue;
            }
            i32 flags = GridLookup(grid, b, a);
            if (flags & 0x939) {
                continue;
            }
            if ((flags & 2) == 0) {
                arg->Notify(b, a, 0, 0xd87, 0, 0);
                arg->m_2ec = 0;
                return 1;
            }
        }
    }
    return 1;
}

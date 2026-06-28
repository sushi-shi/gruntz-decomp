// CTileScan.cpp - a 3x3 tile-region scan (orphan COMDAT @0x35f10). Gated on a
// per-frame threshold + a tile-state probe, it walks the 3x3 tile region around the
// argument's screen position (skipping the centre + out-of-bounds tiles), and on
// the first neighbour tile whose grid flags request it (the inlined grid lookup =
// the 0x75a40 archetype), notifies the argument (0x1640) and clears its latch.
// Offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

struct CScanCell {
    i32 m_flags;
    char _pad[0x1c - 4];
};
struct CScanGrid {
    char _00[8];
    CScanCell** m_rows; // +0x08
    i32 m_width;        // +0x0c
    i32 m_height;       // +0x10
};
struct CScanPos {
    char _00[0x5c];
    i32 m_pixelY; // +0x5c
    i32 m_pixelX; // +0x60
};
struct CScanArg {
    char _00[0x10];
    CScanPos* m_position; // +0x10
    char _14[0x2e8 - 0x14];
    i32 m_2e8;                                             // +0x2e8
    i32 m_latch;                                           // +0x2ec
    void Notify(i32 x, i32 y, i32 a, i32 b, i32 c, i32 d); // 0x1640
};
struct CTileScan {
    char _00[4];
    char* m_tileArray; // +0x04  tile array base (stride 568)
    char _08[0xc - 8];
    CScanGrid* m_grid; // +0x0c
    char _10[0xc8 - 0x10];
    i32 m_threshold;         // +0xc8
    i32 Scan(CScanArg* arg); // 0x35f10
};

// The inlined grid lookup (the 0x75a40 archetype): the cell's first dword, or 1
// when out of bounds.
static inline i32 GridLookup(CScanGrid* g, i32 x, i32 y) {
    if ((u32)x < (u32)g->m_width && (u32)y < (u32)g->m_height) {
        return g->m_rows[y][x].m_flags;
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
    if (arg->m_latch <= m_threshold) {
        return 1;
    }
    i32 v = arg->m_2e8;
    i32 ok = 0;
    if (v != -1) {
        char* t = m_tileArray + v * 568;
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

    CScanPos* p = arg->m_position;
    i32 pixelX = p->m_pixelX;
    i32 pixelY = p->m_pixelY;
    i32 a0 = pixelX >> 5;
    i32 b0 = pixelY >> 5;
    for (i32 a = a0 - 1; a < a0 + 2; a++) {
        for (i32 b = b0 - 1; b < b0 + 2; b++) {
            if (b == (pixelY >> 5) && a == (pixelX >> 5)) {
                continue;
            }
            CScanGrid* grid = m_grid;
            if ((u32)b >= (u32)grid->m_width || (u32)a >= (u32)grid->m_height) {
                continue;
            }
            i32 flags = GridLookup(grid, b, a);
            if (flags & 0x939) {
                continue;
            }
            if ((flags & 2) == 0) {
                arg->Notify(b, a, 0, 0xd87, 0, 0);
                arg->m_latch = 0;
                return 1;
            }
        }
    }
    return 1;
}

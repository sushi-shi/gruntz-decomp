// GruntPathScan.cpp - CGrunt::PathScan57db0 (0x057db0, __thiscall ret 0, /GX). The
// per-tick grunt path-cell scan over the level plane (g_mgrSettings->m_tileGrid): recompute
// the 5x5 dirty rect around the grunt tile, walk the tracked-coord list (m_31c)
// firing the plane trigger (Probe20f4, flags m_248|0x20000000 / m_24c) on flagged
// cells, and on a hit recycle the pending-coord nodes back onto g_freeList /
// g_coordPool. With no hit it re-scans the 9x9 neighbourhood (flag 0x20040002) firing
// the same trigger, recomputes the plane dirty rect and returns 0. The scratch list
// local (CObList block-10, ctor 0x1b4867 / dtor 0x1b48c6) makes MSVC emit the /GX EH
// frame. Shares the CGrunt coord-pool / plane family; big body (0x8f8).
#include <rva.h>

#include <Ints.h>
#include <Win32.h> // RECT + IntersectRect
#include <Gruntz/CScanRectInit.h>

extern "C" char* g_mgrSettings; // _g_mgrSettings @0x64556c (plane at +0x70)

extern void* g_freeList;       // ?g_freeList@@3PAXA (0x645544)
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA (0x64554c)

struct CScanCoordPool {
    void Recycle163b(void* node); // 0x163b
};
extern CScanCoordPool g_coordPool; // ?g_coordPool@@3UCoordPool@@A (0x645540)

// --- offset-faithful views (offsets + called methods load-bearing; reloc-masked) ---
struct CScanKeyNode {
    i32 m_0, m_4; // col, row
};
struct CScanNode {     // m_320 tracked-coord node
    CScanNode* m_next; // +0x00
    i32 _04;
    CScanKeyNode* m_8; // +0x08 -> coord
};
struct CScanNode324 { // m_324
    char _00[8];
    CScanKeyNode* m_8; // +0x08
};
struct CScanList {             // scratch CObList (block size 10) - forces /GX
    CScanList(i32 blockSize);  // 0x1b4867
    ~CScanList();              // 0x1b48c6
    void* Head1b4a03();        // 0x1b4a03
    void Add1b4991(void* p);   // 0x1b4991
    void RemoveAll1b48a6();    // 0x1b48a6
    void* Find1de8(void** it); // 0x1de8
    char _00[0x18];
    i32 m_18; // +0x18 trigger out-slot
};
struct CScanSub10 { // this->m_10
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c, +0x60
};
struct CScanCell { // 0x1c bytes/cell
    char _00[3];
    u8 m_3; // +0x03 flag byte
    char _04[0x1c - 4];
};
struct CScanPlane { // grid (settings->m_70)
    char _00[8];
    CScanCell** m_8; // +0x08 row table
    i32 m_c, m_10;   // +0x0c width, +0x10 height
    char _14[0x60 - 0x14];
    i32 m_60, m_64, m_68, m_6c; // +0x60 dirty rect
    i32 m_70, m_74;             // +0x70 width, +0x74 height
    i32 Probe20f4(i32 a, i32 b, i32 col, i32 row, void* out, i32 one, i32 f, i32 g); // 0x20f4
    void Method43ea(i32 a);                                                          // 0x43ea
};
struct CGrunt { // this (ebx)
    char _00[0x10];
    CScanSub10* m_10; // +0x10
    char _14[0x248 - 0x14];
    i32 m_248, m_24c; // +0x248, +0x24c trigger flags
    char _250[0x31c - 0x250];
    char m_31c[4];       // +0x31c CObList base
    CScanNode* m_320;    // +0x320 head
    CScanNode324* m_324; // +0x324 current node
    i32 m_328;           // +0x328 pending latch
    i32 PathScan57db0();
};

// Recompute the plane dirty rect (m_60) as {0,0,w,h} intersected with a copy.
#define SCAN_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        ((CScanRectInit*)&ra)->Set34a4(0, 0, (grid)->m_c, (grid)->m_10);                           \
        RECT* pb = ((CScanRectInit*)&rb)->Set34a4(0, 0, (grid)->m_c, (grid)->m_10);                \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect((RECT*)&(grid)->m_60, &ra, &rb)) {                                      \
            *(RECT*)&(grid)->m_60 = ra;                                                            \
        }                                                                                          \
        (grid)->m_70 = (grid)->m_68 - (grid)->m_60;                                                \
        (grid)->m_74 = (grid)->m_6c - (grid)->m_64;                                                \
    }

#define FREELIST_PUSH(elem)                                                                        \
    {                                                                                              \
        void** node = (void**)((char*)(elem) - g_freeListNodeBias);                                \
        *node = g_freeList;                                                                        \
        g_freeList = node;                                                                         \
    }

// @early-stop
// large grunt path-cell scan reconstruction (final-sweep candidate): the /GX EH frame
// from the scratch CObList local, the m_328 gate, the 5x5 dirty box + IntersectRect
// clamp, the tracked-coord scan loop firing Probe20f4 (m_248|0x20000000 / m_24c)
// capped at five hits, the g_freeList pop/push + g_coordPool recycle drains, the 9x9
// neighbour re-scan (flag 0x20040002) and the plane dirty-rect recompute are
// byte-shaped and the DATA refs (g_mgrSettings / g_freeList family / g_coordPool /
// IntersectRect) pair. Residual walls: the overlapping stack-slot schedule of the
// box/coord temps, the per-iteration CObList EH-state stamps and the 8-arg Probe20f4
// push ordering diverge from retail's regalloc - re-attack leaf-first in the sweep.
RVA(0x00057db0, 0x8f8)
i32 CGrunt::PathScan57db0() {
    CScanPlane* grid = *(CScanPlane**)(g_mgrSettings + 0x70);
    if (m_328 == 0) {
        return 1;
    }
    CScanNode* node = m_320;

    i32 col5 = m_10->m_5c >> 5;
    i32 row5 = m_10->m_60 >> 5;
    RECT box;
    box.left = col5 - 2;
    box.top = row5 - 2;
    box.right = col5 + 3;
    box.bottom = row5 + 3;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_c;
    gb.bottom = grid->m_10;
    if (!IntersectRect((RECT*)&grid->m_60, &box, &gb)) {
        *(RECT*)&grid->m_60 = box;
    }
    grid->m_70 = grid->m_68 - grid->m_60;
    grid->m_74 = grid->m_6c - grid->m_64;

    i32 tcol = m_324->m_8->m_0;
    i32 trow = m_324->m_8->m_4;
    i32 hits = 0;
    i32 hitFound = 0;

    while (node != 0) {
        CScanNode* cur = node;
        node = node->m_next;
        CScanKeyNode* co = cur->m_8;
        if (co != 0) {
            i32 c = co->m_0;
            i32 r = co->m_4;
            i32 fire = 1;
            if (grid->m_8[r][c].m_3 & 0x20) {
                fire = (co->m_0 == tcol && co->m_4 == trow) ? 1 : 0;
            }
            if (fire) {
                CScanList s(0xa);
                i32 res =
                    grid->Probe20f4(c, r, co->m_0, co->m_4, &s.m_18, 1, m_248 | 0x20000000, m_24c);
                if (res != 0) {
                    if (s.m_18 != 0) {
                        hitFound = 1;
                        break;
                    }
                } else {
                    hits++;
                }
            }
        }
        if (hits == 5) {
            break;
        }
    }

    if (hitFound) {
        // recover the remaining tracked nodes onto the free-list
        while (node != 0) {
            CScanNode* cur = node;
            node = node->m_next;
            if (g_freeList != 0) {
                CScanKeyNode* co = cur->m_8;
                void** fn = (void**)g_freeList;
                fn[0] = (void*)co->m_0;
                fn[1] = (void*)co->m_4;
                g_freeList = *fn;
                ((CScanList*)m_31c)->Add1b4991(fn);
            }
        }
        if (m_328 != 0) {
            CScanNode* nd = m_320;
            while (nd != 0) {
                void* r = ((CScanList*)m_31c)->Find1de8((void**)&nd);
                if (*(void**)r != 0) {
                    g_coordPool.Recycle163b(*(void**)r);
                }
            }
            ((CScanList*)m_31c)->RemoveAll1b48a6();
        }
        void* elem = ((CScanList*)m_31c)->Head1b4a03();
        if (elem != 0) {
            FREELIST_PUSH(elem);
        }
        ((CScanList*)m_31c)->RemoveAll1b48a6();
        SCAN_BOUNDS(grid);
        return 1;
    }

    // ---- no hit: 9x9 neighbour re-scan ----
    SCAN_BOUNDS(grid);
    i32 nl = col5 - 4;
    i32 nr = col5 + 4;
    i32 nt = row5 - 4;
    i32 nb = row5 + 4;
    if (col5 >= nl && col5 < nr && row5 >= nt && row5 < nb) {
        SCAN_BOUNDS(grid);
        for (i32 dy = 0; dy < 2; dy++) {
            for (i32 dx = 0; dx < 2; dx++) {
                i32 rr = row5 + dy;
                i32 cc = col5 * 7 + dx;
                i32 cf = 1;
                if ((u32)rr < (u32)grid->m_10 && (u32)cc < (u32)grid->m_c) {
                    cf = ((i32*)grid->m_8[rr])[cc];
                }
                if (((m_248 | 0x20040002) & cf) & 0x20000000) {
                    continue;
                }
                if (((m_248 | 0x20040002) & cf) != 0 && (m_24c & cf) == 0) {
                    continue;
                }
                CScanList s(0xa);
                i32 res =
                    grid->Probe20f4(col5, row5, col5, row5, &s.m_18, 1, m_248 | 0x20040002, m_24c);
                if (res != 0 && s.m_18 != 0) {
                    void* elem = s.Head1b4a03();
                    if (elem != 0) {
                        FREELIST_PUSH(elem);
                    }
                }
            }
        }
    }
    grid->Method43ea(0);
    return 0;
}

SIZE_UNKNOWN(CScanCoordPool);
SIZE_UNKNOWN(CScanKeyNode);
SIZE_UNKNOWN(CScanList);
SIZE_UNKNOWN(CScanNode);
SIZE_UNKNOWN(CScanPlane);
SIZE_UNKNOWN(CScanSub10);

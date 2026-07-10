// DemoCameraTools.cpp - a Ghidra-missed contiguous .text block (0x3c300..0x3cf6c)
// that Ghidra never carved. It hosts the demo/attract auto-scroll camera director
// (0x3c300), a pair of orphan orientation-step helpers (0x3c850/0x3c8a0, table-
// driven, no callers/RTTI), and the two bute-config debug editor DialogProcs
// (0x3c990 "Attributez.txt" / 0x3cdd0 "dwrects.txt"). Field names are placeholders;
// only OFFSETS + code bytes are load-bearing.
#include <Mfc.h> // CString + Win32 (GetDlgItemTextA/SetDlgItemTextA/EndDialog)
#include <Ints.h>
#include <rva.h>
#include <stdlib.h>           // rand (0x11fee0, the engine LCG)
#include <Gruntz/GameLevel.h> // canonical CLevelPlane (RecomputePlaneCoords 0x161c90)

// ---------------------------------------------------------------------------
// 0x3c300 - the demo/attract auto-scroll camera director (__cdecl(owner)).
// ---------------------------------------------------------------------------
// @identity-TODO: owner->m_7c is the demo/level render context (owns the plane
// list + main plane via m_c->m_24; Ghidra carved neither, no RTTI). The plane
// objects are the canonical CLevelPlane; the surrounding holders are modeled by
// their proven offsets until the context class is recovered.
SIZE_UNKNOWN(ScrollGeomHolder);
struct ScrollGeomHolder {
    char p0[0x38];
    CLevelPlane** m_38; // +0x38  plane-layer array
    i32 m_3c;           // +0x3c  plane-layer count
    char p40[0x5c - 0x40];
    CLevelPlane* m_5c; // +0x5c  main plane
};
SIZE_UNKNOWN(ScrollViewHolder);
struct ScrollViewHolder {
    char p0[0x24];
    ScrollGeomHolder* m_24; // +0x24
};
SIZE_UNKNOWN(AutoScrollState);
struct AutoScrollState {
    char p0[0xc];
    ScrollViewHolder* m_c; // +0xc
    char p10[0x1c - 0x10];
    i32 m_1c; // +0x1c  mode (0 = pick target, 1 = scroll toward it)
    char p20[0x4c - 0x20];
    i32 m_4c, m_50; // +0x4c  per-axis scroll target
};
SIZE_UNKNOWN(ScrollOwner);
struct ScrollOwner {
    char p0[0x7c];
    AutoScrollState* m_7c; // +0x7c
};

// @early-stop
// Structure exact (switch mode-dispatch sub/je/dec/jne matches, x87 scale-multiply
// + plane loop + RecomputePlaneCoords all correct). Residual is a regalloc butterfly:
// retail pins m_c->m_24 in esi (freeing edi for the plane-loop counter i), cl pins it
// in edi (spilling i to a 3rd stack slot -> sub esp,0xc vs 0x8) + holds curY in
// edx/[esp+0x20] vs retail's ecx/[esp+0x1c]. Not source-steerable; ~73% code-correct.
RVA(0x0003c300, 0x183)
i32 DemoAutoScrollStep(ScrollOwner* owner) {
    AutoScrollState* st = owner->m_7c;
    switch (st->m_1c) {
        case 1: {
            // step the current scroll position one unit toward the target.
            ScrollGeomHolder* gh = st->m_c->m_24;
            i32 curX = gh->m_5c->m_originX;
            i32 curY = gh->m_5c->m_originY;
            if (curX < st->m_4c) {
                curX++;
            } else if (curX > st->m_4c) {
                curX--;
            }
            if (curY < st->m_50) {
                curY++;
            } else if (curY > st->m_50) {
                curY--;
            }
            // apply the (optionally parallax-scaled) coords to the main plane + recompute.
            CLevelPlane* mg = gh->m_5c;
            float fx = (float)curX;
            float fy = (float)curY;
            if (!(mg->m_flags & 1)) {
                fx *= mg->m_scaleX;
                fy *= mg->m_scaleY;
            }
            mg->m_scaledX = fx;
            mg->m_scaledY = fy;
            mg->RecomputePlaneCoords();
            // apply the same coords to every plane layer.
            for (i32 i = 0; i < gh->m_3c; i++) {
                CLevelPlane* p = gh->m_38[i];
                float px = (float)curX;
                float py = (float)curY;
                if (!(p->m_flags & 1)) {
                    px *= p->m_scaleX;
                    py *= p->m_scaleY;
                }
                p->m_scaledX = px;
                p->m_scaledY = py;
                p->RecomputePlaneCoords();
            }
            // reached the target -> back to mode 0.
            if (st->m_4c == curX && st->m_50 == curY) {
                st->m_1c = 0;
            }
            return 1;
        }
        case 0: {
            // pick a fresh random per-axis target within the main plane's wrap range.
            i32 rx = st->m_c->m_24->m_5c->m_wrapW;
            st->m_4c = (rx == -1) ? (rand() % 2 - 1) : (rand() % (rx + 1));
            i32 ry = st->m_c->m_24->m_5c->m_wrapH;
            st->m_50 = (ry == -1) ? (rand() % 2 - 1) : (rand() % (ry + 1));
            st->m_1c = 1;
            break;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x3c850 / 0x3c8a0 - twin orientation-step helpers (ORPHAN: no callers, no RTTI).
// __thiscall(count) over a 3-int orientation {facing, sub, dir}; each step looks up
// the next {facing, sub, dir} in a table indexed by (facing*3 + sub). The two tables
// (0x60d008 / 0x60d078) are the CW/CCW rotation transitions.
// @orphan - identity genuinely unrecovered (no xref/RTTI); modeled on a local helper.
// ---------------------------------------------------------------------------
DATA(0x0020d008)
extern const i32 g_rotTableA_60d008[30]; // 10 * {facing, sub, dir}
DATA(0x0020d078)
extern const i32 g_rotTableB_60d078[30];

SIZE_UNKNOWN(Orient3);
struct Orient3 {
    i32 m_0, m_4, m_8;
    void StepA(i32 count); // 0x3c850
    void StepB(i32 count); // 0x3c8a0
};

// @early-stop
// Counter-register regalloc wall: retail pins the loop counter in edi (push edi at
// entry, callee-saved) which frees edx for the m_4 temp + esi for the `e` pointer,
// so it materializes `e` once (lea edx,[eax*4+tbl]; mov esi,edx) and reads e[0..2]
// via [esi]. cl assigns count to edx (scratch), reads e[0] directly via [tbl+eax*4]
// then re-leas the base for e[1..2]. Same instrs, ~correct; not source-steerable (a
// count-copy local didn't flip it). Logic + the table DIR32 reloc are exact.
RVA(0x0003c850, 0x38)
void Orient3::StepA(i32 count) {
    if (count > 0) {
        do {
            const i32* e = &g_rotTableA_60d008[(m_0 * 3 + m_4) * 3];
            m_0 = e[0];
            m_4 = e[1];
            m_8 = e[2];
        } while (--count);
    }
}

// @early-stop
// Same counter-register regalloc wall as StepA (edi vs edx); logic + table reloc exact.
RVA(0x0003c8a0, 0x38)
void Orient3::StepB(i32 count) {
    if (count > 0) {
        do {
            const i32* e = &g_rotTableB_60d078[(m_0 * 3 + m_4) * 3];
            m_0 = e[0];
            m_4 = e[1];
            m_8 = e[2];
        } while (--count);
    }
}

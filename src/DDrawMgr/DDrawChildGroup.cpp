#include <rva.h>
#include <Wap32/Object.h>
// DDrawChildGroup.cpp - six leaf methods of the tomalla-named ddrawmgr
// sub-manager CDDrawChildGroup (a CDirectDrawMgr surface/page sub-manager in the
// "DDraw surface manager" family; see docs/ddraw-family-names.md).
//
// All six share ONE shape: walk an intrusive singly-linked list anchored at
// CDDrawChildGroup+0x14 (each node's first dword is the next-node pointer, and
// node+0x8 holds a child object), dispatching one of the child's sibling virtuals
// with a varying number of forwarded args. Some methods also dispatch the
// object's OWN +0x2c virtual after the walk.
//
// The children are:
//   +0x1c -> tail-call thunk to the object's own +0x3c virtual (no list walk)
//   +0x28 -> per-node child Slot2C(a1)                            [1 arg]
//   +0x2c -> per-node child Slot30(a1,a2)                          [2 args]
//   +0x30 -> per-node child Vfunc34(a1,a2,a3), then this->Vfunc2C  [3 args]
//   +0x34 -> per-node child Vfunc38(a1,a2,a3), then this->Vfunc2C  [3 args]
//   +0x38 -> per-node child field_0xd8 = -1 (no vtable dispatch)   [0 args]
//
// Plain /O2 /MT leaves: NO SEH frame, NO relocations - they touch only the +0x14
// list anchor, the node next/object offsets, and sibling/child vtables. Field
// names are placeholders; only OFFSETS + emitted code bytes are load-bearing.
//
// The child and "self" virtuals are modeled as a polymorphic stub (virtuals at
// the right slots) ONLY so each `p->Vfunc(...)` lowers to the exact
// `mov eax,[p]; call [eax+slot]` __thiscall dispatch; the stubs' virtuals are
// never defined, so no vtable is emitted in this TU.
// ---------------------------------------------------------------------------

#include <Gruntz/ObList.h>
#include <DDrawMgr/DDrawChildGroup.h> // THE single-source CDDrawChildGroup shape
#include <Gruntz/Viewport.h>          // CViewport (m_parent->m_24->m_5c world transform)
#include <Gruntz/ResLoadersViews.h>   // ResLoaders::DrawHost_164380 (counter draw)
#include <Win32.h>                    // SetRect + RECT

// The object reached via m_parent->+0x24->+0x5c is a CImageSet3 (the WWD image-set
// collection, defined in src/Image/ImageSet3.cpp); its Prune_1628d0 (0x1628d0)
// forwards to the spatial grid's Prune. Run by the ClearAll cleanup (0x1591f0).
class CImageSet3 : public CObject {
public:
    i32 Prune_1628d0(); // 0x1628d0 (__thiscall)
};

// CDDrawChildGroup::IsReady (0x001575e0) is now an inline member in the header.


// CDDrawChildGroup::ForwardTo3C (0x001591e0) is now an inline member in the header.


// ---------------------------------------------------------------------------
// Walk the +0x14 list dispatching child->Slot2C(a1) per node. No post-loop
// dispatch.
//
// RESIDUE: same loop-advance scheduling plateau as WalkDispatch34/34 — see comment
// below for details.
RVA(0x00159c90, 0x23)
void CDDrawChildGroup::WalkDispatch2C(i32 a1) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->Slot2C(a1);
        } while (n != 0);
    }
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list dispatching child->Slot30(a1,a2) per node. No post-loop
// dispatch.
RVA(0x00159cc0, 0x2a)
void CDDrawChildGroup::WalkDispatch30(i32 a1, i32 a2) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->Slot30(a1, a2);
        } while (n != 0);
    }
}

// Walks the +0x14 list, calling child->Vfunc34(a1,a2,a3) per node, then the
// object's own +0x2c virtual with (a2,a3). Written out in full below rather than
// sharing a helper (a function-pointer slot would not reproduce the direct
// `call [eax+0x34]`).
//
// RESIDUE (~89%, NOT a logic/offset/type/CFG error - documented store/load-
// scheduling entropy, see docs/matching-patterns.md "optimizer reorders field
// stores" and match-learnings.md loop-advance plateaus): in the per-node loop the
// target keeps the live node in ESI across the virtual call and advances it at the
// BOTTOM (`mov ecx,[esi+8]` obj-load; ...; call; `mov esi,[esi]` advance-after).
// MSVC5/c2.dll on this source instead floats the next-pointer load ABOVE the call
// via a node copy (`mov eax,esi; mov esi,[esi]; ...; mov ecx,[eax+8]`). Every loop
// form tried (for / while / do-while, with and without an explicit obj temp, and
// advancing before vs after the dispatch) yielded the same 4-instruction slip; the
// register set, offsets, arg order, and CFG are byte-exact otherwise. Left as the
// plateau - both methods share it identically.

// ---------------------------------------------------------------------------
// For each node in the +0x14 list, dispatch child +0x34 with (a1,a2,a3); then
// dispatch this->+0x2c with (a2,a3).
RVA(0x00159cf0, 0x42)
void CDDrawChildGroup::WalkDispatch34(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            n->m_obj->Vfunc34(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

// ---------------------------------------------------------------------------
// As WalkDispatch34 but the loop dispatches child +0x38.
RVA(0x00159d40, 0x42)
void CDDrawChildGroup::WalkDispatch38(i32 a1, i32 a2, i32 a3) {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            n->m_obj->Vfunc38(a1, a2, a3);
            n = n->m_next;
        } while (n != 0);
    }
    WalkDispatch30(a2, a3);
}

// ---------------------------------------------------------------------------
// Walk the +0x14 list setting each child's field at +0xd8 to -1. No vtable
// dispatch, no stack args.
RVA(0x00159d90, 0x1c)
void CDDrawChildGroup::ResetChildD8() {
    CDDrawGroupNode* n = m_head;
    if (n != 0) {
        do {
            CDDrawGroupNode* cur = n;
            n = n->m_next;
            cur->m_obj->m_d8 = -1;
        } while (n != 0);
    }
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// 0x1591f0: ClearAll cleanup - run m_parent->+0x24->+0x5c->0x1628d0 (when present),
// walk the +0x10 CObList destroying each node's child via its scalar-deleting
// destructor, then RemoveAll the +0x10 list and the +0x2c / +0x48 collections.
RVA(0x001591f0, 0x54)
void CDDrawChildGroup::DestroyChildren() {
    void* p = *(void**)((char*)m_parent + 0x24);
    if (p != 0) {
        CImageSet3* q = *(CImageSet3**)((char*)p + 0x5c);
        if (q != 0) {
            q->Prune_1628d0();
        }
    }
    CDDrawGroupNode* n = m_head;
    while (n != 0) {
        CDDrawGroupNode* cur = n;
        n = n->m_next;
        CDDrawGroupChild* obj = cur->m_obj;
        if (obj != 0) {
            delete obj;
        }
    }
    ((CObList*)((char*)this + 0x10))->RemoveAll();
    m_map2c.RemoveAll();
    m_map48.RemoveAll();
}

// NOTE: 0x159a70 (vtable slot 9, the per-frame kill-cue tick) is reconstructed as
// CWwdObjMgr::TickKillCues_159a70 in CDDrawSubMgr.cpp — that TU already models the
// real class (list/map ops + CWwdObject + InsertSorted sibling).

// ---------------------------------------------------------------------------
// 0x15a650: per-object debug-count overlay.  Only active when the +0x08 flag word
// carries 0x200000.  For each child in the +0x14 list, box its screen position
// (SetRect a 0x40x0x10 rect around +0x5c/+0x60), world-wrap the top-left into the
// visible range (the +0x04/+0x08 flag bits gate the X/Y wrap), transform the
// bottom-right through the viewport's WrapCoord, and draw the object's +0x74 count
// into the resulting rect via the counter draw-host (m_parent->m_4->m_14).  The
// viewport (m_parent->m_24->m_5c) is the shared CViewport; the child objects are
// raw-offset accessed (campaign doctrine).  __thiscall, no args.
// @early-stop
// 86.8% — logic/CFG/field-offsets/arg-order byte-identical (the SetRect box, both
// X/Y world-wrap blocks with their normalize + far-edge adjust, the top-left
// transform, the WrapCoord of the bottom-right corner, and the DrawCount all match
// instruction-for-instruction; SetRect resolves to the same `call [IAT]` import,
// reloc-masked DIR32-vs-absolute).  Residual is a zero-register-pinning wall: retail
// rotates the (drawHost/view/obj/box) live values through edx/ecx/eax/ebx where our
// cl picks ecx/eax/edx/ebx, and allocates one fewer scratch slot (box@0x34 vs our
// 0x38) — flipping the ModRM byte of most accesses.  No source lever picks the
// register order (docs/patterns/zero-register-pinning.md).
RVA(0x0015a650, 0x12c)
void CDDrawChildGroup::DrawObjectCounts_15a650() {
    if (!(m_flags08 & 0x200000)) {
        return;
    }
    char* mgr = (char*)m_parent;
    CDDrawGroupNode* node = m_head;
    ResLoaders::DrawHost_164380* drawHost =
        *(ResLoaders::DrawHost_164380**)(*(char**)(mgr + 4) + 0x14);
    CViewport* view = *(CViewport**)(*(char**)(mgr + 0x24) + 0x5c);
    if (node == 0) {
        return;
    }
    do {
        char* obj = (char*)node->m_obj;
        node = node->m_next;
        i32 ox = *(i32*)(obj + 0x5c);
        i32 oy = *(i32*)(obj + 0x60);
        RECT box;
        SetRect(&box, ox - 0x20, oy - 8, ox + 0x20, oy + 8);
        RECT rc;
        rc.right = box.right;
        rc.bottom = box.bottom;
        i32 wl = box.left;
        i32 wt = box.top;
        i32 fl = view->m_flags;
        if (fl & 4) {
            i32 w = view->m_worldWidth;
            if (box.left < 0) {
                wl = box.left + w;
            } else if (box.left >= w) {
                wl = box.left - w;
            }
            i32 farEdge = view->m_edgeR;
            if (farEdge >= w && wl < view->m_edgeL && wl <= farEdge - w) {
                wl += w;
            }
        }
        if (fl & 8) {
            i32 h = view->m_worldHeight;
            if (box.top < 0) {
                wt = box.top + h;
            } else if (box.top >= h) {
                wt = box.top - h;
            }
            i32 farEdge = view->m_edgeB;
            if (farEdge >= h && wt < view->m_edgeT && wt <= farEdge - h) {
                wt += h;
            }
        }
        rc.left = wl - view->m_edgeL + view->m_scrollX;
        rc.top = wt - view->m_edgeT + view->m_scrollY;
        view->WrapCoord(&rc.right, &rc.bottom);
        drawHost->DrawCount(&rc, *(i32*)(obj + 0x74));
    } while (node != 0);
}

SIZE_UNKNOWN(CImageSet3);

// @early-stop
// 0x15a210 (1074 B) = a CDDrawChildGroup-family debug OVERLAY, twin of
// CDDrawChildGroup::DrawObjectCounts_15a650 (same subsystem, both dead-in-retail).
// __thiscall, gated on a +0x08 debug flag; walks the +0x14 child list and per object draws
// debug geometry via CViewport::WrapCoord: CDDrawSurfacePair::DrawBox(RECT*,color) x3,
// DrawCross(x,y), ResLoaders::DrawHost2_164420::DrawLabel(RECT*,char*) (falling back to
// "???" @0x1f0a94). Draws gated by g_dbg61ab28/2c/30. Homed from GapFunctions.cpp
// (matcher-5). Homed pending reconstruction (>512 B, novel per-object geometry; the
// WrapCoord/DrawHost scaffolding is proven by 15a650).
RVA(0x0015a210, 0x432)
i32 Gap_15a210(void) {
    return 0;
}

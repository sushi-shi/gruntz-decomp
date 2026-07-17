// GruntzMapMgr.cpp - the grunt-map manager (C:\Proj\Gruntz): its two vtable
// overrides + the container teardown, in ascending retail-RVA order.
//
// The two overrides arrived here from the ex-"CMapLogic" view, which CONFLATED two
// unrelated classes (a 0x54-byte CBrickz tile leaf and this 0x94-byte manager). The
// split is binary-proven, three ways:
//   * ??_7CGruntzMapMgr @0x1e9bb4 slot[0] -> thunk 0x4430 -> 0x85480  (Reset)
//     ??_7CGruntzMapMgr @0x1e9bb4 slot[1] -> thunk 0x16a9 -> 0x82430  (Visit)
//     and slots 2..5 are BIT-IDENTICAL to ??_7CMapMgr @0x1ea3b4 - so this class
//     overrides exactly slots 0 and 1, which is exactly these two bodies.
//   * ??_7CMapMgr @0x1ea3b4 slot[0] -> thunk 0x1a91 -> 0x9ec30 (CMapMgr::Reset) and
//     slot[1] -> thunk 0x26b2 -> 0x9f7f0 (CMapMgr::Visit) - the two bases each of
//     these bodies tail-calls. Both are overrides that chain their own base.
//   * neither body has a rel32 caller: they are reached ONLY through those slots.
// The dtor half of the old view (0x113c0) is CBrickz's and is pinned as
// ??1CBrickz@@UAE@XZ in TileLogicPump.cpp; nothing of the view survives.
//
// The node-pool teardown idiom (push every non-null CPtrArray element's node back
// onto g_coordPool.m_freeHead, SetSize(0,-1), then the base grid Reset @0x9ec30) is
// shared by Reset (0x85480) and ~CGruntzMapMgr (0x85d10) - the SAME loop, which is
// why the ex-view's two halves looked alike and got conflated in the first place.
#include <Gruntz/GruntzMapMgr.h>

#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)

// The intrusive free-list node allocator (head @0x645544, raw subtrahend @0x64554c);
// the node body pointer is recovered as (element - g_coordPool.m_linkOffset). Shared with
// MapLogic.cpp / Projectile.cpp (DATA-bound there; extern here).
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) are
// fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].

// ===========================================================================
// CGruntzMapMgr::Visit  (0x082430) - vtable slot 1, __thiscall
// ===========================================================================
// Stream the +0x7c pointer-array (m_pData@+0x80, m_nSize@+0x84) and the +0x90 scratch
// dword through `ar` keyed by `mode`, then chain the base CMapMgr::Visit probe.
// mode 4 = write-out (stream slot +0x30): write m_90, the count, then each non-null
// node body (size 8). mode 7 = read-in (slot +0x2c): read m_90 and the new count, push
// every existing node back onto g_coordPool.m_freeHead, resize the array to empty then
// to count, and pull a fresh node off g_coordPool.m_freeHead per slot (body = node+4),
// reading each (size 8). Returns the base probe's bool.
//
// (Was ?SerializeNodes@CMapLogic@@QAEHPAVCFileMemBase@@HHH@Z - a non-virtual method of
// the conflated view. The name is the BASE SLOT's: an override must carry it. The tail
// was spelled `((CMapMgr*)this)->CMapMgr::Visit(...)` through a cross-cast that only
// existed because the view had no real relation to CMapMgr; with the true inheritance
// it is the plain qualified base call it always was, and the cast is gone.)
// @early-stop
// stack-slot coalescing entropy tail (~96%): instruction selection + block order
// byte-match retail; residual is the write-count local landing at esp+0x14 where
// retail spreads the read/write counts across esp+0x14 / esp+0x18 (frame-layout,
// not steerable without guessing the local-allocation order).
RVA(0x00082430, 0x161)
i32 CGruntzMapMgr::Visit(CSerialArchive* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 7: {
            // read-in: m_90, the new count; recycle every existing node onto
            // g_coordPool.m_freeHead; resize empty->count; pull a fresh node per slot.
            ar->Read(&m_90, 4);
            i32 count;
            ar->Read(&count, 4);
            for (i32 fi = 0; fi < m_arr.GetSize(); fi++) {
                void* elem = m_arr.GetData()[fi];
                if (elem != 0) {
                    void** node = (void**)((char*)elem - g_coordPool.m_linkOffset);
                    *node = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
            m_arr.SetSize(count, -1);
            for (u32 ri = 0; ri < static_cast<u32>(count); ri++) {
                void** node = (void**)g_coordPool.m_freeHead;
                void* elem = 0;
                if (*node != 0) {
                    elem = (char*)node + 4;
                    g_coordPool.m_freeHead = *node;
                }
                ar->Read(elem, 8);
                m_arr.GetData()[ri] = elem;
            }
            break;
        }
        case 4: {
            // write-out: m_90, the count (a local copy of m_nSize), each node body.
            ar->Write(&m_90, 4);
            i32 wn = m_arr.GetSize();
            ar->Write(&wn, 4);
            for (u32 wi = 0; wi < static_cast<u32>(wn); wi++) {
                void* elem = m_arr.GetData()[wi];
                if (elem == 0) {
                    return 0;
                }
                ar->Write(elem, 8);
            }
            break;
        }
    }
    // Retail tail: `mov ecx,this; call 0x26b2` (the ?Visit@CMapMgr@@ ILT thunk ->
    // 0x9f7f0) - this override chaining its own base, non-virtually.
    return CMapMgr::Visit(ar, mode, a2, a3) != 0;
}

// ===========================================================================
// CGruntzMapMgr::Reset  (0x085480) - vtable slot 0, __thiscall
// ===========================================================================
// Walk the +0x7c CPtrArray's pointer body, push each non-null element's node
// (element - g_coordPool.m_linkOffset) back onto the global g_coordPool.m_freeHead,
// then shrink the array to empty (SetSize(0, -1)) and chain the base grid cleanup.
//
// (Was ?FreeNodes@CMapLogic@@QAEXXZ. The tail `Reset()` was a declared-only
// CMapLogic::Reset ALIAS whose RVA had to be pinned to the ILT thunk 0x1a91 by hand -
// `@data-symbol: ?Reset@CMapLogic@@QAEXXZ 0x00001a91` - because 0x9ec30 already
// carried CMapMgr::Reset's label and the dup-RVA guard rejects a second. With the
// true inheritance that whole workaround dissolves: it is a qualified base call, the
// same one ~CGruntzMapMgr below already makes.)
RVA(0x00085480, 0x52)
void CGruntzMapMgr::Reset() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetData()[i];
        if (elem != 0) {
            void** node = (void**)((char*)elem - g_coordPool.m_linkOffset);
            *node = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset(); // @0x9ec30 (base slot-0 grid cleanup, direct call)
}

// ===========================================================================
// ~CGruntzMapMgr  (0x085d10)
// ===========================================================================
// A /GX dtor: cl re-stamps the derived vtable (the implicit stamp-first), then the
// body runs the same node-pool teardown as Reset above, and the implicit member dtor
// (~CPtrArray on m_arr) plus the out-of-line base dtor (~CMapMgr @0x9e9e0, reached
// through thunk 0x135c) close the frame.
RVA(0x00085d10, 0xa7)
CGruntzMapMgr::~CGruntzMapMgr() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetAt(i);
        if (elem != 0) {
            void** node = (void**)((char*)elem - g_coordPool.m_linkOffset);
            *node = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset(); // @0x9ec30 (base slot-0 grid cleanup, direct call)
}

#include <Gruntz/GruntzMapMgr.h>

#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540

VTBL(CGruntzMapMgr, 0x001e9bb4); // vtable_names -> code (RTTI game class)
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
// REGALLOC wall (~96.27%): a 3-way register-assignment permutation, nothing more.
// Instruction selection, block order AND stack layout all byte-match retail; every
// differing instruction is the same opcode with a different register field:
//       value      retail   ours
//       this       edi      ebx
//       mode       ebx      eax
//       &m_arr     ebp      edi
// Retail also materializes `this` (`push edi; mov edi,ecx`) BEFORE the `test esi,esi`
// null check, where cl sinks it past the test for us.
//
// NB the diagnosis this comment carried while the body lived on the ex-"CMapLogic"
// view - "stack-slot coalescing entropy tail ... the write-count local landing at
// esp+0x14 where retail spreads the read/write counts across esp+0x14 / esp+0x18
// (frame-layout)" - is REFUTED by the measurement: `mov ecx,dword ptr [esp+0x14]`
// is IDENTICAL on both sides and no frame offset differs anywhere in the body. The
// wall is register assignment, not frame layout. (Re-measured with
// `gruntz sema disasm 0x00082430 --diff` after the split; don't inherit the old
// text.) Structure is correct (real inheritance, real qualified base call, no
// casts), so the residue is the permuter's job, not a source-shape bug.
RVA(0x00082430, 0x161)
i32 CGruntzMapMgr::Visit(CFileMemBase* ar, i32 mode, i32 a2, i32 a3) {
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
                    CoordPoolNode* node = g_coordPool.NodeOf(elem);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
            m_arr.SetSize(count, -1);
            for (u32 ri = 0; ri < static_cast<u32>(count); ri++) {
                CoordPoolNode* node = g_coordPool.m_freeHead;
                void* elem = 0;
                if (node->m_next != 0) {
                    elem = &node->m_coord;
                    g_coordPool.m_freeHead = node->m_next;
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
// CMapLogic::Reset ALIAS whose RVA had to be hand-pinned as a data-symbol row
// (?Reset@CMapLogic@@QAEXXZ at the ILT thunk 0x00001a91) - because 0x9ec30 already
// carried CMapMgr::Reset's label and the dup-RVA guard rejects a second. With the
// true inheritance that whole workaround dissolves: it is a qualified base call, the
// same one ~CGruntzMapMgr below already makes.)
RVA(0x00085480, 0x52)
void CGruntzMapMgr::Reset() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetData()[i];
        if (elem != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(elem);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset(); // @0x9ec30 (base slot-0 grid cleanup, direct call)
}

RVA(0x00085d10, 0xa7)
CGruntzMapMgr::~CGruntzMapMgr() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetAt(i);
        if (elem != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(elem);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset(); // @0x9ec30 (base slot-0 grid cleanup, direct call)
}

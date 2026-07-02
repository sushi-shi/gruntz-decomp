// MapLogic.cpp - the CUserLogic-derived 2D terrain/influence grid game-logic
// object (placeholder name CMapLogic; see <Gruntz/MapLogic.h>). Migrated out of
// the trace-mislabeled "CBrickz" block in src/Stub/Discovered.cpp - the matched
// CBrickz container (the self-contained node graph) lives in Brickz.cpp; THIS is
// the CUserLogic leaf with the CObArray-at-+0x7c serializer family.
//
// Functions in ascending retail-RVA order. CUserLogic/CUserBase/EngStr come from
// <Gruntz/UserLogic.h>; the archive object + the free-list node pool from
// <Gruntz/MapLogic.h>. Engine callees are reloc-masked (no body).
//
// BANKED (all byte-exact): ~CMapLogic (0x113c0), MapSerializeCurve (0xec230),
//   CMapLogic::FreeNodes (0x85480), CMapVisitTarget::Visit (0x9f7f0).
//   SerializeNodes (0x82430) reconstructed here (~96%, stack-slot entropy tail).
// The terrain-grid methods (0x77790/0x81e10/0x82030) operate on the CBrickz grid
// shape (+0x4 cell pool / +0x8 column table / +0xc/+0x10 dims / +0x4c mask) and
// were moved to Brickz.cpp; 0x9eca0/0x9f010 are CBrickz CONTAINER methods (they
// call CBrickz::Insert/Find/Unlink/CellPop) - see Brickz.cpp. (0x9356c is a thin
// Serialize wrapper over MapSerializeCurve + the +0x7c sub-object serializer.)
#include <Mfc.h>

#include <Gruntz/MapLogic.h>

#include <rva.h>

// ===========================================================================
// CMapLogic::~CMapLogic  (0x0113c0)
// ===========================================================================
// The leaf adds no destructible members of its own beyond the CUserLogic base, so
// its dtor folds the bare CUserLogic teardown: store the CUserLogic vptr
// (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame. The
// empty body is enough (same shape as CGruntPuddle::~CGruntPuddle).
RVA(0x000113c0, 0x44)
CMapLogic::~CMapLogic() {}

// ===========================================================================
// MapSerializeCurve  (0x0ec230) - __cdecl
// ===========================================================================
// Stream the monotone float curve g_mapCurve through the archive `ar` keyed by
// `mode`: mode 7 reads (vtable slot +0x2c), mode 4 writes (slot +0x30). The first
// block transfers the two leading floats as one 8-byte block each; the second
// transfers six trailing floats one at a time (size 4). Any other mode is a no-op
// that still returns 1. Returns 0 only for a null archive.
RVA(0x000ec230, 0x11c)
i32 MapSerializeCurve(CMapArchive* ar, i32 mode) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            ar->Write(&g_mapCurve[0], 8);
            ar->Write(&g_mapCurve[2], 8);
            break;
        case 7:
            ar->Read(&g_mapCurve[0], 8);
            ar->Read(&g_mapCurve[2], 8);
            break;
    }
    switch (mode) {
        case 4:
            ar->Write(&g_mapCurve[4], 4);
            ar->Write(&g_mapCurve[5], 4);
            ar->Write(&g_mapCurve[6], 4);
            ar->Write(&g_mapCurve[7], 4);
            ar->Write(&g_mapCurve[8], 4);
            ar->Write(&g_mapCurve[9], 4);
            break;
        case 7:
            ar->Read(&g_mapCurve[4], 4);
            ar->Read(&g_mapCurve[5], 4);
            ar->Read(&g_mapCurve[6], 4);
            ar->Read(&g_mapCurve[7], 4);
            ar->Read(&g_mapCurve[8], 4);
            ar->Read(&g_mapCurve[9], 4);
            break;
    }
    return 1;
}

// ===========================================================================
// CMapLogic::FreeNodes  (0x085480) - __thiscall
// ===========================================================================
// Tear-down helper: walk the +0x7c CObArray's pointer body, push each non-null
// element's node (element - g_freeListNodeBias) back onto the global g_freeList,
// then shrink the array to empty (SetSize(0, -1)) and run the grid Reset (0x9ec30).
RVA(0x00085480, 0x52)
void CMapLogic::FreeNodes() {
    for (i32 i = 0; i < m_arr.m_nSize; i++) {
        void* elem = m_arr.m_pData[i];
        if (elem != 0) {
            void** node = (void**)((char*)elem - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_arr.SetSize(0, -1);
    Reset();
}

// ===========================================================================
// CMapLogic::SerializeNodes  (0x082430) - __thiscall
// ===========================================================================
// Stream the +0x7c pointer-array (m_80 body, m_84 count) and the +0x90 scratch
// dword through `ar` keyed by `mode`, then tail-dispatch the polymorphic Visit
// probe (0x9f7f0) with the archive. mode 4 = write-out (slot +0x30): write m_90,
// the count, then each non-null node body (size 8). mode 7 = read-in (slot +0x2c):
// read m_90, the new count, push every existing node back onto g_freeList, resize
// the array to empty then to count, and pull a fresh node off g_freeList per slot
// (body = node+4), reading each (size 8). Returns the Visit probe's bool.
// @early-stop
// stack-slot coalescing entropy tail (~96%): instruction selection + block order
// byte-match retail; residual is the write-count local landing at esp+0x14 where
// retail spreads the read/write counts across esp+0x14 / esp+0x18 (frame-layout,
// not steerable without guessing the local-allocation order).
RVA(0x00082430, 0x161)
i32 CMapLogic::SerializeNodes(CMapArchive* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 7: {
            // read-in: m_90, the new count; recycle every existing node onto
            // g_freeList; resize empty->count; pull a fresh node per slot.
            ar->Read(&m_90, 4);
            i32 count;
            ar->Read(&count, 4);
            for (i32 fi = 0; fi < m_arr.m_nSize; fi++) {
                void* elem = m_arr.m_pData[fi];
                if (elem != 0) {
                    void** node = (void**)((char*)elem - g_freeListNodeBias);
                    *node = g_freeList;
                    g_freeList = node;
                }
            }
            m_arr.SetSize(0, -1);
            m_arr.SetSize(count, -1);
            for (u32 ri = 0; ri < (u32)count; ri++) {
                void** node = (void**)g_freeList;
                void* elem = 0;
                if (*node != 0) {
                    elem = (char*)node + 4;
                    g_freeList = *node;
                }
                ar->Read(elem, 8);
                m_arr.m_pData[ri] = elem;
            }
            break;
        }
        case 4: {
            // write-out: m_90, the count (a local copy of m_84), each node body.
            ar->Write(&m_90, 4);
            i32 wn = m_arr.m_nSize;
            ar->Write(&wn, 4);
            for (u32 wi = 0; wi < (u32)wn; wi++) {
                void* elem = m_arr.m_pData[wi];
                if (elem == 0) {
                    return 0;
                }
                ar->Write(elem, 8);
            }
            break;
        }
    }
    return ((CMapVisitTarget*)this)->Visit(ar, mode, a2, a3) != 0;
}

// ===========================================================================
// CMapVisitTarget::Visit  (0x09f7f0) - __thiscall
// ===========================================================================
// Probe the visited object through its own vtable: mode 4 calls slot +0x08, mode 7
// calls slot +0x0c, each with the buffer arg. Returns 1 unless the probe returned
// non-zero (then the slot's truthiness short-circuits to a 0 return). A null buffer
// returns 0; any other mode returns 1.
RVA(0x0009f7f0, 0x3b)
i32 CMapVisitTarget::Visit(void* buf, i32 mode, i32 a2, i32 a3) {
    if (buf == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (Slot08(buf) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Slot0C(buf) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

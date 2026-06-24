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
// The larger grid/serializer methods (0x77790/0x81e10/0x82030/0x82430/0x9356c) stay
// in the src/Stub/ "CBrickz" block for the final sweep; 0x9eca0/0x9f010 are CBrickz
// CONTAINER methods (they call CBrickz::Insert/Find/Unlink/CellPop) - see Brickz.cpp.
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
int MapSerializeCurve(CMapArchive* ar, int mode) {
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
    for (int i = 0; i < m_arr.m_nSize; i++) {
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
// CMapVisitTarget::Visit  (0x09f7f0) - __thiscall
// ===========================================================================
// Probe the visited object through its own vtable: mode 4 calls slot +0x08, mode 7
// calls slot +0x0c, each with the buffer arg. Returns 1 unless the probe returned
// non-zero (then the slot's truthiness short-circuits to a 0 return). A null buffer
// returns 0; any other mode returns 1.
RVA(0x0009f7f0, 0x3b)
int CMapVisitTarget::Visit(void* buf, int mode, int a2, int a3) {
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

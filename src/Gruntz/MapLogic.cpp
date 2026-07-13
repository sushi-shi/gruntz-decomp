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

// The scroll-state block at 0x24cfb0: g_scrollAccum (i64) leads it, and the
// serializer below streams the accumulator pair (two i64) plus six scroll dwords.
// (Ex g_mapCurve[12] - a fake float view: MapSerializeCurve is really the
// scroll-state serializer, and the fake name's DATA(0x0024cfb0) binding collided
// with the real g_scrollAccum at the same RVA and won the per-RVA keep-last dedup,
// leaving cmdscrollapply's `?g_scrollAccum@@3_JA` reference UNBOUND. Anchored on the
// real g_scrollAccum now - its DATA binding lives in MgrAutoScroll.cpp.)
extern i64 g_scrollAccum; // 0x24cfb0 (bound in MgrAutoScroll.cpp)

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
// Stream the scroll-state block (anchored on g_scrollAccum @0x24cfb0) through the
// archive `ar` keyed by `mode`: mode 7 reads (vtable slot +0x2c), mode 4 writes
// (slot +0x30). The first block transfers the two leading 8-byte accumulators; the
// second transfers six trailing dwords (size 4). Any other mode is a no-op that
// still returns 1. Returns 0 only for a null archive. (Every operand is
// g_scrollAccum+addend -> reloc-masked, byte-identical to the ex g_mapCurve[k] form.)
RVA(0x000ec230, 0x11c)
i32 MapSerializeCurve(CSerialArchive* ar, i32 mode) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            ar->Write(&g_scrollAccum, 8);
            ar->Write((char*)&g_scrollAccum + 0x8, 8);
            break;
        case 7:
            ar->Read(&g_scrollAccum, 8);
            ar->Read((char*)&g_scrollAccum + 0x8, 8);
            break;
    }
    switch (mode) {
        case 4:
            ar->Write((char*)&g_scrollAccum + 0x10, 4);
            ar->Write((char*)&g_scrollAccum + 0x14, 4);
            ar->Write((char*)&g_scrollAccum + 0x18, 4);
            ar->Write((char*)&g_scrollAccum + 0x1c, 4);
            ar->Write((char*)&g_scrollAccum + 0x20, 4);
            ar->Write((char*)&g_scrollAccum + 0x24, 4);
            break;
        case 7:
            ar->Read((char*)&g_scrollAccum + 0x10, 4);
            ar->Read((char*)&g_scrollAccum + 0x14, 4);
            ar->Read((char*)&g_scrollAccum + 0x18, 4);
            ar->Read((char*)&g_scrollAccum + 0x1c, 4);
            ar->Read((char*)&g_scrollAccum + 0x20, 4);
            ar->Read((char*)&g_scrollAccum + 0x24, 4);
            break;
    }
    return 1;
}

// ===========================================================================
// CMapLogic::FreeNodes  (0x085480) - __thiscall
// ===========================================================================
// Tear-down helper: walk the +0x7c CObArray's pointer body, push each non-null
// element's node (element - g_coordPool.m_linkOffset) back onto the global g_coordPool.m_freeHead,
// then shrink the array to empty (SetSize(0, -1)) and run the grid Reset (0x9ec30).
//
// The trailing Reset() call: retail routes the rel32 through the ILT jmp-thunk 0x1a91
// (-> 0x9ec30 = ?Reset@CMapMgr@@UAEXXZ, bound in the mapmgr unit). CMapLogic::Reset is
// declared-only (MapLogic.h) - it aliases the SAME body, and 0x9ec30 can't take a 2nd
// func label (dup-RVA guard vs CMapMgr::Reset), so bind the alias to the THUNK the call
// literally targets; reloc_fidelity thunk-resolves both sides to 0x9ec30 -> CORRECT.
// @data-symbol: ?Reset@CMapLogic@@QAEXXZ 0x00001a91
RVA(0x00085480, 0x52)
void CMapLogic::FreeNodes() {
    for (i32 i = 0; i < m_arr.m_nSize; i++) {
        void* elem = m_arr.m_pData[i];
        if (elem != 0) {
            void** node = (void**)((char*)elem - g_coordPool.m_linkOffset);
            *node = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    ((CObArray*)&m_arr)->SetSize(0, -1);
    Reset();
}

// ===========================================================================
// CMapLogic::SerializeNodes  (0x082430) - __thiscall
// ===========================================================================
// Stream the +0x7c pointer-array (m_80 body, m_84 count) and the +0x90 scratch
// dword through `ar` keyed by `mode`, then tail-dispatch the polymorphic Visit
// probe (0x9f7f0) with the archive. mode 4 = write-out (slot +0x30): write m_90,
// the count, then each non-null node body (size 8). mode 7 = read-in (slot +0x2c):
// read m_90, the new count, push every existing node back onto g_coordPool.m_freeHead, resize
// the array to empty then to count, and pull a fresh node off g_coordPool.m_freeHead per slot
// (body = node+4), reading each (size 8). Returns the Visit probe's bool.
// @early-stop
// stack-slot coalescing entropy tail (~96%): instruction selection + block order
// byte-match retail; residual is the write-count local landing at esp+0x14 where
// retail spreads the read/write counts across esp+0x14 / esp+0x18 (frame-layout,
// not steerable without guessing the local-allocation order).
RVA(0x00082430, 0x161)
i32 CMapLogic::SerializeNodes(CSerialArchive* ar, i32 mode, i32 a2, i32 a3) {
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
            for (i32 fi = 0; fi < m_arr.m_nSize; fi++) {
                void* elem = m_arr.m_pData[fi];
                if (elem != 0) {
                    void** node = (void**)((char*)elem - g_coordPool.m_linkOffset);
                    *node = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            ((CObArray*)&m_arr)->SetSize(0, -1);
            ((CObArray*)&m_arr)->SetSize(count, -1);
            for (u32 ri = 0; ri < (u32)count; ri++) {
                void** node = (void**)g_coordPool.m_freeHead;
                void* elem = 0;
                if (*node != 0) {
                    elem = (char*)node + 4;
                    g_coordPool.m_freeHead = *node;
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

// CMapVisitTarget::Visit (0x9f7f0) lives in its home TU per the interval dossier
// (#10a seam): src/Gruntz/MapMgr.cpp - the single fn between the CBrickzGrid
// block and CMapMgr::Save/Load.

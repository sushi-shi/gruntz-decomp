// MapLogic.cpp - the scroll-state serializer MapSerializeCurve (0x0ec230).
//
// This TU is what is LEFT of the ex-"CMapLogic" view, which conflated two unrelated
// classes and has now been SPLIT onto both real owners (nothing of it survives):
//   * the dtor 0x113c0 is ??1CBrickz - pinned in TileLogicPump.cpp (that TU emits
//     the CBrickz vtable/??_G, hence the implicit ??1 COMDAT). CBrickz is 0x54 bytes
//     (allocation-site proven at LogicDispatchB 0x10d3d0: `push 0x54; call
//     ??2@YAPAXI@Z; mov ecx,eax; call 0x3701`), so it could never have owned the
//     view's +0x7c/+0x90 members - they sit 0x3c bytes past its end.
//   * the serializer half (0x82430 / 0x85480) is CGruntzMapMgr's slot-1 / slot-0
//     vtable overrides - homed in GruntzMapMgr.cpp, on the 0x94-byte object where
//     the +0x7c CPtrArray and +0x90 dword actually fit.
//
// MapSerializeCurve is a __cdecl free function belonging to neither class, so it
// stays here. Engine callees are reloc-masked (no body).
#include <Mfc.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)

#include <Gruntz/MapLogic.h>

#include <rva.h>

// The scroll-state block at 0x24cfb0: g_scrollAccum (i64) leads it, and the
// serializer below streams the accumulator pair (two i64) plus six scroll dwords.
// (Ex g_mapCurve[12] - a fake float view: MapSerializeCurve is really the
// scroll-state serializer, and the fake name's DATA(0x0024cfb0) binding collided
// with the real g_scrollAccum at the same RVA and won the per-RVA keep-last dedup,
// leaving cmdscrollapply's `?g_scrollAccum@@3_JA` reference UNBOUND. Anchored on the
// real g_scrollAccum now - its DATA binding lives in MgrAutoScroll.cpp.)
#include <Gruntz/ScrollState.h> // g_scrollAccum (bound in MgrAutoScroll.cpp)

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

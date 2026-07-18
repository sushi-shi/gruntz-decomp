// WapMisc.cpp - assorted tiny engine leaves.
//
//  0x11d100 - a CNoTrackObject base-subobject vptr stamp (*this = &??_7CNoTrackObject);
//             transitional stamp - the class lives in the MFC library.
//  0x1155b0 - construct the g_largeFont global (tail-jmp to ??0Font).
#include <rva.h>
#include <Ints.h>

#include <Font/Font.h> // real Font (the g_largeFont global)
#include <Wap32/ZVec.h>
#include <Wap32/NoTrackObjectStamp.h> // canonical CNoTrackObjectStamp (the vptr-stamp leaf)

// --- 0x11d100 : CNoTrackObject base-vptr restamp -----------------------------
// The standalone ??_7CNoTrackObject vptr restamp leaf. Modeled as the empty dtor of the
// polymorphic CNoTrackObjectStamp placeholder (<Wap32/NoTrackObjectStamp.h>): the
// compiler emits `mov [ecx],??_7CNoTrackObjectStamp; ret` and RELOC_VTBL(...,0x001ec26c)
// binds that emitted vptr reference to the retail NAFXCW ??_7CNoTrackObject@@6B@
// (0x5ec26c) - so the ex `Vtbl_5ec26c` reloc-masked extern is gone. Nothing constructs
// it, so no ??_G is emitted; the explicit empty body is the byte-exact 7-byte restamp.
// @interleaver emitted-in winmain (WinMain's AfxWinInit touches CNoTrackObject); homing
// to WinMain.cpp still awaits the CNoTrackObject identity (kept-in-place + flagged).
RVA(0x0011d100, 0x7)
CNoTrackObjectStamp::~CNoTrackObjectStamp() {}

// --- 0x1155b0 : construct the g_largeFont global -----------------------------
// A dynamic-initializer thunk that constructs the global font in place via the
// explicit-ctor-call tail-jmp (mov ecx,&g_largeFont; jmp ??0Font@@QAE@XZ 0x179700 -
// no placement-new null-guard). Font is the real engine font class (<Font/Font.h>);
// g_largeFont is declared there.
// @interleaver Unmatched_1155b0 (g_largeFont dyn-init thunk) emitted-in <boundary: fonts?>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are glyphstr ShowHudMessageAlt
// @0x115520 (before) + fonts Forward_115630 @0x115630 (after), NOT one host both sides. This is
// a CRT dynamic-init thunk for the g_largeFont Font global; candidate home is the `fonts` unit
// (immediately after) but deferred - the fonts obj boundary is not yet pinned. Rule (a)-ish
// dyn-init fragment; kept in place + flagged.)
RVA(0x001155b0, 0xa)
void Unmatched_1155b0() {
    g_largeFont.Font::Font();
}

// (The 0xc76d0 "Unmatched_c76d0" that was parked here is really
//  CDroppedObjectShadow::InitActReg - src/Gruntz/DroppedObject.cpp, wave2-H.)

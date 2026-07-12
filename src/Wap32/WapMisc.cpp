// WapMisc.cpp - assorted tiny engine leaves.
//
//  0x11d100 - a CNoTrackObject base-subobject vptr stamp (*this = &??_7CNoTrackObject);
//             transitional stamp - the class lives in the MFC library.
//  0x1155b0 - construct the g_largeFont global (tail-jmp to ??0Font).
#include <rva.h>
#include <Ints.h>

#include <Font/Font.h> // real Font (the g_largeFont global)
#include <Wap32/ZVec.h>

// --- 0x11d100 : CNoTrackObject vptr stamp ------------------------------------
// Stamps the ??_7CNoTrackObject vtable (VA 0x5ec26c = the vtable modeled as
// CNoTrackObject). Kept as a bare stamp leaf: making it a real CNoTrackObject
// ctor would add a `mov eax,ecx` (return-this) the 7-byte retail body does not carry
// (the vtable-realization boundary - matcher.md). CNoTrackObject itself is MFC
// library code (NAFXCW), so the stamp is modeled locally against the same vtable
// symbol; the DATA() pin names the external library vtable ??_7CNoTrackObject@@6B@
// (VA 0x5ec26c) so this reloc masks (formerly bound by ComHelperThunks' VTBL()).
DATA(0x001ec26c)
extern void* const Vtbl_5ec26c; // ??_7CNoTrackObject@@6B@ (NAFXCW library vtable)
struct CNoTrackObjectStamp {
    void* vptr;
    void Stamp(); // 0x11d100
};
// Out-of-line so it emits: an inline member of a leaf with no in-TU caller folds
// away and never produces a standalone body at 0x11d100.
// @interleaver CNoTrackObjectStamp::Stamp emitted-in winmain - blocked: local view
// (REHOME D10: retail emits this COMDAT INSIDE winmain's block - _WinMain@16 @0x11c860
// (before) + ??1CObjStamp11e8dc @0x11e8dc (after), both winmain - a rule-(c) interleaver
// surrounded by winmain on both sides (WinMain's AfxWinInit touches CNoTrackObject).
// Homing to WinMain.cpp is BLOCKED: CNoTrackObjectStamp is a local placeholder view of
// the MFC-library CNoTrackObject; needs a shared decl (or the real MFC class) in winmain
// first. Kept-in-place + flagged.)
RVA(0x0011d100, 0x7)
void CNoTrackObjectStamp::Stamp() {
    vptr = (void*)&Vtbl_5ec26c;
}

// --- 0x1155b0 : construct the g_largeFont global -----------------------------
// A dynamic-initializer thunk that constructs the global font in place via the
// explicit-ctor-call tail-jmp (mov ecx,&g_largeFont; jmp ??0Font@@QAE@XZ 0x179700 -
// no placement-new null-guard). Font is the real engine font class (<Font/Font.h>).
extern Font g_largeFont; // ?g_largeFont@@3VFont@@A (0x64eac0)
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

SIZE_UNKNOWN(CNoTrackObjectStamp); // CNoTrackObject vptr-stamp leaf

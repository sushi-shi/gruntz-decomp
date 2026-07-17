// MapLogic.h - MapSerializeCurve (0x0ec230), the scroll-state serializer.
//
// THE `CMapLogic` CLASS IS GONE. It was a placeholder view that CONFLATED TWO
// UNRELATED CLASSES, and it has been SPLIT onto both real owners (MI1 proved it,
// ML1 re-verified every claim from the PE bytes and executed the split):
//
//   * sizeof(CBrickz) == 0x54, read off the ALLOCATION SITE (not inferred from vtable
//     slots or ctor writes - both unsound for size). CBrickz::CBrickz(CGameObject*)
//     @0x10e800 has exactly one caller, its ILT thunk 0x3701; chasing THROUGH the
//     thunk reaches the real new-site LogicDispatchB @0x10d3d0:
//         push 0x54 ; call 0x1b9b46 (??2@YAPAXI@Z) ; add esp,4 ; mov ecx,eax ; call 0x3701
//     So a CBrickz CANNOT own the view's +0x7c CPtrArray / +0x90 dword: they sit 0x3c
//     bytes past the end of it. Folding the view onto CBrickz would have moved a
//     manager's serializers onto a tile leaf.
//   * the DTOR 0x113c0 IS CBrickz's: ??_7CBrickz @0x1e7c54 slot[0] -> thunk 0x2b99 ->
//     scalar-deleting dtor 0x11390 -> thunk 0x216c -> 0x113c0. Pinned as
//     ??1CBrickz@@UAE@XZ in TileLogicPump.cpp (the TU that emits CBrickz's vtable).
//   * the SERIALIZER half (0x82430 / 0x85480) is CGruntzMapMgr's, on the 0x94-byte
//     object where the +0x7c CPtrArray / +0x90 dword fit. ??_7CGruntzMapMgr @0x1e9bb4
//     slot[0] -> thunk 0x4430 -> 0x85480 and slot[1] -> thunk 0x16a9 -> 0x82430; that
//     vtable's slots 2..5 are BIT-IDENTICAL to ??_7CMapMgr @0x1ea3b4, so the class
//     overrides exactly slots 0 and 1 - exactly these two bodies. Neither has a rel32
//     caller: they are reached ONLY through those slots. Homed in GruntzMapMgr.cpp as
//     CGruntzMapMgr::Reset / ::Visit (the base slot names an override must carry).
//
// Their two tails were the tell all along: each chains its OWN base through an ILT
// thunk - 0x85480's `Reset()` -> thunk 0x1a91 -> 0x9ec30 (??_7CMapMgr slot[0]) and
// 0x82430's `Visit()` -> thunk 0x26b2 -> 0x9f7f0 (??_7CMapMgr slot[1]). The view had
// to spell those as a hand-pinned thunk alias and a `((CMapMgr*)this)` cross-cast;
// under the real inheritance both are plain qualified base calls.
#ifndef GRUNTZ_MAPLOGIC_H
#define GRUNTZ_MAPLOGIC_H

#include <Ints.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)

// The .data block at 0x24cfb0 that 0xec230 serializes is the scroll-state block
// (g_scrollAccum @0x24cfb0, bound in MgrAutoScroll.cpp) - not a float curve. The
// former g_mapCurve[12] fake float view is dissolved onto g_scrollAccum (its DATA
// binding collided with the real g_scrollAccum at the same RVA); see MapLogic.cpp.

// 0xec230: the scroll-state serializer. __cdecl free function (caller-cleanup,
// `ret`); drives the archive's read/write slots over the g_scrollAccum block keyed by
// `mode` (7=read via +0x2c, 4=write via +0x30). Belongs to no class - declared at
// namespace scope.
// NB: src/Gruntz/Brickz.cpp re-declares this as a per-TU `extern "C" i32 __cdecl
// MapSerializeCurve(i32,i32,i32,i32)` - a DIFFERENT arity AND a different symbol
// (extern "C" `_MapSerializeCurve` vs this C++-mangled
// ?MapSerializeCurve@@YAHPAVCFileMemBase@@H@Z), so that call binds to nothing. Left
// alone deliberately: its caller CBrickzGrid::Serialize (0x9356c) is @early-stop'd on
// a documented reg-forwarding wall, and settling the true arity is its own pass.
i32 MapSerializeCurve(CSerialArchive* ar, i32 mode); // 0x0ec230

#endif // GRUNTZ_MAPLOGIC_H

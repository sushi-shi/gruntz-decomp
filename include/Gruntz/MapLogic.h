// MapLogic.h - the CUserLogic-derived 2D terrain/influence grid game-logic object
// (placeholder name CMapLogic; distinct from the 0x60-byte CMapMgr container in
// MapMgr.h). The trace grouped these __thiscall methods under "CBrickz", but the
// matched CBrickz container is the self-contained node graph in Brickz.cpp; this
// class is the *CUserLogic leaf* whose ctor (0x113c0) stores the CUserLogic vftable
// (0x5e705c) and constructs the +0x18 link sub-object (the /GX EH frame), then owns
// a terrain-flag cell grid, an MFC CArray at +0x7c and a parallel pointer array at
// +0x80/+0x84.
//
// RTTI: the most-derived vftable the ctor stores is CUserLogic's own (0x5e705c) -
// this leaf adds no virtuals, so there is no distinct RTTI class for it; the name
// is a structural placeholder. ONLY offsets + code bytes are load-bearing.
//
// The grid cells are 0x1c bytes stride; each cell's +0x00 holds packed terrain
// flags (the 0x939 passability mask, also seen in CGruntPuddle), +0x04 holds a
// per-cell id. The serializer methods (0xec230 / 0x82430 / 0x9f7f0) drive an
// archive object through its vtable: slot +0x2c = read-in (mode 7), slot +0x30 =
// write-out (mode 4); 0x9f7f0 dispatches slots +0x08 / +0x0c.
#ifndef GRUNTZ_MAPLOGIC_H
#define GRUNTZ_MAPLOGIC_H

#include <rva.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/UserLogic.h>

// The CArchive-like serializer object the Serialize methods drive is the shared WAP32
// CSerialArchive (Read @ vtable +0x2c / Write @ +0x30), now the one modeled class in
// <Gruntz/SerialArchive.h> - the former local `CMapArchive` view is folded away.

// The .data block at 0x24cfb0 that 0xec230 serializes is the scroll-state block
// (g_scrollAccum @0x24cfb0, bound in MgrAutoScroll.cpp) - not a float curve. The
// former g_mapCurve[12] fake float view is dissolved onto g_scrollAccum (its DATA
// binding collided with the real g_scrollAccum at the same RVA); see MapLogic.cpp.

// The intrusive free-list node allocator the +0x84 pointer-array serializer pulls
// nodes from (shared with Projectile.cpp / BattlezMapConfig.cpp). The node body
// pointer is recovered as (slot - g_freeListNodeBias).
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// g_freeList / g_freeListNodeBias were g_coordPool's interior fields
// (+0x04 / +0x0c), not globals. The pool is DEFINED in GameText.cpp.
extern FreeNodePool g_coordPool;

// ---------------------------------------------------------------------------
// (The CMapPtrArray VIEW is DISSOLVED: the +0x7c array IS the MFC ::CPtrArray.  The
// SetSize it calls, 0x1b4f75, lies in [0x1b4f0b, 0x1b527e) - the band whose head ctor
// 0x1b4f0b DIR32s ??_7CPtrArray@@6B@ (0x1ec2dc).  The FID row that named it CDWordArray
// is AMBIG: the four MFC array classes are byte-identical.  See
// `python -m gruntz.analysis.mfc_class 0x1b4f75`.)

// ---------------------------------------------------------------------------
// CMapLogic : CUserLogic - the terrain/influence grid leaf. Field names are
// placeholders; offsets are recovered from the field stores in the methods below.
// The CUserLogic base occupies +0x00..+0x3f; this leaf's grid/array members start
// past it.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CMapLogic);
// IDENTITY PROVEN (vtable-owner probe): this class IS CBrickz. Its dtor (0x113c0) is
// dispatched from ??_7CBrickz @0x1e7c54 (RTTI-named, bound in <Gruntz/CBrickz.h>) slot 0
// via the scalar-deleting dtor 0x11390, and that vtable's slot 2 is CBrickz::GetTypeTag
// (0x11300 -> LOGIC_BRICKZ). The header note above even guessed it ("the trace grouped
// these under CBrickz") and then talked itself out of it - the binary says yes.
// The old RELOC_VTBL pointed at 0x1e705c (CUserLogic's vtable): a MISBINDING, corrected
// here to the true rva.
// @identity-TODO: CMapLogic == CBrickz. The FOLD is blocked on a layout conflict that
// must be settled by the ALLOCATION SITE, not by either header's guess: CBrickz.h asserts
// SIZE 0x54 while this view reads +0x7c/+0x90 - so at least one is wrong, and neither is
// evidence (a class's size is bounded only by `push <n>; call ??2@YAPAXI@Z` at its
// new-site; CBrickz's 1-arg ctor 0x10e800 is declared-only, so the oracle has no row yet).
RELOC_VTBL(CMapLogic, 0x001e7c54); // == ??_7CBrickz (true rva; fold pending the size oracle)
class CMapLogic : public CUserLogic, public CWapX {
public:
public:
    CMapLogic();                   // no-arg shape (only the teardown is here)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    // The +0x7c pointer-array serializer (0x082430) + its tear-down helper
    // (0x085480). __thiscall; both free the array's nodes back to g_freeList,
    // resize the CObArray via SetSize, and dispatch the archive read/write slots.
    i32 SerializeNodes(CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // 0x082430
    void FreeNodes();                                                 // 0x085480

    // The grid-reset cleanup the tear-down chains (0x9ec30, __thiscall, no args).
    // External/no-body so the `mov ecx,this; call rel32` reloc-masks (it shares the
    // mapmgr unit's Reset address; redefining it would clash). NOT a virtual here.
    void Reset(); // 0x09ec30

    // grid/array members (offsets from the field stores; the CUserLogic base owns
    // +0x00..+0x3f). Only the touched offsets are named.
    char m_pad54[0x7c - 0x54];
    CPtrArray m_arr; // +0x7c  ::CPtrArray (m_pData@+0x80, m_nSize@+0x84)
    i32 m_90;        // +0x90  scratch dword the node serializer streams
};

// 0xec230: the float-curve serializer. __cdecl free function (caller-cleanup,
// `ret`); drives the archive's read/write slots over the g_mapCurve slice keyed by
// `mode` (7=read via +0x2c, 4=write via +0x30). Declared at namespace scope.
i32 MapSerializeCurve(CSerialArchive* ar, i32 mode); // 0x0ec230

// DISSOLVED (Fable A2, 2026-07-14): the "CMapVisitTarget" shell (4 fabricated
// slots + a non-virtual Visit) is gone - 0x9f7f0 IS CMapMgr::Visit, slot [1] of
// ??_7CMapMgr @0x1ea3b4 (<Gruntz/MapMgr.h>; MapMgr.cpp owns the body).
// SerializeNodes tail-calls it DIRECTLY (retail: `mov ecx,this; call 0x26b2`, the
// ?Visit@CMapMgr@@ ILT thunk) - spelled as the qualified CMapMgr::Visit call in
// MapLogic.cpp so no vtable load is emitted.

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_MAPLOGIC_H

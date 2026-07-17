// GruntzMapMgr.h - the grunt-map manager container (C:\Proj\Gruntz). Its dtor
// (0x85d10) and its slot-0 override Reset (0x85480) share the SAME node-pool teardown
// idiom (the freelist push of every CPtrArray element back onto g_coordPool, then
// SetSize(0,-1) + the base CMapMgr::Reset @0x9ec30) - the resemblance that got Reset
// conflated into the ex-"CMapLogic" view until the split (see <Gruntz/MapLogic.h>).
//
// Layout recovered from the dtor: a polymorphic CMapMgr base (dtor @0x135c) with
// its vptr at +0x00 occupying +0x00..+0x7b, then the MFC CObArray of map-node
// pointers embedded at +0x7c (vptr@+0x7c, m_pData@+0x80, m_nSize@+0x84). The class
// is modeled real-polymorphic so cl emits the implicit vptr re-stamp stamp-FIRST
// (docs/patterns/eh-dtor-implicit-vptr-stamp-first.md); the emitted ??_7 vtables
// reloc-mask against retail's 0x5e9bb4 / base vtable. Only offsets + code bytes are
// load-bearing; field names are placeholders.
//
// m_arr IS the real MFC afxcoll CObArray (RTTI-confirmed: 5-slot CObject-derived
// container; SetSize @0x1b4f75 and its dtor @0x1b4f3e - the COMDAT-folded ~C*Array
// body - are external/no-body so the calls reloc-mask). The destructible member
// forces the /GX EH frame. The dtor loop reads the node table through the container
// accessors GetSize()/GetAt(), which inline to the same [+0x84]/[+0x80] field reads.
#ifndef GRUNTZ_CGRUNTZMAPMGR_H
#define GRUNTZ_CGRUNTZMAPMGR_H

#include <Ints.h>
#include <Mfc.h>           // the real MFC CPtrArray (m_arr node table)
#include <Gruntz/MapMgr.h> // the ONE real CMapMgr base (was duplicated in this header)
#include <rva.h>

// The serialize stream: the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it). Pointer-only here, so the fwd decl + typedef suffice;
// an elaborated `struct CSerialArchive*` would re-declare a DISTINCT class and
// silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

// The engine CMapMgr base (~CMapMgr @0x135c). Out-of-line virtual dtor so cl emits
// a CALL (reloc-masks against retail's base dtor) rather than inlining it. This is
// the reduced-vtable model of the CMapMgr in MapMgr.h: here CMapMgr's dtor is the
// slot-0 virtual (the shape ~CGruntzMapMgr's implicit stamp-first codegen needs);
// MapMgr.h models the same class with its full 6-slot vftable (Reset at slot 0).
// The two models can't be one header - the vtable-emission requirements differ -
// so this TU carries the reduced CMapMgr just as the CGruntzMgr/CGameRegistry pair
// carries two views of the game manager.
// (The SECOND, "reduced" CMapMgr definition that lived here is DELETED - it was a
//  redefinition of the ONE real class (<Gruntz/MapMgr.h>, RTTI .?AVCMapMgr@@). The claim
//  that "the two models can't be one header - the vtable-emission requirements differ" is
//  refuted by the binary: both vtables (0x1ea3b4 base / 0x1e9bb4 derived) are 6 slots and
//  share slots 2..5 outright. The real class carries the row-table + Notify + the slot
//  names now; this header keeps only the DERIVED class.)
SIZE_UNKNOWN(CGruntzMapMgr);
VTBL(CGruntzMapMgr, 0x001e9bb4); // vtable_names -> code (RTTI game class)
// This class overrides EXACTLY slots 0 and 1, and the binary says so three ways:
// ??_7CGruntzMapMgr @0x1e9bb4 slot[0] -> thunk 0x4430 -> 0x85480 and slot[1] -> thunk
// 0x16a9 -> 0x82430; that vtable's slots 2..5 (0x1c49/0x1186/0x3f7b/0x33eb) are
// BIT-IDENTICAL to ??_7CMapMgr @0x1ea3b4's, so 0 and 1 are the only divergences; and
// each of those two bodies tail-chains the base slot it overrides (0x85480 -> thunk
// 0x1a91 -> CMapMgr::Reset @0x9ec30; 0x82430 -> thunk 0x26b2 -> CMapMgr::Visit
// @0x9f7f0). Both were DECLARED-ONLY here until ML1 homed their bodies (they had been
// stranded on the ex-"CMapLogic" view as non-virtual FreeNodes/SerializeNodes).
class CGruntzMapMgr : public CMapMgr {
public:
    ~CGruntzMapMgr(); // 0x85d10 (CMapMgr's dtor is non-virtual, so is this)
    // slot 0 (0x085480): free the node table back to g_coordPool, then chain the base
    // grid cleanup. The dtor runs the same teardown inline.
    virtual void Reset() OVERRIDE;
    // slot 1 (0x082430): stream the node table + m_90 through the archive, then chain
    // the base probe. CGruntzMgr::BroadcastCmd drives it as a 4-arg command dispatch
    // (`mov eax,[ecx]; call [eax+4]`).
    virtual i32 Visit(CSerialArchive* ar, i32 b, i32 c, i32 d) OVERRIDE;

    // The level-load terrain parser (0x0810f0): allocates the grid, rolls per-cell
    // brick colours off the "Brickz" bute section, packs the cell flags, then seeds
    // moving-object footprints from the free-node pool. Body in BrickzLoad.cpp.
    i32 LoadAttributes(i32 width, i32 height); // 0x0810f0

    // ::CPtrArray, not CObArray: ~CGruntzMapMgr's member teardown calls into
    // [0x1b4f0b, 0x1b527e) (ctor 0x1b4f0b stamps ??_7CPtrArray@@6B@), not CObArray's
    // [0x1b55e9, 0x1b59cc).  The elements are raw void* nodes, not CObject*.
    CPtrArray m_arr; // +0x7c  footprint array (m_pData@+0x80, m_nSize@+0x84)
    i32 m_90;        // +0x90  cleared at LoadAttributes start (object size 0x94)
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H

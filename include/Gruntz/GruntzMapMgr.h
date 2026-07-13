// GruntzMapMgr.h - the grunt-map manager container (C:\Proj\Gruntz). Its dtor
// (0x85d10) sits in the 0x85xxx text region next to CMapLogic::FreeNodes (0x85480)
// and shares the SAME node-pool teardown idiom (the freelist push of every CObArray
// element back onto g_freeList, then SetSize(0,-1) + Reset @0x9ec30).
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
class CGruntzMapMgr : public CMapMgr {
public:
    ~CGruntzMapMgr();              // 0x85d10 (CMapMgr's dtor is non-virtual, so is this)
    virtual void Reset() OVERRIDE; // slot 0 (own Reset; the dtor calls the BASE CMapMgr::Reset)
    // slot 1 (0x082430 SerializeNodes): the base's Visit slot, overridden. CGruntzMgr::
    // BroadcastCmd drives it as a 4-arg command dispatch (`mov eax,[ecx]; call [eax+4]`).
    virtual i32 Visit(CSerialArchive* ar, i32 b, i32 c, i32 d) OVERRIDE;
    // ::CPtrArray, not CObArray: ~CGruntzMapMgr's member teardown calls into
    // [0x1b4f0b, 0x1b527e) (ctor 0x1b4f0b stamps ??_7CPtrArray@@6B@), not CObArray's
    // [0x1b55e9, 0x1b59cc).  The elements are raw void* nodes, not CObject*.
    CPtrArray m_arr; // +0x7c
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H

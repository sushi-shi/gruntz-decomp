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
#include <Mfc.h> // the real MFC CObArray (m_arr node table)
#include <rva.h>

// The engine CMapMgr base (~CMapMgr @0x135c). Out-of-line virtual dtor so cl emits
// a CALL (reloc-masks against retail's base dtor) rather than inlining it. This is
// the reduced-vtable model of the CMapMgr in MapMgr.h: here CMapMgr's dtor is the
// slot-0 virtual (the shape ~CGruntzMapMgr's implicit stamp-first codegen needs);
// MapMgr.h models the same class with its full 6-slot vftable (Reset at slot 0).
// The two models can't be one header - the vtable-emission requirements differ -
// so this TU carries the reduced CMapMgr just as the CGruntzMgr/CGameRegistry pair
// carries two views of the game manager.
SIZE_UNKNOWN(CMapMgr);
struct CMapMgr {
    ~CMapMgr();           // 0x9e9e0 (external, NON-virtual QAE) - the base dtor the
                          //         derived dtor calls directly (reloc-masks)
    virtual void Reset(); // 0x9ec30 slot 0 (UAE) - the base grid cleanup the dtor calls
    // slot 1 (+0x04): the 4-arg command dispatch CGruntzMgr::BroadcastCmd drives on the
    // manager's +0x70 sub-object (`mov eax,[ecx]; call [eax+4]`). It used to be modeled as
    // `CmdSinkV::Command` on an INVENTED class; it is THIS slot (SerializeNodes 0x16a9 in
    // the vtable_names catalog). Declared-only, so the dispatch reloc-masks.
    virtual i32 MapCommand(i32 a, i32 b, i32 c, i32 d);

    // Non-virtual services the +0x70 consumers reach (external/declared-only -> the calls
    // reloc-mask). These were the fake CmdSinkV::Set / CTileGrid::Notify view methods.
    void SetCellHeight(i32 row, i32 col, i32 value); // was CmdSinkV::Set
    void Notify(i32 x, i32 y, i32 state);            // tile-system dirty/updated notify

    // The tile board lives in the base's data region (previously the opaque m_pad[0x78]).
    // The field NAMES are the ones every consumer already used through the fake CTileGrid
    // view, so typing +0x70 as the REAL class costs no consumer rename.
    i32 m_4;                   // +0x04
    i32** m_8;                 // +0x08  row-pointer table (cell = (i32*)m_8[tileY] + tileX*7)
    i32 m_c;                   // +0x0c  width in tiles
    i32 m_10;                  // +0x10  height in tiles
    char m_pad14[0x7c - 0x14]; // +0x14..+0x7b
};

SIZE_UNKNOWN(CGruntzMapMgr);
VTBL(CGruntzMapMgr, 0x001e9bb4); // vtable_names -> code (RTTI game class)
class CGruntzMapMgr : public CMapMgr {
public:
    ~CGruntzMapMgr();              // 0x85d10 (CMapMgr's dtor is non-virtual, so is this)
    virtual void Reset() OVERRIDE; // slot 0 (own Reset; the dtor calls the BASE CMapMgr::Reset)
    virtual i32 MapCommand(i32 a, i32 b, i32 c, i32 d) OVERRIDE; // slot 1 (0x16a9)
    CObArray m_arr;                                              // +0x7c
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H

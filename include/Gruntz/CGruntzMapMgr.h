// CGruntzMapMgr.h - the grunt-map manager container (C:\Proj\Gruntz). Its dtor
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
    virtual ~CMapMgr(); // 0x135c (external)
    char m_pad[0x78];   // +0x04..+0x7b (vptr at +0x00)
};

SIZE_UNKNOWN(CGruntzMapMgr);
class CGruntzMapMgr : public CMapMgr {
public:
    virtual ~CGruntzMapMgr(); // 0x85d10
    void Reset();             // 0x9ec30 (external, reloc-masked)
    CObArray m_arr;           // +0x7c
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H

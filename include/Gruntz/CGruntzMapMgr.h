// CGruntzMapMgr.h - the grunt-map manager container (C:\Proj\Gruntz). Its dtor
// (0x85d10) sits in the 0x85xxx text region next to CMapLogic::FreeNodes (0x85480)
// and shares the SAME node-pool teardown idiom (the freelist push of every CObArray
// element back onto g_freeList, then SetSize(0,-1) + Reset @0x9ec30).
//
// Layout recovered from the dtor: a polymorphic CMapMgr base (dtor @0x135c) with
// its vptr at +0x00 occupying +0x00..+0x7b, then a CObArray-like pointer array at
// +0x7c. The class is modeled real-polymorphic so cl emits the implicit vptr
// re-stamp stamp-FIRST (docs/patterns/eh-dtor-implicit-vptr-stamp-first.md); the
// emitted ??_7 vtables reloc-mask against retail's 0x5e9bb4 / base vtable. Only
// offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CGRUNTZMAPMGR_H
#define GRUNTZ_CGRUNTZMAPMGR_H

#include <Ints.h>
#include <rva.h>

// The MFC CObArray of map-node pointers embedded at +0x7c (CObject layout:
// vtable@+0, m_pData@+4, m_nSize@+8, m_nMaxSize@+0xc, m_nGrowBy@+0x10). SetSize
// @0x1b4f75 and the member dtor @0x1b4f3e (shared with ~CByteArray) are
// external/no-body so the calls reloc-mask. The destructible member forces the
// /GX EH frame. This is a reduced local model of the MFC CObArray container (the
// real one lives in <Mfc.h>, not pulled into this pure-engine TU).
SIZE_UNKNOWN(CObArray);
struct CObArray {
    void SetSize(i32 n, i32 growBy); // 0x1b4f75 (__thiscall, ret 8)
    ~CObArray();                     // 0x1b4f3e (shared ~C*Array body)
    void* m_vtbl;                    // +0x00
    void** m_pData;                  // +0x04
    i32 m_nSize;                     // +0x08
    i32 m_nMaxSize;                  // +0x0c
    i32 m_nGrowBy;                   // +0x10
};

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

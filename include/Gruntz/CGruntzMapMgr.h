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

// CObArray-like MFC pointer array embedded at +0x7c (CObject layout: vtable@+0,
// m_pData@+4, m_nSize@+8, m_nMaxSize@+0xc, m_nGrowBy@+0x10). SetSize @0x1b4f75 and
// the member dtor @0x1b4f3e (~CByteArray) are external/no-body so the calls
// reloc-mask. The destructible member forces the /GX EH frame.
struct CGmmPtrArray {
    void SetSize(i32 n, i32 growBy); // 0x1b4f75 (__thiscall, ret 8)
    ~CGmmPtrArray();                 // 0x1b4f3e (~CByteArray)
    void* m_vtbl;                    // +0x00
    void** m_pData;                  // +0x04
    i32 m_nSize;                     // +0x08
    i32 m_nMaxSize;                  // +0x0c
    i32 m_nGrowBy;                   // +0x10
};

// The polymorphic base container (~CMapMgr @0x135c). Out-of-line virtual dtor so cl
// emits a CALL (reloc-masks against retail's base dtor) rather than inlining it.
struct CGmmBase {
    virtual ~CGmmBase(); // 0x135c (~CMapMgr; external)
    char m_pad[0x78];    // +0x04..+0x7b (vptr at +0x00)
};

class CGruntzMapMgr : public CGmmBase {
public:
    virtual ~CGruntzMapMgr(); // 0x85d10
    void Reset();             // 0x9ec30 (external, reloc-masked)
    CGmmPtrArray m_arr;       // +0x7c
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H

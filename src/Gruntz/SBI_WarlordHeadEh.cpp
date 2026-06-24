#include <rva.h>
#include <Ints.h>
// SBI_WarlordHeadEh.cpp - the /GX EH-framed CSBI_WarlordHead scalar destructor,
// split off the frameless sbi_warlordhead TU (C:\Proj\Gruntz). MSVC5's /GX frames
// the dtor's 5-level base-subobject teardown walk; it cannot share the base TU's
// frameless flags. The split is matching-neutral (each function is RVA-keyed).
//
// CSBI_WarlordHead : CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.

// The retail vtables stamped as the destructor unwinds the hierarchy, reproduced by
// address (DATA() externs, reloc-masked) - the transitional manual-stamp device
// while the full hierarchy's vtables are not yet modeled. Same symbols as
// CStatusBarMgr.cpp / SBI_RectOnlyEh.cpp's g_vtbl_t3/t4.
DATA(0x001ead24)
extern void* g_vtbl_warlord[]; // 0x5ead24 (CSBI_WarlordHead most-derived subobject)
DATA(0x001eac4c)
extern void* g_vtbl_t4[]; // 0x5eac4c (CSBI_ImageSet subobject)
DATA(0x001eac0c)
extern void* g_vtbl_t3[]; // 0x5eac0c (CSBI_Image subobject)
DATA(0x001eab8c)
extern void* g_vtbl_rectBase[]; // 0x5eab8c (CSBI_RectOnly subobject)
DATA(0x001eabcc)
extern void* g_vtbl_statusBase[]; // 0x5eabcc (CStatusBarItem base subobject)

class CSBI_WarlordHead {
public:
    ~CSBI_WarlordHead();
    void DtorReset();  // 0xe7400  WarlordHead + ImageSet member teardown (shared)
    void DtorImage();  // 0xe6d90  CSBI_Image base teardown
    void DtorRect();   // 0xe8760  CSBI_RectOnly base teardown
    void DtorStatus(); // 0x10bfa0 CStatusBarItem base teardown
    char m_pad[0x60];
};

// The CSBI_WarlordHead scalar destructor (0x104a00): walks the full 5-level chain,
// re-stamping each base's vtable before its teardown. /GX frames the whole walk
// (the trylevel writes 0/1/2/3/-1 are the EH-state machine's, auto-generated).
// @early-stop
// eh-dtor-needs-base-subobject wall: the five vptr-stamp + base-teardown-call pairs
// are byte-exact, but the whole /GX SEH frame (push -1/handler/fs:0 + the
// 0/1/2/3/-1 trylevel stamps) is MISSING - MSVC only frames a dtor whose base
// SUBOBJECT has a non-trivial dtor, which the manual-vptr non-polymorphic model
// can't express (docs/patterns/eh-dtor-needs-base-subobject.md). The real 5-level
// polymorphic hierarchy would re-shape the ctor + emit ??_7/??_G and regress the
// frameless leaves. Deferred to the final sweep (whole-class model).
RVA(0x00104a00, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    *(void**)this = g_vtbl_warlord;
    DtorReset();
    *(void**)this = g_vtbl_t4;
    DtorReset();
    *(void**)this = g_vtbl_t3;
    DtorImage();
    *(void**)this = g_vtbl_rectBase;
    DtorRect();
    *(void**)this = g_vtbl_statusBase;
    DtorStatus();
}

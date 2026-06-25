#include <rva.h>
#include <Ints.h>
// SBI_WellGooEh.cpp - the /GX EH-framed CSBI_WellGoo scalar destructor, split off
// the frameless sbi_wellgoo TU (C:\Proj\Gruntz). MSVC5's /GX frames the dtor's
// 4-level base-subobject teardown walk (plus the WellGoo-level goo-surface free);
// it cannot share the base TU's frameless flags. The split is matching-neutral
// (each function is RVA-keyed).
//
// CSBI_WellGoo : CSBI_Image : CSBI_RectOnly : CStatusBarItem.

// The retail vtables stamped as the destructor unwinds the hierarchy, reproduced
// by address (DATA() externs, reloc-masked) - the transitional manual-stamp device
// while the full hierarchy's vtables are not yet modeled. Same base symbols as
// SBI_ImageEh.cpp / SBI_WarlordHeadEh.cpp.
DATA(0x001eadfc)
extern void* g_vtbl_wellGoo[]; // 0x5eadfc (CSBI_WellGoo most-derived subobject)
DATA(0x001eac0c)
extern void* g_vtbl_image[]; // 0x5eac0c (CSBI_Image base subobject)
DATA(0x001eab8c)
extern void* g_vtbl_rectBase[]; // 0x5eab8c (CSBI_RectOnly base subobject)
DATA(0x001eabcc)
extern void* g_vtbl_statusBase[]; // 0x5eabcc (CStatusBarItem base subobject)

// The pool collection the goo surface is returned to: RemoveItemA (0x142160,
// __thiscall) frees one held surface. Reached via m_24->m_1c (the host's pool).
struct CGooPool {
    void RemoveItemA(void* item); // 0x142160
};
struct CGooHost {
    char m_pad0[0x1c];
    CGooPool* m_1c; // +0x1c  surface pool
};

class CSBI_WellGoo {
public:
    ~CSBI_WellGoo();
    void DtorImage();  // 0xe6d90  CSBI_Image base teardown
    void DtorRect();   // 0xe8760  CSBI_RectOnly base teardown
    void DtorStatus(); // 0x10bfa0 CStatusBarItem base teardown

    char m_pad0[0x24];
    CGooHost* m_24; // +0x24  config host (its m_1c is the surface pool)
    char m_pad28[0x34 - 0x28];
    void* m_34; // +0x34  goo source surface (freed back to the pool)
    char m_pad38[0x60 - 0x38];
};

// The CSBI_WellGoo scalar destructor (0x104bb0): stamps the WellGoo vtable, frees
// the goo source surface back to the pool (its own member teardown), then walks
// the 3 base levels, re-stamping each base's vtable before its teardown. /GX frames
// the whole walk (the trylevel writes 0/1/2/-1 are the EH-state machine's,
// auto-generated).
// @early-stop
// eh-dtor-needs-base-subobject wall: the WellGoo-level goo free + the three
// vptr-stamp + base-teardown-call pairs are byte-exact, but the whole /GX SEH frame
// (push -1/handler/fs:0 + the 0/1/2/-1 trylevel stamps) is MISSING - MSVC only
// frames a dtor whose base SUBOBJECT has a non-trivial dtor, which the manual-vptr
// non-polymorphic model can't express (docs/patterns/eh-dtor-needs-base-subobject.md).
// The real 4-level polymorphic hierarchy would re-shape the ctor + emit ??_7/??_G
// and regress the frameless leaves. Deferred to the final sweep (whole-class model).
RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    *(void**)this = g_vtbl_wellGoo;
    if (m_34) {
        m_24->m_1c->RemoveItemA(m_34);
        m_34 = 0;
    }
    *(void**)this = g_vtbl_image;
    DtorImage();
    *(void**)this = g_vtbl_rectBase;
    DtorRect();
    *(void**)this = g_vtbl_statusBase;
    DtorStatus();
}

#include <rva.h>
// SBI_MenuItemEh.cpp - the /GX EH-framed CSBI_MenuItem base destructor, split off
// the frameless sbi_menuitem TU (C:\Proj\Gruntz). MSVC5's /GX frames the dtor's
// base-subobject teardown walk; it cannot share the base TU's frameless flags.
// The split is matching-neutral (each function is RVA-keyed).

// The retail vtables stamped as the destructor unwinds the hierarchy
//   CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem
// reproduced by address (DATA() externs, reloc-masked) - the transitional
// manual-stamp device while the full hierarchy's vtables are not yet modeled.
DATA(0x001eab4c)
extern void* g_vtbl_menuItem[]; // 0x5eab4c (CSBI_MenuItem most-derived subobject)
DATA(0x001eac0c)
extern void* g_vtbl_image[]; // 0x5eac0c (CSBI_Image base subobject)
DATA(0x001eab8c)
extern void* g_vtbl_rectBase[]; // 0x5eab8c (CSBI_RectOnly base subobject)
DATA(0x001eabcc)
extern void* g_vtbl_statusBase[]; // 0x5eabcc (CStatusBarItem base subobject)

class CSBI_MenuItem {
public:
    ~CSBI_MenuItem();
    void DtorMenu();   // 0xe81a0  most-derived subobject teardown
    void DtorImage();  // 0xe6d90  CSBI_Image base teardown
    void DtorRect();   // 0xe8760  CSBI_RectOnly base teardown
    void DtorStatus(); // 0x10bfa0 CStatusBarItem base teardown
    char m_pad[0x40];
};

// The scalar destructor walks the class's base-subobject chain: before tearing
// down each base it re-stamps the vptr to that base's retail vtable, then calls
// that base's teardown. /GX frames the whole walk (the trylevel writes 0/1/2/-1
// are the EH-state machine's, auto-generated).
// @early-stop
// ~43% (eh-dtor-needs-base-subobject wall): the four vptr-stamp + base-dtor-call
// pairs are byte-exact, but the whole /GX SEH frame (push -1/handler/fs:0 + the
// 0/1/2/-1 trylevel stamps) is MISSING - MSVC only frames a dtor whose base
// SUBOBJECT has a non-trivial dtor, which the manual-vptr non-polymorphic model
// can't express. Documented wall (docs/patterns/eh-dtor-needs-base-subobject.md);
// converting to a real base hierarchy would re-shape the ctor + emit a ??_7/??_G
// and regress every exact sibling. Deferred to the final sweep (whole-class model).
RVA(0x001007d0, 0x7f)
CSBI_MenuItem::~CSBI_MenuItem() {
    *(void**)this = g_vtbl_menuItem;
    DtorMenu();
    *(void**)this = g_vtbl_image;
    DtorImage();
    *(void**)this = g_vtbl_rectBase;
    DtorRect();
    *(void**)this = g_vtbl_statusBase;
    DtorStatus();
}

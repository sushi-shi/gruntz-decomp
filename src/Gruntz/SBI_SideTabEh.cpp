#include <rva.h>
#include <Ints.h>
// SBI_SideTabEh.cpp - the /GX EH-framed CSBI_SideTab scalar destructor, split off
// the frameless sbi_sidetab TU (C:\Proj\Gruntz). MSVC5's /GX frames the dtor's
// base-subobject teardown walk; it cannot share the base TU's frameless flags.
// The split is matching-neutral (each function is RVA-keyed).

// The retail vtables stamped as the destructor unwinds the hierarchy
//   CSBI_SideTab : CStatusBarItem
// reproduced by address (DATA() externs, reloc-masked) - the transitional
// manual-stamp device while the full hierarchy's vtables are not yet modeled.
DATA(0x001eae3c)
extern void* g_vtbl_sideTab[]; // 0x5eae3c (CSBI_SideTab most-derived subobject)
DATA(0x001eabcc)
extern void* g_vtbl_statusBase[]; // 0x5eabcc (CStatusBarItem base subobject)

class CSBI_SideTab {
public:
    ~CSBI_SideTab();
    void Reset();      // 0xe9800  most-derived (SideTab) member teardown
    void DtorStatus(); // 0x10bfa0 CStatusBarItem base teardown
    char m_pad[0x60];
};

// The scalar destructor stamps the SideTab vptr, runs its member teardown (Reset),
// stamps the base vptr, then runs the base teardown. /GX frames the whole walk
// (the trylevel writes 0/-1 are the EH-state machine's, auto-generated).
// @early-stop
// ~43% (eh-dtor-needs-base-subobject wall): the two vptr-stamp + dtor-call pairs
// are byte-exact, but the whole /GX SEH frame (push -1/handler/fs:0 + the 0/-1
// trylevel stamps) is MISSING - MSVC only frames a dtor whose base SUBOBJECT has a
// non-trivial dtor, which the manual-vptr non-polymorphic model can't express.
// Documented wall (docs/patterns/eh-dtor-needs-base-subobject.md); converting to a
// real base hierarchy would re-shape the ctor + emit a ??_7/??_G and regress the
// frameless leaves. Deferred to the final sweep (whole-class model).
RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    *(void**)this = g_vtbl_sideTab;
    Reset();
    *(void**)this = g_vtbl_statusBase;
    DtorStatus();
}

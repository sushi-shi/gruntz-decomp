#include <rva.h>
#include <Ints.h>
// SBI_StatzTabGruntBarEh.cpp - the /GX EH-framed CSBI_StatzTabGruntBar scalar
// destructor, split off the frameless sbi_statztabgruntbar TU (C:\Proj\Gruntz).
// MSVC5's /GX frames the dtor's 2-level base-subobject teardown walk; it cannot share
// the base TU's frameless flags. The split is matching-neutral (each function is
// RVA-keyed). CSBI_StatzTabGruntBar : CStatusBarItem.

// The retail vtables stamped as the destructor unwinds the hierarchy, reproduced by
// address (DATA() externs, reloc-masked) - the transitional manual-stamp device while
// the full hierarchy's vtables are not yet modeled. The base vtable 0x5eabcc is the
// same CStatusBarItem subobject vtable the rest of the SBI family stamps.
DATA(0x001eace4)
extern void* g_vtbl_statz[]; // 0x5eace4 (CSBI_StatzTabGruntBar most-derived subobject)
DATA(0x001eabcc)
extern void* g_vtbl_statusBase[]; // 0x5eabcc (CStatusBarItem base subobject)

class CSBI_StatzTabGruntBar {
public:
    ~CSBI_StatzTabGruntBar();
    void Reset();      // 0xea470  StatzTab member teardown (drops the glyphs)
    void DtorStatus(); // 0x10bfa0 CStatusBarItem base teardown
    char m_pad[0x88];
};

// The CSBI_StatzTabGruntBar scalar destructor (0x104b00): stamps the most-derived
// vptr, runs its member teardown (Reset), stamps the CStatusBarItem base vptr, runs
// the base teardown. /GX frames the whole walk (the trylevel writes 0/-1 are the
// EH-state machine's, auto-generated).
// @early-stop
// eh-dtor-needs-base-subobject wall: the two vptr-stamp + teardown-call pairs are
// byte-exact, but the whole /GX SEH frame (push -1/handler/fs:0 + the 0/-1 trylevel
// stamps) is MISSING - MSVC only frames a dtor whose base SUBOBJECT has a non-trivial
// dtor, which the manual-vptr non-polymorphic model can't express
// (docs/patterns/eh-dtor-needs-base-subobject.md). The real polymorphic hierarchy
// would re-shape the ctor + emit ??_7/??_G and regress the frameless leaves. Deferred
// to the final sweep (whole-class model). Same wall + shape as CSBI_RectOnly::~
// CSBI_RectOnly (0x100700).
RVA(0x00104b00, 0x55)
CSBI_StatzTabGruntBar::~CSBI_StatzTabGruntBar() {
    *(void**)this = g_vtbl_statz;
    Reset();
    *(void**)this = g_vtbl_statusBase;
    DtorStatus();
}

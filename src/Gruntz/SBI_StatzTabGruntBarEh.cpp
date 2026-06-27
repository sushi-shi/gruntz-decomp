#include <rva.h>
#include <Ints.h>
// SBI_StatzTabGruntBarEh.cpp - the /GX EH-framed CSBI_StatzTabGruntBar destructor (C:\Proj\Gruntz). The split
// off the frameless base TU is matching-neutral (each function is RVA-keyed).
//
// REAL polymorphic hierarchy:  CSBI_StatzTabGruntBar : CStatusBarItem.
// Each base subobject has a non-trivial (inline) virtual dtor, so MSVC folds the
// base teardown chain into the most-derived ~CSBI_StatzTabGruntBar and emits the full /GX SEH
// frame (push -1/handler/fs:0 + the descending trylevel stamps) plus the per-level
// vptr re-stamps. The ??_7 vftables auto-derive on the target (RTTI;
// config/vtable_names.csv), replacing the manual `*(void**)this = &g_vtbl_*` stamps.

struct CStatusBarItem {
    virtual ~CStatusBarItem();
    virtual void Sf1();
    virtual void Sf2();
    virtual void Sf3();
    virtual void Sf4();
    virtual void Sf5();
    virtual void Sf6();
    virtual void Sf7();
    virtual void Sf8();
    virtual void Sf9();
    virtual void Sf10();
    void DtorStatus(); // reloc-masked member teardown
    char m_pad[0x60 - 0x04];
};
inline CStatusBarItem::~CStatusBarItem() { DtorStatus(); }

struct CSBI_StatzTabGruntBar : CStatusBarItem {
    virtual ~CSBI_StatzTabGruntBar();
    void Reset(); // 0xea470  most-derived member teardown (reloc-masked)
};

RVA(0x00104b00, 0x55)
CSBI_StatzTabGruntBar::~CSBI_StatzTabGruntBar() {
    Reset();
}

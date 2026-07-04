#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiDtorChain.h>
// SBI_StatzTabGruntBarEh.cpp - the /GX EH-framed CSBI_StatzTabGruntBar destructor
// (C:\Proj\Gruntz). The split off the frameless base TU is matching-neutral (each
// function is RVA-keyed). Chain: CSBI_StatzTabGruntBar : CStatusBarItem (shared).

struct CSBI_StatzTabGruntBar : CStatusBarItem {
    virtual ~CSBI_StatzTabGruntBar() OVERRIDE;
    void Reset(); // 0xea470  most-derived member teardown (reloc-masked)
};

RVA(0x00104b00, 0x55)
CSBI_StatzTabGruntBar::~CSBI_StatzTabGruntBar() {
    Reset();
}

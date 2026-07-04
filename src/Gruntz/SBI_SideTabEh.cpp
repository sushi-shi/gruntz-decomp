#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiDtorChain.h>
// SBI_SideTabEh.cpp - the /GX EH-framed CSBI_SideTab destructor (C:\Proj\Gruntz).
// The split off the frameless base TU is matching-neutral (each function is
// RVA-keyed). Chain: CSBI_SideTab : CStatusBarItem (shared).

struct CSBI_SideTab : CStatusBarItem {
    virtual ~CSBI_SideTab() OVERRIDE;
    void Reset(); // 0xe9800  most-derived member teardown (reloc-masked)
};

RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}

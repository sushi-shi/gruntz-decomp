#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiDtorChain.h>
// SBI_MenuItemEh.cpp - the /GX EH-framed CSBI_MenuItem destructor (C:\Proj\Gruntz).
// The split off the frameless base TU is matching-neutral (each function is
// RVA-keyed). Chain: CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem
// (the three bases carried by the shared SbiDtorChain.h).

struct CSBI_MenuItem : CSBI_Image {
    virtual ~CSBI_MenuItem() OVERRIDE;
    void DtorMenu(); // 0xe81a0  most-derived member teardown (reloc-masked)
};

RVA(0x001007d0, 0x7f)
CSBI_MenuItem::~CSBI_MenuItem() {
    DtorMenu();
}

#include <rva.h>
#include <Ints.h>

// SBI_MenuItemEh.cpp - the /GX EH-framed CSBI_MenuItem destructor (C:\Proj\Gruntz).
// The split off the frameless base TU is matching-neutral (each function is
// RVA-keyed). Chain: CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem
// (the four levels carried by the shared SbiDtorChain.h). This TU owns the
// out-of-line ~CSBI_MenuItem, so it defines SBI_OWN_MENUITEM_DTOR to suppress the
// header's inline body and supply the RVA-keyed definition below.
#define SBI_OWN_MENUITEM_DTOR
#include <Gruntz/SbiDtorChain.h>

RVA(0x001007d0, 0x7f)
CSBI_MenuItem::~CSBI_MenuItem() {
    DtorMenu();
}

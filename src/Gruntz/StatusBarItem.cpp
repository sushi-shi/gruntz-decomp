// StatusBarItem.cpp - Gruntz CStatusBarItem (C:\Proj\Gruntz).
// Matched: ??0CStatusBarItem@@QAE@XZ @ RVA 0x1005d0 (byte-exact).
#include "StatusBarItem.h"
#include "../rva.h"

// ---------------------------------------------------------------------------
// CStatusBarItem::CStatusBarItem()
// Out-of-line complete-object ctor: zeroes m_4/m_8/m_24/m_28 after the vftable
// is installed.
RVA(0x1005d0, 0x17)
CStatusBarItem::CStatusBarItem()
{
    m_4  = 0;
    m_8  = 0;
    m_24 = 0;
    m_28 = 0;
}

// Out-of-line stubs anchor ??_7CStatusBarItem@@6B@ in this TU (not matched).
CStatusBarItem::~CStatusBarItem() {}
int CStatusBarItem::SbiVfunc0() { return 0; }

// StatusBarItem.cpp - labeling stand-in for the standalone complete-object ctor.
//
// CStatusBarItem is ONE class, defined canonically in <Gruntz/StatusBarItem.h>
// with an INLINE ctor (so the derived CSBI_RectOnly folds it - see SBI_RectOnly.cpp).
// Retail emits that same inline ctor out-of-line as a COMDAT (the complete-object
// ctor at 0x1005d0) wherever a bare CStatusBarItem is constructed. MSVC 5.0,
// however, inlines this tiny ctor at every instantiation we can synthesize, so it
// will not emit a labelable standalone ??0 from the canonical inline form.
//
// To keep the 0x1005d0 byte-match, this TU therefore does NOT include the header:
// it locally redeclares the class with an OUT-OF-LINE ctor purely so MSVC emits a
// standalone ??0 we can label. This is a tooling workaround for the inline-ctor
// COMDAT, NOT a second class the developers wrote.
#include <rva.h>

class CStatusBarItem {
public:
    CStatusBarItem();
    virtual ~CStatusBarItem();
    virtual int SbiVfunc0();

    int m_4;                  // +0x04
    int m_8;                  // +0x08
    char m_padc[0x24 - 0x0c]; // +0x0c..0x23
    int m_24;                 // +0x24
    int m_28;                 // +0x28
};

// ---------------------------------------------------------------------------
// CStatusBarItem::CStatusBarItem()
// Out-of-line complete-object ctor: zeroes m_4/m_8/m_24/m_28 after the vftable
// is installed. Byte-identical to the COMDAT copy of the header's inline ctor.
RVA(0x001005d0, 0x17)
CStatusBarItem::CStatusBarItem() {
    m_4 = 0;
    m_8 = 0;
    m_24 = 0;
    m_28 = 0;
}

// Out-of-line stubs anchor the CStatusBarItem vftable in this TU (not matched).
CStatusBarItem::~CStatusBarItem() {}
int CStatusBarItem::SbiVfunc0() {
    return 0;
}

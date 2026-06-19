#include "../rva.h"
// SBI_RectOnly.cpp - Gruntz CSBI_RectOnly (C:\Proj\Gruntz).
// The constructor is matched byte-exact.
//
// CSBI_RectOnly derives from CStatusBarItem. The retail ctor inlines the base
// ctor (zeroing m_4/m_24/m_28; the base's m_8=0 store is dropped as dead
// because the derived ctor sets m_8=1), then stores its own vptr, then m_8=1.
//
// To reproduce that schedule byte-for-byte the base ctor MUST be inline (so the
// optimiser folds it ahead of the derived vptr store and elides the dead store)
// -- MSVC 5.0 will not inline an out-of-line base ctor. CStatusBarItem is
// therefore declared here with an INLINE ctor, distinct from StatusBarItem.cpp
// (which carries the out-of-line complete-object copy).
// Same class, two TUs: a quirk of MSVC 5.0 ctor emission, noted in the report.

// vftable (CSBI_RectOnly), base vftable (CStatusBarItem).
class CStatusBarItem {
public:
    CStatusBarItem() { m_4 = 0; m_8 = 0; m_24 = 0; m_28 = 0; }
    virtual ~CStatusBarItem();
    virtual int SbiVfunc0();

    int  m_4;                  // +0x04
    int  m_8;                  // +0x08
    char m_padc[0x24 - 0x0c];  // +0x0c..0x23
    int  m_24;                 // +0x24
    int  m_28;                 // +0x28
};

class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    virtual int SbiVfunc0();

    // Engine-label backlog stubs.
    void Stub_0ffde0();
};

CStatusBarItem::~CStatusBarItem() {}
int CStatusBarItem::SbiVfunc0() { return 0; }

// ---------------------------------------------------------------------------
// CSBI_RectOnly::CSBI_RectOnly()
// Inlines the CStatusBarItem base ctor (the dead m_8=0 store is elided), stores
// its own vptr, then sets m_8 = 1.
RVA(0x101fa0, 0x1b)
CSBI_RectOnly::CSBI_RectOnly()
{
    m_8 = 1;
}

int CSBI_RectOnly::SbiVfunc0() { return 1; }

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x0ffde0, 0x5b1)
void CSBI_RectOnly::Stub_0ffde0() {}

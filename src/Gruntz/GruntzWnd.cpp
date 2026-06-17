// GruntzWnd.cpp - CGruntzWnd, the Gruntz main window (a CGameWnd subclass).
// CGruntzWnd adds no fields (size 0x10 = the CGameWnd base). The ctor chains the
// matched CGameWnd ctor (RVA 0x094c10) and installs the CGruntzWnd vftable.
// (The full SEH-framed ~CGruntzWnd @0x946a0 + the vector-deleting dtor @0x94670
// are not matched here - the out-of-line virtual stubs below only exist to emit
// the vftable that the ctor stores.)
#include "../Wap32/Wap32.h"

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd();
    virtual int Wap32GameWndVfunc0();
};

// @address: 0x94640
// @size:    0x12
CGruntzWnd::CGruntzWnd() {}

// ---------------------------------------------------------------------------
// CGameWnd::CGameWnd  @0x094c10 (22 bytes)
// The Gruntz-side base-constructor instance. Calls Destroy() to tear down any
// existing OS-window state (guarded by m_4 == 0 -> no-op), then clears the
// active-window singleton. The vptr store is the implicit compiler prologue.
//
// @address: 0x094c10
// @size:    0x16
// ---------------------------------------------------------------------------
CGameWnd::CGameWnd()
{
    Destroy();
    s_activeWnd = 0;
}

// ---------------------------------------------------------------------------
// The auto-generated scalar-deleting destructor (??_G) emitted by the compiler
// from the inline ~CGameWnd body in Wap32.h. The body inlines Destroy() +
// s_activeWnd = 0, then does the delete-flag check. The @symbol override pins
// the delinker to the ??_G name (the inline dtor itself has no @address).
//
// @address: 0x094d80
// @size:    0x2f
// @symbol:  ??_GCGameWnd@@UAEPAXI@Z
// ---------------------------------------------------------------------------
// Out-of-line stubs so the vftable (??_7CGruntzWnd@@6B@) is emitted in this TU;
// not matched / not @address-annotated.
CGruntzWnd::~CGruntzWnd() {}
int CGruntzWnd::Wap32GameWndVfunc0() { return 0; }

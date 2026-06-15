// GruntzWnd.cpp - CGruntzWnd, the Gruntz main window (a CGameWnd subclass).
// CGruntzWnd adds no fields (size 0x10 = the CGameWnd base). The ctor chains the
// matched CGameWnd ctor (RVA 0x13cf00) and installs the CGruntzWnd vftable.
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

// Out-of-line stubs so the vftable (??_7CGruntzWnd@@6B@) is emitted in this TU;
// not matched / not @address-annotated.
CGruntzWnd::~CGruntzWnd() {}
int CGruntzWnd::Wap32GameWndVfunc0() { return 0; }

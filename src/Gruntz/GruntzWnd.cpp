// GruntzWnd.cpp - CGruntzWnd, the Gruntz main window (a CGameWnd subclass).
// CGruntzWnd adds no fields (size 0x10 = the CGameWnd base). The ctor chains the
// matched CGameWnd ctor and installs the CGruntzWnd vftable.
// (The full SEH-framed ~CGruntzWnd + the vector-deleting dtor
// are not matched here - the out-of-line virtual stubs below only exist to emit
// the vftable that the ctor stores.)
#include <Wap32/Wap32.h>
#include <rva.h>

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd() OVERRIDE;
    virtual i32 Wap32GameWndVfunc0();

    // Engine-label backlog stubs.
    void Stub_094670();
};

RVA(0x00094640, 0x12)
CGruntzWnd::CGruntzWnd() {}

// Out-of-line stubs so the CGruntzWnd vftable is emitted in this TU;
// not matched / not @address-annotated.
CGruntzWnd::~CGruntzWnd() {}
i32 CGruntzWnd::Wap32GameWndVfunc0() {
    return 0;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: tomalla
// @stub
RVA(0x00094670, 0x1e)
void CGruntzWnd::Stub_094670() {}

// size 0x10 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntzWnd, 0x10);

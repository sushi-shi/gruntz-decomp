// GruntzWnd.cpp - CGruntzWnd, the Gruntz main window (a CGameWnd subclass).
// CGruntzWnd adds no fields (size 0x10 = the CGameWnd base). The ctor chains the
// matched CGameWnd ctor (RVA 0x094c10) and installs the CGruntzWnd vftable.
// (The full SEH-framed ~CGruntzWnd @0x946a0 + the vector-deleting dtor @0x94670
// are not matched here - the out-of-line virtual stubs below only exist to emit
// the vftable that the ctor stores.)
#include "../Wap32/Wap32.h"

extern "C" {
__declspec(dllimport) int __stdcall ShowCursor(int bShow);
__declspec(dllimport) int __stdcall ValidateRect(void *hWnd, void *lpRect);
}

// External no-body game manager methods (reloc-masked call targets).
struct CGruntzMgr {
    void OnMsg(int wParam, int lParam) { }
    void Close() { }
    int  Check() { return 0; }
};

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd();
    virtual int Wap32GameWndVfunc0();

    int  UnknownMsgHandler(int wParam, int lParam);   // 0x094b20
    void MessageCloseHandler();                        // 0x094b90
    int  ActivateAppHandler(int wParam, int lParam);   // 0x094bc0
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

// ---------------------------------------------------------------------------
// CGruntzWnd::UnknownMsgHandler @0x094b20 (73 B)
// @address: 0x094b20
// @size:    0x49
// ---------------------------------------------------------------------------
int CGruntzWnd::UnknownMsgHandler(int wParam, int lParam)
{
    CGameApp *app = (CGameApp *)m_8;
    if (app) {
        CGruntzMgr *mgr = (CGruntzMgr *)app->m_8;
        if (mgr) mgr->OnMsg(wParam, lParam);
    }
    if (wParam == 0) {
        if (ShowCursor(1) < 0)
            while (ShowCursor(1) < 0) ;
    }
    SetAppField(wParam, lParam);
    return 0;
}

// ---------------------------------------------------------------------------
// CGruntzWnd::MessageCloseHandler @0x094b90 (27 B)
// @address: 0x094b90
// @size:    0x1b
// ---------------------------------------------------------------------------
void CGruntzWnd::MessageCloseHandler()
{
    CGameApp *app = (CGameApp *)m_8;
    if (app) {
        CGruntzMgr *mgr = (CGruntzMgr *)app->m_8;
        if (mgr) mgr->Close();
    }
    DestroyWindowSelf();
}

// ---------------------------------------------------------------------------
// CGruntzWnd::ActivateAppHandler @0x094bc0 (49 B)
// @address: 0x094bc0
// @size:    0x31
// ---------------------------------------------------------------------------
int CGruntzWnd::ActivateAppHandler(int wParam, int lParam)
{
    CGameApp *app = (CGameApp *)m_8;
    if (app) {
        CGruntzMgr *mgr = (CGruntzMgr *)app->m_8;
        if (mgr && mgr->Check()) {
            if (m_4) ValidateRect(m_4, 0);
            return 1;
        }
    }
    return 0;
}

// GruntzWnd.cpp - CGruntzWnd, the Gruntz main window (a CGameWnd subclass).
// CGruntzWnd adds no fields (size 0x10 = the CGameWnd base). The ctor chains the
// matched CGameWnd ctor and installs the CGruntzWnd vftable.
//
// CGruntzWnd overrides several CGameWnd message handlers (vtable 0x5ea2d4); each
// override forwards the message to the running CGruntzMgr (reached through the
// owning CGameApp: this->m_owner == CGameApp owner, owner->m_8 == the CGruntzMgr) and
// chains the CGameWnd base handler. The forwarders are declared as plain methods
// (not OVERRIDE) so the partial vftable below - whose remaining derived slots are
// still out-of-line stubs - is left undisturbed; the vtable thunk is each
// forwarder's only caller and is reloc-masked, so the mangling is match-neutral.
//
// (The full SEH-framed ~CGruntzWnd + the vector-deleting dtor are partially
// reconstructed; the remaining out-of-line virtual stubs only exist to emit the
// vftable that the ctor stores.)
#include <Wap32/Wap32.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

class CGruntzWnd : public CGameWnd {
public:
    CGruntzWnd();
    virtual ~CGruntzWnd() OVERRIDE;
    virtual i32 PreDispatchMessage(UINT, WPARAM, LPARAM) OVERRIDE; // slot 1
    virtual i32 Wap32GameWndVfunc2() OVERRIDE;                     // slot 2
    virtual i32 OnPaint() OVERRIDE;                                // slot 7
    virtual i32 OnChar(WPARAM, LPARAM) OVERRIDE;                   // slot 8
    virtual i32 OnKeyDown(WPARAM, LPARAM) OVERRIDE;                // slot 9
    virtual i32 OnLButtonDown(WPARAM, i32, i32) OVERRIDE;          // slot 14
    virtual i32 OnLButtonUp(WPARAM, i32, i32) OVERRIDE;            // slot 16
    virtual i32 OnMouseMove(WPARAM, i32, i32) OVERRIDE;            // slot 18
    virtual i32 OnLButtonDblClk(WPARAM, i32, i32) OVERRIDE;        // slot 19
    i32 Wap32GameWndVfunc0(); // non-virtual: vftable now emitted by the 16 real overrides

    // Message-handler overrides (vtable 0x5ea2d4), reconstructed below. Declared
    // non-virtual on purpose (see file header); each forwards to the CGruntzMgr.
    virtual i32 OnClose() OVERRIDE;                                   // slot 4  @0x094b90
    virtual i32 OnKeyUp(WPARAM wParam, LPARAM lParam) OVERRIDE;       // slot 10 @0x094920
    virtual i32 OnActivateApp(WPARAM wParam, LPARAM lParam) OVERRIDE; // slot 12 @0x094b20
    virtual i32 OnRButtonDown(WPARAM keys, i32 x, i32 y) OVERRIDE;    // slot 15 @0x094a20
    virtual i32 OnRButtonUp(WPARAM keys, i32 x, i32 y) OVERRIDE;      // slot 17 @0x094a60
    virtual i32 OnRButtonDblClk(WPARAM keys, i32 x, i32 y) OVERRIDE;  // slot 20 @0x094ae0

    // Reaches the running game manager through the owning CGameApp.
    CGruntzMgr* GameMgr() {
        return (CGruntzMgr*)m_owner->m_gameMgr;
    }
};

RVA(0x00094640, 0x12)
CGruntzWnd::CGruntzWnd() {}

// -------------------------------------------------------------------------
// CGruntzWnd::~CGruntzWnd (0x946a0; /GX EH frame). Runs Destroy() (the inherited
// window teardown) at trylevel 0, then the inline CGameWnd base dtor (Destroy +
// clear the active-window singleton) folds in at trylevel -1. The non-trivial
// CGameWnd base subobject earns the /GX SEH frame once the TU is compiled /GX
// (flags="eh" in config/units.toml) - both vptr re-stamps (??_7CGruntzWnd /
// ??_7CGameWnd, catalog auto-named) reloc-mask. 100%.
RVA(0x000946a0, 0x5f)
CGruntzWnd::~CGruntzWnd() {
    Destroy();
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnChar (WM_CHAR, vtable slot 8). Forwards (wParam, lParam) to the
// game manager's per-state notifier.
RVA(0x000948a0, 0x21)
i32 CGruntzWnd::OnChar(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0b(wParam, lParam);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnKeyDown (WM_KEYDOWN, vtable slot 9).
RVA(0x000948e0, 0x21)
i32 CGruntzWnd::OnKeyDown(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0c(wParam, lParam);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnKeyUp (WM_KEYUP, vtable slot 10). Forwards (wParam, lParam) to the
// game manager's per-state notifier; no manager => not handled (0).
RVA(0x00094920, 0x21)
i32 CGruntzWnd::OnKeyUp(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0d(wParam, lParam);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnLButtonDown (WM_LBUTTONDOWN, vtable slot 14). Forwards (keys, x, y)
// to the manager's per-state notifier.
RVA(0x00094960, 0x26)
i32 CGruntzWnd::OnLButtonDown(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0e(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnLButtonUp (WM_LBUTTONUP, vtable slot 16).
RVA(0x000949a0, 0x26)
i32 CGruntzWnd::OnLButtonUp(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState0f(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnMouseMove (WM_MOUSEMOVE, vtable slot 18).
RVA(0x000949e0, 0x26)
i32 CGruntzWnd::OnMouseMove(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState14(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnRButtonDown (WM_RBUTTONDOWN, vtable slot 15). Forwards (keys, x, y)
// to the manager's per-state notifier.
RVA(0x00094a20, 0x26)
i32 CGruntzWnd::OnRButtonDown(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState11(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnRButtonUp (WM_RBUTTONUP, vtable slot 17).
RVA(0x00094a60, 0x26)
i32 CGruntzWnd::OnRButtonUp(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState12(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnLButtonDblClk (WM_LBUTTONDBLCLK, vtable slot 19).
RVA(0x00094aa0, 0x26)
i32 CGruntzWnd::OnLButtonDblClk(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState10(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnRButtonDblClk (WM_RBUTTONDBLCLK, vtable slot 20).
RVA(0x00094ae0, 0x26)
i32 CGruntzWnd::OnRButtonDblClk(WPARAM keys, i32 x, i32 y) {
    CGruntzMgr* mgr = GameMgr();
    if (!mgr) {
        return 0;
    }
    return mgr->NotifyState13(keys, x, y);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnActivateApp (WM_ACTIVATEAPP, vtable slot 12). Runs a per-frame
// advance through the manager, restores the OS cursor when deactivating
// (wParam == 0: ShowCursor up to >= 0), then chains the CGameWnd base handler.
RVA(0x00094b20, 0x49)
i32 CGruntzWnd::OnActivateApp(WPARAM wParam, LPARAM lParam) {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->AdvanceFrame(wParam, lParam);
    }
    if (!wParam) {
        while (ShowCursor(TRUE) < 0) {
        }
    }
    return CGameWnd::OnActivateApp(wParam, lParam);
}

// -------------------------------------------------------------------------
// CGruntzWnd::OnClose (WM_CLOSE, vtable slot 4). Tears down the manager's sound
// chain, then chains the CGameWnd base WM_CLOSE handler (destroys the window).
RVA(0x00094b90, 0x1b)
i32 CGruntzWnd::OnClose() {
    CGruntzMgr* mgr = GameMgr();
    if (mgr) {
        mgr->UnloadSoundChain();
    }
    return CGameWnd::OnClose();
}

// Out-of-line stubs so the CGruntzWnd vftable is emitted in this TU;
// not matched / not @address-annotated.
i32 CGruntzWnd::Wap32GameWndVfunc0() {
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzWnd::`scalar deleting destructor' (0x094670) - the compiler-generated
// ??_G thunk (call ~CGruntzWnd; if (flags & 1) operator delete(this); return this).
// It has no source body (cl synthesizes it into the emitted vftable), so pin it by
// mangled name; the dtor + delete rel32 calls reloc-mask.
// @rva-symbol: ??_GCGruntzWnd@@UAEPAXI@Z 0x00094670 0x1e

// size 0x10 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntzWnd, 0x10);
VTBL(CGruntzWnd, 0x001ea2d4); // vtable_names -> code (RTTI game class)

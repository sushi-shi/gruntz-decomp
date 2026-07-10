// ActionBeginHost.cpp - the one-shot "begin action" driver (0x0d7220, C:\Proj\Gruntz).
//
// @identity-TODO: the concrete host class is not yet recovered. Its only caller is
// CUserLogic::LoadGruntTypeTable (0x04dd50, a 0x22c0-byte stub not yet
// reconstructed), which drives it on a large game-object action state (fields at
// +0x40c..+0x510). xref evidence gathered (do not re-derive): the +0x410 sub-object
// is a real MFC CString (its accept call 0x1bedde is ?LoadStringA@CString@@QAEHI@Z -
// loads the action's label string by resource id), and the window it posts to is the
// g_gameReg (WwdGameReg @0x24556c) CGameMgr window-holder chain (+0x04 -> +0x04 HWND).
// Full identity recovery is gated on reconstructing LoadGruntTypeTable. Until then the
// host is modeled by its touched offsets (load-bearing); the CString member is typed
// from the proven xref, the rest kept as documented offset views.
#include <Mfc.h> // CString (LoadStringA), PostMessageA, HWND
#include <Gruntz/WwdGameReg.h>
#include <rva.h>

// The g_gameReg base CGameMgr window-holder chain g_gameReg->m_gameWnd->m_hwnd the
// action posts its WM_COMMAND to; reached via a minimal cast (WwdGameReg pads over
// +0x04). wwdfile owns the DATA label at 0x24556c.
extern WwdGameReg* g_gameReg;

SIZE_UNKNOWN(ActionRegWnd);
struct ActionRegWnd {
    char m_pad0[4];
    HWND m_hwnd; // +0x04
};
SIZE_UNKNOWN(ActionRegWndHolder);
struct ActionRegWndHolder {
    char m_pad0[4];
    ActionRegWnd* m_gameWnd; // +0x04
};

// The +0x4e4 peer object the action arms (sets bit 0 on its +0x40 flag word).
SIZE_UNKNOWN(ActionPeer);
struct ActionPeer {
    char m_pad0[0x40];
    i32 m_40; // +0x40
};

SIZE_UNKNOWN(ActionBeginHost);
struct ActionBeginHost {
    char m_pad0[0x40c];
    i32 m_40c;    // +0x40c  the accepted action arg
    CString m_410; // +0x410  the action's label CString (LoadStringA @0x1bedde)
    char m_pad414[0x4e4 - 0x414];
    ActionPeer* m_4e4; // +0x4e4
    char m_pad4e8[0x500 - 0x4e8];
    i32 m_500; // +0x500  begun-guard flag
    char m_pad504[0x510 - 0x504];
    i32 m_510; // +0x510  armed-state field
    i32 Begin(i32 arg);
};

// --- ActionBeginHost::Begin (0x0d7220) ---
// Guarded by m_500: if already begun, bail; otherwise load the action's label
// string (m_410.LoadStringA(arg)); if that resolves, stash the arg, arm the state,
// post WM_COMMAND 0x816e to the top window, and set bit 0 on the +0x4e4 peer.
RVA(0x000d7220, 0x7b)
i32 ActionBeginHost::Begin(i32 arg) {
    if (m_500) {
        return 0;
    }
    if (!m_410.LoadStringA(arg)) {
        return 0;
    }
    m_40c = arg;
    m_510 = 2;
    m_500 = 1;
    PostMessageA(((ActionRegWndHolder*)g_gameReg)->m_gameWnd->m_hwnd, 0x111, 0x816e, 0);
    if (m_4e4) {
        m_4e4->m_40 |= 1;
    }
    return 1;
}

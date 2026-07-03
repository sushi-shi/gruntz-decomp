// Wap32.h - WAP32 engine class declarations (Brian Goble's engine; shared
// C:\Proj\Incs\). Minimal reconstructions sufficient to byte-match the small
// self-contained constructors. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS are load-bearing (they are what the byte-exact ctor proves).
#ifndef WAP32_H
#define WAP32_H

#include <Ints.h>

// <Mfc.h> brings <windows.h> (handle types, WNDCLASSA, MSG, CREATESTRUCTA,
// USER32/GDI32 imports), the MFC-controlled way (afx.h first).
#include <Mfc.h>

// CGameWnd::CreateAndShow receives its 12 CreateWindowExA arguments as a
// CREATESTRUCTA* (the <windows.h> layout, same one CGameApp stores in
// m_createStruct): lpCreateParams@0 .. dwExStyle@0x2c. The window loads [eax+0]
// first (the first stdcall push = the rightmost CreateWindowExA arg,
// lpCreateParams) up through dwExStyle - the interleaved load/push idiom falls
// straight out of reading the CREATESTRUCTA fields in that order.
class CGameApp; // owner back-pointer (CGameWnd::m_owner)

// ---------------------------------------------------------------------------
// CGameWnd - WAP32 window wrapper.
//   The ctor zeroes m_hwnd (+0x04) and m_closeGuard (+0x0c); vptr stored first (natural
//   single-class form).
//
//   The class's window procedure (CGameApp::GameWindowProc, the static stored in
//   WNDCLASS.lpfnWndProc) dispatches every Win32 message to the *active* CGameWnd
//   singleton through this vtable: a per-message virtual handler for each WM_* the
//   engine cares about. The handler returns nonzero "handled" => WndProc returns 0;
//   zero "not handled" => WndProc falls through to DefWindowProcA. The full 22-slot
//   vtable (0x00..0x54) is load-bearing: GameWindowProc's switch dispatches on the
//   exact slot offsets (e.g. WM_MOVE -> +0x14, WM_COMMAND -> +0x54). Most handlers
//   are out-of-line stubs here (vtable anchors); only the ones reconstructed in
//   their own right (the ctor, QuitMessageLoop @ +0x34 = WM_DESTROY) carry bodies.
// ---------------------------------------------------------------------------
class CGameWnd;
// Active-window singleton (DAT_00653c68): the one CGameWnd currently driving the
// WNDPROC. Set by CreateAndShow, cleared by Destroy / ~CGameWnd. Shared so the
// inline ~CGameWnd below (which CGruntzWnd's cross-TU dtor folds) resolves it; the
// reloc that names it is masked in objdiff.
extern CGameWnd* g_activeGameWnd;

class CGameWnd {
public:
    CGameWnd();
    // ~CGameWnd is INLINE in the engine header: it re-runs the window teardown
    // (Destroy) then clears the active-window singleton. The body must be visible
    // here so CGruntzWnd's cross-TU ~CGruntzWnd folds the base-subobject teardown
    // (the call Destroy + s_activeWnd=0) and earns its /GX EH frame. vtbl +0x00.
    virtual ~CGameWnd() {
        Destroy();
        g_activeGameWnd = 0;
    } // +0x00  idx0  dtor
    // Pre-dispatch hook: GameWindowProc calls this for EVERY message before the
    // switch; nonzero swallows the message (WndProc returns 0).
    virtual i32 PreDispatchMessage(UINT uMsg, WPARAM wParam, LPARAM lParam); // +0x04 idx1
    virtual i32 Wap32GameWndVfunc2(); // +0x08  idx2 (unused by WndProc)

    // Per-message virtual handlers (return nonzero = handled). The argument shape
    // mirrors the Win32 message: point messages split lParam into LOWORD/HIWORD.
    virtual i32 OnCreate(LPARAM lParam);                     // +0x0c  idx3  WM_CREATE
    virtual i32 OnClose();                                   // +0x10  idx4  WM_CLOSE
    virtual i32 OnMove(i32 x, i32 y);                        // +0x14  idx5  WM_MOVE
    virtual i32 OnSize(WPARAM type, i32 cx, i32 cy);         // +0x18  idx6  WM_SIZE
    virtual i32 OnPaint();                                   // +0x1c  idx7  WM_PAINT
    virtual i32 OnChar(WPARAM wParam, LPARAM lParam);        // +0x20  idx8  WM_CHAR
    virtual i32 OnKeyDown(WPARAM wParam, LPARAM lParam);     // +0x24  idx9  WM_KEYDOWN
    virtual i32 OnKeyUp(WPARAM wParam, LPARAM lParam);       // +0x28  idx10 WM_KEYUP
    virtual i32 OnSysKeyDown(WPARAM wParam, LPARAM lParam);  // +0x2c  idx11 WM_SYSKEYDOWN
    virtual i32 OnActivateApp(WPARAM wParam, LPARAM lParam); // +0x30  idx12 WM_ACTIVATEAPP
    // +0x34 idx13: WM_DESTROY handler == QuitMessageLoop (frees the manager,
    // optionally reports the error, posts WM_QUIT).
    virtual i32 QuitMessageLoop();                          // +0x34  idx13 WM_DESTROY
    virtual i32 OnLButtonDown(WPARAM keys, i32 x, i32 y);   // +0x38  idx14 WM_LBUTTONDOWN
    virtual i32 OnRButtonDown(WPARAM keys, i32 x, i32 y);   // +0x3c  idx15 WM_RBUTTONDOWN
    virtual i32 OnLButtonUp(WPARAM keys, i32 x, i32 y);     // +0x40  idx16 WM_LBUTTONUP
    virtual i32 OnRButtonUp(WPARAM keys, i32 x, i32 y);     // +0x44  idx17 WM_RBUTTONUP
    virtual i32 OnMouseMove(WPARAM keys, i32 x, i32 y);     // +0x48  idx18 WM_MOUSEMOVE
    virtual i32 OnLButtonDblClk(WPARAM keys, i32 x, i32 y); // +0x4c  idx19 WM_LBUTTONDBLCLK
    virtual i32 OnRButtonDblClk(WPARAM keys, i32 x, i32 y); // +0x50  idx20 WM_RBUTTONDBLCLK
    virtual i32 OnCommand(WPARAM wParam, LPARAM lParam);    // +0x54  idx21 WM_COMMAND

    // Creates the OS window from the CreateWindowExA parameter block (the
    // CGameApp's m_createStruct), registers this object as the active window
    // singleton, then ShowWindow. Returns nonzero on success.
    i32 CreateAndShow(CREATESTRUCTA* pParams, CGameApp* pOwner);
    void Destroy();
    // Drains up to `count` queued messages for this window's HWND, all filtered to
    // a single message id (PeekMessageA min==max==filterMsg, PM_REMOVE). Stops at
    // the first empty peek. Returns nothing (retail leaves eax undefined).
    // @0x13d4e0 (13 callers across the engine).
    void PumpMessages(u32 filterMsg, i32 count);

    HWND m_hwnd;       // +0x04  HWND (set by CreateAndShow / zeroed by ctor)
    CGameApp* m_owner; // +0x08  owning app (set by CreateAndShow; not touched by ctor)
    i32 m_closeGuard;  // +0x0c  guard flag (zeroed by ctor and by CreateAndShow)
};

// CGameMgr - the WAP32 game manager base class (vftable ??_7CGameMgr@@6B@ @
// 0x5e9b8c, 6 slots). The TRUE object is 0x2c bytes (CGameApp::Initialize-
// GameManager does `new CGameMgr` => operator new(0x2c)); the ctor seeds the
// frame clock and a couple of run-state flags. VirtualUnknownMethod02 starts it
// with Run(pGameWnd, szCmdLine) (vtable +0x4) and `delete`s it (scalar-deleting
// dtor @ vtable slot 0) on failure.
//
// This is the GENUINE 0x2c base. Gruntz's own game manager is the derived
// CGruntzMgr (0xa30 bytes, its own vftable @0x5e9b64; see <Gruntz/GruntzMgr.h>):
// CGruntzApp::InitializeGameManager (@0x080a20) does `new CGruntzMgr` =>
// `push 0xa30`, while the engine's own CGameApp::InitializeGameManager
// (@0x13dbc0) does `new CGameMgr` => `push 0x2c`. The two managers no longer
// share one (padded) class - the base is its true size and the derived game
// manager carries the 0xa30 of game state.
namespace WAP32 {
    class CGameMgr {
    public:
        CGameMgr();
        // ~CGameMgr is INLINE: it re-stores the base vftable then runs UnknownClose
        // (clearing the owned pointers). It must be visible here so the derived
        // CGruntzMgr's dtor (another TU) inlines the base-subobject teardown exactly
        // as the retail dtor does (store base vptr + devirtualized UnknownClose
        // call) instead of emitting an out-of-line base-dtor call.
        virtual ~CGameMgr() {
            UnknownClose();
        } // +0x00 idx0 dtor
        virtual i32 Run(CGameWnd* pGameWnd, char* szCmdLine); // +0x04 idx1
        virtual void UnknownClose();                          // +0x08 idx2
        virtual i32 Wap32GameMgrVfunc3();                     // +0x0c idx3 (active? gate)
        virtual void Tick();                                  // +0x10 idx4  per-frame tick
        virtual void Wap32GameMgrVfunc5();                    // +0x14 idx5

        // Non-virtual ctor helpers (called directly from the ctor / Run).
        void InitTimeFields(i32 reset);           // @0x13de70
        void UnknownMethodInitializeTimeGlobal(); // @0x13dea0

        CGameWnd* m_gameWnd; // +0x04  bound game window (set by Run)
        CGameApp* m_owner;   // +0x08  owning app (pGameWnd->m_owner; set by Run)
        i32 m_frameGate;     // +0x0c  nonzero suppresses the per-frame advance
        i32 m_soundEnabled;  // +0x10  sound-on flag (=1 in ctor; WriteInt "Sound")
        i32 m_musicEnabled;  // +0x14  music-on flag (=1 in ctor; WriteInt "Music")
        i32 m_prevTick;      // +0x18  frame-clock reset sentinel (=-1 on reset)
        i32 m_pauseFlag;     // +0x1c  run-state companion (cleared by ctor/Run; inferred)
        i32 m_elapsedMs;     // +0x20  frame-clock accumulator (cleared by InitTimeFields)
        i32 m_startTick;     // +0x24  start tick (timeGetTime, by InitTimeFields)

        // Engine-label backlog stub @0x133380. NOT actually a CGameMgr method (it
        // scalar-deletes the DirectInput device-config grand-base, vftable 0x5ef670)
        // - but the retail symbol is labelled `?...@CGameMgr@WAP32@@QAEXXZ`, so the
        // base obj must mangle it through this class. A method adds NO storage, so
        // sizeof stays 0x2c. Vector-deleting form: stamp the C vftable, run the base
        // subobject teardown (0x134d50), then the delete-flag tail; returns `this`.
        void* vector_deleting_destructor(unsigned int flags);

    private:
        char m_pad28[0x2c - 0x28];
    };
} // namespace WAP32

// CREATESTRUCTA (m_createStruct @ CGameApp+0x210; the same 0x30 <windows.h> layout).

// GameInfo - the 0x1d4-byte window/launch descriptor. Embedded in CGameApp at
// +0x14 (m_gameInfo); VirtualUnknownMethod03 builds one on the stack and hands
// it to VirtualUnknownMethod02, which copies it into the member and uses it to
// register the class + create the window.
struct GameInfo {
    i32 size;                     // +0x000  == sizeof(GameInfo) == 0x1d4
    i32 windowClassFlags;         // +0x004  bit1=Windowed, bit2=DialogFrame
    HINSTANCE hInstance;          // +0x008
    char szCmdLine[0x80];         // +0x00c
    char szGameIdentifier[0x40];  // +0x08c  (cursor/icon/menu resource name)
    char szWindowName[0x40];      // +0x0cc
    char _pad10c[0x40];           // +0x10c
    char szWindowClassName[0x80]; // +0x14c
    i32 windowWidth;              // +0x1cc
    i32 windowHeight;             // +0x1d0
}; // 0x1d4 bytes

// ---------------------------------------------------------------------------
// CGameApp - WAP32 application object.
//   The ctor zeroes a handful of fields then bumps a file-scope instance
//   counter.
//   The ctor schedule emits the +0x10 store BEFORE the +0x0c store, which the
//   source mirrors (m_hAccel initialised before m_hInstance).
//
//   The dispatch methods (VirtualUnknownMethod02/03, InitializeDefaultCreate-
//   Struct) call the other CGameApp methods through the vtable (call [vptr+N]),
//   so the WHOLE class is virtual with the tomalla slot order; matched methods
//   keep their bodies (virtual mangles `U`, not `Q`).
// ---------------------------------------------------------------------------
// CGameApp instance counter. Bumped by the
// ctor, decremented by ~CGameApp. Shared across the gameapp / gruntzapp TUs so
// the inline ~CGameApp below (which CGruntzApp's dtor inlines) resolves it; the
// reloc that names it is masked in objdiff (only the load/store bytes matter).
extern i32 g_gameAppInstanceCount;

class CGameApp {
public:
    CGameApp();
    // ~CGameApp is INLINE in the engine header: it tears down the engine
    // resources then decrements the instance counter. The body must be visible
    // here so CGruntzApp's cross-TU dtor inlines it (CloseResources() call +
    // counter dec under the base-subobject teardown). vtbl +0x00.
    virtual ~CGameApp() {
        CloseResources();
        --g_gameAppInstanceCount;
    }

    // The class's own dispatch surface (this TU matches 02/03 + the two
    // InitializeDefault* + the resource/error helpers); unmatched slots are
    // inline stubs so the vtable indices land on the binary's layout.
    virtual i32 VirtualUnknownMethod02(
        GameInfo* pGameInfo,
        WNDCLASSA* pWndClass,
        CREATESTRUCTA* pCreateStruct
    ); // +0x04
    virtual i32 VirtualUnknownMethod03(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    );                                                      // +0x08
    virtual void VirtualUnknownMethod04() {}                // +0x0c
    virtual void CloseResources();                          // +0x10
    virtual void VirtualUnknownMethod06() {}                // +0x14
    virtual i32 RunMessageLoop();                           // +0x18
    virtual void ReportError(WPARAM wParam, LPARAM lParam); // +0x1c
    virtual void VirtualUnknownMethod09();   // +0x20 idle virtual (tail-calls m_gameMgr->Tick)
    virtual void FreeGameManager();          // +0x24
    virtual void VirtualUnknownMethod11() {} // +0x28
    virtual BOOL InitializeAccelerators(LPCSTR lpTable); // +0x2c
    virtual void ShowError() {}                          // +0x30
    virtual CGameWnd* InitializeGameWindow();            // +0x34
    virtual WAP32::CGameMgr* InitializeGameManager();    // +0x38  (0x13dbc0: new CGameMgr)
    virtual void InitializeDefaultWindowClass();         // +0x3c
    virtual void InitializeDefaultCreateStruct();        // +0x40

    // Static window procedure stored into m_wc.lpfnWndProc.
    static LRESULT CALLBACK GameWindowProc(HWND, UINT, WPARAM, LPARAM);

    // Non-virtual modal-screen handler (reloc-masked; ?@2b0d).
    void RunModal(i32 id, HWND hwnd);

    CGameWnd* m_gameWnd;          // +0x04  the game window (deleted by CloseResources)
    WAP32::CGameMgr* m_gameMgr;   // +0x08  the game manager (deleted by CloseResources)
    HINSTANCE m_hInstance;        // +0x0c  hInstance
    HACCEL m_hAccel;              // +0x10  accelerator table
    GameInfo m_gameInfo;          // +0x14  (0x1d4 bytes; szGameIdentifier @ +0xa0 etc.)
    WNDCLASSA m_wc;               // +0x1e8  registered window class
    CREATESTRUCTA m_createStruct; // +0x210  the CreateWindowEx parameters
    i32 m_appActive;              // +0x240  app-active flag (WM_ACTIVATEAPP wParam; idle gate)
    i32 m_running;                // +0x244  run/resume gate (paired with m_appActive at idle)
    i32 m_errorReported;          // +0x248  error-reported guard (report-once)
    i32 m_errorCode;              // +0x24c  error message id (ShowError; default 0x8009)
    i32 m_errorDetail;            // +0x250  error detail (sprintf "(%i)")

    // The CGameApp scalar-deleting destructor (0x080dd0): stamp the vtable, run
    // CloseResources, decrement the live-instance counter, then the delete-flag tail.
    void* Stub_080dd0(unsigned int flags);
};

#endif // WAP32_H

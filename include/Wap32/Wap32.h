#ifndef WAP32_H
#define WAP32_H

#include <Ints.h>
#include <rva.h> // VTBL

#include <Mfc.h>

enum GruntzCommand;

class CGameApp; // owner back-pointer (CGameWnd::m_owner)

class CGameWnd;
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
    virtual i32
    Wap32GameWndVfunc2(i32 notifyCode, i32 cmdId, i32 lParam); // +0x08 idx2 (OnCommand fan-out)

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
    // As PumpMessages but over a [filterMin, filterMax] message-id range. @0x13d530
    // (orphan copy - fully inlined at all call sites).
    void PumpMessagesRange(u32 filterMin, u32 filterMax, i32 count);

    HWND m_hwnd;       // +0x04  HWND (set by CreateAndShow / zeroed by ctor)
    CGameApp* m_owner; // +0x08  owning app (set by CreateAndShow; not touched by ctor)
    i32 m_closeGuard;  // +0x0c  guard flag (zeroed by ctor and by CreateAndShow)
};

    class CGameMgr;
VTBL(CGameMgr, 0x001e9b8c); // ??_7CGameMgr@@6B@ (RTTI-real, global-ns)
class CGameMgr {
    public:
        CGameMgr();
        // ~CGameMgr is INLINE: it re-stores the base vftable then runs Close
        // (clearing the owned pointers). It must be visible here so the derived
        // CGruntzMgr's dtor (another TU) inlines the base-subobject teardown exactly
        // as the retail dtor does (store base vptr + devirtualized Close
        // call) instead of emitting an out-of-line base-dtor call.
        virtual ~CGameMgr() {
            Close();
        } // +0x00 idx0 dtor
        virtual i32 Run(CGameWnd* pGameWnd, char* szCmdLine); // +0x04 idx1
        virtual void Close();                                 // +0x08 idx2
        virtual i32 IsActive();                     // +0x0c idx3 (active? gate)
        // +0x10 idx4 - the base per-frame tick (body @0x13ddc0, base vtable slot 4
        // holds it DIRECTLY - verified against retail ??_7CGameMgr @0x5e9b8c): sample
        // timeGetTime into the g_wap32Now/g_wap32FrameDelta clock pair, run down the
        // run-state countdown, busy-wait to the ms budget when pacing is armed, and
        // fold the 2s frame-count window into m_fps. CGruntzMgr overrides it (slot 4
        // thunk 0x1c7b -> 0x8b740) with the game tick, which calls this base body
        virtual i32 PerFrameTick();               // +0x10 idx4  @0x13ddc0
        virtual i32 HandleCommand(i32, GruntzCommand, i32); // +0x14 idx5

        // Non-virtual ctor helpers (called directly from the ctor / Run).
        void InitTimeFields(i32 reset); // @0x13de70
        void InitializeTimeGlobal();    // @0x13dea0
        // Frame-pacing helpers (bodies in GameApp.cpp, inside CGameMgr's own
        // contiguous retail method block 0x13dd10..0x13df30; ex RezMgr::):
        void SpinWaitUntil(i32 ms);   // @0x13dec0  busy-wait to the pacing budget
        void SetFrameRate(i32 fps);   // @0x13dee0  arm m_pacingGate + derive m_frameBudgetMs
        i32 TrySetFrameRate(i32 fps); // @0x13df00  install only when pacing inactive

        CGameWnd* m_gameWnd;   // +0x04  bound game window (set by Run)
        CGameApp* m_owner;     // +0x08  owning app (pGameWnd->m_owner; set by Run)
        i32 m_frameGate;       // +0x0c  nonzero suppresses the per-frame advance
        i32 m_soundEnabled;    // +0x10  sound-on flag (=1 in ctor; WriteInt "Sound")
        i32 m_musicEnabled;    // +0x14  music-on flag (=1 in ctor; WriteInt "Music")
        i32 m_fps;             // +0x18  measured frames-per-second (debug HUD "Fps = %i"; =-1 on
                               //        frame-clock reset = no measurement yet; PerFrameTick
                               //        stores count>>1 over each 2000 ms window)
        i32 m_pacingGate;      // +0x1c  frame-pacing gate: the target fps SetFrameRate stores
                               //        (>0 arms PerFrameTick's busy-wait; cleared by ctor/Run;
                               //        ex "m_pauseFlag (inferred)")
        i32 m_frameCounter;    // +0x20  frames-this-window counter (PerFrameTick ++ per frame;
                               //        zeroed by InitTimeFields; a COUNT - ex "m_elapsedMs")
        i32 m_windowStartTick; // +0x24  fps-window start tick (timeGetTime, by InitTimeFields)
        i32 m_frameBudgetMs;   // +0x28  target ms-per-frame (SetFrameRate: 1000/fps)

        // (The former `vector_deleting_destructor` stub @0x133380 is gone: it was never
        // a CGameMgr method at all - it is CInputDevRoot's scalar-deleting destructor
        // ??_GCInputDevRoot@@UAEPAXI@Z, now named at its real rva in src/DinMgr2/DinMgr2.cpp
        // where cl already emits that COMDAT.)
    };

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
    virtual i32 InitInstance(
        GameInfo* pGameInfo,
        WNDCLASSA* pWndClass,
        CREATESTRUCTA* pCreateStruct
    ); // +0x04
    virtual i32 Init(
        HINSTANCE hInstance,
        char* szWindowName,
        char* szGameIdentifier,
        char* szCmdLine,
        i32 windowClassFlags,
        i32 windowWidth,
        i32 windowHeight
    );                                                          // +0x08
    virtual i32 InitDefault(HINSTANCE hInstance, char* szName); // +0x0c  0x080d20
    virtual void CloseResources();                              // +0x10
    // +0x14 slot 5 (0x080d60): readiness gate - `return m_gameWnd && m_gameMgr;`
    // (both the game window and manager are constructed). Out-of-line default body.
    virtual i32 HasWindowAndManager();                      // +0x14  0x080d60
    virtual i32 RunMessageLoop();                           // +0x18
    virtual void ReportError(WPARAM wParam, LPARAM lParam); // +0x1c
    virtual void OnIdle();          // +0x20 idle virtual (tail-calls m_gameMgr->Tick)
    virtual void FreeGameManager(); // +0x24
    // +0x28 slot 10 (0x080d90): default WM_COMMAND handler - `return 0;` (unhandled).
    // Called by CGameWnd::OnCommand; CGruntzApp overrides it (0x080aa0).
    virtual i32
    HandleCommand(i32 notifyCode, GruntzCommand cmdId, i32 lParam); // +0x28 slot 10 0x080d90
    virtual BOOL InitializeAccelerators(LPCSTR lpTable); // +0x2c
    virtual void ShowError() {}                          // +0x30
    virtual CGameWnd* InitializeGameWindow();            // +0x34
    virtual CGameMgr* InitializeGameManager();    // +0x38  (0x13dbc0: new CGameMgr)
    virtual void InitializeDefaultWindowClass();         // +0x3c
    virtual void InitializeDefaultCreateStruct();        // +0x40

    // Static window procedure stored into m_wc.lpfnWndProc.
    static LRESULT CALLBACK GameWindowProc(HWND, UINT, WPARAM, LPARAM);

    // Non-virtual modal-screen handler @0x80c00 (reloc-masked; ILT 0x2b0d). `msg` is the
    // modal TEXT, not an id: the body strlen/strcpy's it into the g_644ea0 message buffer,
    // then DialogBoxParamA's the message dialog on m_hInstance.
    void RunModal(const char* msg, HWND hwnd);

    CGameWnd* m_gameWnd;          // +0x04  the game window (deleted by CloseResources)
    CGameMgr* m_gameMgr;   // +0x08  the game manager (deleted by CloseResources)
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
};

#endif // WAP32_H

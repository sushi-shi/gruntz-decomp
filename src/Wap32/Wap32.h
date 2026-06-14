// Wap32.h - WAP32 engine class declarations (Brian Goble's engine; shared
// C:\Proj\Incs\). Minimal reconstructions sufficient to byte-match the small
// self-contained constructors. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS are load-bearing (they are what the byte-exact ctor proves).
#ifndef WAP32_H
#define WAP32_H

// ---------------------------------------------------------------------------
// Minimal Win32 surface (we do NOT pull in <windows.h> to keep the visible
// symbol SET small - the compiler hashes the visible set and entropy follows
// header churn; see docs/matching-patterns.md). Only the few __stdcall imports
// and the WNDCLASSA layout that the matched methods touch are declared.
// ---------------------------------------------------------------------------
typedef int                 BOOL;
typedef unsigned int        UINT;
typedef unsigned long       DWORD;
typedef unsigned int        WPARAM;
typedef long                LPARAM;
typedef long                LRESULT;
typedef const char *        LPCSTR;
typedef void *              HANDLE;
typedef HANDLE              HWND;
typedef HANDLE              HINSTANCE;
typedef HANDLE              HICON;
typedef HANDLE              HCURSOR;
typedef HANDLE              HBRUSH;
typedef HANDLE              HMENU;
typedef HANDLE              HACCEL;
typedef HANDLE              HGDIOBJ;

typedef LRESULT (__stdcall *WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagWNDCLASSA {
    UINT      style;          // +0x00
    WNDPROC   lpfnWndProc;    // +0x04
    int       cbClsExtra;     // +0x08
    int       cbWndExtra;     // +0x0c
    HINSTANCE hInstance;      // +0x10
    HICON     hIcon;          // +0x14
    HCURSOR   hCursor;        // +0x18
    HBRUSH    hbrBackground;  // +0x1c
    LPCSTR    lpszMenuName;   // +0x20
    LPCSTR    lpszClassName;  // +0x24
} WNDCLASSA;                  // 0x28 bytes (10 dwords)

extern "C" {
__declspec(dllimport) BOOL    __stdcall DestroyAcceleratorTable(HACCEL hAccel);
__declspec(dllimport) HACCEL  __stdcall LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR lpTableName);
__declspec(dllimport) BOOL    __stdcall PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
__declspec(dllimport) HCURSOR __stdcall LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
__declspec(dllimport) HICON   __stdcall LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName);
__declspec(dllimport) HGDIOBJ __stdcall GetStockObject(int i);
__declspec(dllimport) HWND    __stdcall CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
                                                        LPCSTR lpWindowName, DWORD dwStyle,
                                                        int X, int Y, int nWidth, int nHeight,
                                                        HWND hWndParent, HMENU hMenu,
                                                        HINSTANCE hInstance, void *lpParam);
__declspec(dllimport) BOOL    __stdcall ShowWindow(HWND hWnd, int nCmdShow);
__declspec(dllimport) HMENU   __stdcall LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName);
__declspec(dllimport) int     __stdcall GetSystemMetrics(int nIndex);
__declspec(dllimport) short   __stdcall RegisterClassA(const WNDCLASSA *lpWndClass);
}

// CreateWindowExA's 12 arguments, packed into one params struct (CGameWnd's
// caller fills this and passes its address to CreateAndShow). Fields are stored
// in REVERSE CreateWindowExA-arg order: the binary loads [eax+0] first (the
// first stdcall push = the LAST/rightmost arg, lpParam) up through [eax+0x2c]
// (dwExStyle). Reading them in this declared order reproduces the exact
// interleaved load/push idiom.
struct CGameWndCreateParams {
    void     *lpParam;       // +0x00
    HINSTANCE hInstance;     // +0x04
    HMENU     hMenu;         // +0x08
    HWND      hWndParent;    // +0x0c
    int       nHeight;       // +0x10
    int       nWidth;        // +0x14
    int       Y;             // +0x18
    int       X;             // +0x1c
    DWORD     dwStyle;       // +0x20
    LPCSTR    lpWindowName;  // +0x24
    LPCSTR    lpClassName;   // +0x28
    DWORD     dwExStyle;     // +0x2c
};

// ---------------------------------------------------------------------------
// CGameWnd - WAP32 window wrapper.
//   vftable @0x5ea344. ctor (RVA 0x13cf00, 17 bytes) zeroes m_4 (+0x04) and
//   m_c (+0x0c); vptr stored first (natural single-class form).
// ---------------------------------------------------------------------------
class CGameWnd {
public:
    CGameWnd();
    virtual ~CGameWnd();
    virtual int Wap32GameWndVfunc0();

    // Creates the OS window from a 12-field params struct (CreateWindowExA),
    // registers this object as the active window singleton, then ShowWindow.
    // Returns nonzero on success.  RVA 0x13cf20.
    int CreateAndShow(CGameWndCreateParams *pParams, void *pOwner);

    HWND m_4;   // +0x04  HWND (set by CreateAndShow / zeroed by ctor)
    void *m_8;  // +0x08  owner pointer (set by CreateAndShow; not touched by ctor)
    int  m_c;   // +0x0c  guard flag (zeroed by ctor and by CreateAndShow)
};

// Minimal polymorphic resource objects whose pointers live in CGameApp::m_4 /
// m_8. CloseResources `delete`s them; the scalar-deleting destructor is at
// vtable slot 0, so a single virtual dtor reproduces `mov [ecx];push 1;call`.
// m_4 additionally exposes a window handle (+0x4) + a guard flag (+0xc) used
// by ReportError to post WM_CLOSE.
class CGameResource {
public:
    virtual ~CGameResource();

    int m_4;   // +0x04  (HWND for ReportError's PostMessageA)
    int m_8;   // +0x08
    int m_c;   // +0x0c  guard flag
};

// CGameMgr - the game manager allocated by CGameApp::InitializeGameManager
// (the engine puts it in namespace WAP32). VirtualUnknownMethod02 starts it
// with Run(pGameWnd, szCmdLine) (vtable +0x4) and `delete`s it (scalar-deleting
// dtor @ vtable slot 0) on failure. Forward-declared here so the
// InitializeGameManager return type is covariant with CGruntzApp's override;
// the full definition lives in the TU that uses it.
namespace WAP32 { class CGameMgr; }

// CREATESTRUCTA layout (the 12 fields CreateWindowEx receives), embedded in
// CGameApp at +0x210 (m_createStruct); InitializeDefaultCreateStruct fills it.
typedef struct tagCREATESTRUCTA {
    void *    lpCreateParams; // +0x00
    HINSTANCE hInstance;      // +0x04
    HMENU     hMenu;          // +0x08
    HWND      hwndParent;     // +0x0c
    int       cy;             // +0x10
    int       cx;             // +0x14
    int       y;              // +0x18
    int       x;              // +0x1c
    long      style;          // +0x20
    LPCSTR    lpszName;       // +0x24
    LPCSTR    lpszClass;      // +0x28
    DWORD     dwExStyle;      // +0x2c
} CREATESTRUCTA;             // 0x30 bytes

// GameInfo - the 0x1d4-byte window/launch descriptor. Embedded in CGameApp at
// +0x14 (m_gameInfo); VirtualUnknownMethod03 builds one on the stack and hands
// it to VirtualUnknownMethod02, which copies it into the member and uses it to
// register the class + create the window.
struct GameInfo {
    int       size;                // +0x000  == sizeof(GameInfo) == 0x1d4
    int       windowClassFlags;    // +0x004  bit1=Windowed, bit2=DialogFrame
    HINSTANCE hInstance;           // +0x008
    char      szCmdLine[0x80];     // +0x00c
    char      szGameIdentifier[0x40]; // +0x08c  (cursor/icon/menu resource name)
    char      szWindowName[0x40];  // +0x0cc
    char      _pad10c[0x40];       // +0x10c
    char      szWindowClassName[0x80]; // +0x14c
    int       windowWidth;         // +0x1cc
    int       windowHeight;        // +0x1d0
};                                 // 0x1d4 bytes

// ---------------------------------------------------------------------------
// CGameApp - WAP32 application object.
//   vftable @0x5e9b0c. ctor (RVA 0x13d590, 60 bytes) zeroes a handful of
//   fields then bumps a file-scope instance counter at 0x653c6c.
//   The ctor schedule emits the +0x10 store BEFORE the +0x0c store, which the
//   source mirrors (m_10 initialised before m_c).
//
//   The dispatch methods (VirtualUnknownMethod02/03, InitializeDefaultCreate-
//   Struct) call the other CGameApp methods through the vtable (call [vptr+N]),
//   so the WHOLE class is virtual with the tomalla slot order; matched methods
//   keep their bodies (virtual mangles `U`, not `Q`).
// ---------------------------------------------------------------------------
// CGameApp instance counter (binary: global int @ 0x653c6c). Bumped by the
// ctor, decremented by ~CGameApp. Shared across the gameapp / gruntzapp TUs so
// the inline ~CGameApp below (which CGruntzApp's dtor inlines) resolves it; the
// reloc that names it is masked in objdiff (only the load/store bytes matter).
extern int g_gameAppInstanceCount;

class CGameApp {
public:
    CGameApp();
    // ~CGameApp is INLINE in the engine header: it tears down the engine
    // resources then decrements the instance counter. The body must be visible
    // here so CGruntzApp's cross-TU dtor inlines it (CloseResources() call +
    // counter dec under the base-subobject teardown). vtbl +0x00.
    virtual ~CGameApp() { CloseResources(); --g_gameAppInstanceCount; }

    // The class's own dispatch surface (this TU matches 02/03 + the two
    // InitializeDefault* + the resource/error helpers); unmatched slots are
    // inline stubs so the vtable indices land on the binary's layout.
    virtual int VirtualUnknownMethod02(GameInfo *pGameInfo, WNDCLASSA *pWndClass,
                                       CREATESTRUCTA *pCreateStruct);     // +0x04  0x13d5d0
    virtual int VirtualUnknownMethod03(HINSTANCE hInstance, char *szWindowName,
                                       char *szGameIdentifier, char *szCmdLine,
                                       int windowClassFlags, int windowWidth,
                                       int windowHeight);                  // +0x08  0x13d7b0
    virtual void VirtualUnknownMethod04() {}                              // +0x0c
    virtual void CloseResources();                                       // +0x10  0x13d8c0
    virtual void VirtualUnknownMethod06() {}                              // +0x14
    virtual void VirtualUnknownMethod07() {}                              // +0x18
    virtual void ReportError(WPARAM wParam, LPARAM lParam);              // +0x1c  0x13dcb0
    virtual void VirtualUnknownMethod09() {}                             // +0x20
    virtual void FreeGameManager() {}                                    // +0x24
    virtual void VirtualUnknownMethod11() {}                             // +0x28
    virtual BOOL InitializeAccelerators(LPCSTR lpTable);                 // +0x2c  0x13dc20
    virtual void ShowError() {}                                          // +0x30
    virtual CGameWnd *InitializeGameWindow();                            // +0x34  0x13db60
    virtual WAP32::CGameMgr *InitializeGameManager() { return 0; }       // +0x38
    virtual void InitializeDefaultWindowClass();                        // +0x3c  0x13d9b0
    virtual void InitializeDefaultCreateStruct();                       // +0x40  0x13da50

    // Static window procedure stored into m_wc.lpfnWndProc (RVA 0x13cff0).
    static LRESULT __stdcall GameWindowProc(HWND, UINT, WPARAM, LPARAM);

    CGameResource *m_4;  // +0x04  deleted by CloseResources (the CGameWnd)
    CGameResource *m_8;  // +0x08  deleted by CloseResources (the CGameMgr)
    HINSTANCE m_c;       // +0x0c  hInstance
    HACCEL m_10;         // +0x10  accelerator table
    GameInfo m_gameInfo; // +0x14  (0x1d4 bytes; szGameIdentifier @ +0xa0 etc.)
    WNDCLASSA m_wc;      // +0x1e8  registered window class
    CREATESTRUCTA m_createStruct; // +0x210  the CreateWindowEx parameters
    int  m_240;          // +0x240
    int  m_244;          // +0x244
    int  m_248;          // +0x248  error-reported guard
    int  m_24c;          // +0x24c  error code
    int  m_250;          // +0x250  error detail
};

#endif // WAP32_H

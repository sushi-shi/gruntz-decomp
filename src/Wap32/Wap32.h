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

// ---------------------------------------------------------------------------
// CGameApp - WAP32 application object.
//   vftable @0x5e9b0c. ctor (RVA 0x13d590, 60 bytes) zeroes a handful of
//   fields then bumps a file-scope instance counter at 0x653c6c.
//   The ctor schedule emits the +0x10 store BEFORE the +0x0c store, which the
//   source mirrors (m_10 initialised before m_c).
// ---------------------------------------------------------------------------
class CGameApp {
public:
    CGameApp();
    virtual ~CGameApp();
    virtual int Wap32GameAppVfunc0();

    void CloseResources();                       // 0x13d8c0
    BOOL InitializeAccelerators(LPCSTR lpTable);  // 0x13dc20
    void ReportError(WPARAM wParam, LPARAM lParam); // 0x13dcb0
    void InitializeDefaultWindowClass();          // 0x13d9b0

    // Allocates a CGameWnd (operator new + ctor under a C++ EH frame) and
    // returns it; `this` is never touched.  RVA 0x13db60.
    CGameWnd *InitializeGameWindow();

    // Static window procedure stored into m_wc.lpfnWndProc (RVA 0x13cff0).
    static LRESULT __stdcall GameWindowProc(HWND, UINT, WPARAM, LPARAM);

    CGameResource *m_4;  // +0x04  deleted by CloseResources
    CGameResource *m_8;  // +0x08  deleted by CloseResources
    HINSTANCE m_c;       // +0x0c  hInstance
    HACCEL m_10;         // +0x10  accelerator table
    char m_pad14[0x18 - 0x14];
    char m_18;           // +0x18  flag byte (bit 0x1: use system arrow cursor)
    char m_pad19[0xa0 - 0x19];
    char m_a0[0x160 - 0xa0]; // +0xa0  cursor/icon resource name buffer
    char m_160[0x1e8 - 0x160]; // +0x160 window class name buffer
    WNDCLASSA m_wc;      // +0x1e8  registered window class
    char m_pad210[0x240 - 0x210];
    int  m_240;          // +0x240
    int  m_244;          // +0x244
    int  m_248;          // +0x248  error-reported guard
    int  m_24c;          // +0x24c  error code
    int  m_250;          // +0x250  error detail
};

#endif // WAP32_H

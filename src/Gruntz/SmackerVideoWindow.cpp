// SmackerVideoWindow.cpp - the Smacker playback host's "create the fullscreen
// video window" method (0x17c2a0), which the ApiCaller stub misfiled as
// directx_wrapper_caller_17c2a0_DirectDrawCreate. It registers a private window
// class (AfxRegisterWndClass), refuses if the window already exists, allocates +
// creates a screen-sized top-level MFC CWnd titled "Smacker Video Window", gives it
// focus, then hands its HWND to the host's Init (0x17c040).
//
// The MFC callees (AfxRegisterWndClass / CString ctor+dtor / operator new /
// CWnd ctor+CreateEx+SetFocus) are modeled as reloc-masked externs (match-by-shape);
// their real NAFXCW mangled names differ but objdiff pairs the relocs by type. The
// destructible CString local forces the /GX EH frame (this unit uses the `eh`
// profile). Field NAMES are placeholders; offsets + call-site bytes load-bearing.
#include <Win32.h> // HWND, HMENU, GetSystemMetrics
#include <Ints.h>
#include <rva.h>

// LPCTSTR AFXAPI AfxRegisterWndClass(UINT, HCURSOR=0, HBRUSH=0, HICON=0). __stdcall.
extern "C" const char* __stdcall
AfxRegisterWndClass(u32 style, void* cur, void* brush, void* icon); // 0x1bc09d

// CString == its 4-byte m_pchData; passing it where LPCTSTR is wanted yields that ptr.
struct CString {
    char* m_pchData;
    CString(const char* s); // 0x1b9d4c
    ~CString();             // 0x1b9cde
    operator const char*() const {
        return m_pchData;
    }
};

// The created window (MFC CWnd shape: 0x3c bytes, m_hWnd at +0x1c). Ctor/CreateEx/
// SetFocus are external (the CObject vtable is stamped by the real ctor).
struct CWnd {
    char _00[0x1c];
    HWND m_hWnd; // +0x1c
    char _20[0x3c - 0x20];
    CWnd(); // 0x1baecf
    int CreateEx(
        u32 exStyle,
        const char* cls,
        const char* wnd,
        u32 style,
        int x,
        int y,
        int w,
        int h,
        HWND parent,
        HMENU id,
        void* param
    );               // 0x1bb875
    void SetFocus(); // 0x1be6ce
};

struct CSmackWin {
    char _00[0x53c];
    CWnd* m_53c;                           // +0x53c  the video window
    int Init(HWND h, i32 a0, i32 a1);      // 0x17c040
    int CreateVideoWindow(i32 a0, i32 a1); // 0x17c2a0
};

// @early-stop
// Complete + correct (~98%). Residual is two documented walls: (1) the /GX EH
// scope-table + handler-thunk relocs are named differently ($L.../__except_list vs
// the retail Unwind@... funclet), a reloc-typing artifact; (2) MSVC5 tail-merges the
// two `return 0` guards (already-open + CreateEx-fail) differently than retail (one
// extra `jmp; xor eax,eax`). All logic + args (AfxRegisterWndClass(3), the CString,
// `new CWnd`, CreateEx(8, cls, "Smacker Video Window", 0x90000000, 0,0, GSM(0), GSM(1),
// 0,0,0), SetFocus, Init) + the GetSystemMetrics IAT-cache-in-ebp are byte-exact.
RVA(0x0017c2a0, 0x14e)
int CSmackWin::CreateVideoWindow(i32 a0, i32 a1) {
    CString cls(AfxRegisterWndClass(3, 0, 0, 0));
    if (m_53c != 0) {
        return 0;
    }
    m_53c = new CWnd;
    if (!m_53c->CreateEx(
            8,
            cls,
            "Smacker Video Window",
            0x90000000,
            0,
            0,
            GetSystemMetrics(0),
            GetSystemMetrics(1),
            0,
            0,
            0
        )) {
        return 0;
    }
    m_53c->SetFocus();
    HWND h = m_53c ? m_53c->m_hWnd : 0;
    return Init(h, a0, a1);
}
SIZE_UNKNOWN(CSmackWin);

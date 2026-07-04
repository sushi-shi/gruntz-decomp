// Wnd.h - the single shared minimal view of MFC's CWnd used by the Win32/dialog
// TUs. The wrapped window handle lives at +0x1c (m_hWnd); the object is 0x3c
// bytes. Every method here is a real statically-linked MFC entrypoint modeled with
// NO body, so its `call rel32` displacement reloc-masks (match-by-shape). Field
// offsets + code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_GRUNTZ_CWND_H
#define GRUNTZ_GRUNTZ_CWND_H

#include <Ints.h>
#include <Win32.h>
#include <rva.h>

struct HWND__; // the strong HWND tag MFC's FromHandle signature mangles in

class CWnd {
public:
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
    );                                               // 0x1bb875
    void SetFocus();                                 // 0x1be6ce
    i32 EnableWindow(i32 bEnable);                   // 0x1be6a7
    static CWnd* __stdcall FromHandle(HWND__* hWnd); // MFC permanent/temporary map

    char m_pad00[0x1c];
    HWND m_hWnd; // +0x1c  the wrapped window handle
    char m_pad20[0x3c - 0x20];
};

#endif // GRUNTZ_GRUNTZ_CWND_H

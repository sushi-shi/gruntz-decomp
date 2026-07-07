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
#include <Wap32/Object.h> // Wap::CObject (recognized MFC base)

struct HWND__; // the strong HWND tag MFC's FromHandle signature mangles in

SIZE_UNKNOWN(CCmdTarget);
class CCmdTarget : public Wap::CObject {
public:
    virtual void CtVsl5();               // slot 5
    virtual void CtVsl6();               // slot 6
    virtual void CtVsl7();               // slot 7
    virtual void CtVsl8();               // slot 8
    virtual void CtVsl9();               // slot 9
    virtual void CtVsl10();              // slot 10
    virtual void CtVsl11();              // slot 11
    virtual const void* GetMessageMap(); // slot 12
    virtual void CtVsl13();              // slot 13
    virtual void CtVsl14();              // slot 14
    virtual void CtVsl15();              // slot 15
    virtual void CtVsl16();              // slot 16
    virtual void CtVsl17();              // slot 17
    virtual void CtVsl18();              // slot 18
    virtual void CtVsl19();              // slot 19
    virtual void CtVsl20();              // slot 20
    virtual void CtVsl21();              // slot 21
};

SIZE_UNKNOWN(CWnd);
class CWnd : public CCmdTarget {
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
    virtual void WndVsl22();                         // slot 22
    virtual void WndVsl23();                         // slot 23
    virtual void WndVsl24();                         // slot 24
    virtual void WndVsl25();                         // slot 25
    virtual void WndVsl26();                         // slot 26
    virtual void WndVsl27();                         // slot 27
    virtual void WndVsl28();                         // slot 28
    virtual void WndVsl29();                         // slot 29
    virtual void WndVsl30();                         // slot 30
    virtual void WndVsl31();                         // slot 31
    virtual void WndVsl32();                         // slot 32
    virtual void WndVsl33();                         // slot 33
    virtual void WndVsl34();                         // slot 34
    virtual void WndVsl35();                         // slot 35
    virtual void WndVsl36();                         // slot 36
    virtual void WndVsl37();                         // slot 37
    virtual void WndVsl38();                         // slot 38
    virtual void WndVsl39();                         // slot 39
    virtual void WndVsl40();                         // slot 40
    virtual void WndVsl41();                         // slot 41
    virtual void WndVsl42();                         // slot 42
    virtual void WndVsl43();                         // slot 43
    virtual void WndVsl44();                         // slot 44
    virtual void WndVsl45();                         // slot 45
    virtual void WndVsl46();                         // slot 46
    virtual void WndVsl47();                         // slot 47
    char m_pad04[0x1c - 4];                          // +0x04 (vptr @+0x00)
    HWND m_hWnd;                                     // +0x1c  the wrapped window handle
    char m_pad20[0x3c - 0x20];
};

#endif // GRUNTZ_GRUNTZ_CWND_H

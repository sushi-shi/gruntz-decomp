#include <Win32.h>

#include <rva.h>
#include <string.h>

// Centred text/number overlay drawn through a counter window's polymorphic DC sink
// (RVA 0x164380 / 0x164420), re-homed out of the artificial src/Stub/ApiCallers.cpp
// aggregate. GAME code. No module-scope globals.

namespace ApiCallerStubs {
    i32 __cdecl Format_11f890(char* buf, const char* fmt, ...); // RVA 0x11f890

    // A polymorphic DC source: GetDC is vtable slot 0x44 (#17), Done is slot 0x68 (#26).
    struct DcSink_164380 {
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual void v8();
        virtual void v9();
        virtual void v10();
        virtual void v11();
        virtual void v12();
        virtual void v13();
        virtual void v14();
        virtual void v15();
        virtual void v16();
        virtual i32 GetDC(HDC* out); // slot 17 == vtable +0x44
        virtual void v18();
        virtual void v19();
        virtual void v20();
        virtual void v21();
        virtual void v22();
        virtual void v23();
        virtual void v24();
        virtual void v25();
        virtual void Done(HDC dc); // slot 26 == vtable +0x68
    };
    struct CounterWnd_164380 {
        char m_pad0[8];
        DcSink_164380* m_8; // +0x08
    };
    struct DrawHost_164380 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawCount(RECT* rc, i32 n);
    };
    // __thiscall(rc, n): print n centred into rc using the counter window's DC.
    RVA(0x00164380, 0x98)
    void DrawHost_164380::DrawCount(RECT* rc, i32 n) {
        char buf[0x20];
        Format_11f890(buf, "%i", n);
        CounterWnd_164380* w = m_2c;
        if (!w) {
            return;
        }
        HDC hdc = 0;
        w->m_8->GetDC(&hdc);
        if (!hdc) {
            return;
        }
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);
        DrawTextA(hdc, buf, strlen(buf), rc, 0x25);
        w->m_8->Done(hdc);
    }

    struct DrawHost2_164420 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawLabel(RECT* rc, char* text);
    };
    // __thiscall(rc, text): print text centred into rc using the counter window's DC.
    RVA(0x00164420, 0x79)
    void DrawHost2_164420::DrawLabel(RECT* rc, char* text) {
        CounterWnd_164380* w = m_2c;
        if (!w) {
            return;
        }
        HDC hdc = 0;
        w->m_8->GetDC(&hdc);
        if (!hdc) {
            return;
        }
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);
        DrawTextA(hdc, text, strlen(text), rc, 0x25);
        w->m_8->Done(hdc);
    }

} // namespace ApiCallerStubs

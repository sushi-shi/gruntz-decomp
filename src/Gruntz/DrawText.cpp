// DrawText.cpp - a dialog label text-measure/render helper re-homed out of
// src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// measure a CString label into the item's rect (DrawTextA with
// DT_CALCRECT-ish flags 0x420), clamp the used width into g_62b434, then run the
// engine text renderer at that origin. The scratch draw object is the same
// three-level image-worker/imgHolder hierarchy as m4_FlashRect (vtables in other TUs
// -> reloc-masked); its dtor chain inlines but its 3-arg ctor is out-of-line.
// Placeholder names; only offsets + code bytes are load-bearing. base+/GX.
#include <Win32.h>

#include <Ints.h>
#include <rva.h>
#include <string.h>

namespace m4 {

    // Cached measured text width (0x0062b434), read by the caller after measure.
    extern i32 g_62b434;

    // DrawTextA through the game Win32 pointer table (0x6c454c) -> reloc-masked.
    extern int(WINAPI* g_pDrawTextA)(HDC, LPCSTR, int, LPRECT, UINT); // 0x006c454c

    // The image-worker/imgHolder scratch (see m4_FlashRect): inline dtor chain, but
    // this call site builds it with a 3-arg out-of-line ctor.
    struct SevWorker2 {
        virtual ~SevWorker2() {}
    };
    struct ImgHolder2 : SevWorker2 {
        void Release1c6a5c();
        virtual ~ImgHolder2() OVERRIDE {
            Release1c6a5c();
        }
    };
    struct DrawScratch : ImgHolder2 {
        DrawScratch(i32 a, i32 b, i32 c); // 0x001c6a72
    };

    // The item's rect source: built from the host's +0x1c sub-object; its first
    // field points at the item RECT. Destructible (EH-tracked).
    struct RectSrc {
        RECT* m_rect;             // +0x00
        RectSrc(void* rectField); // 0x001b9ba3
        ~RectSrc();               // 0x001b9cde
    };

    // The engine text renderer bound to the DC (returned by Bind1c56ef).
    struct TextRenderer {
        i32 Push1c58ea(void* obj);                  // 0x001c58ea (save/select, returns prior)
        void MoveTo1c6059(void* out, i32 x, i32 y); // 0x001c6059
        void Draw1c60a5(i32 x, i32 y);              // 0x001c60a5
    };
    TextRenderer* Bind1c56ef(HDC hdc); // 0x001c56ef

    // The dialog host; the item rect sub-object lives at +0x1c.
    struct DrawHost {
        char m_pad00_1c[0x1c];
        i32 MeasureLabel21f20(HDC hdc, const char* text); // 0x00021f20
    };

    // @early-stop
    // regalloc + EH-state wall (~72%). Complete correct reconstruction: the /GX
    // frame, the empty-CString (*(text-8)==0) gate, the rect copy + DrawTextA
    // measure, the min(provW,textW) clamp into g_62b434, and the Bind/Push/MoveTo/
    // Draw render with the inline image-worker/imgHolder scratch dtor chain all align
    // by shape (llvm-objdump -dr). Residual is MSVC5 using a 4th callee-saved reg
    // (ebp) where retail packs into ebx/esi/edi (so the arg + local offsets shift)
    // plus the EH scope-table addend (0 vs 8) - not steerable from source.
    RVA(0x00021f20, 0x162)
    i32 DrawHost::MeasureLabel21f20(HDC hdc, const char* text) {
        if (hdc == 0) {
            return 0;
        }
        RectSrc src((char*)this + 0x1c);
        RECT* rp = src.m_rect;
        if (*((i32*)text - 2) == 0) {
            g_62b434 = 0;
        } else {
            RECT rc;
            rc.left = rp->left;
            rc.top = rp->top;
            rc.right = rp->right;
            rc.bottom = rp->bottom;
            g_pDrawTextA(hdc, text, *((i32*)text - 2), &rc, 0x420);
            i32 textW = rc.right - rc.left;
            i32 provW = rp->right - rp->left;
            g_62b434 = provW;
            if (provW >= textW) {
                g_62b434 = textW;
            }
        }
        TextRenderer* tr = Bind1c56ef(hdc);
        if (tr != 0) {
            DrawScratch scratch(0, 2, 0);
            i32 pen;
            i32 saved = tr->Push1c58ea(&pen);
            i32 origin;
            tr->MoveTo1c6059(&origin, rp->left + g_62b434, rp->top);
            tr->Draw1c60a5(rp->left + g_62b434, rp->top + 0xc);
            tr->Push1c58ea((void*)saved);
        }
        return 1;
    }

    // -------------------------------------------------------------------------
    // the password edit-control render path. Copies the control's
    // CString (this->m_1c), and when Ctrl is held, masks every char with '*';
    // runs a blink countdown (g_62b438) toggling g_62b43c; then (unless blinked-
    // off + empty) selects the control font, DrawTextA-measures the masked text,
    // right-aligns it if it overflows maxWidth, and renders it into the rect.
    // thiscall member, /GX (destructible CString). Placeholder names.

    // The game's Win32 pointer table entries (0x6c44xx/0x6c3exx) -> reloc-masked.
    extern SHORT(WINAPI* g_pGetAsyncKeyState)(int);        // 0x006c4500
    extern HGDIOBJ(WINAPI* g_pSelectObject)(HDC, HGDIOBJ); // 0x006c3ec4

    // Password blink timer + last-format cache (reached by address).
    extern i32 g_645584; // 0x00645584 elapsed-time delta
    extern i32 g_62b438; // 0x0062b438 blink countdown
    extern i32 g_62b43c; // 0x0062b43c blink on/off state
    extern i32 g_60c7a8; // 0x0060c7a8 last DrawText format

    // The control's MFC CString (data ptr at +0, length at [data-8]); copy ctor +
    // SetAt + dtor are out-of-line (other TU) -> reloc-masked.
    struct PwdStr {
        char* m_data;
        PwdStr(void* srcCString);   // 0x001b9ba3 (CString copy ctor)
        ~PwdStr();                  // 0x001b9cde
        void SetAt(i32 i, char ch); // 0x001ba282
        i32 Len() {
            return *((i32*)m_data - 2);
        }
    };

    struct PwdHost {
        char m_pad00[0x1c];
        char* m_1c; // +0x1c CString data ptr (edit text)
        char m_pad20[0x38 - 0x20];
        HGDIOBJ m_38;                       // +0x38 control font
        void Draw258b(HDC hdc, RECT* rect); // 0x0000258b (caret/underline draw)
        i32 Render22160(HDC hdc, i32 maxWidth, RECT* rect);
    };

    // @early-stop
    // regalloc/EH-state wall (~sibling of MeasureLabel21f20). Complete correct
    // reconstruction: the /GX frame, the arg-null gate before the CString copy, the
    // Ctrl-held '*'-mask loop, the g_645584/g_62b438 countdown + g_62b43c toggle,
    // the blink-off-empty caret branch, the font SelectObject save/restore, the
    // DT_CALCRECT measure + overflow right-align, and both DrawTextA renders align
    // by shape (llvm-objdump -dr). Residual is MSVC5 pinning the shared zero in edi
    // + reusing dead arg slots for the CString/RECT locals differently, shifting the
    // [esp+N] operands and EH scope addend - not steerable from source.
    RVA(0x00022160, 0x18e)
    i32 PwdHost::Render22160(HDC hdc, i32 maxWidth, RECT* rect) {
        if (hdc == 0) {
            return 0;
        }
        PwdStr text((char*)this + 0x1c);
        if (g_pGetAsyncKeyState(0x11) & 0x8000) {
            for (i32 i = 0; i < text.Len(); i++) {
                text.SetAt(i, '*');
            }
        }
        i32 t;
        if ((u32)g_645584 < (u32)g_62b438) {
            t = g_62b438 - g_645584;
        } else {
            t = 0;
        }
        g_62b438 = t;
        if (t == 0) {
            g_62b438 = 0xc8;
            g_62b43c ^= 1;
        }
        if (g_62b43c != 0 && text.Len() == 0) {
            Draw258b(hdc, rect);
        } else {
            HGDIOBJ prev = 0;
            if (m_38) {
                prev = g_pSelectObject(hdc, m_38);
            }
            if (g_62b43c) {
                Draw258b(hdc, rect);
            }
            int(WINAPI * pDraw)(HDC, LPCSTR, int, LPRECT, UINT) = g_pDrawTextA;
            RECT rc;
            rc.left = rect->left;
            rc.top = rect->top;
            rc.right = rect->right;
            rc.bottom = rect->bottom;
            pDraw(hdc, text.m_data, text.Len(), &rc, 0x420);
            i32 fmt = ((rc.right - rc.left) <= maxWidth) ? 0x20 : 0x22;
            g_60c7a8 = fmt;
            pDraw(hdc, text.m_data, text.Len(), rect, fmt);
            if (prev) {
                g_pSelectObject(hdc, prev);
            }
        }
        return 1;
    }

    // -------------------------------------------------------------------------
    // a centered "3D" text renderer. Selects one of two control
    // fonts, sets transparent bk, copies a source CString, DT_CALCRECT-measures
    // it centered in the dst rect, and draws it centered - first a black shadow
    // pass offset by (dx,dy) when the shadow flag is set, then the RGB(r,g,b)
    // main pass. thiscall member, 10 args, /GX (destructible CString).

    extern int(WINAPI* g_pSetBkMode)(HDC, int);              // 0x006c3eb8
    extern COLORREF(WINAPI* g_pSetBkColor)(HDC, COLORREF);   // 0x006c3eb0
    extern COLORREF(WINAPI* g_pSetTextColor)(HDC, COLORREF); // 0x006c3eb4

    struct TextHost {
        char m_pad00[0x3c];
        HGDIOBJ m_3c; // +0x3c font A
        HGDIOBJ m_40; // +0x40 font B
        i32 Draw3DText22810(
            void* strSrc,
            HDC hdc,
            RECT* dst,
            i32 fontFlag,
            i32 r,
            i32 g,
            i32 b,
            i32 shadow,
            i32 dx,
            i32 dy
        );
    };

    // @early-stop
    // regalloc/scheduling wall. Complete correct reconstruction: the /GX frame, the
    // three arg-null gates before the CString copy, the two-font SelectObject, the
    // transparent-bk setup, the DT_CALCRECT centering math (signed /2 round-toward-
    // zero on both axes), the black-shadow offset pass and the RGB main pass all
    // align by shape (llvm-objdump -dr). Residual is MSVC5 permuting the rc/centering
    // temporaries across ebx/ebp/esi/edi vs retail and reusing dead arg slots for the
    // rc + selPrev locals differently, shifting the [esp+N] operands - not steerable.
    RVA(0x00022810, 0x22a)
    i32 TextHost::Draw3DText22810(
        void* strSrc,
        HDC hdc,
        RECT* dst,
        i32 fontFlag,
        i32 r,
        i32 g,
        i32 b,
        i32 shadow,
        i32 dx,
        i32 dy
    ) {
        if (hdc == 0) {
            return 0;
        }
        if (dst == 0) {
            return 0;
        }
        if (strSrc == 0) {
            return 0;
        }
        HGDIOBJ selPrev = 0;
        RECT rc;
        rc.left = dst->left;
        rc.top = dst->top;
        rc.right = dst->right;
        rc.bottom = dst->bottom;
        HGDIOBJ obj = fontFlag ? m_40 : m_3c;
        if (obj) {
            selPrev = g_pSelectObject(hdc, obj);
        }
        g_pSetBkMode(hdc, 1);
        g_pSetBkColor(hdc, 0);
        PwdStr text(strSrc);
        g_pDrawTextA(hdc, text.m_data, strlen(text.m_data), &rc, 0x411);
        i32 hoff = (dst->right + rc.left - dst->left - rc.right) / 2;
        i32 voff = (dst->bottom - dst->top + rc.top - rc.bottom) / 2;
        rc.left += hoff;
        rc.right += hoff;
        rc.top += voff;
        rc.bottom += voff;
        if (shadow) {
            g_pSetTextColor(hdc, 0);
            rc.left += dx;
            rc.top += dy;
            rc.right += dx;
            rc.bottom += dy;
            g_pDrawTextA(hdc, text.m_data, strlen(text.m_data), &rc, 0x11);
            rc.right -= dx;
            rc.left -= dx;
            rc.bottom -= dy;
            rc.top -= dy;
        }
        g_pSetTextColor(hdc, RGB(r, g, b));
        g_pDrawTextA(hdc, text.m_data, strlen(text.m_data), &rc, 0x11);
        if (selPrev) {
            g_pSelectObject(hdc, selPrev);
        }
        return 1;
    }

} // namespace m4

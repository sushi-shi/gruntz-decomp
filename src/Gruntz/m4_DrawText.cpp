// m4_DrawText.cpp - a dialog label text-measure/render helper re-homed out of
// src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// 0x00021f20: measure a CString label into the item's rect (DrawTextA with
// DT_CALCRECT-ish flags 0x420), clamp the used width into g_62b434, then run the
// engine text renderer at that origin. The scratch draw object is the same
// three-level severus/imgHolder hierarchy as m4_FlashRect (vtables in other TUs
// -> reloc-masked); its dtor chain inlines but its 3-arg ctor is out-of-line.
// Placeholder names; only offsets + code bytes are load-bearing. base+/GX.
#include <Win32.h>

#include <Ints.h>
#include <rva.h>

namespace m4 {

    // Cached measured text width (0x0062b434), read by the caller after measure.
    extern i32 g_62b434;

    // DrawTextA through the game Win32 pointer table (0x6c454c) -> reloc-masked.
    extern int(__stdcall* g_pDrawTextA)(HDC, LPCSTR, int, LPRECT, UINT); // 0x006c454c

    // The severus/imgHolder scratch (see m4_FlashRect): inline dtor chain, but
    // this call site builds it with a 3-arg out-of-line ctor.
    struct SevWorker2 {
        virtual ~SevWorker2() {}
    };
    struct ImgHolder2 : SevWorker2 {
        void Release1c6a5c();
        virtual ~ImgHolder2() { Release1c6a5c(); }
    };
    struct DrawScratch : ImgHolder2 {
        DrawScratch(i32 a, i32 b, i32 c); // 0x001c6a72
    };

    // The item's rect source: built from the host's +0x1c sub-object; its first
    // field points at the item RECT. Destructible (EH-tracked).
    struct RectSrc {
        RECT* m_rect; // +0x00
        RectSrc(void* rectField); // 0x001b9ba3
        ~RectSrc();               // 0x001b9cde
    };

    // The engine text renderer bound to the DC (returned by Bind1c56ef).
    struct TextRenderer {
        i32 Push1c58ea(void* obj);          // 0x001c58ea (save/select, returns prior)
        void MoveTo1c6059(void* out, i32 x, i32 y); // 0x001c6059
        void Draw1c60a5(i32 x, i32 y);      // 0x001c60a5
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
    // Draw render with the inline severus/imgHolder scratch dtor chain all align
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

} // namespace m4

// m4_FlashRect.cpp - two near-identical dialog "flash a highlight rectangle"
// helpers re-homed out of src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// Both walk a 4-entry item table, map each child window's client rect into the
// host window's client coords, build a random-gray (or fixed 0x808080) solid
// brush, and FillRect it - the 0x160f0 variant deflates the rect by 2px, the
// 0x0c2e20 variant does not (and returns 1). The scratch draw object is a
// three-level severus/imgHolder hierarchy: its vtables live in other TUs so
// they are referenced by address (reloc-masked); we model the hierarchy with
// real virtual dtors so MSVC5 emits the /GX EH frame + the inline vptr-stamp
// destructor chain that retail shows. Placeholder names; only offsets + code
// bytes are load-bearing. Compiled base+/GX.
#include <Win32.h>

#include <Ints.h>
#include <rva.h>

namespace m4 {

    // MS-CRT-style LCG RNG state (shared with the ApiCaller stubs); reached by
    // address -> reloc-masked. timeGetTime seeds it lazily.
    extern char g_rngSeeded;                     // 0x006c127d  bit0 = seeded
    extern i32 g_rngState;                       // 0x006c1288  32-bit LCG state
    extern u32(__stdcall* g_pTimeGetTime)(void); // 0x006c4650

    // The game's Win32 pointer table (0x6c44xx / 0x6c3eac) - typed so the
    // indirect calls reloc-mask.
    extern BOOL(__stdcall* g_pGetClientRect)(HWND, LPRECT);       // 0x006c44e4
    extern BOOL(__stdcall* g_pClientToScreen)(HWND, LPPOINT);     // 0x006c44ec
    extern BOOL(__stdcall* g_pScreenToClient)(HWND, LPPOINT);     // 0x006c44e8
    extern HBRUSH(__stdcall* g_pCreateSolidBrush)(COLORREF);      // 0x006c3eac
    extern int(__stdcall* g_pFillRect)(HDC, const RECT*, HBRUSH); // 0x006c44e0

    // The three-level draw-scratch hierarchy (severus worker <- image holder <-
    // most-derived). Vtables are external (other TUs) -> reloc-masked stamps.
    struct SevWorker {
        virtual ~SevWorker() {}
    };
    struct ImgHolder : SevWorker {
        HBRUSH m_brush;
        void Release1c6a5c(); // 0x001c6a5c (brush release)
        virtual ~ImgHolder() {
            Release1c6a5c();
        }
        // MFC-style safe handle: NULL-guards the receiver (retail keeps the
        // neg/sbb/and select even for a stack object).
        HBRUSH SafeBrush() {
            return this ? m_brush : (HBRUSH)0;
        }
    };
    struct FlashScratch : ImgHolder {
        FlashScratch() {
            m_brush = 0;
        }
        void Init1c6a05(HBRUSH br); // 0x001c6a05
    };

    // A GDI DC wrapper built from the host window (hdc at +0); ctor/dtor external.
    struct FlashHost;
    struct FlashDC {
        HDC m_hdc;
        FlashDC(FlashHost* host); // 0x001c68b2
        ~FlashDC();               // 0x001c6924
    };

    // A dialog child-item record: its window handle lives at +0x1c.
    struct FlashItem {
        char m_pad00_1c[0x1c];
        HWND m_1c;          // +0x1c
        BOOL Check1be68c(); // 0x001be68c
    };
    // The dialog host: window at +0x1c, plus the two item accessors.
    struct FlashHost {
        char m_pad00_1c[0x1c];
        HWND m_1c;                        // +0x1c
        FlashItem* GetItem2c52(i32 slot); // 0x00002c52
        FlashItem* GetItem30da(i32 slot); // 0x000030da
        void FlashRect160f0();            // 0x000160f0
        i32 FlashRectC2e20();             // 0x000c2e20
    };

    // Advance the shared LCG one step (lazily seeded); returns 15-bit value.
    // Retail inlines this three times per color, so force it inline.
    static __inline i32 GameRand() {
        i32 seed;
        if (!(g_rngSeeded & 1)) {
            g_rngSeeded |= 1;
            seed = (i32)g_pTimeGetTime();
        } else {
            seed = g_rngState;
        }
        g_rngState = seed * 214013 + 2531011;
        return (g_rngState >> 0x10) & 0x7fff;
    }

    // @early-stop
    // EH frame-size + regalloc wall (~84%). Complete correct reconstruction: the
    // /GX EH frame, the 4-slot loop, the child->host rect map, the 3x inlined LCG
    // color, the severus/imgHolder ctor + inline vptr-stamp dtor chain, the
    // rect-deflate and the NULL-guarded brush select all match by shape
    // (llvm-objdump -dr). Residual is MSVC5 reserving a 0x70 frame vs our 0x24
    // (so dc/EH-state slots shift) and swapping the ecx/edx scratch regs in the
    // strength-reduced *214013 LCG (+ spilling the ScreenToClient ptr once the
    // rect-deflate raises register pressure) - not steerable from source. Its
    // twin FlashRectC2e20 (no deflate) reaches ~95% from the same idiom.
    RVA(0x000160f0, 0x245)
    void FlashHost::FlashRect160f0() {
        FlashDC dc(this);
        BOOL(__stdcall * cts)(HWND, LPPOINT) = g_pClientToScreen;
        BOOL(__stdcall * stc)(HWND, LPPOINT) = g_pScreenToClient;
        for (i32 i = 0; i < 4; i++) {
            FlashItem* it = GetItem2c52(i);
            if (it == 0) {
                continue;
            }
            RECT rc;
            g_pGetClientRect(it->m_1c, &rc);
            cts(it->m_1c, (LPPOINT)&rc);
            cts(it->m_1c, (LPPOINT)&rc + 1);
            stc(m_1c, (LPPOINT)&rc);
            stc(m_1c, (LPPOINT)&rc + 1);
            FlashScratch scratch;
            i32 color;
            if (it->Check1be68c()) {
                GameRand();
                GameRand();
                i32 v = (GameRand() % 0xff) & 0xff;
                color = (v << 8 | v) << 8 | v;
            } else {
                color = 0x808080;
            }
            scratch.Init1c6a05(g_pCreateSolidBrush(color));
            rc.left += 2;
            rc.top += 2;
            rc.right -= 2;
            rc.bottom -= 2;
            g_pFillRect(dc.m_hdc, &rc, scratch.SafeBrush());
        }
    }

    // @early-stop
    // EH frame-size wall (~95%). Complete correct reconstruction (same idiom as
    // the 0x160f0 twin, minus the rect-deflate, returning 1). Residual is MSVC5's
    // 0x70 vs 0x20 frame reservation shifting the dc-handle / EH-state stack slots
    // - not steerable from source.
    RVA(0x000c2e20, 0x21d)
    i32 FlashHost::FlashRectC2e20() {
        FlashDC dc(this);
        BOOL(__stdcall * cts)(HWND, LPPOINT) = g_pClientToScreen;
        BOOL(__stdcall * stc)(HWND, LPPOINT) = g_pScreenToClient;
        for (i32 i = 0; i < 4; i++) {
            FlashItem* it = GetItem30da(i);
            if (it == 0) {
                continue;
            }
            RECT rc;
            g_pGetClientRect(it->m_1c, &rc);
            cts(it->m_1c, (LPPOINT)&rc);
            cts(it->m_1c, (LPPOINT)&rc + 1);
            stc(m_1c, (LPPOINT)&rc);
            stc(m_1c, (LPPOINT)&rc + 1);
            FlashScratch scratch;
            i32 color;
            if (it->Check1be68c()) {
                GameRand();
                GameRand();
                i32 v = (GameRand() % 0xff) & 0xff;
                color = (v << 8 | v) << 8 | v;
            } else {
                color = 0x808080;
            }
            scratch.Init1c6a05(g_pCreateSolidBrush(color));
            g_pFillRect(dc.m_hdc, &rc, scratch.SafeBrush());
        }
        return 1;
    }

} // namespace m4

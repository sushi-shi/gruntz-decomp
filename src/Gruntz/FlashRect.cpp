// FlashRect.cpp - the two dialog "flash the four team-colour swatches" methods,
// re-homed to their REAL classes: CBattlezDlg::FlashCtrlD (0x160f0, inside the
// retail dialogs .obj block) and its CMultiStartDlg twin (0xc2e20, inside the
// multi-start roster block). Each walks the 4-entry GetCtrlD swatch-control
// family, maps the child's client rect into the host dialog's client coords,
// builds a random-gray (enabled) or fixed 0x808080 (disabled) solid brush, and
// FillRects it - the CBattlezDlg variant deflates the rect by 2px; the
// CMultiStartDlg variant does not and returns 1.
//
// The scratch object is a REAL MFC `CBrush` - proven by RTTI: the vtable its
// inline ctor stamps (0x1e8cf4) carries the COL .?AVCBrush@@, and the dtor chain
// stamps .?AVCGdiObject@@ (0x1e8cd4) / .?AVCObject@@ (0x1e8cb4). The former
// SevWorker/ImgHolder/FlashScratch "image-worker hierarchy" was a fake-view nest
// over CBrush:CGdiObject:CObject; FlashDC was MFC CPaintDC (??0 0x1c68b2 /
// ??1 0x1c6924, FID HIGH); Init1c6a05 was CGdiObject::Attach; Release1c6a5c was
// CGdiObject::DeleteObject (its Detach calls afxMapHGDIOBJ 0x1c697f and the
// indirect call goes through the GDI32 DeleteObject IAT slot); SafeBrush was the
// inline CBrush::operator HBRUSH() NULL-guard; Check1be68c was
// CWnd::IsWindowEnabled (0x1be68c); the "FlashHost/FlashItem" views were the
// dialogs themselves (GetItem2c52 -> CBattlezDlg::GetCtrlD @0x15c40 via thunk
// 0x2c52; GetItem30da -> CMultiStartDlg::GetCtrlD @0xc2840 via thunk 0x30da).
// The g_p* "Win32 pointer table" externs were the plain dllimport IAT slots -
// `&::ClientToScreen` loads __imp__ClientToScreen@8 exactly as retail's cached
// `mov ebp/ebx, ds:[imp]`.
//
// This TU also first-instantiates the CBrush vtable pair the dialogs block keeps
// (the INLINE `CBrush scratch;` default ctor forces ??_7CBrush + ??_GCBrush; the
// out-of-line ??1CBrush copy is kept from Dialogs.cpp - see the pins there):
// @rva-symbol: ??_GCBrush@@UAEPAXI@Z 0x000164d0 0x1e
#include <Gruntz/Dialogs.h> // pulls <Mfc.h>/afxwin (CWnd/CDialog/CBrush/CPaintDC)
#include <Gruntz/Random.h> // g_randSeed/g_randSeeded

#include <Ints.h>
#include <rva.h>

// MS-CRT-style LCG RNG state (shared with the ApiCaller stubs); reached by
// address -> reloc-masked. timeGetTime seeds it lazily. Retail names per
// symbol_names: ?g_randSeeded@@3EA / ?g_randSeed@@3HA / _g_pTimeGetTime.

// Advance the shared LCG one step (lazily seeded); returns 15-bit value.
// Retail inlines this three times per colour, so force it inline.
static __inline i32 GameRand() {
    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = (i32)::timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return (g_randSeed >> 0x10) & 0x7fff;
}

// @early-stop
// EH frame-size + regalloc wall (~84%). Complete correct reconstruction: the
// /GX EH frame, the 4-slot loop, the child->host rect map, the 3x inlined LCG
// colour, the inline CBrush ctor + inline vptr-stamp dtor chain, the rect-deflate
// and the NULL-guarded operator HBRUSH select all match by shape
// (llvm-objdump -dr). Residual is MSVC5 reserving a 0x70 frame vs ours
// (so dc/EH-state slots shift) and swapping the ecx/edx scratch regs in the
// strength-reduced *214013 LCG - not steerable from source. The CMultiStartDlg
// twin (no deflate) reaches ~95% from the same idiom.
RVA(0x000160f0, 0x245)
void CBattlezDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == 0) {
            continue;
        }
        RECT rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, (LPPOINT)&rc);
        cts(it->m_hWnd, (LPPOINT)&rc + 1);
        stc(m_hWnd, (LPPOINT)&rc);
        stc(m_hWnd, (LPPOINT)&rc + 1);
        CBrush scratch;
        i32 color;
        if (it->IsWindowEnabled()) {
            GameRand();
            GameRand();
            i32 v = (GameRand() % 0xff) & 0xff;
            color = (v << 8 | v) << 8 | v;
        } else {
            color = 0x808080;
        }
        scratch.Attach(::CreateSolidBrush(color));
        rc.left += 2;
        rc.top += 2;
        rc.right -= 2;
        rc.bottom -= 2;
        ::FillRect(dc.m_hDC, &rc, scratch);
    }
}

// @early-stop
// EH frame-size wall (~95%). Complete correct reconstruction (same idiom as
// the CBattlezDlg twin, minus the rect-deflate, returning 1). Residual is MSVC5's
// 0x70 vs 0x20 frame reservation shifting the dc-handle / EH-state stack slots
// - not steerable from source.
RVA(0x000c2e20, 0x21d)
i32 CMultiStartDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == 0) {
            continue;
        }
        RECT rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, (LPPOINT)&rc);
        cts(it->m_hWnd, (LPPOINT)&rc + 1);
        stc(m_hWnd, (LPPOINT)&rc);
        stc(m_hWnd, (LPPOINT)&rc + 1);
        CBrush scratch;
        i32 color;
        if (it->IsWindowEnabled()) {
            GameRand();
            GameRand();
            i32 v = (GameRand() % 0xff) & 0xff;
            color = (v << 8 | v) << 8 | v;
        } else {
            color = 0x808080;
        }
        scratch.Attach(::CreateSolidBrush(color));
        ::FillRect(dc.m_hDC, &rc, scratch);
    }
    return 1;
}

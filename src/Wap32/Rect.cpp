// Rect.cpp - CRect geometry methods (the WAP32 engine rectangle class). Both are
// __thiscall leaves whose `this` IS the tagRECT; they route through the engine's
// Win32-API function-pointer table (reloc-masked). Re-homed from the ApiMiscHelpers
// grab-bag (the old RectHost_08c380/RectHost_115b30 per-fn views were CRect).
// (The former ~CHolder8c400 @0x8c400 is GONE from this TU: RTTI at its vtable
// 0x1ea2a4 names .?AVCRgn@@ - the body is the MFC ??1CRgn COMDAT, now pinned by
// @rva-symbol in src/Gruntz/CreditsState.cpp whose obj really emits it. The
// "CImageList::DeleteImageList" story was the FID AMBIG GDI/ImageList twin; the
// callee 0x1c6a5c is CGdiObject::DeleteObject.)
#include <Wap32/Rect.h>
#include <rva.h>

RVA(0x00029ac0, 0x20)
CRect::CRect(i32 l, i32 t, i32 r, i32 b) {
    left = l;
    top = t;
    right = r;
    bottom = b;
}

RVA(0x0008c380, 0x1e)
void CRect::SetRect(i32 l, i32 t, i32 r, i32 b) {
    ::SetRect(this, l, t, r, b);
}

RVA(0x00115b30, 0x15)
CRect& CRect::operator=(const tagRECT& src) {
    ::CopyRect(this, &src);
    return *this;
}

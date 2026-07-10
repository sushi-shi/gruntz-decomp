// Rect.cpp - CRect geometry methods (the WAP32 engine rectangle class). Both are
// __thiscall leaves whose `this` IS the tagRECT; they route through the engine's
// Win32-API function-pointer table (reloc-masked). Re-homed from the ApiMiscHelpers
// grab-bag (the old RectHost_08c380/RectHost_115b30 per-fn views were CRect).
#include <Wap32/Rect.h>
#include <rva.h>

// CRect(l, t, r, b): direct-store 4-int ctor (Ghidra/FID: ??0CRect@@QAE@HHHH@Z).
// Folded here from src/Stub/DiscoveredSmall.cpp's QuadIntRecord placeholder.
RVA(0x00029ac0, 0x20)
CRect::CRect(i32 l, i32 t, i32 r, i32 b) {
    left = l;
    top = t;
    right = r;
    bottom = b;
}

// SetRect(this, l, t, r, b) through the engine's g_pSetRect wrapper.
RVA(0x0008c380, 0x1e)
void CRect::SetRect(i32 l, i32 t, i32 r, i32 b) {
    ::SetRect(this, l, t, r, b);
}

// CopyRect(this, &src) through the engine's g_pCopyRect wrapper; returns *this.
RVA(0x00115b30, 0x15)
CRect& CRect::operator=(const tagRECT& src) {
    ::CopyRect(this, &src);
    return *this;
}

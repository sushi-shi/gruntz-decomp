// Rect.cpp - CRect geometry methods (the WAP32 engine rectangle class). Both are
// __thiscall leaves whose `this` IS the tagRECT; they route through the engine's
// Win32-API function-pointer table (reloc-masked). Re-homed from the ApiMiscHelpers
// grab-bag (the old RectHost_08c380/RectHost_115b30 per-fn views were CRect).
// CHolder8c400 (the 0x8c400 /GX leaf dtor) pulls <Mfc.h> via <Wap32/Object.h>; keep it
// FIRST so afx brings windows.h the afx-way before Rect.h's <Win32.h> (MFC-wall order).
#include <Gruntz/BoundaryLowerDtorsViews.h>
#include <Wap32/Rect.h>
#include <rva.h>

// DeleteImageList @0x1c6a5c IS MFC CImageList::DeleteImageList (afxcmn); minimal local decl.
SIZE_UNKNOWN(CImageList);
class CImageList {
public:
    void DeleteImageList();
};

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

// ---------------------------------------------------------------------------
// 0x08c400 - ~CHolder8c400 (/GX): stamp the derived vtable (0x5e8cd4), run the +0x00
// teardown (0x1c6a5c == CImageList::DeleteImageList), then fold the CObject base
// subobject (restamp 0x5e8cb4). __thiscall. RVA-homed here (RVA-contiguous with
// CRect::SetRect @0x8c380). @orphan: owning class unrecovered; owns an MFC CImageList.
RVA(0x0008c400, 0x46)
CHolder8c400::~CHolder8c400() {
    ((CImageList*)this)->DeleteImageList();
}

// CopyRect(this, &src) through the engine's g_pCopyRect wrapper; returns *this.
RVA(0x00115b30, 0x15)
CRect& CRect::operator=(const tagRECT& src) {
    ::CopyRect(this, &src);
    return *this;
}



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

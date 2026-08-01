#ifndef WAP32_RECT_H
#define WAP32_RECT_H

#include <Ints.h>
#include <Win32.h>
#include <rva.h>

// Kept local because label-generation clang cannot consume MFC's CRect inlines.
struct CRect : public tagRECT {
    CRect() {}

    CRect(i32 l, i32 t, i32 r, i32 b);
    void SetRect(i32 l, i32 t, i32 r, i32 b);
    CRect& operator=(const tagRECT& src);
    i32 Width();
};
SIZE(0x10);

#endif // WAP32_RECT_H

#ifndef WAP32_RECT_H
#define WAP32_RECT_H

#include <Ints.h>
#include <Win32.h> // tagRECT + the SetRect/CopyRect dllimports
#include <rva.h>

struct CRect : public tagRECT {
    CRect() {}                                // trivial default ctor (no code; enables `CRect t;`
                                              // then `t = *rc` -> the 0x115b30 operator= build)
    CRect(i32 l, i32 t, i32 r, i32 b);        // 0x29ac0 (direct-store ctor, was QuadIntRecord)
    void SetRect(i32 l, i32 t, i32 r, i32 b); // 0x8c380
    CRect& operator=(const tagRECT& src);     // 0x115b30 (returns *this)
    i32 Width(); // 0x17b500 right-left (out-of-line IN THE FONT TU - retail placement;
                 // was the fabricated TextRange::Span - all 5 retail callers pass a CRect)
};
SIZE(16);

#endif // WAP32_RECT_H

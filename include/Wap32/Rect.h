// Rect.h - CRect, the WAP32 engine rectangle class (derives from tagRECT, i.e.
// left/top/right/bottom at +0x0/+0x4/+0x8/+0xc, sizeof 16). Its geometry methods
// route the Win32 rect APIs through the engine's own function-pointer wrapper
// table (retail: g_pSetRect @0x6c44b8 / g_pCopyRect @0x6c44bc, both reloc-masked;
// the __declspec(dllimport) SetRect/CopyRect emit the same `ff 15 [ptr]` bytes).
// Identity: Ghidra/FID attests ??0CRect@@QAE@HHHH@Z for the direct-store 4-int
// ctor @0x29ac0 (folded here from QuadIntRecord in src/Stub/DiscoveredSmall.cpp).
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
};
SIZE(CRect, 16);

#endif // WAP32_RECT_H

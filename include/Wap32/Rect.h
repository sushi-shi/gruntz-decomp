#ifndef WAP32_RECT_H
#define WAP32_RECT_H

#include <Ints.h>
#include <Win32.h> // tagRECT + the SetRect/CopyRect dllimports
#include <rva.h>

// THIS CRect IS A HAND-ROLLED DUPLICATE OF MFC's CRect - a fake view of a real library
// class, and it is the reason the SbiRect/SbRect fold could not land on it (2026-07-28).
// Evidence:
//  * layout + members are MFC's CRect exactly (`class CRect : public tagRECT` with the
//    4-arg ctor, SetRect, operator=(const RECT&) and Width);
//  * the bodies ARE MFC's inlines emitted as out-of-line COMDATs - 0x29ac0 is literally
//    `left=l; top=t; right=r; bottom=b` (`mov [eax],ecx / [eax+4],edx / [eax+8],ecx /
//    [eax+0xc],edx; ret 0x10`) and 0x17b500 is `mov eax,[ecx+8]; mov edx,[ecx]; sub
//    eax,edx; ret` == `right - left`;
//  * it COLLIDES with the real thing: any TU that sees both this header and <afxwin.h>
//    fails C2011 'CRect' : 'class' type redefinition (measured in GruntzMgr.cpp and
//    GruntzMgrCmd.cpp).
// Dissolving it onto MFC's CRect is blocked on two things, both real: (1) <afxwin.h>
// cannot be pulled into these headers because the label pass' clang rejects MFC 4.2's
// implicit-int inlines in afxwin1.inl (CMenu::operator==), and the codebase's workaround
// `#undef _AFX_ENABLE_INLINES` also throws away CRect's inline ctor - which would turn
// every inline-temporary construction into a call; (2) the four RVAs below then have no
// declaration to hang on, so they need the `#define`-gated own-TU device the SBI dtor
// chain uses. Not papered over here - stated, with the bytes.
struct CRect : public tagRECT {
    CRect() {}                                // trivial default ctor (no code; enables `CRect t;`
                                              // then `t = *rc` -> the 0x115b30 operator= build)
    CRect(i32 l, i32 t, i32 r, i32 b);        // 0x29ac0 (direct-store ctor, was QuadIntRecord)
    void SetRect(i32 l, i32 t, i32 r, i32 b); // 0x8c380
    CRect& operator=(const tagRECT& src);     // 0x115b30 (returns *this)
    i32 Width(); // 0x17b500 right-left (out-of-line IN THE FONT TU - retail placement;
                 // was the fabricated TextRange::Span - all 5 retail callers pass a CRect)
};
SIZE(0x10);

#endif // WAP32_RECT_H

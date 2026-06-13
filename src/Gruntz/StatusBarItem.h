// StatusBarItem.h - Gruntz status-bar item base class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the small constructor. Field names
// are placeholders; the offsets are the load-bearing fact the match proves.
#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

// ---------------------------------------------------------------------------
// CStatusBarItem - base of the SBI_* family.
//   vftable @0x5eabcc. ctor (RVA 0x1005d0, 23 bytes) zeroes m_4 (+0x04),
//   m_8 (+0x08), m_24 (+0x24), m_28 (+0x28); vptr stored first.
//
// NOTE on the layout vs the derived CSBI_RectOnly (see SBI_RectOnly.cpp): in
// the retail binary the *complete-object* CStatusBarItem ctor at 0x1005d0
// zeroes all four fields, while when its body is folded into CSBI_RectOnly the
// optimiser drops the dead m_8=0 store (the derived ctor overwrites m_8 with
// 1). Both behaviours come from the SAME class; they are matched as two
// separate translation units because MSVC 5.0 only emits the out-of-line ctor
// copy when the ctor is defined out-of-line.
// ---------------------------------------------------------------------------
class CStatusBarItem {
public:
    CStatusBarItem();
    virtual ~CStatusBarItem();
    virtual int SbiVfunc0();

    int  m_4;                  // +0x04
    int  m_8;                  // +0x08
    char m_padc[0x24 - 0x0c];  // +0x0c..0x23
    int  m_24;                 // +0x24
    int  m_28;                 // +0x28
};

#endif // STATUSBARITEM_H

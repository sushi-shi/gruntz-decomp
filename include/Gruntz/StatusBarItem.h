// StatusBarItem.h - Gruntz status-bar item base class (C:\Proj\Gruntz).
// Reconstruction sufficient to byte-match the small constructor. Field names
// are placeholders; the offsets are the load-bearing fact the match proves.
#ifndef STATUSBARITEM_H
#define STATUSBARITEM_H

// ---------------------------------------------------------------------------
// CStatusBarItem - base of the SBI_* family. One class, one definition.
//
// The ctor is INLINE: the derived CSBI_RectOnly ctor folds it (zeroing
// m_4/m_24/m_28; the base's m_8=0 store is then dead, since CSBI_RectOnly sets
// m_8=1, and the optimiser drops it). MSVC 5.0 only folds a base ctor that is
// visible inline, so inline is the load-bearing choice the match pins.
//
// Retail also has a standalone complete-object ctor (one that *does* zero all
// four fields, at its own RVA 0x1005d0): MSVC's out-of-line COMDAT copy of this
// same inline ctor. It is NOT a second class. MSVC 5.0 inlines this tiny ctor at
// every instantiation we can synthesize, so the canonical inline form cannot emit
// a labelable standalone ??0 to diff against it. To keep that byte-match,
// src/Gruntz/StatusBarItem.cpp is a stand-in TU that locally redeclares the class
// with an out-of-line ctor purely as a labeling device for the COMDAT - a tooling
// workaround, not a second class the developers wrote.
// ---------------------------------------------------------------------------
class CStatusBarItem {
public:
    CStatusBarItem() {
        m_4 = 0;
        m_8 = 0;
        m_24 = 0;
        m_28 = 0;
    }
    virtual ~CStatusBarItem();
    virtual int SbiVfunc0();

    int m_4;                  // +0x04
    int m_8;                  // +0x08
    char m_padc[0x24 - 0x0c]; // +0x0c..0x23
    int m_24;                 // +0x24
    int m_28;                 // +0x28
};

#endif // STATUSBARITEM_H

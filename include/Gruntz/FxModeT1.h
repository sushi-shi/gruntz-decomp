// FxModeT1.h - the type-1 CFxMode variant (a DISTINCT class from CFxModeT3, which
// Ghidra RTTI-named both "CFxModeT3"): it carries an MFC CString member at +0x24 and
// is 0x2c bytes. Split into its own header (Mfc.h first, afx-rule) so FxModeDesc.h
// stays MFC-free for its non-MFC includers (Fader.cpp / SoundFxEmitter.h); only
// FxModeDesc.cpp pulls this. Ctor 0x17e7c0.
#ifndef GRUNTZ_FXMODET1_H
#define GRUNTZ_FXMODET1_H

#include <Mfc.h> // real MFC CString (the +0x24 member; ctor 0x1b9b93 / op= 0x1b9e74) - afx-first
#include <Gruntz/FxModeDesc.h>

// CFxModeT1 : CFxModeDesc - base + (type=1, m_10=0x32, m_14=1, m_18=1) + the CString
// name member. The destructible CString forces the ctor's /GX frame.
SIZE(CFxModeT1, 0x2c);
class CFxModeT1 : public CFxModeDesc {
public:
    CFxModeT1(); // 0x17e7c0
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
    CString m_24; // +0x24
    i32 m_28;     // +0x28
};

#endif // GRUNTZ_FXMODET1_H

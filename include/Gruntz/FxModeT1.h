#ifndef GRUNTZ_FXMODET1_H
#define GRUNTZ_FXMODET1_H

#include <Mfc.h> // real MFC CString (the +0x24 member; ctor 0x1b9b93 / op= 0x1b9e74) - afx-first
#include <rva.h>
#include <Gruntz/FxModeDesc.h>

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
SIZE(0x2c);

#endif // GRUNTZ_FXMODET1_H

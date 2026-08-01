#ifndef GRUNTZ_FXMODET1_H
#define GRUNTZ_FXMODET1_H

#include <Mfc.h>
#include <rva.h>
#include <Gruntz/FxModeDesc.h>

class CFxModeT1 : public CFxModeDesc {
public:
    CFxModeT1();
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    class CShadeTable* m_20;
    CString m_24;
    class CDDPalette* m_28;
};
SIZE(0x2c);

#endif // GRUNTZ_FXMODET1_H

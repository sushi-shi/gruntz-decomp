// TitleApp.h - CTitleApp, the title-sequence front-end state.
//
// CTitleApp : CState - OnStart (0xf9880) calls RunTitleSeq @0xfa350 (a CState base
// method) on its own `this`. Its exact leaf identity is still @identity-TODO: the
// RunTitleSeq call demangles to ?RunTitleSeq@CAttract@@ (so `this` looks CAttract*),
// but OnStart @0xf9880 sits far from the CAttract object block and writes an int timer
// to +0x1b8 where Attract.h models a CAttractHost* sound/host sub-object - a real
// layout conflict. So deriving CState is proven + sufficient to bind the base symbol;
// resolve the +0x1b8 conflict + the obj placement in a dedicated CAttract pass before
// binding the leaf. The +0x1b8 timer sits past the CState base (ends at +0x1a8),
// exactly as CPreviewState's own m_1b8 does.
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_TITLEAPP_H
#define GRUNTZ_TITLEAPP_H

#include <rva.h>
#include <Gruntz/State.h> // the CState base this title state derives (RunTitleSeq @0xfa350)

class CTitleApp : public CState {
public:
    int OnStart(int unused); // 0xf9880
    char m_pad1a8[0x1b8 - 0x1a8];
    int m_1b8; // +0x1b8 timer
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TITLEAPP_H

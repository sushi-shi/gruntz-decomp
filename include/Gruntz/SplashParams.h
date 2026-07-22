#ifndef GRUNTZ_GRUNTZ_SPLASHPARAMS_H
#define GRUNTZ_GRUNTZ_SPLASHPARAMS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/String.h> // MFC CString (the loaded splash caption)

// The splash-caption params CState::InputVirtual's "loading imagez" draw feeds
// EngStr_DrawText (ex the CMgrPersistObj fake view's header).
struct SplashParams {
    CString text; // +0x00
    i32 m_04;     // +0x04
    i32 m_08, m_0c, m_10, m_14;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_SPLASHPARAMS_H

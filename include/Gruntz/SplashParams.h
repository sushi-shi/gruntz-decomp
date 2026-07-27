#ifndef GRUNTZ_GRUNTZ_SPLASHPARAMS_H
#define GRUNTZ_GRUNTZ_SPLASHPARAMS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/String.h> // MFC CString (the loaded splash caption)

// The splash-caption params CState::InputVirtual's "loading imagez" draw feeds
// EngStr_DrawText (ex the CMgrPersistObj fake view's header).
struct SplashParams {
    CString text; // +0x00  the caption (EngStr_DrawText's CString* arg)
    RECT rect;    // +0x04  its bounds  (EngStr_DrawText's RECT* arg - the call
                  //        passed &m_04, i.e. exactly this block)
    i32 m_14;     // +0x14
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_SPLASHPARAMS_H

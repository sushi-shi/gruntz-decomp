#ifndef GRUNTZ_GRUNTZ_SPLASHPARAMS_H
#define GRUNTZ_GRUNTZ_SPLASHPARAMS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/String.h> // MFC CString (the loaded splash caption)

// The splash-caption params CState::InputVirtual's "loading imagez" draw feeds
// EngStr_DrawText (ex the CMgrPersistObj fake view's header).
// Exactly 0x14 bytes: retail's CState::InputVirtual reserves `sub esp,0x14` for
// this block, and the only stores are text (+0x00) and the four RECT fields
// (+0x04..+0x13) - there is no trailing member.
struct SplashParams {
    CString text; // +0x00  the caption (EngStr_DrawText's CString* arg)
    RECT rect;    // +0x04  its bounds  (EngStr_DrawText's RECT* arg - the call
                  //        passed &m_04, i.e. exactly this block)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_SPLASHPARAMS_H

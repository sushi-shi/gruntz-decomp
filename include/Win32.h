#ifndef GRUNTZ_WIN32_H
#define GRUNTZ_WIN32_H

// The pure Win32/DirectX platform root; <Mfc.h> is the MFC one, and the two are
// mutually exclusive as a TU's first include. Reached after <Mfc.h> - the order
// the canonical include block enforces - <windows.h> is an already-guarded no-op.
//
// timeGetTime is declared in <Mfc.h> only: no pure-Win32 unit calls it, and a
// second copy would make one hand-rolled import into two. A Win32 unit that
// needs the timer includes <mmsystem.h>, as GruntzWnd.cpp, SoundBuffer.cpp
// and SoundStream.cpp do.

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#endif // GRUNTZ_WIN32_H

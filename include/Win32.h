// Win32.h - the real <windows.h> for PURE Win32 / DirectX TUs (those that use NO
// MFC). MFC TUs must use <Mfc.h> instead (afx.h pulls windows.h the controlled
// way; the two are mutually exclusive - MFC hard-errors if windows.h came first).
//
// Include this FIRST in a TU, before any DirectX header. It also supplies the few
// things VC5's <windows.h> lacks / doesn't pull in, so they live in ONE place
// instead of scattered hand-rolled decls: INT_PTR and WINMM's timeGetTime.
#ifndef GRUNTZ_WIN32_H
#define GRUNTZ_WIN32_H

#define WIN32_LEAN_AND_MEAN // trim windows.h to the core (no OLE / RPC / etc.)
#include <windows.h>

typedef int INT_PTR; // VC5 predates the <basetsd.h> pointer-width aliases

// WINMM timeGetTime (frame clock); in <mmsystem.h> (modern <timeapi.h>), not in
// <windows.h> - declared here so it isn't hand-rolled per TU, in the real
// <mmsystem.h> form (WINAPI == __stdcall, resolved from the windows.h above).
extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_WIN32_H

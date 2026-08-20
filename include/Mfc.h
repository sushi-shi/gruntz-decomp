#ifndef GRUNTZ_MFC_H
#define GRUNTZ_MFC_H

// The MFC platform root. <MfcWin.h> and <MfcNoInline.h> are supersets that pull
// this file themselves; <Win32.h> is the pure Win32/DirectX root and is mutually
// exclusive with this one as a TU's first include (<afxv_w32.h> rejects a
// <windows.h> that got there first).

#define VC_EXTRALEAN

#include <afx.h>
#include <afxcoll.h>

// API-forced: AFX_MSGMAP_ENTRY erases each typed member-function pointer to
// the single AFX_PMSG representation consumed by MFC's dispatcher.
#define GZ_MFC_PMSG(method) reinterpret_cast<AFX_PMSG>(method)

// MMSYSTEM.H:1984, transcribed because cl 5.0 cannot take the SDK header here:
// docs/patterns/large-sdk-header-in-a-shared-prelude.md has the measurement.
// A unit that can afford <mmsystem.h> includes it instead.
extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_MFC_H

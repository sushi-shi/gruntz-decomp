// Mfc.h - the real MFC headers (statically-linked NAFXCW in retail), pulled from
// the VC5 toolchain the period-correct way: <afx.h> brings in <windows.h> itself.
//
// We compile with the original VC5 toolchain, so MFC's own headers give the exact
// CString/CFile/CObject/CException/CArchive layouts + inline functions retail used
// - no hand-rolled shells to keep in sync, or to confuse with the reconstructed
// engine classes. `VC_EXTRALEAN` selects the lean MFC subset (no OLE / DB / sockets).
//
// INCLUDE THIS FIRST in a TU - BEFORE any <windows.h>/DirectX header - because MFC
// hard-errors if windows.h was included first ("MFC apps must not #include
// <windows.h>"). Engine headers that use an MFC type include Mfc.h at their top.
#ifndef GRUNTZ_MFC_H
#define GRUNTZ_MFC_H

#define VC_EXTRALEAN // trim MFC: no OLE / DB / sockets / DAO
#include <afx.h>     // CObject, CString, CFile, CException, CArchive (+ windows.h)
#include <afxcoll.h> // CObList, CMapStringToOb, CByteArray, CPtrArray/List, ...

// VC5 predates the <basetsd.h> pointer-width aliases; the engine's DialogProcs
// return INT_PTR. Defined once here (and in Win32.h) instead of scattered.
typedef int INT_PTR;

// WINMM timeGetTime (the engine's frame clock). It lives in <mmsystem.h> (modern
// <timeapi.h>), which we don't pull in for one function; declared once here in the
// real <mmsystem.h> form (WINAPI == __stdcall, resolved from the windows.h above).
extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_MFC_H

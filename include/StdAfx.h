#ifndef GRUNTZ_STDAFX_H
#define GRUNTZ_STDAFX_H

// Project-wide prelude: every translation unit includes it first, as Monolith's
// stdafx.h did. MFC keeps its release default (_AFX_ENABLE_INLINES).

#define VC_EXTRALEAN

#include <afx.h>
#include <afxcoll.h>

// Clang (label extraction, clangd) cannot parse afxwin's inline file.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <afxcmn.h>
#include <afxtempl.h>

// Clang's header mirror qualifies VC5's implicit member-pointer addresses.
#ifdef __clang__
#define MFC_MESSAGE_MAP_CLASS(theClass) typedef theClass MfcMessageMapClass;
#else
#define MFC_MESSAGE_MAP_CLASS(theClass)
#endif

// VC_EXTRALEAN drops mmsystem.h; winmm's import is declared as the SDK does.
extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_STDAFX_H

#ifndef GRUNTZ_STDAFX_H
#define GRUNTZ_STDAFX_H

#define VC_EXTRALEAN

#include <afx.h>
#include <afxcoll.h>

#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <afxcmn.h>
#include <afxtempl.h>

#ifdef __clang__
#define MFC_MESSAGE_MAP_CLASS(theClass) typedef theClass MfcMessageMapClass;
#else
#define MFC_MESSAGE_MAP_CLASS(theClass)
#endif

extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif

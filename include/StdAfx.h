#ifndef GRUNTZ_STDAFX_H
#define GRUNTZ_STDAFX_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <Utils/Text.h>
#include <Utils/Sequence.h>

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

#include <Wap32/PlatformText.h>

extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif

#ifndef GRUNTZ_MFC_H
#define GRUNTZ_MFC_H

// MFC platform root; do not mix it with the Win32.h prelude.

#define VC_EXTRALEAN

#include <afx.h>
#include <afxcoll.h>

extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_MFC_H

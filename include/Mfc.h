#ifndef GRUNTZ_MFC_H
#define GRUNTZ_MFC_H

#define VC_EXTRALEAN // trim MFC: no OLE / DB / sockets / DAO
#include <afx.h>     // CObject, CString, CFile, CException, CArchive (+ windows.h)
#include <afxcoll.h> // CObList, CMapStringToOb, CByteArray, CPtrArray/List, ...

typedef int INT_PTR;

extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_MFC_H

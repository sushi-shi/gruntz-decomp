#ifndef GRUNTZ_WIN32_H
#define GRUNTZ_WIN32_H

#define WIN32_LEAN_AND_MEAN // trim windows.h to the core (no OLE / RPC / etc.)
#include <windows.h>

typedef int INT_PTR; // VC5 predates the <basetsd.h> pointer-width aliases

extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);

#endif // GRUNTZ_WIN32_H

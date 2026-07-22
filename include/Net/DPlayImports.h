// DPlayImports.h - the dplayx.dll import decls NetMgr.cpp calls. PRIVATE to
// NetMgr.cpp: dplay.h #defines DirectPlayEnumerate -> DirectPlayEnumerateA, so a
// TU that saw both this and dplay.h would C2733; nothing else includes this.
#ifndef GRUNTZ_NET_DPLAYIMPORTS_H
#define GRUNTZ_NET_DPLAYIMPORTS_H

#include <Ints.h>

extern "C" i32 __stdcall DirectPlayCreate(void* lpGUID, void* lplpDP, void* pUnk);
extern "C" i32 __stdcall DirectPlayEnumerate(void* lpEnumCallback, void* lpContext);

#endif // GRUNTZ_NET_DPLAYIMPORTS_H

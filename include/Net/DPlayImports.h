#ifndef GRUNTZ_NET_DPLAYIMPORTS_H
#define GRUNTZ_NET_DPLAYIMPORTS_H

#include <Ints.h>
#include <Net/NetMgr.h>

extern "C" i32 __stdcall DirectPlayCreate(void* lpGUID, void* lplpDP, void* pUnk);
extern "C" i32 __stdcall
DirectPlayEnumerate(NetEnumProvidersCallback lpEnumCallback, void* lpContext);

#endif // GRUNTZ_NET_DPLAYIMPORTS_H

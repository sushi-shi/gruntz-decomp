#ifndef GRUNTZ_NET_NETMGRREPORTERROR_H
#define GRUNTZ_NET_NETMGRREPORTERROR_H

#include <rva.h>

extern "C" i32 g_code; // should be a DWORD (stores the low 16 bits of the HRESULT with error code)
extern "C" char g_szCode[];
extern "C" char g_szMsg[];
#endif // GRUNTZ_NET_NETMGRREPORTERROR_H

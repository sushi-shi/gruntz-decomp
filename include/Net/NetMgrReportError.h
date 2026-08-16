#ifndef GRUNTZ_NET_NETMGRREPORTERROR_H
#define GRUNTZ_NET_NETMGRREPORTERROR_H

#include <rva.h>

extern i32 g_code; // should be a DWORD (stores the low 16 bits of the HRESULT with error code)
extern char g_szCode[];
extern char g_szMsg[];
#endif // GRUNTZ_NET_NETMGRREPORTERROR_H

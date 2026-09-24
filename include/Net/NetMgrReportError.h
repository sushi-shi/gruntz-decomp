#ifndef GRUNTZ_NET_NETMGRREPORTERROR_H
#define GRUNTZ_NET_NETMGRREPORTERROR_H

#include <rva.h>

#include <string.h>

extern i32 g_code;
extern char g_szCode[];
extern char g_szMsg[];

inline static void SetError(const char* szCode, const char* szDesc) {
    strcpy(g_szCode, szCode);
    strcpy(g_szMsg, szDesc);
}

#endif // GRUNTZ_NET_NETMGRREPORTERROR_H

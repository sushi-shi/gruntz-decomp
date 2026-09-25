#include <rva.h>

#include <Gruntz/Sparam.h>

#include <Win32.h>

#include <stdio.h>
#include <string.h>

RVA(0x000f9160, 0xd2)
BOOL Sparam_Get(char* sDest, const char* sSource, const char* sId) {
    char sRealId[256];
    sprintf(sRealId, "[%s:", sId);

    char* sStart = strstr(sSource, sRealId);
    if (!sStart) {
        return (FALSE);
    }

    int nLen = strlen(sRealId);
    sStart = &sStart[nLen];
    if (strlen(sStart) < 2) {
        return (FALSE);
    }

    char* pEnd = strstr(sStart, DATA_COMPGEN(0x00213eec, "]"));
    if (!pEnd) {
        return (FALSE);
    }
    if (pEnd <= sStart) {
        return (FALSE);
    }

    int i = 0;

    while (&sStart[i] != pEnd) {
        sDest[i] = sStart[i];
        i++;
    }

    sDest[i] = '\0';

    return (TRUE);
}

RVA(0x000f9280, 0xe4)
BOOL Sparam_Add(char* sSource, const char* sId, const char* sParam) {
    if (!sParam) {
        return (FALSE);
    }

    strcat(sSource, "[");
    strcat(sSource, sId);
    strcat(sSource, ":");

    strcat(sSource, sParam);

    strcat(sSource, "]");

    return (TRUE);
}

RVA(0x000f93b0, 0x41)
BOOL Sparam_Add(char* sSource, const char* sId, int nParam) {
    char sTmp[256];
    sprintf(sTmp, "%i", nParam);

    return (Sparam_Add(sSource, sId, sTmp));
}

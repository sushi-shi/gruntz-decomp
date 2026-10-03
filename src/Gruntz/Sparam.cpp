#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/Sparam.h>

#include <stdio.h>
#include <string.h>

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

    char* pEnd = strstr(sStart, "]");
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

BOOL Sparam_Add(char* sSource, const char* sId, int nParam) {
    char sTmp[256];
    sprintf(sTmp, "%i", nParam);

    return (Sparam_Add(sSource, sId, sTmp));
}

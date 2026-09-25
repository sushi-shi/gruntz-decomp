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

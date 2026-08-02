#include <rva.h>

#include <Ints.h>

#include <stdio.h>
#include <string.h>

#pragma intrinsic(strlen)

RVA(0x000f9160, 0xd2)
i32 ExtractBracketValue(char* out, char* src, char* key) {
    char marker[256];
    sprintf(marker, "[%s:", key);
    char* p = strstr(src, marker);
    if (!p) {
        return 0;
    }
    p += strlen(marker);
    if (strlen(p) < 2) {
        return 0;
    }
    char* end = strstr(p, "]");
    if (!end) {
        return 0;
    }
    if (end <= p) {
        return 0;
    }
    i32 i = 0;
    while (&p[i] != end) {
        out[i] = p[i];
        i++;
    }
    out[i] = 0;
    return 1;
}

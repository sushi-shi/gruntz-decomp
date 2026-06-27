// BracketValueParse.cpp - 0xf9160 (__cdecl). Extract the value of a "[key:value]"
// token out of a larger string. Build the "[key:" marker into a 256-byte stack
// buffer (sprintf "[%s:"), find it in the source, skip past it, then copy the run
// up to the next ']' into the caller's output buffer. Self-contained leaf; the two
// DAT_006xxxxx are the "[%s:" / "]" $SG literals (reloc-masked), and strlen lowers
// to the inline `repne scasb` intrinsic.
#include <Ints.h>
#include <rva.h>

extern "C" {
int __cdecl sprintf(char* buf, const char* fmt, ...); // 0x11f890
char* __cdecl strstr(const char* hay, const char* needle); // 0x120090
unsigned int __cdecl strlen(const char* s);
}
#pragma intrinsic(strlen)

// ---------------------------------------------------------------------------
// 0xf9160: out <- value of "[key:value]" found in src; 1 on success, 0 if absent.
// ---------------------------------------------------------------------------
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

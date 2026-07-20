#include <Net/NetMgr.h> // <Mfc.h> (reloc-masked externs)
#include <stdio.h>      // engine sprintf (reloc-masked)
#include <rva.h>

i32 MakeButeSectionKey(char* dst, const char* section, const char* key);

RVA(0x000f93b0, 0x41)
void AppendInt(char* dst, const char* sep, i32 n) {
    char buf[256];
    sprintf(buf, "%i", n);
    MakeButeSectionKey(dst, sep, buf);
}

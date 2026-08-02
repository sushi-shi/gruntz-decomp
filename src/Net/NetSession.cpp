#include <rva.h>

#include <Net/NetSession.h>

#include <Net/NetMgr.h>

#include <stdio.h>

RVA(0x000f93b0, 0x41)
void AppendInt(char* dst, const char* sep, i32 n) {
    char buf[256];
    sprintf(buf, "%i", n);
    MakeButeSectionKey(dst, sep, buf);
}

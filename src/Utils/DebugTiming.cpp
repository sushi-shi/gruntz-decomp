#include <StdAfx.h>

#include <Ints.h>

#include <stdarg.h>
#include <stdio.h>

void ActiveWait(u32 milliseconds) {
    DWORD target = timeGetTime() + milliseconds;
    while (timeGetTime() < target)
        ;
}

void DebugTrace(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
}

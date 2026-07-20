#include <Mfc.h>
#include <rva.h>
#include <stdarg.h> // va_list for the DebugTrace varargs formatter
#include <stdio.h>  // vsprintf

RVA(0x0013dfe0, 0x21)
void ActiveWait(u32 milliseconds) {
    DWORD target = timeGetTime() + milliseconds;
    while (timeGetTime() < target)
        ;
}

RVA(0x0013e010, 0x32)
void DebugTrace(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
}
